/*
    This is a SampVoice project file
    Developer: CyberMor <cyber.mor.2020@gmail.com>

    See more here https://github.com/CyberMor/sampvoice

    Copyright (c) Daniel (CyberMor) 2020 All rights reserved
*/

#pragma once

// Host adapter for the SA:MP server (YSF structures). The open.mp component supplies its own
// `util/host.hpp` with the same functions, see control/omp and contracts/host-adapter.md.

#include <cstddef>

#include <ysf/structs.h>
#include <ysf/globals.h>

namespace host
{
    // True when the server game data can be read.
    inline bool Available() noexcept
    {
        return pNetGame != nullptr;
    }

    // True when at least one player is connected.
    inline bool HasPlayers() noexcept
    {
        return pNetGame != nullptr && pNetGame->pPlayerPool != nullptr &&
               pNetGame->pPlayerPool->dwConnectedPlayers != 0;
    }

    // Exclusive upper bound of player ids; call only when HasPlayers() is true.
    inline size_t PlayerLimit() noexcept
    {
        return static_cast<size_t>(pNetGame->pPlayerPool->dwPlayerPoolSize) + 1;
    }

    inline bool PlayerPosition(const size_t player, CVector& position) noexcept
    {
        if (pNetGame == nullptr || pNetGame->pPlayerPool == nullptr) return false;
        if (pNetGame->pPlayerPool->pPlayer[player] == nullptr) return false;

        position = pNetGame->pPlayerPool->pPlayer[player]->vecPosition;
        return true;
    }

    // True when `target` is streamed in for `viewer`.
    inline bool PlayerStreamedIn(const size_t viewer, const size_t target) noexcept
    {
        return pNetGame->pPlayerPool->pPlayer[viewer]->byteStreamedIn[target] != 0;
    }

    inline bool VehiclePosition(const size_t vehicle, CVector& position) noexcept
    {
        if (pNetGame == nullptr || pNetGame->pVehiclePool == nullptr) return false;
        if (pNetGame->pVehiclePool->pVehicle[vehicle] == nullptr) return false;

        position = pNetGame->pVehiclePool->pVehicle[vehicle]->vecPosition;
        return true;
    }

    inline bool VehicleStreamedIn(const size_t viewer, const size_t vehicle) noexcept
    {
        return pNetGame->pPlayerPool->pPlayer[viewer]->byteVehicleStreamedIn[vehicle] != 0;
    }

    inline bool ObjectPosition(const size_t object, CVector& position) noexcept
    {
        if (pNetGame == nullptr || pNetGame->pObjectPool == nullptr) return false;
        if (pNetGame->pObjectPool->pObjects[object] == nullptr) return false;

        position = pNetGame->pObjectPool->pObjects[object]->matWorld.pos;
        return true;
    }

    // Called from AmxLoad: the SA:MP server hands out the game pointer lazily.
    inline void OnAmxLoad() noexcept
    {
        if (pNetGame == nullptr)
            pNetGame = reinterpret_cast<CNetGame*(*)()>(ppPluginData[PLUGIN_DATA_NETGAME])();
    }
}
