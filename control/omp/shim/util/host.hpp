#pragma once

// open.mp build: replaces control/libraries/util/host.hpp. The functions are defined in control/omp/Component.cpp.

#include <cstddef>

#include <ysf/globals.h>

namespace host
{
    bool Available() noexcept;
    bool HasPlayers() noexcept;
    size_t PlayerLimit() noexcept;

    bool PlayerPosition(size_t player, CVector& position) noexcept;
    bool PlayerStreamedIn(size_t viewer, size_t target) noexcept;

    bool VehiclePosition(size_t vehicle, CVector& position) noexcept;
    bool VehicleStreamedIn(size_t viewer, size_t vehicle) noexcept;

    bool ObjectPosition(size_t object, CVector& position) noexcept;

    inline void OnAmxLoad() noexcept {}
}
