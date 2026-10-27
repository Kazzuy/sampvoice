/*
    This is a SampVoice project file
    Developer: CyberMor <cyber.mor.2020@gmail.com>

    See more here https://github.com/CyberMor/sampvoice

    Copyright (c) Daniel (CyberMor) 2020 All rights reserved
*/

#pragma once

// Voice relay embedded in a host process (implemented by voice/embed.cpp).
// Used by the open.mp component so that no separate voice executable has to be started.

// Starts the relay on its own thread. It connects to the control service, so call it after the control core is loaded.
bool SvRelayStart() noexcept;

// Value of SvRelayExitCode() while the relay runs, or before it was started.
constexpr int kSvRelayRunning = 0x7FFFFFFF;

// Exit code of the relay's main once it stopped; while the host is alive that only happens on an error (-1).
// Lets the host report a relay that died, for example because voice.cfg is missing or the voice port is taken.
int SvRelayExitCode() noexcept;

// Waits for the relay thread to finish. Stop the control core first: the relay exits when its control link closes.
void SvRelayStop() noexcept;
