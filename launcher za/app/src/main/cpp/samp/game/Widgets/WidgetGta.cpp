//
// Created by x1y2z on 03.02.2023.
//

#include "WidgetGta.h"
#include "main.h"
#include "game/game.h"
#include "net/netgame.h"
#include "vendor/armhook/patch.h"
#include "WidgetRegionLook.h"
#include "TouchInterface.h"
#include "WidgetButton.h"

extern CNetGame *pNetGame;
extern CGame *pGame;

CWidgetGta* m_pWidgets[WidgetIDs::NUM_WIDGETS];

enum eWidgetState {
    STATE_NONE,
    STATE_FIXED
};

CWidgetGta* GetWidgetById(WidgetIDs id)
{
    if (CTouchInterface::m_pWidgets && CTouchInterface::m_pWidgets[id])
        return CTouchInterface::m_pWidgets[id];
    return m_pWidgets[id];
}

WidgetIDs GetWidgetTypeFromWidget(CWidgetGta* pWidget)
{
    if(pWidget)
    {
        for (int id = 0; id < WidgetIDs::NUM_WIDGETS; ++id)
        {
            const WidgetIDs widgetId = static_cast<WidgetIDs>(id);
            if (GetWidgetById(widgetId) == pWidget)
                return widgetId;
        }
    }

    return static_cast<WidgetIDs>(-1);
}

void SetWidgetFromId(int idWidget, CWidgetGta* pWidget)
{
    m_pWidgets[idWidget] = pWidget;
}

void SetWidgetFromName(const char* name, CWidgetGta* pWidget)
{
    if(!strcmp("accelerate", name)) SetWidgetFromId(WidgetIDs::WIDGET_ACCELERATE, pWidget);
    if(!strcmp("hud_car", name) || !strcmp("enter_car", name)) SetWidgetFromId(WidgetIDs::WIDGET_ENTER_CAR, pWidget);
    if(!strcmp("brake", name)) SetWidgetFromId(WidgetIDs::WIDGET_BRAKE, pWidget);
    if(!strcmp("attack", name) || !strcmp("punch", name)) SetWidgetFromId(WidgetIDs::WIDGET_ATTACK, pWidget);
    if(!strcmp("handbrake", name)) SetWidgetFromId(WidgetIDs::WIDGET_HANDBRAKE, pWidget);
    if(!strcmp("horn", name)) SetWidgetFromId(WidgetIDs::WIDGET_HORN, pWidget);
    if(!strcmp("cam_toggle", name) || !strcmp("camera", name)) SetWidgetFromId(WidgetIDs::WIDGET_CAM_TOGGLE, pWidget);
    if(!strcmp("targeting", name) || !strcmp("enter_targeting", name)) SetWidgetFromId(WidgetIDs::WIDGET_ENTER_TARGETING, pWidget);
    if(!strcmp("exit_targeting", name)) SetWidgetFromId(WidgetIDs::WIDGET_EXIT_TARGETING, pWidget);
    if(!strcmp("sprint", name) || !strcmp("sprint_and_swim", name)) SetWidgetFromId(WidgetIDs::WIDGET_SPRINT, pWidget);
    if(!strcmp("button_sprint", name) || !strcmp("button_sprint_and_swim", name)) SetWidgetFromId(WidgetIDs::WIDGET_BUTTON_SPRINT, pWidget);
    if(!strcmp("ped_move", name) || !strcmp("move", name)) SetWidgetFromId(WidgetIDs::WIDGET_PED_MOVE, pWidget);
    if(!strcmp("look", name) || !strcmp("look_region", name) || !strcmp("look_area", name))
        SetWidgetFromId(WidgetIDs::WIDGET_LOOK, pWidget);
    if(!strcmp("shoot_look", name) || !strcmp("shootlook", name) || !strcmp("shoot_look_region", name))
        SetWidgetFromId(WidgetIDs::WIDGET_SHOOT_LOOK, pWidget);
    if(!strcmp("radar", name)) SetWidgetFromId(WidgetIDs::WIDGET_RADAR, pWidget);
    if(!strcmp("map", name)) SetWidgetFromId(WidgetIDs::WIDGET_MAP, pWidget);
}

eWidgetState ProcessFixedWidget(CWidgetGta* pWidget)
{
    WidgetIDs widgetType = GetWidgetTypeFromWidget(pWidget);

    CPlayerPed *pPlayerPed = pGame->FindPlayerPed();
    switch(widgetType)
    {
        case -1:
            return STATE_NONE;
        case WidgetIDs::WIDGET_ATTACK:
        case WidgetIDs::WIDGET_SPRINT:
            if(pPlayerPed->IsInVehicle() ||
               pPlayerPed->IsInJetpackMode())
            {
                return STATE_FIXED;
            }
            break;
        case WidgetIDs::WIDGET_ACCELERATE:
        case WidgetIDs::WIDGET_BRAKE:
            if(!pPlayerPed->IsInVehicle() &&
               !pPlayerPed->IsInJetpackMode())
            {
                return STATE_FIXED;
            }
            break;
        case WidgetIDs::WIDGET_ENTER_CAR:
            if(pPlayerPed->IsInJetpackMode()) return STATE_NONE;

            if(pNetGame)
            {
                CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
                if(pVehiclePool)
                {
                    VEHICLEID vehicleId = pVehiclePool->FindNearestToLocalPlayerPed();
                    if(vehicleId == INVALID_VEHICLE_ID) return STATE_FIXED;

                    if(vehicleId != INVALID_VEHICLE_ID)
                    {
                        CVehicle *pVehicle = pVehiclePool->GetAt(vehicleId);
                        if(pVehicle)
                        {
                            if(!pPlayerPed->IsInVehicle() &&
                               pVehicle->m_pVehicle->GetDistanceFromLocalPlayerPed() > 10.0f)
                            {
                                return STATE_FIXED;
                            }
                        }
                    }
                }
            }
            break;
    }

    return STATE_NONE;
}

void CWidgetGta::SetEnabled(bool bEnabled) {
    m_bEnabled = bEnabled;
}

bool (*CWidget__IsTouched)(uintptr_t *thiz, CVector2D *pVecOut);
bool CWidget__IsTouched_hook(uintptr_t *thiz, CVector2D *pVecOut) {
//    if(*thiz == CWidgetGta::pWidgets[WIDGET_POSITION_HORN]) {
//        return true;
//    }
//    if(!CHUD::bIsShow)
//        return false;


    return CWidget__IsTouched(thiz, pVecOut);
}

uintptr_t (*CWidget)(CWidgetButton* thiz, const char* name, uintptr_t* a3, int a4, uintptr_t* a5);
uintptr_t CWidget_hook(CWidgetButton* thiz, const char* name, uintptr_t*  a3, int a4, uintptr_t* a5)
{
    FLog("New Widget: \"%s\" 0x%X", name, thiz-g_libGTASA);

    SetWidgetFromName(name, thiz);
    return CWidget(thiz, name, a3, a4, a5);
}

void (*CWidget__SetEnabled)(CWidgetGta* pWidget, bool bEnabled);
void CWidget__SetEnabled_hook(CWidgetGta* pWidget, bool bEnabled)
{
    if(pNetGame)
    {
        const WidgetIDs widgetType = GetWidgetTypeFromWidget(pWidget);
        // Não dependemos do nome usado no arquivo CFG: qualquer widget nativo
        // criado durante o gameplay fica invisível e desativado. As regiões
        // de câmera e o cam_toggle permanecem habilitados, sem textura, para
        // preservar o look e reutilizar o ciclo nativo de modos.
        if (pWidget) pWidget->m_Sprite.m_pTexture = nullptr;
        const bool keepCameraRegion = widgetType == WidgetIDs::WIDGET_LOOK ||
                                      widgetType == WidgetIDs::WIDGET_SHOOT_LOOK ||
                                      widgetType == WidgetIDs::WIDGET_CAM_TOGGLE;
        // Quando o widget ainda não foi associado pelo nome, não o desativamos:
        // assim as regiões de câmera continuam funcionais. Botões desconhecidos
        // continuam sendo ocultados no CWidgetButton__Update_hook abaixo.
        if (widgetType != static_cast<WidgetIDs>(-1) && !keepCameraRegion)
            bEnabled = false;

        switch(ProcessFixedWidget(pWidget))
        {
            case STATE_NONE: break;
            case STATE_FIXED:
                bEnabled = false;
                break;
        }
    }

    CWidget__SetEnabled(pWidget, bEnabled);
}

void (*CWidgetButton__Update)(CWidgetButton* thiz);
void CWidgetButton__Update_hook(CWidgetButton* thiz) {
    if(pNetGame)
    {
        const WidgetIDs widgetType = GetWidgetTypeFromWidget(thiz);
        const bool keepCameraRegion = widgetType == WidgetIDs::WIDGET_LOOK ||
                                      widgetType == WidgetIDs::WIDGET_SHOOT_LOOK ||
                                      widgetType == WidgetIDs::WIDGET_CAM_TOGGLE;
        if (!keepCameraRegion)
        {
            if (thiz) thiz->m_Sprite.m_pTexture = nullptr;
            return;
        }

        switch(ProcessFixedWidget(thiz))
        {
            case STATE_NONE: break;
            case STATE_FIXED: return;
        }
    }

    CWidgetButton__Update(thiz);
}

void CWidgetGta::InjectHooks() {
    CHook::InstallPLT(g_libGTASA + (VER_x32 ? 0x00671424 : 0x842408), &CWidgetButton__Update_hook, &CWidgetButton__Update);
    CHook::InlineHook("_ZN13CWidgetButtonC2EPKcRK14WidgetPositionjj10HIDMapping", &CWidget_hook, &CWidget);
    CHook::InlineHook("_ZN7CWidget10SetEnabledEb", &CWidget__SetEnabled_hook, &CWidget__SetEnabled);
}

void CWidgetGta::SetTexture(const char *name) {
    m_Sprite.m_pTexture = CUtil::LoadTextureFromDB("mobile", name);
}

bool CWidgetGta::IsReleased(CVector2D *pVecOut) {
    return CHook::CallFunction<bool>(g_libGTASA + (VER_x32 ? 0x002B3484 + 1 : 0x372794), this, pVecOut);
}

bool CWidgetGta::IsTouched(CVector2D *pVecOut) {
    return CHook::CallFunction<bool>(g_libGTASA + (VER_x32 ? 0x002B3324 + 1 : 0x3725D0), this, pVecOut);
}


