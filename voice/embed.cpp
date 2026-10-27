/*
    This is a SampVoice project file
    Developer: CyberMor <cyber.mor.2020@gmail.com>

    See more here https://github.com/CyberMor/sampvoice

    Copyright (c) Daniel (CyberMor) 2020 All rights reserved
*/

// Builds voice/main.cpp unchanged as a library: its `main` becomes `sv_relay::sv_relay_main` and every voice symbol
// (Logger, CommandService, ...) lives in `sv_relay`, because the control core in the same binary defines its own
// `Logger` and friends. Standard and common headers are included first so they stay in the global namespace.

#include <atomic>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <system/types.hpp>
#include <system/clock.hpp>
#include <system/time.hpp>
#include <memory/structures/array.hpp>
#include <memory/structures/bitset.hpp>
#include <memory/structures/pool.hpp>
#include <network/address.hpp>
#include <network/socket.hpp>
#include <other/config.hpp>
#include <other/utils.hpp>
#include <other/version.hpp>
#include <relay.hpp>

namespace sv_relay
{
    #define main sv_relay_main
    #include "main.cpp"
    #undef main
}

namespace
{
    std::thread gRelayThread;
}

bool SvRelayStart() noexcept
{
    if (gRelayThread.joinable()) return false;

    sv_relay::gCommandAbort.store(false, std::memory_order_release);
    gRelayThread = std::thread([]() noexcept -> void
    {
        static const char* const argv[] = { "sampvoice", nullptr };
        sv_relay::sv_relay_main(1, argv);
    });
    return true;
}

void SvRelayStop() noexcept
{
    // Without this the relay would wait forever for the control server to come back.
    sv_relay::gCommandAbort.store(true, std::memory_order_release);
    if (gRelayThread.joinable()) gRelayThread.join();
}
