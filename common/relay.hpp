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

// Waits for the relay thread to finish. Stop the control core first: the relay exits when its control link closes.
void SvRelayStop() noexcept;
