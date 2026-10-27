/*
    This is a SampVoice project file
    Developer: CyberMor <cyber.mor.2020@gmail.com>

    See more here https://github.com/CyberMor/sampvoice

    Copyright (c) Daniel (CyberMor) 2020 All rights reserved
*/

// open.mp native component that hosts the SampVoice `control` core (control/main.cpp).
// The core talks to the host only through control/libraries/util/host.hpp and util/raknet.hpp; this file
// provides the open.mp versions of both (see control/omp/shim). See specs/001-port-03dl-openmp/research.md R-08.

#include <array>
#include <cstdarg>
#include <cstdint>
#include <cstring>

#include <sdk.hpp>
#include <Server/Components/Pawn/pawn.hpp>
#include <Server/Components/Vehicles/vehicles.hpp>
#include <Server/Components/Objects/objects.hpp>
#include <bitstream.hpp>

#include <pawn/amx/amx.h>
#include <pawn/plugincommon.h>

#include <ysf/globals.h>
#include <util/host.hpp>
#include <util/raknet.hpp>
#include <relay.hpp>

// Entry points of the control core (control/main.cpp)
// ----------------------------------------------------------------

PLUGIN_EXPORT bool PLUGIN_CALL Load(void** ppData) noexcept;
PLUGIN_EXPORT void PLUGIN_CALL Unload() noexcept;
PLUGIN_EXPORT void PLUGIN_CALL ProcessTick() noexcept;
PLUGIN_EXPORT int  PLUGIN_CALL AmxLoad(AMX* amx) noexcept;
PLUGIN_EXPORT int  PLUGIN_CALL AmxUnload(AMX* amx) noexcept;

// Globals expected by the core (the SA:MP build defines them in ysf.cpp)
// ----------------------------------------------------------------

logprintf_t logprintf = nullptr;
void** ppPluginData = nullptr;

namespace
{
    constexpr int      kConnectRpcId = 25; // ClientJoin
    constexpr uint8_t  kSignature[4] = { 0xDE, 0xAD, 0xC0, 0xDE }; // 0xDEADC0DE in network byte order
    constexpr uint16_t kMaxPlayers   = PLAYER_POOL_SIZE;

    ICore*              gCore     = nullptr;
    IPlayerPool*        gPlayers  = nullptr;
    IVehiclesComponent* gVehicles = nullptr;
    IObjectsComponent*  gObjects  = nullptr;

    void* gPluginData[0x100] = {};
    std::array<bool, kMaxPlayers> gConnected = {};

    void OmpLogPrintf(const char* const format, ...)
    {
        if (gCore == nullptr)
            return;

        va_list args;
        va_start(args, format);
        gCore->vprintLn(format, args);
        va_end(args);
    }

    // Finds `signature, length, data` at the end of the ClientJoin payload; every length is range-checked.
    bool FindHandshake(const uint8_t* const data, const size_t size, const uint8_t*& payload, uint8_t& length) noexcept
    {
        if (data == nullptr || size < sizeof(kSignature) + 1)
            return false;

        for (size_t i = size - (sizeof(kSignature) + 1); i != SIZE_MAX; --i)
        {
            if (std::memcmp(data + i, kSignature, sizeof(kSignature)) != 0)
                continue;

            const size_t declared = data[i + sizeof(kSignature)];
            const size_t begin    = i + sizeof(kSignature) + 1;

            if (declared > size - begin)
                return false;

            payload = data + begin;
            length  = static_cast<uint8_t>(declared);
            return true;
        }

        return false;
    }
}

// Host adapter (open.mp)
// ----------------------------------------------------------------

namespace host
{
    bool Available() noexcept
    {
        return gPlayers != nullptr;
    }

    bool HasPlayers() noexcept
    {
        return gPlayers != nullptr && gPlayers->players().size() != 0;
    }

    size_t PlayerLimit() noexcept
    {
        return kMaxPlayers;
    }

    bool PlayerPosition(const size_t player, CVector& position) noexcept
    {
        if (gPlayers == nullptr || player >= kMaxPlayers) return false;

        const IPlayer* const entity = gPlayers->get(static_cast<int>(player));
        if (entity == nullptr) return false;

        const Vector3 value = entity->getPosition();
        position = CVector { value.x, value.y, value.z };
        return true;
    }

    bool PlayerStreamedIn(const size_t viewer, const size_t target) noexcept
    {
        if (gPlayers == nullptr || viewer >= kMaxPlayers || target >= kMaxPlayers) return false;

        IPlayer* const viewer_entity = gPlayers->get(static_cast<int>(viewer));
        IPlayer* const target_entity = gPlayers->get(static_cast<int>(target));

        return viewer_entity != nullptr && target_entity != nullptr && target_entity->isStreamedInForPlayer(*viewer_entity);
    }

    bool VehiclePosition(const size_t vehicle, CVector& position) noexcept
    {
        if (gVehicles == nullptr) return false;

        const IVehicle* const entity = gVehicles->get(static_cast<int>(vehicle));
        if (entity == nullptr) return false;

        const Vector3 value = entity->getPosition();
        position = CVector { value.x, value.y, value.z };
        return true;
    }

    bool VehicleStreamedIn(const size_t viewer, const size_t vehicle) noexcept
    {
        if (gPlayers == nullptr || gVehicles == nullptr || viewer >= kMaxPlayers) return false;

        IPlayer* const viewer_entity = gPlayers->get(static_cast<int>(viewer));
        IVehicle* const vehicle_entity = gVehicles->get(static_cast<int>(vehicle));

        return viewer_entity != nullptr && vehicle_entity != nullptr && vehicle_entity->isStreamedInForPlayer(*viewer_entity);
    }

    bool ObjectPosition(const size_t object, CVector& position) noexcept
    {
        if (gObjects == nullptr) return false;

        const IObject* const entity = gObjects->get(static_cast<int>(object));
        if (entity == nullptr) return false;

        const Vector3 value = entity->getPosition();
        position = CVector { value.x, value.y, value.z };
        return true;
    }
}

// Component
// ----------------------------------------------------------------

class SampVoiceControlComponent final : public IComponent,
    public NetworkInEventHandler,
    public PlayerConnectEventHandler,
    public PawnEventHandler,
    public CoreEventHandler
{
public:
    PROVIDE_UID(0x5B7C0A1D9E3F4A62);

    StringView componentName() const override
    {
        return "SampVoice Control";
    }

    SemanticVersion componentVersion() const override
    {
        return SemanticVersion(4, 2, 0, 0);
    }

    void onLoad(ICore* const core) override
    {
        _core = core;
        gCore = core;
        _core->printLn("[sv:omp] SampVoice control component loading");
    }

    void onInit(IComponentList* const components) override
    {
        _pawn = components->queryComponent<IPawnComponent>();
        gVehicles = components->queryComponent<IVehiclesComponent>();
        gObjects  = components->queryComponent<IObjectsComponent>();

        if (_pawn == nullptr)
        {
            _core->logLn(LogLevel::Error, "[sv:omp] Pawn component is not loaded");
            return;
        }

        gPlayers = &_core->getPlayers();

        // Host data handed to the control core, the same entries the SA:MP server provides
        gPluginData[PLUGIN_DATA_LOGPRINTF]    = reinterpret_cast<void*>(&OmpLogPrintf);
        gPluginData[PLUGIN_DATA_AMX_EXPORTS]  = const_cast<void*>(static_cast<const void*>(&_pawn->getAmxFunctions()));

        RakNet::Instance().SendHook = [](const uword_t player, const adr_t packet, const uint_t length) noexcept -> bool
        {
            if (gPlayers == nullptr) return false;

            const auto send = [packet, length](IPlayer& entity) noexcept
            {
                return entity.sendPacket(Span<uint8_t>(reinterpret_cast<uint8_t*>(packet),
                    static_cast<size_t>(length) * 8), OrderingChannel_Reliable);
            };

            if (player == 0xFFFF)
            {
                bool result = true;
                for (IPlayer* const entity : gPlayers->players())
                    result = send(*entity) && result;
                return result;
            }

            IPlayer* const entity = player < kMaxPlayers ? gPlayers->get(static_cast<int>(player)) : nullptr;
            return entity != nullptr && send(*entity);
        };

        if (!Load(gPluginData))
        {
            _core->logLn(LogLevel::Error, "[sv:omp] control core failed to load");
            return;
        }

        _loaded = true;

        if (!SvRelayStart())
            _core->logLn(LogLevel::Error, "[sv:omp] voice relay failed to start");

        _pawn->getEventDispatcher().addEventHandler(this);
        _core->getEventDispatcher().addEventHandler(this);
        _core->getPlayers().getPlayerConnectDispatcher().addEventHandler(this);

        for (INetwork* const network : _core->getNetworks())
            network->getInEventDispatcher().addEventHandler(this);

        _core->printLn("[sv:omp] SampVoice control component ready");
    }

    void onFree(IComponent* const component) override
    {
        if (component == this || component == _pawn)
            Detach();
    }

    void free() override
    {
        Detach();
        delete this;
    }

    void reset() override {}

    // Tick
    // ------------------------------------------------------------

    void onTick(Microseconds, TimePoint) override
    {
        if (!_loaded)
            return;

        ProcessTick();

        // The relay stops by itself only on an error (voice.cfg missing, voice port in use); say so once.
        if (!_relayReported)
        {
            const int code = SvRelayExitCode();
            if (code != kSvRelayRunning)
            {
                _relayReported = true;
                _core->logLn(LogLevel::Error, "[sv:omp] voice relay stopped (code %d), voice chat is down, see voice.log", code);
            }
        }
    }

    // Network
    // ------------------------------------------------------------

    bool onReceiveRPC(IPlayer& peer, const int id, NetworkBitStream& bs) override
    {
        if (!_loaded || id != kConnectRpcId)
            return true;

        const int player_id = peer.getID();
        if (player_id < 0 || player_id >= kMaxPlayers)
            return true;

        RakNet& raknet = RakNet::Instance();

        if (gConnected[player_id] && raknet.OnDisconnect != nullptr)
            raknet.OnDisconnect(static_cast<uword_t>(player_id));

        gConnected[player_id] = true;

        if (raknet.OnConnect != nullptr)
        {
            const uint8_t* payload = nullptr;
            uint8_t length = 0;
            uint8_t copy[256];

            if (FindHandshake(bs.GetData(), static_cast<size_t>(bs.GetNumberOfBytesUsed()), payload, length) && length != 0)
            {
                std::memcpy(copy, payload, length); // the core byte-swaps in place, so never hand out the RPC buffer
                raknet.OnConnect(static_cast<uword_t>(player_id), copy, length);
            }
            else
            {
                raknet.OnConnect(static_cast<uword_t>(player_id), nullptr, 0);
            }
        }

        return true;
    }

    void onPlayerDisconnect(IPlayer& player, PeerDisconnectReason) override
    {
        const int player_id = player.getID();
        if (player_id < 0 || player_id >= kMaxPlayers || !gConnected[player_id])
            return;

        gConnected[player_id] = false;

        if (_loaded && RakNet::Instance().OnDisconnect != nullptr)
            RakNet::Instance().OnDisconnect(static_cast<uword_t>(player_id));
    }

    // Pawn
    // ------------------------------------------------------------

    void onAmxLoad(IPawnScript& script) override
    {
        if (_loaded)
            AmxLoad(script.GetAMX());
    }

    void onAmxUnload(IPawnScript& script) override
    {
        if (_loaded)
            AmxUnload(script.GetAMX());
    }

private:
    void Detach() noexcept
    {
        if (!_loaded)
            return;

        _loaded = false;

        for (INetwork* const network : _core->getNetworks())
            network->getInEventDispatcher().removeEventHandler(this);

        _core->getPlayers().getPlayerConnectDispatcher().removeEventHandler(this);
        _core->getEventDispatcher().removeEventHandler(this);

        if (_pawn != nullptr)
            _pawn->getEventDispatcher().removeEventHandler(this);

        Unload();
        SvRelayStop(); // the relay exits once the control link is closed by Unload()

        RakNet::Instance().SendHook = nullptr;

        _pawn = nullptr;
        gPlayers = nullptr;
        gVehicles = nullptr;
        gObjects = nullptr;
    }

private:
    ICore*          _core   = nullptr;
    IPawnComponent* _pawn   = nullptr;
    bool            _loaded = false;
    bool            _relayReported = false;
};

COMPONENT_ENTRY_POINT()
{
    return new SampVoiceControlComponent();
}
