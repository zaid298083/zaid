#include "CrossHair.h"
#include "common.h"
#include "../main.h"
#include "playerped.h"
#include "net/netgame.h"
#include "game.h"
#include "sprite2d.h"
#include "Camera.h"

extern CNetGame *pNetGame;

void CCrossHair::Init()
{
    // No longer need to load a texture.
}

void CCrossHair::Render()
{
    if(!pNetGame || !pNetGame->GetPlayerPool() || !pNetGame->GetPlayerPool()->GetLocalPlayer())
        return;

    auto pPed = pNetGame->GetPlayerPool()->GetLocalPlayer()->GetPlayerPed();
    if (!pPed) return;

    CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
    int iCamState = TheCamera.m_aCams[TheCamera.m_nActiveCam].m_nMode;
    int currentWeapon = pPed->GetCurrentWeapon();

    // WEAPONTYPE_SNIPERRIFLE, WEAPONTYPE_COUNTRYRIFLE
    if (currentWeapon == 34 || currentWeapon == 33)
    {
        // Only draw the dot when the camera is in sniper mode.
        if (iCamState == 4) // MODE_SNIPER
        {
            float fScreenX = RsGlobal->maximumWidth / 2.0f;
            float fScreenY = RsGlobal->maximumHeight / 2.0f;
            float fDotSize = 2.0f;

            CRect rect(fScreenX - fDotSize, fScreenY - fDotSize, fScreenX + fDotSize, fScreenY + fDotSize);
            CRGBA color(255, 255, 255, 255); // White dot

            // Disable texture usage for drawing a solid rectangle
            RwRenderStateSet(rwRENDERSTATETEXTURERASTER, RWRSTATE(NULL));
            CSprite2d::DrawRect(rect, color);
        }
    }
}
