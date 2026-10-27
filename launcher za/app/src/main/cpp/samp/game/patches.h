#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <sstream>

void ApplyFPSPatch(uint8_t fps);
void ApplyGlobalPatches();
void ApplyPatches_level0();
void ApplySAMPPatchesInGame();
void DisableAutoAim();
void InstallVehicleEngineLightPatches();
void readVehiclesAudioSettings();
void PreloadTextures();
void ClearFileCache();
void ForceGameSave();
void CacheFile(const char* filename);
void HideBirds();
void OptimizeReflections(); // To optimize reflections
void EnableOptimizedHeadlights(); // Optimized headlights