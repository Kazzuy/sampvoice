#pragma once

// open.mp build: replaces control/libraries/ysf/globals.h. Only the pieces used by the control core.

typedef void(*logprintf_t)(const char* format, ...);

extern logprintf_t logprintf;
extern void* pAMXFunctions;
extern void** ppPluginData;

struct CVector
{
    float fX = 0.0f;
    float fY = 0.0f;
    float fZ = 0.0f;

    CVector() = default;
    CVector(const float x, const float y, const float z) : fX { x }, fY { y }, fZ { z } {}

    CVector operator-(const CVector& other) const
    {
        return CVector { fX - other.fX, fY - other.fY, fZ - other.fZ };
    }
};
