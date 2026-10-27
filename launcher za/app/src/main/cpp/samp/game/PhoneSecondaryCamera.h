#pragma once

#include "common.h"
#include <atomic>
#include <vector>

struct CCam;

class CPhoneSecondaryCamera {
public:
    enum Mode : int {
        MODE_OFF = 0,
        MODE_FRONT = 1,
        MODE_REAR = 2,
    };

    bool Initialize();
    void Shutdown();
    void SetMode(int mode);
    void AdjustView(float dx, float dy);
    void ApplyMainFirstPerson(CCam* camera);
    void Tick();

    bool IsInitialized() const { return m_camera != nullptr; }
    bool IsActive() const { return m_mode != MODE_OFF; }
    const char* GetFramePath() const { return m_framePath; }

private:
    bool CreateRenderTarget();
    void DestroyRenderTarget();
    bool UpdateTransform();
    bool RenderSecondaryPass();
    bool ReadbackCurrentFrame();
    bool WriteFrameToRaw();

    RwCamera* m_camera = nullptr;
    RwFrame* m_frame = nullptr;
    RwRaster* m_frameBuffer = nullptr;
    RwRaster* m_zBuffer = nullptr;
    RwTexture* m_texture = nullptr;
    RpLight* m_light = nullptr;
    bool m_worldCameraAdded = false;
    std::atomic<int> m_requestedMode{MODE_OFF};
    std::atomic<float> m_yawOffset{0.0f};
    std::atomic<float> m_pitchOffset{0.0f};
    int m_mode = MODE_OFF;
    uint32_t m_lastFrameTick = 0;
    uint32_t m_lastDiagnosticTick = 0;
    uint32_t m_lastWorldDiagnosticTick = 0;
    uint32_t m_lastWriterDiagnosticTick = 0;
    uint32_t m_lastRawDiagnosticTick = 0;
    uint32_t m_lastMainFirstPersonDiagnosticTick = 0;
    bool m_mainFirstPersonActive = false;
    int m_savedMainCameraMode = 0;
    bool m_mainNearClipSaved = false;
    float m_savedMainNearClip = 0.0f;
    std::vector<RwUInt8> m_readbackPixels;
    char m_framePath[512]{};
};

extern CPhoneSecondaryCamera gPhoneSecondaryCamera;
