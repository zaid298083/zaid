#include "fps_manager.h"
#include "../main.h"

uint8_t FPSManager::targetFPS = 120;
bool FPSManager::initialized = false;

void FPSManager::Initialize() {
    if (!initialized) {
        SetFPS(120);
        initialized = true;
    }
}

void FPSManager::SetFPS(uint8_t fps) {
    targetFPS = fps;
    ForceUpdate();
}

void FPSManager::Update() {
    // Esta função é chamada a cada frame e ajusta a taxa de quadros
    static int counter = 0;
    counter++;
    
    // Ajustamos a taxa de quadros a cada 10 frames para desempenho ideal
    if (counter >= 10) {
        ForceUpdate();
        counter = 0;
    }
}

void FPSManager::ForceUpdate() {
    // Ajuste direto da taxa de quadros usando a função ApplyFPSPatch
    extern void ApplyFPSPatch(uint8_t fps);
    ApplyFPSPatch(targetFPS);
}