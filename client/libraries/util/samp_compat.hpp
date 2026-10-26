/*
    This is a SampVoice project file
    Developer: CyberMor <cyber.mor.2020@gmail.com>

    See more here https://github.com/CyberMor/sampvoice

    Copyright (c) Daniel (CyberMor) 2020 All rights reserved
*/

#pragma once

#include <samp/CRemotePlayer.h>
#include <samp/CLocalPlayer.h>

// Accessors for SAMP structure fields whose position differs in SA:MP 0.3.DL.
// Verified against the 0.3.DL R1 samp.dll, see specs/001-port-03dl-openmp/research.md R-07.

namespace samp_compat
{
    inline SAMP::CPedSA* remote_player_ped(SAMP::CRemotePlayer* const player) noexcept
    {
#if defined(SAMP_DL)
        // 0.3.DL stores the player id at offset 0 and the ped pointer at offset 4
        return *reinterpret_cast<SAMP::CPedSA**>(reinterpret_cast<char*>(player) + 4);
#else
        return player->m_pPed;
#endif
    }

    inline bool local_player_active(SAMP::CLocalPlayer* const player) noexcept
    {
#if defined(SAMP_DL)
        // shortcut: the 0.3.DL offset of CLocalPlayer::m_bIsActive is not verified, so a ped
        // pointer (offset 0 in CLocalPlayer, confirmed in 0.3.DL Spawn) stands in for it
        return player->m_pPed != nullptr;
#else
        return player->m_bIsActive != FALSE;
#endif
    }
}
