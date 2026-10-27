#include "nexioncontrols.h"

#include "../main.h"
#include "../game/RW/RenderWare.h"
#include "../game/game.h"
#include "../game/pad.h"
#include "../game/playerped.h"
#include "../game/Widgets/TouchInterface.h"
#include "../game/Widgets/WidgetGta.h"
#include "../game/Enums/eWeaponType.h"
#include "../game/vehicle.h"
#include "../net/netgame.h"
#include "../net/playerpool.h"
#include "../net/localplayer.h"
#include "../net/vehiclepool.h"
#include "gui.h"
#include "samp_widgets/nexionsettingsmenu.h"
#include "../settings.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>

extern CGame* pGame;
extern CNetGame* pNetGame;
extern UI* pUI;
extern CSettings* pSettings;
extern CWidgetGta* m_pWidgets[WidgetIDs::NUM_WIDGETS];
extern void (*AND_TouchEvent)(int type, int num, int posX, int posY);

namespace NexionControls
{
    namespace
    {
        constexpr int kButtonTextureWidth = 256;
        constexpr int kButtonTextureHeight = 256;
        constexpr int kButtonBytesPerPixel = 4;
        constexpr float kMaxAxis = 32767.0f;
        constexpr float kTouchSlop = 18.0f;

        enum class ButtonId : uint8_t
        {
            Punch,
            Fire,
            Aim,
            Sprint,
            Slip,
            Accelerate,
            Brake,
            Left,
            Right,
            Horn,
            Vehicle,
            Door,
            Lock,
            Camera,
            Radio,
            Voip,
            FireExtinguish,
            Count
        };

        enum class Action : uint8_t
        {
            Fire,
            Aim,
            Sprint,
            Jump,
            Handbrake,
            Accelerate,
            Brake,
            Horn,
            SecondaryAttack,
            VehicleLeft,
            VehicleRight,
            Count
        };

        struct ButtonTexture
        {
            const char* name = nullptr;
            RwTexture* texture = nullptr;
            uint32_t nextAttempt = 0;
        };

        struct ButtonPlacement
        {
            ButtonId id;
            ImVec2 center;
            float size;
            float opacity;
            bool visible;
        };

        std::array<ButtonTexture, static_cast<size_t>(ButtonId::Count)> g_textures = {{
            {"punch", nullptr, 0},
            {"fire", nullptr, 0},
            {"aim", nullptr, 0},
            {"sprint", nullptr, 0},
            {"slip", nullptr, 0},
            {"accelerate", nullptr, 0},
            {"brake", nullptr, 0},
            {"left", nullptr, 0},
            {"right", nullptr, 0},
            {"horn", nullptr, 0},
            {"vehicle", nullptr, 0},
            {"door", nullptr, 0},
            {"lock", nullptr, 0},
            {"camera", nullptr, 0},
            {"radio", nullptr, 0},
            {"voip", nullptr, 0},
            {"fire_extinguish", nullptr, 0},
        }};

        std::array<int, static_cast<size_t>(ButtonId::Count)> g_buttonPointer = {
            -1, -1, -1, -1, -1, -1, -1, -1, -1,
            -1, -1, -1, -1, -1, -1, -1, -1
        };
        std::array<bool, static_cast<size_t>(Action::Count)> g_actionHeld{};
        std::array<bool, static_cast<size_t>(Action::Count)> g_actionJustPressed{};

        uint8_t g_savedWeapon = WEAPON_UNARMED;
        uint8_t g_savedWeaponSlot = 0;
        bool g_punchWeaponToggle = false;

        bool g_enabled = false;
        int g_movePointer = -1;
        int g_lookPointer = -1;
        ImVec2 g_moveCenter(0.0f, 0.0f);
        float g_moveRadius = 0.0f;
        ImVec2 g_moveAxis(0.0f, 0.0f);
        ImVec2 g_moveDownPosition(0.0f, 0.0f);
        ImVec2 g_lookLast(0.0f, 0.0f);
        ImVec2 g_lastDisplaySize(0.0f, 0.0f);
        float g_lastScale = 1.0f;
        uint32_t g_moveDownAt = 0;
        uint32_t g_sprintDownAt = 0;
        uint32_t g_jumpPulseUntil = 0;
        bool g_moveTapCandidate = false;
        bool g_lastInVehicle = false;
        bool g_hasVehicleState = false;
        bool g_nativeCameraEventForwarded = false;
        int g_nativeCameraPointer = -1;

        size_t ToIndex(ButtonId id)
        {
            return static_cast<size_t>(id);
        }

        size_t ToIndex(Action action)
        {
            return static_cast<size_t>(action);
        }

        void SetAction(Action action, bool held);

        bool StartNativeLookEvent(int pointerIndex, const ImVec2& point)
        {
            if (!AND_TouchEvent || g_lookPointer != -1)
                return false;

            CWidgetGta* nativeLook = nullptr;
            if (CTouchInterface::m_pWidgets)
            {
                nativeLook = CTouchInterface::m_pWidgets[WidgetIDs::WIDGET_SHOOT_LOOK];
                if (!nativeLook)
                    nativeLook = CTouchInterface::m_pWidgets[WidgetIDs::WIDGET_LOOK];
            }
            if (!nativeLook)
            {
                nativeLook = m_pWidgets[WidgetIDs::WIDGET_SHOOT_LOOK];
                if (!nativeLook)
                    nativeLook = m_pWidgets[WidgetIDs::WIDGET_LOOK];
            }
            if (!nativeLook)
                return false;

            const CVector2D center = nativeLook->m_RectPaddedScreen.GetCenter();
            AND_TouchEvent(2, pointerIndex,
                           static_cast<int>(center.x), static_cast<int>(center.y));
            g_lookPointer = pointerIndex;
            g_lookLast = point;
            g_nativeCameraEventForwarded = true;
            return true;
        }

        bool ForwardNativeCameraEvent(int type, int pointerIndex)
        {
            if (!AND_TouchEvent || pointerIndex < 0 || pointerIndex >= 16)
                return false;

            CWidgetGta* nativeCamera = nullptr;
            if (CTouchInterface::m_pWidgets)
                nativeCamera = CTouchInterface::m_pWidgets[WidgetIDs::WIDGET_CAM_TOGGLE];
            if (!nativeCamera)
                nativeCamera = m_pWidgets[WidgetIDs::WIDGET_CAM_TOGGLE];
            if (!nativeCamera)
                return false;

            if (type == 2)
            {
                if (g_nativeCameraPointer != -1 && g_nativeCameraPointer != pointerIndex)
                    return false;
                g_nativeCameraPointer = pointerIndex;
            }
            else if (g_nativeCameraPointer != pointerIndex)
            {
                return false;
            }

            const CVector2D center = nativeCamera->m_RectPaddedScreen.GetCenter();
            AND_TouchEvent(type, pointerIndex,
                           static_cast<int>(center.x), static_cast<int>(center.y));
            g_nativeCameraEventForwarded = true;

            if (type == 1)
                g_nativeCameraPointer = -1;
            return true;
        }

        ImVec2 ClampJoystick(const ImVec2& point)
        {
            const ImVec2 delta = point - g_moveCenter;
            const float length = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (length <= g_moveRadius || length <= 0.001f)
                return point;
            return g_moveCenter + delta * (g_moveRadius / length);
        }

        void UpdateMoveAxis(const ImVec2& point)
        {
            const ImVec2 clamped = ClampJoystick(point);
            g_moveAxis = ImVec2((clamped.x - g_moveCenter.x) / g_moveRadius,
                                (clamped.y - g_moveCenter.y) / g_moveRadius);
            const ImVec2 delta = point - g_moveDownPosition;
            const float travel = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (travel > kTouchSlop)
                g_moveTapCandidate = false;

        }

        void SetAction(Action action, bool held)
        {
            const size_t index = ToIndex(action);
            if (held && !g_actionHeld[index])
                g_actionJustPressed[index] = true;
            g_actionHeld[index] = held;
        }

        void ClearPointer(int pointerIndex)
        {
            if (g_movePointer == pointerIndex)
            {
                g_movePointer = -1;
                g_moveAxis = ImVec2(0.0f, 0.0f);
                g_moveTapCandidate = false;
                g_moveDownAt = 0;
            }
            if (g_lookPointer == pointerIndex)
                g_lookPointer = -1;

            for (size_t i = 0; i < g_buttonPointer.size(); ++i)
            {
                if (g_buttonPointer[i] == pointerIndex)
                    g_buttonPointer[i] = -1;
            }
        }

        bool ReadButtonTexture(ButtonId id)
        {
            ButtonTexture& button = g_textures[ToIndex(id)];
            if (button.texture && button.texture->raster)
                return true;

            const uint32_t now = GetTickCount();
            if (now < button.nextAttempt)
                return false;
            button.nextAttempt = now + 2500;

            if (!g_pszStorage || !RwImageCreate || !RwImageAllocatePixels || !RwImageDestroy ||
                !RwRasterCreate || !RwRasterSetFromImage || !RwRasterDestroy || !RwTextureCreate)
                return false;

            char path[512]{};
            std::snprintf(path, sizeof(path), "%sSAMP/buttons/%s.rgba", g_pszStorage, button.name);
            std::FILE* file = std::fopen(path, "rb");
            if (!file)
            {
                FLog("NexionControls: asset ausente: %s", path);
                return false;
            }

            RwImage* image = RwImageCreate(kButtonTextureWidth, kButtonTextureHeight, 32);
            if (!image || !RwImageAllocatePixels(image) || !image->cpPixels)
            {
                if (image) RwImageDestroy(image);
                std::fclose(file);
                return false;
            }

            bool ok = true;
            const size_t rowBytes = static_cast<size_t>(kButtonTextureWidth) * kButtonBytesPerPixel;
            for (int row = 0; row < kButtonTextureHeight; ++row)
            {
                uint8_t* pixels = image->cpPixels + static_cast<size_t>(row) * image->stride;
                if (std::fread(pixels, 1, rowBytes, file) != rowBytes)
                {
                    ok = false;
                    break;
                }
            }
            std::fclose(file);

            if (!ok)
            {
                RwImageDestroy(image);
                FLog("NexionControls: asset incompleto: %s", path);
                return false;
            }

            RwRaster* raster = RwRasterCreate(kButtonTextureWidth, kButtonTextureHeight, 32,
                                               rwRASTERFORMAT8888 | rwRASTERTYPETEXTURE);
            if (!raster || !RwRasterSetFromImage(raster, image))
            {
                if (raster && RwRasterDestroy) RwRasterDestroy(raster);
                RwImageDestroy(image);
                return false;
            }
            RwImageDestroy(image);

            button.texture = RwTextureCreate(raster);
            if (!button.texture)
            {
                if (RwRasterDestroy) RwRasterDestroy(raster);
                return false;
            }

            RwTextureSetFilterMode(button.texture, rwFILTERLINEAR);
            FLog("NexionControls: textura carregada: %s", button.name);
            return true;
        }

        void DestroyButtonTextures()
        {
            if (!RwTextureDestroy) return;
            for (ButtonTexture& button : g_textures)
            {
                if (button.texture)
                {
                    RwTextureDestroy(button.texture);
                    button.texture = nullptr;
                }
            }
        }

        void DrawButton(ImDrawList* draw, ButtonId id, const ImVec2& center,
                        float size, float opacity, bool active)
        {
            if (!draw) return;
            // O aumento é apenas visual; HandleTouch continua usando `size`
            // original, então a jogabilidade e a área de toque não mudam.
            const float visualSize = active ? size * 1.08f : size;
            const ImVec2 half(visualSize * 0.5f, visualSize * 0.5f);
            const ImVec2 min = center - half;
            const ImVec2 max = center + half;
            const int opacityByte = std::max(0, std::min(255, static_cast<int>(std::lround(opacity * 255.0f))));
            const ImU32 tint = IM_COL32(255, 255, 255,
                                        active ? opacityByte : static_cast<int>(opacityByte * 0.82f));

            const bool weaponMode = id == ButtonId::Punch &&
                                    (g_punchWeaponToggle ||
                                     (pGame && pGame->FindPlayerPed() &&
                                      pGame->FindPlayerPed()->GetCurrentWeapon() != WEAPON_UNARMED));
            if (weaponMode)
            {
                const ImU32 icon = g_punchWeaponToggle ? IM_COL32(76, 213, 151, opacityByte)
                                                       : tint;
                const float r = visualSize * 0.20f;
                const ImVec2 c = center;
                draw->AddCircleFilled(c, visualSize * 0.5f,
                                      IM_COL32(20, 24, 31, opacityByte));
                draw->AddCircle(c, visualSize * 0.5f,
                                IM_COL32(240, 243, 247, static_cast<int>(opacityByte * 0.72f)),
                                32, 2.0f);
                draw->AddRectFilled(ImVec2(c.x - r * 1.25f, c.y - r * 0.52f),
                                    ImVec2(c.x + r * 0.95f, c.y + r * 0.20f), icon, r * 0.16f);
                draw->AddRectFilled(ImVec2(c.x - r * 1.25f, c.y - r * 0.25f),
                                    ImVec2(c.x + r * 1.55f, c.y - r * 0.02f), icon, r * 0.12f);
                draw->AddTriangleFilled(ImVec2(c.x - r * 0.05f, c.y + r * 0.18f),
                                        ImVec2(c.x + r * 0.62f, c.y + r * 0.18f),
                                        ImVec2(c.x + r * 0.28f, c.y + r * 1.30f), icon);
                draw->AddLine(ImVec2(c.x - r * 0.98f, c.y + r * 0.22f),
                              ImVec2(c.x - r * 1.48f, c.y + r * 0.75f), icon, r * 0.34f);
                return;
            }

            if (ReadButtonTexture(id) && g_textures[ToIndex(id)].texture &&
                g_textures[ToIndex(id)].texture->raster)
            {
                draw->AddImage(reinterpret_cast<ImTextureID>(g_textures[ToIndex(id)].texture->raster),
                               min, max, ImVec2(0, 0), ImVec2(1, 1), tint);
            }
            else
            {
                draw->AddCircleFilled(center, visualSize * 0.5f,
                                       IM_COL32(20, 24, 31,
                                                active ? opacityByte : static_cast<int>(opacityByte * 0.65f)));
                draw->AddCircle(center, visualSize * 0.5f,
                                IM_COL32(240, 243, 247, static_cast<int>(opacityByte * 0.72f)),
                                32, 2.0f);
            }
        }

        bool IsInside(const ImVec2& point, const ImVec2& center, float size)
        {
            const float half = size * 0.5f + kTouchSlop;
            return point.x >= center.x - half && point.x <= center.x + half &&
                   point.y >= center.y - half && point.y <= center.y + half;
        }

        bool HasFirearm(CPlayerPed* ped)
        {
            if (!ped) return false;
            const int weapon = ped->GetCurrentWeapon();
            return weapon >= WEAPON_PISTOL && weapon <= WEAPON_MINIGUN;
        }

        CVehicle* GetNearestVehicle(CPlayerPed* ped)
        {
            if (!ped || ped->IsInVehicle() || !pNetGame || !pNetGame->GetVehiclePool()) return nullptr;
            const VEHICLEID vehicleId = pNetGame->GetVehiclePool()->FindNearestToLocalPlayerPed();
            CVehicle* vehicle = pNetGame->GetVehiclePool()->GetAt(vehicleId);
            if (!vehicle || !vehicle->m_pVehicle) return nullptr;
            return vehicle->m_pVehicle->GetDistanceFromLocalPlayerPed() <= 8.0f ? vehicle : nullptr;
        }

        bool IsNearVehicle(CPlayerPed* ped)
        {
            return GetNearestVehicle(ped) != nullptr;
        }

        void ExecuteTap(ButtonId id)
        {
            if (!pGame) return;
            CPlayerPed* ped = pGame->FindPlayerPed();
            if (!ped) return;

            if (id == ButtonId::Vehicle)
            {
                if (ped->IsInVehicle()) return;
                if (!pNetGame || !pNetGame->GetPlayerPool()) return;
                CLocalPlayer* local = pNetGame->GetPlayerPool()->GetLocalPlayer();
                CVehiclePool* vehicles = pNetGame->GetVehiclePool();
                if (!local || !vehicles) return;
                const VEHICLEID vehicleId = vehicles->FindNearestToLocalPlayerPed();
                CVehicle* vehicle = vehicles->GetAt(vehicleId);
                if (!vehicle || !vehicle->m_pVehicle) return;

                if (!vehicle->m_pVehicle->pDriver)
                {
                    local->EnterVehicleAsDriver();
                }
                else if (vehicle->m_pVehicle->GetDistanceFromLocalPlayerPed() < 8.0f)
                {
                    ped->EnterVehicle(vehicle->m_dwGTAId, true);
                    local->SendEnterVehicleNotification(vehicleId, true);
                }
                return;
            }

            if (id == ButtonId::Door)
            {
                if (ped->IsInVehicle())
                {
                    // A task nativa do GTA restaura a câmera com a transição
                    // original. Não forçamos SetBehindPlayer, pois isso corta.
                    ped->ExitCurrentVehicle();
                    return;
                }
                if (!pNetGame || !pNetGame->GetPlayerPool()) return;
                CLocalPlayer* local = pNetGame->GetPlayerPool()->GetLocalPlayer();
                CVehicle* vehicle = GetNearestVehicle(ped);
                if (!local || !vehicle) return;
                const VEHICLEID vehicleId = pNetGame->GetVehiclePool()->FindNearestToLocalPlayerPed();
                ped->EnterVehicle(vehicle->m_dwGTAId, true);
                local->SendEnterVehicleNotification(vehicleId, true);
                return;
            }

            if (id == ButtonId::Lock)
            {
                // O botão fica reservado no HUD, mas a função de trava será
                // configurada em uma etapa posterior.
                return;
            }


            if (id == ButtonId::FireExtinguish)
            {
                ped->ExtinguishFire();
                return;
            }
        }

        bool BeginButton(ButtonId id, int pointerIndex)
        {
            const size_t index = ToIndex(id);
            if (g_buttonPointer[index] != -1) return false;
            g_buttonPointer[index] = pointerIndex;

            switch (id)
            {
                case ButtonId::Punch:
                    // Este botão é exclusivamente guardar/pegar arma. Não
                    // enviamos SecondaryAttack: assim ele não dá soco, não
                    // dispara e não deixa o hook nativo esconder o HUD.
                    if (pGame)
                    {
                        CPlayerPed* ped = pGame->FindPlayerPed();
                        if (ped && !ped->IsInVehicle() && ped->GetCurrentWeapon() != WEAPON_UNARMED)
                        {
                            g_savedWeapon = ped->GetCurrentWeapon();
                            g_savedWeaponSlot = ped->m_pPed->m_nActiveWeaponSlot;
                            ped->m_pPed->m_nActiveWeaponSlot = 0;
                            ped->SetCurrentWeapon(WEAPON_UNARMED);
                            g_punchWeaponToggle = true;
                            FLog("[SA-MP-WEAPON] GUARDAR arma=%u slot=%u", g_savedWeapon, g_savedWeaponSlot);
                            break;
                        }
                        if (ped && !ped->IsInVehicle() && ped->GetCurrentWeapon() == WEAPON_UNARMED &&
                            g_savedWeapon != WEAPON_UNARMED)
                        {
                            ped->m_pPed->m_nActiveWeaponSlot = g_savedWeaponSlot;
                            ped->SetCurrentWeapon(g_savedWeapon);
                            FLog("[SA-MP-WEAPON] PEGAR arma=%u slot=%u", g_savedWeapon, g_savedWeaponSlot);
                            g_savedWeapon = WEAPON_UNARMED;
                            g_savedWeaponSlot = 0;
                            g_punchWeaponToggle = false;
                            break;
                        }
                    }
                    break;
                case ButtonId::Fire:
                    SetAction(Action::Fire, true);
                    break;
                case ButtonId::Aim:
                    SetAction(Action::Aim, true);
                    break;
                case ButtonId::Sprint:
                    g_sprintDownAt = GetTickCount();
                    SetAction(Action::Sprint, true);
                    break;
                case ButtonId::Slip:
                    if (pGame && pGame->FindPlayerPed() && pGame->FindPlayerPed()->IsInVehicle())
                        SetAction(Action::Handbrake, true);
                    break;
                case ButtonId::Accelerate:
                    SetAction(Action::Accelerate, true);
                    break;
                case ButtonId::Brake:
                    SetAction(Action::Brake, true);
                    break;
                case ButtonId::Left:
                    SetAction(Action::VehicleLeft, true);
                    break;
                case ButtonId::Right:
                    SetAction(Action::VehicleRight, true);
                    break;
                case ButtonId::Horn:
                    SetAction(Action::Horn, true);
                    break;
                case ButtonId::Voip:
                    if (pUI && pUI->voicebutton())
                    {
                        pUI->voicebutton()->setRecording(true);
                        pUI->voicebutton()->countdown = 500;
                    }
                    break;
                default:
                    break;
            }
            return true;
        }

        bool EndButton(ButtonId id, int pointerIndex)
        {
            const size_t index = ToIndex(id);
            if (g_buttonPointer[index] != pointerIndex) return false;
            g_buttonPointer[index] = -1;

            switch (id)
            {
                case ButtonId::Punch:
                    if (g_punchWeaponToggle)
                    {
                        // A troca para desarmado permanece após soltar o botão;
                        // o próximo toque restaura a arma salva.
                    }
                    else
                    {
                        SetAction(Action::SecondaryAttack, false);
                    }
                    break;
                case ButtonId::Fire:
                    SetAction(Action::Fire, false);
                    break;
                case ButtonId::Aim:
                    SetAction(Action::Aim, false);
                    break;
                case ButtonId::Sprint:
                    SetAction(Action::Sprint, false);
                    if (!g_lastInVehicle && GetTickCount() - g_sprintDownAt <= 300)
                        g_jumpPulseUntil = GetTickCount() + 180;
                    g_sprintDownAt = 0;
                    break;
                case ButtonId::Slip:
                    if (pGame && pGame->FindPlayerPed() && pGame->FindPlayerPed()->IsInVehicle())
                        SetAction(Action::Handbrake, false);
                    break;
                case ButtonId::Accelerate:
                    SetAction(Action::Accelerate, false);
                    break;
                case ButtonId::Brake:
                    SetAction(Action::Brake, false);
                    break;
                case ButtonId::Left:
                    SetAction(Action::VehicleLeft, false);
                    break;
                case ButtonId::Right:
                    SetAction(Action::VehicleRight, false);
                    break;
                case ButtonId::Horn:
                    SetAction(Action::Horn, false);
                    break;
                case ButtonId::Voip:
                    if (pUI && pUI->voicebutton())
                        pUI->voicebutton()->setRecording(false);
                    break;
                case ButtonId::Vehicle:
                case ButtonId::Door:
                case ButtonId::Lock:
                case ButtonId::Radio:
                case ButtonId::FireExtinguish:
                    ExecuteTap(id);
                    break;
                default:
                    break;
            }
            return true;
        }

        std::array<ButtonPlacement, 17> BuildPlacements(const ImVec2& display, float scale,
                                                          bool inVehicle, bool isDriver,
                                                          bool hasFirearm, bool freeAim,
                                                          bool nearVehicle)
        {
            // Área confortável para toque, igual ao botão original do SA-MP.
            const float button = 104.0f * scale;
            const float small = 68.0f * scale;
            std::array<ButtonPlacement, 17> placements = {{
                {ButtonId::Punch, ImVec2(display.x * 0.72f, display.y * 0.72f), button, 1.0f, !inVehicle},
                {ButtonId::Fire, ImVec2(display.x * 0.89f, display.y * 0.72f), button, 1.0f, !inVehicle && hasFirearm},
                {ButtonId::Aim, ImVec2(display.x * 0.80f, display.y * 0.60f), button, 1.0f, !inVehicle && hasFirearm && freeAim},
                {ButtonId::Sprint, ImVec2(display.x * 0.28f, display.y * 0.83f), button, 1.0f, !inVehicle},
                {ButtonId::Slip, ImVec2(display.x * 0.69f, display.y * 0.87f), button, 1.0f, inVehicle && isDriver},
                {ButtonId::Accelerate, ImVec2(display.x * 0.89f, display.y * 0.86f), button, 1.0f, inVehicle && isDriver},
                {ButtonId::Brake, ImVec2(display.x * 0.79f, display.y * 0.86f), button, 1.0f, inVehicle && isDriver},
                {ButtonId::Left, ImVec2(display.x * 0.09f, display.y * 0.86f), small, 1.0f, inVehicle && isDriver},
                {ButtonId::Right, ImVec2(display.x * 0.19f, display.y * 0.86f), small, 1.0f, inVehicle && isDriver},
                {ButtonId::Horn, ImVec2(display.x * 0.90f, display.y * 0.47f), small, 1.0f, inVehicle && isDriver},
                {ButtonId::Vehicle, ImVec2(display.x * 0.89f, display.y * 0.56f), button, 1.0f, nearVehicle && !inVehicle},
                {ButtonId::Door, ImVec2(display.x * 0.72f, display.y * 0.87f), small, 1.0f,
                              inVehicle || nearVehicle},
                {ButtonId::Lock, ImVec2(display.x * 0.80f, display.y * 0.56f), small, 1.0f,
                              nearVehicle || inVehicle},
                {ButtonId::Camera, ImVec2(display.x * 0.79f, display.y * 0.47f), small, 1.0f, inVehicle && isDriver},
                {ButtonId::Radio, ImVec2(display.x * 0.06f, display.y * 0.60f), small, 1.0f, false},
                {ButtonId::Voip, ImVec2(display.x * 0.16f, display.y * 0.60f), small, 1.0f, true},
                {ButtonId::FireExtinguish, ImVec2(display.x * 0.69f, display.y * 0.47f), small, 1.0f, false},
            }};

            const int context = inVehicle ? kEditorContextVehicle : kEditorContextOnFoot;
            for (ButtonPlacement& placement : placements)
            {
                if (!EditorButtonVisible(context, static_cast<int>(placement.id)))
                    continue;
                GetEditorButton(context, static_cast<int>(placement.id), display,
                                placement.center, placement.size, placement.opacity);
            }
            return placements;
        }
    }

    void Draw(ImDrawList* draw, const ImVec2& displaySize, float scale)
    {
        if (!draw || displaySize.x <= 0.0f || displaySize.y <= 0.0f || !pGame) return;
        CPlayerPed* ped = pGame->FindPlayerPed();
        if (!ped) return;

        g_enabled = true;
        g_lastDisplaySize = displaySize;
        g_lastScale = scale;
        g_moveCenter = ImVec2(displaySize.x * 0.13f, displaySize.y * 0.80f);
        // Área invisível, porém maior, para facilitar o controle do movimento.
        g_moveRadius = 110.0f * scale;

        const bool inVehicle = ped->IsInVehicle();
        const bool isDriver = inVehicle && !ped->IsAPassenger();
        const bool hasFirearm = HasFirearm(ped);
        const bool freeAim = GetNexionTargetingMode() != 0;
        const bool nearVehicle = IsNearVehicle(ped);
        if (g_hasVehicleState && g_lastInVehicle != inVehicle)
        {
            g_movePointer = -1;
            g_moveAxis = ImVec2(0.0f, 0.0f);
            g_moveTapCandidate = false;
            SetAction(Action::Sprint, false);
        }
        g_lastInVehicle = inVehicle;
        g_hasVehicleState = true;
        // O analógico permanece totalmente invisível; sua área de toque continua
        // ativa e foi ampliada por g_moveRadius acima.

        // O LOOK continua sendo uma área de arrasto transparente do lado direito;
        // os widgets nativos não são desenhados, mas a câmera mantém sua lógica.
        const ImVec2 lookMin(displaySize.x * 0.48f, displaySize.y * 0.18f);
        const ImVec2 lookMax(displaySize.x * 0.98f, displaySize.y * 0.88f);
        (void)lookMin;
        (void)lookMax;

        for (const ButtonPlacement& placement : BuildPlacements(displaySize, scale, inVehicle,
                                                                  isDriver, hasFirearm, freeAim, nearVehicle))
        {
            if (!placement.visible) continue;
            const size_t index = ToIndex(placement.id);
            const bool active = g_buttonPointer[index] != -1;
            DrawButton(draw, placement.id, placement.center, placement.size, placement.opacity, active);
        }
    }

    bool HandleTouch(int type, int pointerIndex, int x, int y)
    {
        g_nativeCameraEventForwarded = false;
        if (!g_enabled || pointerIndex < 0 || pointerIndex >= 16) return false;
        // Com o teclado aberto, os controles não interceptam o toque: as teclas
        // cobrem a mesma área dos botões e precisam responder normalmente. No
        // release ainda liberamos os ponteiros ativos para o analógico não
        // ficar "preso" quando o teclado fechar.
        if (pUI && pUI->keyboard() && pUI->keyboard()->visible())
        {
            if (type == 1) ClearPointer(pointerIndex);
            return false;
        }
        const ImVec2 point(static_cast<float>(x), static_cast<float>(y));

        if ((type == 3 || type == 1) && g_nativeCameraPointer == pointerIndex)
            return ForwardNativeCameraEvent(type, pointerIndex);

        if (type == 2)
        {
            CPlayerPed* ped = pGame ? pGame->FindPlayerPed() : nullptr;
            const bool inVehicle = ped && ped->IsInVehicle();
            const bool isDriver = inVehicle && ped && !ped->IsAPassenger();
            const bool hasFirearm = HasFirearm(ped);
            const bool freeAim = GetNexionTargetingMode() != 0;
            const bool nearVehicle = IsNearVehicle(ped);
            for (const ButtonPlacement& placement : BuildPlacements(g_lastDisplaySize, g_lastScale,
                                                                      inVehicle, isDriver, hasFirearm, freeAim, nearVehicle))
            {
                if (placement.visible && IsInside(point, placement.center, placement.size))
                {
                    if (placement.id == ButtonId::Camera)
                    {
                        // O GTA recebe o gesto completo e decide a sequência de
                        // modos no release, como no botão original.
                        ForwardNativeCameraEvent(type, pointerIndex);
                        return true;
                    }
                    if (BeginButton(placement.id, pointerIndex))
                        return true;
                }
            }

            // O analógico é avaliado depois dos botões para que layouts
            // personalizados nunca tornem fire, aim ou sprint inacessíveis.
            if (!g_lastInVehicle && g_movePointer == -1 &&
                (point - g_moveCenter).x * (point - g_moveCenter).x +
                (point - g_moveCenter).y * (point - g_moveCenter).y <=
                (g_moveRadius + kTouchSlop) * (g_moveRadius + kTouchSlop))
            {
                g_movePointer = pointerIndex;
                g_moveDownPosition = point;
                g_moveDownAt = GetTickCount();
                g_moveTapCandidate = true;
                g_moveAxis = ImVec2(0.0f, 0.0f);
                SetAction(Action::Sprint, false);
                return true;
            }
            return false;
        }

        if (type == 3)
        {
            if (g_movePointer == pointerIndex)
            {
                UpdateMoveAxis(point);
                return true;
            }
            if (g_lookPointer == pointerIndex)
            {
                g_lookLast = point;
                return false;
            }
            for (int assigned : g_buttonPointer)
                if (assigned == pointerIndex) return true;
            return false;
        }

        if (type == 1)
        {
            if (g_movePointer == pointerIndex)
            {
                ClearPointer(pointerIndex);
                return true;
            }
            const bool forwardLookRelease = g_lookPointer == pointerIndex;
            for (size_t i = 0; i < g_buttonPointer.size(); ++i)
            {
                if (g_buttonPointer[i] == pointerIndex)
                {
                    EndButton(static_cast<ButtonId>(i), pointerIndex);
                    return !forwardLookRelease;
                }
            }
            return false;
        }

        return false;
    }

    bool ConsumeNativeCameraEvent()
    {
        // Um segundo dedo pode estar usando a região LOOK enquanto o primeiro
        // mantém fire/aim pressionado. Nesse caso, não podemos cancelar o touch
        // nativo ao consumir o evento do botão de arma.
        const bool preserveNativeTouch = g_nativeCameraEventForwarded ||
                                         g_lookPointer != -1;
        g_nativeCameraEventForwarded = false;
        return preserveNativeTouch;
    }

    bool ShouldForwardCameraTouch(int type, int pointerIndex, int x, int y)
    {
        if (!g_enabled || pointerIndex < 0 || pointerIndex >= 16) return false;
        // Com o teclado aberto, nenhum toque vira olhar/câmera: tudo vai para
        // as teclas do teclado.
        if (pUI && pUI->keyboard() && pUI->keyboard()->visible()) return false;

        const ImVec2 point(static_cast<float>(x), static_cast<float>(y));
        // Durante tiro/mira, qualquer ponto livre da tela pode iniciar o LOOK.
        // O HandleTouch já consumiu antes o analógico e os botões Nexion, então
        // o segundo dedo não interfere no disparo nem no movimento.
        const bool weaponActive = g_actionHeld[ToIndex(Action::Fire)] ||
                                  g_actionHeld[ToIndex(Action::Aim)];
        const bool inCameraArea = weaponActive ||
                                  (point.x >= g_lastDisplaySize.x * 0.32f &&
                                   point.x <= g_lastDisplaySize.x * 0.995f &&
                                   point.y >= g_lastDisplaySize.y * 0.02f &&
                                   point.y <= g_lastDisplaySize.y * 0.98f);

        if (type == 2 && g_lookPointer == -1 && inCameraArea)
        {
            if (!StartNativeLookEvent(pointerIndex, point))
            {
                g_lookPointer = pointerIndex;
                g_lookLast = point;
            }
            return true;
        }
        if (type == 3 && g_lookPointer == pointerIndex)
        {
            g_lookLast = point;
            return true;
        }
        if (type == 1 && g_lookPointer == pointerIndex)
        {
            g_lookPointer = -1;
            g_lookLast = ImVec2(0.0f, 0.0f);
            return true;
        }
        return false;
    }

    void Reset()
    {
        g_movePointer = -1;
        g_lookPointer = -1;
        g_moveAxis = ImVec2(0.0f, 0.0f);
        g_moveDownPosition = ImVec2(0.0f, 0.0f);
        g_lookLast = ImVec2(0.0f, 0.0f);
        g_lastScale = 1.0f;
        g_moveDownAt = 0;
        g_sprintDownAt = 0;
        g_jumpPulseUntil = 0;
        g_moveTapCandidate = false;
        g_lastInVehicle = false;
        g_hasVehicleState = false;
        g_nativeCameraEventForwarded = false;
        g_nativeCameraPointer = -1;
        g_buttonPointer.fill(-1);
        g_actionHeld.fill(false);
        g_actionJustPressed.fill(false);
        g_savedWeapon = WEAPON_UNARMED;
        g_savedWeaponSlot = 0;
        g_punchWeaponToggle = false;
        g_enabled = false;
    }

    void SyncPadState()
    {
        if (!g_enabled) return;

        const bool inVehicle = pGame && pGame->FindPlayerPed() && pGame->FindPlayerPed()->IsInVehicle();
        LocalPlayerKeys.wKeyLR = inVehicle ? VehicleLeftRight() : MoveLeftRight();
        LocalPlayerKeys.wKeyUD = inVehicle ? VehicleUpDown() : MoveUpDown();
        // KEY_FIRE tambem carrega o soco: o servidor SA-MP detecta melee pela
        // tecla de fogo no sync on-foot, e sem isso o soco nao replica nem causa dano.
        LocalPlayerKeys.bKeys[ePadKeys::KEY_FIRE] = ActionHeld(static_cast<int>(Action::Fire)) ||
                                                    ActionHeld(static_cast<int>(Action::SecondaryAttack));
        LocalPlayerKeys.bKeys[ePadKeys::KEY_SECONDARY_ATTACK] = ActionHeld(static_cast<int>(Action::SecondaryAttack));
        LocalPlayerKeys.bKeys[ePadKeys::KEY_HANDBRAKE] = ActionHeld(static_cast<int>(Action::Handbrake));
        LocalPlayerKeys.bKeys[ePadKeys::KEY_SPRINT] = ActionHeld(static_cast<int>(Action::Sprint));
        LocalPlayerKeys.bKeys[ePadKeys::KEY_JUMP] = inVehicle
            ? ActionHeld(static_cast<int>(Action::Brake))
            : ActionHeld(static_cast<int>(Action::Jump));
        LocalPlayerKeys.bKeys[ePadKeys::KEY_CROUCH] = ActionHeld(static_cast<int>(Action::Horn));
    }

    bool Enabled()
    {
        return g_enabled;
    }

    int16_t MoveLeftRight()
    {
        return static_cast<int16_t>(std::clamp(g_moveAxis.x, -1.0f, 1.0f) * kMaxAxis);
    }

    int16_t MoveUpDown()
    {
        // O eixo vertical do pad do GTA usa negativo para avançar.
        return static_cast<int16_t>(std::clamp(g_moveAxis.y, -1.0f, 1.0f) * kMaxAxis);
    }

    int16_t VehicleLeftRight()
    {
        const bool left = ActionHeld(static_cast<int>(Action::VehicleLeft));
        const bool right = ActionHeld(static_cast<int>(Action::VehicleRight));
        if (left == right) return 0;
        return left ? static_cast<int16_t>(-kMaxAxis) : static_cast<int16_t>(kMaxAxis);
    }

    int16_t VehicleUpDown()
    {
        const bool accelerate = ActionHeld(static_cast<int>(Action::Accelerate));
        const bool brake = ActionHeld(static_cast<int>(Action::Brake));
        if (accelerate == brake) return 0;
        return accelerate ? static_cast<int16_t>(-kMaxAxis) : static_cast<int16_t>(kMaxAxis);
    }

    bool ActionHeld(int action)
    {
        if (action < 0 || action >= static_cast<int>(Action::Count)) return false;
        if (action == static_cast<int>(Action::Jump) && !g_lastInVehicle &&
            GetTickCount() < g_jumpPulseUntil)
            return true;
        return g_actionHeld[static_cast<size_t>(action)];
    }

    namespace
    {
        float EditorScale(const ImVec2& display)
        {
            return std::max(0.52f, std::min(display.x / 2048.0f, display.y / 921.0f));
        }

        bool EditorDefault(int context, int buttonId, const ImVec2& display,
                           ImVec2& center, float& size)
        {
            const float scale = EditorScale(display);
            const float button = 82.0f * scale;
            const float small = 68.0f * scale;
            center = ImVec2(0.0f, 0.0f);
            size = button;

            if (context == kEditorContextOnFoot)
            {
                switch (buttonId)
                {
                    case 0: center = ImVec2(display.x * 0.72f, display.y * 0.72f); break;
                    case 1: center = ImVec2(display.x * 0.89f, display.y * 0.72f); break;
                    case 2: center = ImVec2(display.x * 0.80f, display.y * 0.60f); break;
                    case 3: center = ImVec2(display.x * 0.28f, display.y * 0.83f); break;
                    case 10: center = ImVec2(display.x * 0.89f, display.y * 0.56f); break;
                    case 11: center = ImVec2(display.x * 0.72f, display.y * 0.87f); size = small; break;
                    case 12: center = ImVec2(display.x * 0.80f, display.y * 0.56f); size = small; break;
                    case 15: center = ImVec2(display.x * 0.16f, display.y * 0.60f); size = small; break;
                    default: return false;
                }
                return true;
            }

            switch (buttonId)
            {
                case 4: center = ImVec2(display.x * 0.69f, display.y * 0.87f); break;
                case 5: center = ImVec2(display.x * 0.89f, display.y * 0.86f); break;
                case 6: center = ImVec2(display.x * 0.79f, display.y * 0.86f); break;
                case 7: center = ImVec2(display.x * 0.09f, display.y * 0.86f); size = small; break;
                case 8: center = ImVec2(display.x * 0.19f, display.y * 0.86f); size = small; break;
                case 9: center = ImVec2(display.x * 0.90f, display.y * 0.47f); size = small; break;
                case 11: center = ImVec2(display.x * 0.72f, display.y * 0.87f); size = small; break;
                case 13: center = ImVec2(display.x * 0.79f, display.y * 0.47f); size = small; break;
                default: return false;
            }
            return true;
        }
    }

    namespace
    {
        bool EditorHudDefault(int context, int elementId, const ImVec2& display,
                              ImVec2& center, float& size)
        {
            const float scale = EditorScale(display);
            const float margin = 38.0f * scale;
            const bool vehicle = context == kEditorContextVehicle;
            const float cell = (vehicle ? 51.0f : 56.0f) * scale;
            const float gap = 8.0f * scale;
            const int statusCount = vehicle ? 6 : 4;
            const float statusWidth = cell * statusCount + gap * (statusCount - 1);

            if (elementId == 0)
            {
                float rowY = display.y - margin - cell;
                if (vehicle)
                {
                    const float panelWidth = 340.0f * scale;
                    const float panelHeight = 124.0f * scale;
                    const float totalHeight = panelHeight + gap + cell;
                    const float panelY = display.y - margin - totalHeight;
                    rowY = panelY + panelHeight + gap;
                }
                center = ImVec2(display.x - margin - statusWidth * 0.5f,
                                rowY + cell * 0.5f);
                size = statusWidth;
                return true;
            }

            if (vehicle && elementId == 1)
            {
                const float panelWidth = 340.0f * scale;
                const float panelHeight = 124.0f * scale;
                const float totalHeight = panelHeight + gap + cell;
                const float panelY = display.y - margin - totalHeight;
                center = ImVec2(display.x - margin - panelWidth * 0.5f,
                                panelY + panelHeight * 0.5f);
                size = panelWidth;
                return true;
            }
            return false;
        }
    }

    const char* EditorButtonName(int buttonId)
    {
        if (buttonId < 0 || buttonId >= kEditorButtonCount)
            return "Controle";
        return g_textures[static_cast<size_t>(buttonId)].name;
    }

    ImTextureID EditorButtonTexture(int buttonId)
    {
        if (buttonId < 0 || buttonId >= kEditorButtonCount)
            return nullptr;
        const ButtonId id = static_cast<ButtonId>(buttonId);
        if (!ReadButtonTexture(id) || !g_textures[static_cast<size_t>(id)].texture ||
            !g_textures[static_cast<size_t>(id)].texture->raster)
            return nullptr;
        return reinterpret_cast<ImTextureID>(g_textures[static_cast<size_t>(id)].texture->raster);
    }

    bool EditorButtonVisible(int context, int buttonId)
    {
        if (context == kEditorContextOnFoot)
        {
            return buttonId == 0 || buttonId == 1 || buttonId == 2 || buttonId == 3 ||
                   buttonId == 10 || buttonId == 11 || buttonId == 12 || buttonId == 15;
        }
        return buttonId == 4 || buttonId == 5 || buttonId == 6 || buttonId == 7 ||
               buttonId == 8 || buttonId == 9 || buttonId == 11 || buttonId == 12 ||
               buttonId == 13;
    }

    void GetEditorButton(int context, int buttonId, const ImVec2& display,
                         ImVec2& center, float& size, float& opacity)
    {
        if (context < 0 || context >= stSettings::NexionControlContextCount ||
            buttonId < 0 || buttonId >= kEditorButtonCount || !pSettings)
            return;

        EditorDefault(context, buttonId, display, center, size);
        const stSettings& settings = pSettings->GetReadOnly();
        const int x = settings.iNexionControlX[context][buttonId];
        const int y = settings.iNexionControlY[context][buttonId];
        const int savedSize = settings.iNexionControlSize[context][buttonId];
        const int savedOpacity = settings.iNexionControlOpacity[context][buttonId];
        const float scale = EditorScale(display);
        if (x >= 0 && x <= 10000) center.x = display.x * (x / 10000.0f);
        if (y >= 0 && y <= 10000) center.y = display.y * (y / 10000.0f);
        if (savedSize >= 36 && savedSize <= 240) size = savedSize * scale;
        opacity = savedOpacity >= 0 && savedOpacity <= 100 ? savedOpacity / 100.0f : 1.0f;
    }

    void SetEditorButton(int context, int buttonId, const ImVec2& display,
                         const ImVec2& center, float size, float opacity)
    {
        if (!pSettings || context < 0 || context >= stSettings::NexionControlContextCount ||
            buttonId < 0 || buttonId >= kEditorButtonCount)
            return;
        const float scale = EditorScale(display);
        stSettings& settings = pSettings->GetWrite();
        settings.iNexionControlX[context][buttonId] = std::max(0, std::min(10000,
            static_cast<int>(std::lround(center.x / std::max(display.x, 1.0f) * 10000.0f))));
        settings.iNexionControlY[context][buttonId] = std::max(0, std::min(10000,
            static_cast<int>(std::lround(center.y / std::max(display.y, 1.0f) * 10000.0f))));
        settings.iNexionControlSize[context][buttonId] = std::max(36, std::min(240,
            static_cast<int>(std::lround(size / scale))));
        settings.iNexionControlOpacity[context][buttonId] = std::max(0, std::min(100,
            static_cast<int>(std::lround(opacity * 100.0f))));
    }

    void ResetEditorContext(int context)
    {
        if (!pSettings || context < 0 || context >= stSettings::NexionControlContextCount)
            return;
        stSettings& settings = pSettings->GetWrite();
        for (int button = 0; button < kEditorButtonCount; ++button)
        {
            settings.iNexionControlX[context][button] = -1;
            settings.iNexionControlY[context][button] = -1;
            settings.iNexionControlSize[context][button] = -1;
            settings.iNexionControlOpacity[context][button] = -1;
        }
    }

    void SaveEditorLayout()
    {
        if (pSettings)
            pSettings->SaveNexionMenuSettings();
    }

    const char* EditorHudName(int elementId)
    {
        static const char* names[] = { "Status", "Velocímetro" };
        if (elementId < 0 || elementId >= kEditorHudElementCount)
            return "HUD";
        return names[elementId];
    }

    bool EditorHudVisible(int context, int elementId)
    {
        if (elementId < 0 || elementId >= kEditorHudElementCount)
            return false;
        return elementId == 0 || (context == kEditorContextVehicle && elementId == 1);
    }

    void GetEditorHud(int context, int elementId, const ImVec2& display,
                      ImVec2& center, float& size, float& opacity)
    {
        if (!pSettings || context < 0 || context >= stSettings::NexionControlContextCount ||
            elementId < 0 || elementId >= kEditorHudElementCount)
            return;
        EditorHudDefault(context, elementId, display, center, size);
        const stSettings& settings = pSettings->GetReadOnly();
        const int x = settings.iNexionHudX[context][elementId];
        const int y = settings.iNexionHudY[context][elementId];
        const int savedSize = settings.iNexionHudSize[context][elementId];
        const int savedOpacity = settings.iNexionHudOpacity[context][elementId];
        const float scale = EditorScale(display);
        if (x >= 0 && x <= 10000) center.x = display.x * (x / 10000.0f);
        if (y >= 0 && y <= 10000) center.y = display.y * (y / 10000.0f);
        if (savedSize >= 36 && savedSize <= 640) size = savedSize * scale;
        opacity = savedOpacity >= 0 && savedOpacity <= 100 ? savedOpacity / 100.0f : 1.0f;
    }

    void SetEditorHud(int context, int elementId, const ImVec2& display,
                      const ImVec2& center, float size, float opacity)
    {
        if (!pSettings || context < 0 || context >= stSettings::NexionControlContextCount ||
            elementId < 0 || elementId >= kEditorHudElementCount)
            return;
        const float scale = EditorScale(display);
        stSettings& settings = pSettings->GetWrite();
        settings.iNexionHudX[context][elementId] = std::max(0, std::min(10000,
            static_cast<int>(std::lround(center.x / std::max(display.x, 1.0f) * 10000.0f))));
        settings.iNexionHudY[context][elementId] = std::max(0, std::min(10000,
            static_cast<int>(std::lround(center.y / std::max(display.y, 1.0f) * 10000.0f))));
        settings.iNexionHudSize[context][elementId] = std::max(36, std::min(640,
            static_cast<int>(std::lround(size / scale))));
        settings.iNexionHudOpacity[context][elementId] = std::max(0, std::min(100,
            static_cast<int>(std::lround(opacity * 100.0f))));
    }

    void ResetEditorHudContext(int context)
    {
        if (!pSettings || context < 0 || context >= stSettings::NexionControlContextCount)
            return;
        stSettings& settings = pSettings->GetWrite();
        for (int element = 0; element < kEditorHudElementCount; ++element)
        {
            settings.iNexionHudX[context][element] = -1;
            settings.iNexionHudY[context][element] = -1;
            settings.iNexionHudSize[context][element] = -1;
            settings.iNexionHudOpacity[context][element] = -1;
        }
    }
}
