#pragma once

#include "../vendor/armhook/patch.h"

class FPSManager {
private:
    static uint8_t targetFPS;
    static bool initialized;
    
public:
    static void Initialize();
    static void SetFPS(uint8_t fps);
    static void Update();
    static void ForceUpdate();
};