#pragma once

// open.mp build: replaces control/libraries/util/raknet.hpp. The component feeds this object from the
// open.mp network events and gives it the function that sends a raw packet to a player.

#include <functional>

#include <system/types.hpp>

struct RakNet
{
    RakNet(const RakNet&) = delete;
    RakNet(RakNet&&) = delete;
    RakNet& operator=(const RakNet&) = delete;
    RakNet& operator=(RakNet&&) = delete;

private:

    RakNet() noexcept = default;
    ~RakNet() noexcept = default;

public:

    static RakNet& Instance() noexcept
    {
        static RakNet instance;
        return instance;
    }

public:

    bool Initialize(const cptr_t) noexcept
    {
        return true;
    }

    void Deinitialize() noexcept {}

    bool SendPacket(const uword_t player, const adr_t packet, const uint_t length) noexcept
    {
        return SendHook != nullptr && SendHook(player, packet, length);
    }

public:

    std::function<void(uword_t, ptr_t, ubyte_t)> OnConnect;
    std::function<bool(uword_t, ptr_t,  uint_t)> OnPacket;
    std::function<void(uword_t)>                 OnDisconnect;

    std::function<bool(uword_t, adr_t, uint_t)>  SendHook;
};
