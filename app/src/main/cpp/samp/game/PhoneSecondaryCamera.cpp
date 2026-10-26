#include "PhoneSecondaryCamera.h"

#include "../main.h"
#include "game.h"
#include "Scene.h"
#include "Camera.h"
#include "TimeCycle.h"
#include "VisibilityPlugins.h"
#include "Enums/ePedBones.h"
#include "playerped.h"
#include "vehicle.h"
#include "util.h"
#include "log.h"
#include "RW/RenderWare.h"
#include "RW/rwcore.h"
#include "RW/raster.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <unistd.h>
#include <GLES2/gl2.h>

extern CGame* pGame;

CPhoneSecondaryCamera gPhoneSecondaryCamera;

namespace {
constexpr int kSecondaryWidth = 256;
constexpr int kSecondaryHeight = 432;

bool IsFiniteVector(const CVector& value)
{
    return std::isfinite(value.x) && std::isfinite(value.y) &&
           std::isfinite(value.z);
}

uint32_t g_lastFirstPersonOriginDiagnosticTick = 0;

void LogFirstPersonOrigin(const char* source, const CVector& origin,
                         const CVector& fallback)
{
    const uint32_t now = GetTickCount();
    if (g_lastFirstPersonOriginDiagnosticTick != 0 &&
        now - g_lastFirstPersonOriginDiagnosticTick < 1000) {
        return;
    }
    g_lastFirstPersonOriginDiagnosticTick = now;
    LOGI("PHONE_CAM firstperson_origin source=%s offset=(%.2f,%.2f,%.2f)",
         source, origin.x - fallback.x, origin.y - fallback.y,
         origin.z - fallback.z);
}

CVector FirstPersonOrigin(CPlayerPed* player, const CVector& fallback)
{
    if (!player || !player->m_pPed) {
        return fallback;
    }

    // O build de referência atualiza a matriz da cabeça antes de projetar o
    // offset dos olhos. Faça o mesmo, mas nunca aceite uma matriz stale/corrompida.
    player->m_pPed->UpdateRpHAnim();
    player->ProcessHeadMatrix();
    const CVector headOffset(0.35f, 0.10f, 0.10f);
    CVector origin;
    ProjectMatrix(&origin, reinterpret_cast<CMatrix*>(&player->m_HeadBoneMatrix),
                  const_cast<CVector*>(&headOffset));
    const CVector delta = origin - fallback;
    if (IsFiniteVector(origin) && IsFiniteVector(delta) && delta.Magnitude() < 4.0f) {
        LogFirstPersonOrigin("head", origin, fallback);
        return origin;
    }

    // Fallback nativo do próprio GTA: o pescoço é uma posição mundial válida
    // mesmo quando a hierarquia de animação ainda não foi atualizada.
    RwV3d neck{};
    player->m_pPed->GetBonePosition(&neck, BONE_NECK, false);
    const CVector neckOrigin(neck.x, neck.y, neck.z);
    const CVector neckDelta = neckOrigin - fallback;
    if (IsFiniteVector(neckOrigin) && IsFiniteVector(neckDelta) &&
        neckDelta.Magnitude() < 4.0f) {
        LogFirstPersonOrigin("neck", neckOrigin, fallback);
        return neckOrigin;
    }

    const uint32_t now = GetTickCount();
    if (g_lastFirstPersonOriginDiagnosticTick == 0 ||
        now - g_lastFirstPersonOriginDiagnosticTick >= 1000) {
        g_lastFirstPersonOriginDiagnosticTick = now;
        LOGW("PHONE_CAM firstperson_origin fallback=eye height");
    }
    return fallback;
}

CVector FrontSelfieFace(CPlayerPed* player, const CVector& fallback)
{
    if (!player || !player->m_pPed) {
        return fallback;
    }

    RwV3d head{};
    player->m_pPed->GetBonePosition(&head, BONE_HEAD, false);
    const CVector headPosition(head.x, head.y, head.z);
    const CVector delta = headPosition - fallback;
    if (IsFiniteVector(headPosition) && IsFiniteVector(delta) &&
        delta.Magnitude() < 4.0f) {
        return headPosition;
    }

    // O bone pode ainda não estar pronto no primeiro frame após spawn/troca de
    // skin. Nesse caso, não usa uma matriz potencialmente stale: mantém uma
    // altura facial estável derivada da posição do ped.
    return fallback;
}

CVector ActiveCameraForward(const CVector& fallback)
{
    CVector forward = fallback;
    const CVector activeFront = CCamera::GetActiveCamera().Front;
    if (IsFiniteVector(activeFront) && activeFront.Magnitude() > 0.01f) {
        forward = activeFront;
        forward.Normalise();
    }
    return forward;
}

CVector SafeHorizontalForward(const CVector& source)
{
    CVector forward = source;
    forward.z = 0.0f;
    if (forward.Magnitude2D() < 0.01f) {
        forward = CVector(0.0f, 1.0f, 0.0f);
    } else {
        forward.Normalise();
    }
    return forward;
}

void SetCameraFrame(RwFrame* frame, const CVector& position,
                    const CVector& target, bool mirrorHorizontal)
{
    if (!frame) {
        return;
    }

    CVector at = target - position;
    if (at.Magnitude() < 0.001f) {
        at = CVector(0.0f, 1.0f, 0.0f);
    } else {
        at.Normalise();
    }

    const CVector worldUp(0.0f, 0.0f, 1.0f);
    CVector right = at.Cross(worldUp);
    if (right.Magnitude() < 0.001f) {
        right = CVector(1.0f, 0.0f, 0.0f);
    } else {
        right.Normalise();
    }

    // Calcule o vertical antes de inverter a lateral. Recalcular `up` depois
    // de trocar `right` faria o eixo Z apontar para baixo e giraria a imagem.
    CVector up = right.Cross(at);
    if (up.Magnitude() < 0.001f) {
        up = worldUp;
    } else {
        up.Normalise();
    }

    if (mirrorHorizontal) {
        // Corrige somente a lateral do preview traseiro; `up` e `at` ficam
        // preservados, então a imagem não pode virar de cabeça para baixo.
        right = right * -1.0f;
    }

    RwMatrix* matrix = RwFrameGetMatrix(frame);
    if (!matrix) {
        return;
    }

    // Para um RwCamera, os campos seguem a convenção espacial do próprio
    // RenderWare: right = lateral, up = vertical e at = direção de visão.
    // A troca forward/up usada por CMatrix de entidades não deve ser aplicada
    // aqui; o snapshot existente também monta a câmera com o campo `at`
    // apontando para o modelo.
    *RwMatrixGetRight(matrix) = right;
    *RwMatrixGetUp(matrix) = up;
    *RwMatrixGetAt(matrix) = at;
    *RwMatrixGetPos(matrix) = position;
    RwMatrixUpdate(matrix);
    RwFrameUpdateObjects(frame);
}

struct RenderStateGuard
{
    struct Entry {
        RwRenderState state;
        uintptr_t value = 0;
        bool valid = false;
    };

    std::array<Entry, 17> entries{{
        {rwRENDERSTATETEXTURERASTER},
        {rwRENDERSTATETEXTUREADDRESS},
        {rwRENDERSTATETEXTUREPERSPECTIVE},
        {rwRENDERSTATETEXTUREFILTER},
        {rwRENDERSTATEZTESTENABLE},
        {rwRENDERSTATEZWRITEENABLE},
        {rwRENDERSTATESHADEMODE},
        {rwRENDERSTATEVERTEXALPHAENABLE},
        {rwRENDERSTATESRCBLEND},
        {rwRENDERSTATEDESTBLEND},
        {rwRENDERSTATEBORDERCOLOR},
        {rwRENDERSTATEFOGENABLE},
        {rwRENDERSTATEFOGCOLOR},
        {rwRENDERSTATEFOGTYPE},
        {rwRENDERSTATECULLMODE},
        {rwRENDERSTATEALPHATESTFUNCTION},
        {rwRENDERSTATEALPHATESTFUNCTIONREF},
    }};

    void Capture()
    {
        if (!RwRenderStateGet) {
            return;
        }
        for (Entry& entry : entries) {
            uintptr_t value = 0;
            if (RwRenderStateGet(entry.state, &value)) {
                entry.value = value;
                entry.valid = true;
            }
        }
    }

    void Restore() const
    {
        if (!RwRenderStateSet) {
            return;
        }
        for (const Entry& entry : entries) {
            if (entry.valid) {
                RwRenderStateSet(entry.state,
                                 reinterpret_cast<void*>(entry.value));
            }
        }
    }
};

bool IsExcludedEntity(CEntityGTA* entity, CEntityGTA* excludedEntity)
{
    if (!entity || !excludedEntity) {
        return !entity;
    }
    if (entity == excludedEntity) {
        return true;
    }
    // A lista da frustum pode devolver um wrapper diferente para o mesmo
    // objeto GTA. Compare também o objeto/clump RenderWare para não deixar o
    // ped local escapar do filtro da primeira pessoa.
    if (entity->m_pRwObject && excludedEntity->m_pRwObject &&
        entity->m_pRwObject == excludedEntity->m_pRwObject) {
        return true;
    }
    return entity->m_pRwClump && excludedEntity->m_pRwClump &&
           entity->m_pRwClump == excludedEntity->m_pRwClump;
}

int RenderWorldFromCameraFrustum(CEntityGTA* excludedEntity, RwMatrix* cameraMatrix,
                                 int entityCapacity = 2048)
{
#if VER_x32
    (void)excludedEntity;
    (void)cameraMatrix;
    return 0;
#else
    if (!cameraMatrix) {
        return 0;
    }

    // GetObjectsInFrustum é a rotina oficial do mesmo libGTASA. Ela monta a
    // frustum usando a matriz recebida e administra os scan codes internamente;
    // não tocamos nas listas globais de CRenderer nem chamamos ScanWorld direto.
    using GetObjectsInFrustumFunction = int (*)(CEntityGTA** entities,
                                                float distance,
                                                RwMatrix* matrix);
    constexpr uintptr_t kGetObjectsInFrustum = 0x004F76AC;
    auto getObjects = reinterpret_cast<GetObjectsInFrustumFunction>(
        g_libGTASA + kGetObjectsInFrustum);
    if (!getObjects) {
        return 0;
    }

    entityCapacity = std::clamp(entityCapacity, 2048, 8192);
    std::vector<CEntityGTA*> entities(static_cast<size_t>(entityCapacity), nullptr);
    const int count = std::clamp(getObjects(entities.data(), 450.0f,
                                            cameraMatrix), 0, entityCapacity);
    int drawn = 0;
    for (int i = 0; i < count; ++i) {
        CEntityGTA* entity = entities[static_cast<size_t>(i)];
        if (IsExcludedEntity(entity, excludedEntity)) {
            continue;
        }
        RenderEntity(entity);
        ++drawn;
    }
    return drawn;
#endif
}
}

bool CPhoneSecondaryCamera::Initialize()
{
    if (m_camera) {
        return true;
    }
    if (!Scene.m_pRpWorld || !RwCameraBeginUpdate || !RwCameraEndUpdate ||
        !RwCameraCreate || !RwCameraDestroy || !RwFrameCreate || !RwFrameDestroy ||
        !RwRasterCreate || !RwRasterDestroy ||
        !RwTextureCreate || !RwTextureDestroy || !emu_glReadPixels ||
        !RpLightCreate || !RpLightDestroy || !RpWorldAddCamera ||
        !RpWorldAddLight || !RpWorldRemoveLight) {
        LOGE("PHONE_CAM init unavailable world=%d begin=%d end=%d camera=%d raster=%d texture=%d emuRead=%d",
             Scene.m_pRpWorld != nullptr, RwCameraBeginUpdate != nullptr,
             RwCameraEndUpdate != nullptr, RwCameraCreate != nullptr,
             RwRasterCreate != nullptr, RwTextureCreate != nullptr,
             emu_glReadPixels != nullptr);
        return false;
    }

    std::snprintf(m_framePath, sizeof(m_framePath),
                  "/data/user/0/com.nexion.online/cache/samp_phone_secondary.raw");

    m_camera = RwCameraCreate();
    m_frame = RwFrameCreate();
    if (!m_camera || !m_frame) {
        DestroyRenderTarget();
        return false;
    }

    rwObjectHasFrameSetFrame(m_camera, m_frame);

    m_frameBuffer = RwRasterCreate(
        kSecondaryWidth,
        kSecondaryHeight,
        32,
        rwRASTERFORMAT8888 | rwRASTERTYPECAMERATEXTURE
    );
    m_zBuffer = RwRasterCreate(kSecondaryWidth, kSecondaryHeight, 0, rwRASTERTYPEZBUFFER);
    if (!m_frameBuffer || !m_zBuffer) {
        DestroyRenderTarget();
        return false;
    }

    RwCameraSetRaster(m_camera, m_frameBuffer);
    RwCameraSetZRaster(m_camera, m_zBuffer);
    m_texture = RwTextureCreate(m_frameBuffer);
    if (!m_texture) {
        DestroyRenderTarget();
        return false;
    }

    RwTextureSetFilterMode(m_texture, rwFILTERLINEAR);

    m_light = RpLightCreate(2);
    if (!m_light) {
        DestroyRenderTarget();
        return false;
    }
    const RwRGBAReal whiteLight{1.0f, 1.0f, 1.0f, 1.0f};
    RpLightSetColor(m_light, &whiteLight);

    RwCameraSetProjection(m_camera, rwPERSPECTIVE);
    RwCameraSetNearClipPlane(m_camera, 0.08f);
    RwCameraSetFarClipPlane(m_camera, 450.0f);
    const RwV2d viewWindow{
        0.50f,
        0.50f * static_cast<float>(kSecondaryHeight) / static_cast<float>(kSecondaryWidth)
    };
    RwCameraSetViewWindow(m_camera, &viewWindow);

    // O cadastro é necessário para o RenderWare preparar corretamente a
    // câmera, mas nenhum scanner global do CRenderer será chamado com ela.
    RpWorldAddCamera(Scene.m_pRpWorld, m_camera);
    m_worldCameraAdded = true;
    LOGI("PHONE_CAM init ok camera=%p frame=%p color=%p z=%p texture=%p size=%dx%d", 
         m_camera, m_frame, m_frameBuffer, m_zBuffer, m_texture, kSecondaryWidth, kSecondaryHeight);
    return true;
}

void CPhoneSecondaryCamera::DestroyRenderTarget()
{
    if (m_worldCameraAdded && Scene.m_pRpWorld && m_camera) {
        // A câmera fica viva durante a sessão normal. Este caminho existe para
        // falhas de criação e encerramento do processo, quando a remoção é
        // suportada pela implementação RenderWare carregada.
        m_worldCameraAdded = false;
    }

    if (m_light && RpLightDestroy) {
        RpLightDestroy(m_light);
        m_light = nullptr;
    }
    if (m_texture && RwTextureDestroy) {
        RwTextureSetRaster(m_texture, nullptr);
        RwTextureDestroy(m_texture);
        m_texture = nullptr;
    }
    if (m_camera && RwCameraGetFrame(m_camera)) {
        rwObjectHasFrameSetFrame(m_camera, nullptr);
    }
    if (m_frameBuffer && RwRasterDestroy) {
        RwRasterDestroy(m_frameBuffer);
        m_frameBuffer = nullptr;
    }
    if (m_zBuffer && RwRasterDestroy) {
        RwRasterDestroy(m_zBuffer);
        m_zBuffer = nullptr;
    }
    if (m_frame && RwFrameDestroy) {
        RwFrameDestroy(m_frame);
        m_frame = nullptr;
    }
    if (m_camera && RwCameraDestroy) {
        RwCameraDestroy(m_camera);
        m_camera = nullptr;
    }
}

void CPhoneSecondaryCamera::Shutdown()
{
    m_requestedMode.store(MODE_OFF);
    m_mode = MODE_OFF;
    DestroyRenderTarget();
    m_lastFrameTick = 0;
    m_readbackPixels.clear();
    m_framePath[0] = '\0';
}

void CPhoneSecondaryCamera::SetMode(int mode)
{
    if (mode < MODE_OFF || mode > MODE_REAR) {
        mode = MODE_OFF;
    }
    m_requestedMode.store(mode);
    m_yawOffset.store(0.0f);
    m_pitchOffset.store(0.0f);
}

void CPhoneSecondaryCamera::AdjustView(float dx, float dy)
{
    // O telefone envia pixels do gesto. Converta em graus, com limites para
    // impedir que um gesto longo tire a câmera do personagem/mundo.
    float yaw = m_yawOffset.load() + dx * 0.55f;
    float pitch = m_pitchOffset.load() - dy * 0.38f;
    yaw = std::max(-150.0f, std::min(150.0f, yaw));
    pitch = std::max(-55.0f, std::min(55.0f, pitch));
    m_yawOffset.store(yaw);
    m_pitchOffset.store(pitch);
}

void CPhoneSecondaryCamera::ApplyMainFirstPerson(CCam* camera)
{
    if (!camera) {
        return;
    }
    CPlayerPed* player = pGame ? pGame->FindPlayerPed() : nullptr;
    const bool validOnFootPlayer = player && player->m_pPed &&
                                   !player->m_pPed->IsInVehicle();
    if (m_mode != MODE_REAR || !validOnFootPlayer) {
        if (m_mainFirstPersonActive) {
            camera->m_nMode = static_cast<eCamMode>(m_savedMainCameraMode);
            if (m_mainNearClipSaved && Scene.m_pRwCamera) {
                RwCameraSetNearClipPlane(Scene.m_pRwCamera, m_savedMainNearClip);
            }
            m_mainFirstPersonActive = false;
            m_mainNearClipSaved = false;
            LOGI("PHONE_CAM main_firstperson restored mode=%d", m_savedMainCameraMode);
        }
        return;
    }

    if (!m_mainFirstPersonActive) {
        m_savedMainCameraMode = static_cast<int>(camera->m_nMode);
        if (Scene.m_pRwCamera) {
            m_savedMainNearClip = RwCameraGetNearClipPlane(Scene.m_pRwCamera);
            m_mainNearClipSaved = true;
        }
        m_mainFirstPersonActive = true;
        LOGI("PHONE_CAM main_firstperson save mode=%d near=%.3f",
             m_savedMainCameraMode, m_savedMainNearClip);
    }

    if (Scene.m_pRwCamera) {
        RwCameraSetNearClipPlane(Scene.m_pRwCamera, 0.2f);
    }

    // Igual ao build de referência: o processamento normal fornece a direção
    // real da câmera; somente Source/mode são ajustados depois, sem TakeControl.
    CVector viewFront = camera->Front;
    if (!IsFiniteVector(viewFront) || viewFront.Magnitude() < 0.01f) {
        viewFront = SafeHorizontalForward(player->m_pPed->GetForward());
    } else {
        viewFront.Normalise();
    }

    const CVector playerPosition = player->m_pPed->GetPosition();
    const CVector eyeFallback = playerPosition + CVector(0.0f, 0.0f, 1.45f);
    const CVector eyePosition = FirstPersonOrigin(player, eyeFallback);
    camera->Source = eyePosition;

    CVector direction = camera->Source + viewFront * 20.0f - playerPosition;
    direction.z = 0.0f;
    if (direction.Magnitude2D() > 0.01f) {
        float rotation = std::atan2(-direction.x, direction.y) * 57.295776f;
        if (rotation < 0.0f) rotation += 360.0f;
        if (rotation >= 360.0f) rotation -= 360.0f;
        player->SetRotation(rotation);
    }

    camera->m_nMode = MODE_1STPERSON;

    const uint32_t now = GetTickCount();
    if (m_lastMainFirstPersonDiagnosticTick == 0 ||
        now - m_lastMainFirstPersonDiagnosticTick >= 1000) {
        m_lastMainFirstPersonDiagnosticTick = now;
        const int requested = m_requestedMode.load();
        LOGI("PHONE_CAM main_fp activeMode=%d source=(%.2f,%.2f,%.2f) front=(%.2f,%.2f,%.2f) requested=%d",
             static_cast<int>(camera->m_nMode), camera->Source.x, camera->Source.y,
             camera->Source.z, camera->Front.x, camera->Front.y, camera->Front.z,
             requested);
    }
}

bool CPhoneSecondaryCamera::UpdateTransform()
{
    if (!m_camera || !pGame) {
        return false;
    }

    CPlayerPed* player = pGame->FindPlayerPed();
    if (!player || !player->m_pPed) {
        return false;
    }

    const CVector playerPosition = player->m_pPed->GetPosition();
    const CVector forward = SafeHorizontalForward(player->m_pPed->GetForward());

    CVector cameraPosition;
    CVector target;
    CVector viewDirection;
    if (m_mode == MODE_FRONT) {
        // Selfie estável: não usa a matriz do bone neste passe, pois ela pode
        // estar um frame atrasada e deformar o enquadramento. A câmera fica no
        // lado da face e mira uma altura fixa do ped.
        const CVector selfieCenter = playerPosition + CVector(0.0f, 0.0f, 1.15f);
        // Nesta source o eixo forward do ped aponta para o lado oposto ao rosto
        // no modelo RenderWare; -forward coloca a lente diante dele.
        // Câmera nivelada: lente e alvo ficam na mesma altura para não
        // enquadrar o ped de baixo para cima nem inclinar a selfie.
        cameraPosition = selfieCenter - forward * 1.35f;
        target = selfieCenter;
        viewDirection = forward;
    } else {
        // Traseira em primeira pessoa: usa a origem óssea do build de
        // referência, na altura real da cabeça/pescoço, e aponta para frente.
        const CVector eyeFallback = playerPosition + CVector(0.0f, 0.0f, 1.45f);
        cameraPosition = FirstPersonOrigin(player, eyeFallback);
        const CVector firstPersonForward = ActiveCameraForward(forward);
        viewDirection = firstPersonForward;
        target = cameraPosition + viewDirection * 14.0f;
    }
    const float yaw = m_yawOffset.load() * 0.0174532925f;
    const float pitch = m_pitchOffset.load() * 0.0174532925f;
    const float cosYaw = std::cos(yaw);
    const float sinYaw = std::sin(yaw);
    const float rotatedX = viewDirection.x * cosYaw - viewDirection.y * sinYaw;
    const float rotatedY = viewDirection.x * sinYaw + viewDirection.y * cosYaw;
    viewDirection.x = rotatedX;
    viewDirection.y = rotatedY;
    viewDirection.z = std::sin(pitch);
    if (viewDirection.Magnitude() < 0.01f) {
        viewDirection = CVector(0.0f, 1.0f, 0.0f);
    } else {
        viewDirection.Normalise();
    }
    const float targetDistance = m_mode == MODE_FRONT ? 10.0f : 14.0f;
    target = cameraPosition + viewDirection * targetDistance;

    // A frontal usa lente fechada para o close-up; a traseira abre o campo
    // para mostrar a paisagem sem depender da câmera principal.
    const float viewWidth = m_mode == MODE_FRONT ? 0.52f : 0.50f;
    const RwV2d viewWindow{
        viewWidth,
        viewWidth * static_cast<float>(kSecondaryHeight) / static_cast<float>(kSecondaryWidth)
    };
    RwCameraSetViewWindow(m_camera, &viewWindow);

    RwFrame* cameraFrame = RwCameraGetFrame(m_camera);
    SetCameraFrame(cameraFrame, cameraPosition, target, m_mode == MODE_REAR);
    if (cameraFrame && (m_lastDiagnosticTick == 0 ||
                        GetTickCount() - m_lastDiagnosticTick > 1000)) {
        RwMatrix* cameraMatrix = RwFrameGetMatrix(cameraFrame);
        if (cameraMatrix) {
            LOGI("PHONE_CAM transform pos=(%.2f,%.2f,%.2f) target=(%.2f,%.2f,%.2f) at=(%.2f,%.2f,%.2f) up=(%.2f,%.2f,%.2f)",
                 cameraPosition.x, cameraPosition.y, cameraPosition.z,
                 target.x, target.y, target.z,
                 cameraMatrix->at.x, cameraMatrix->at.y, cameraMatrix->at.z,
                 cameraMatrix->up.x, cameraMatrix->up.y, cameraMatrix->up.z);
        }
    }
    return true;
}

bool CPhoneSecondaryCamera::RenderSecondaryPass()
{
    if (!m_camera || !m_frameBuffer || !Scene.m_pRpWorld || !UpdateTransform()) {
        return false;
    }

    RwCamera* mainCamera = Scene.m_pRwCamera;
    if (!mainCamera || mainCamera == m_camera) {
        return false;
    }

    // Não alteramos Scene.m_pRwCamera e não chamamos ScanWorld. O snapshot da
    // source usa exatamente este padrão: BeginUpdate na câmera própria,
    // SetRenderWareCamera temporário e render direto do clump.
    RenderStateGuard renderStateGuard;
    renderStateGuard.Capture();
    // No MainLoop não há um update principal ativo para encerrar. O target
    // secundário é selecionado diretamente e o estado é restaurado ao final.
    CVisibilityPlugins::SetRenderWareCamera(m_camera);
    // O target e o readback já foram comprovados. O fundo usa a cor inferior
    // do céu do ciclo de tempo atual, evitando o preto artificial quando a
    // lista visível não cobre toda a frustum da câmera secundária.
    const CColourSet& colours = CTimeCycle::m_CurrentColours;
    RwRGBA clearColor{
        static_cast<RwUInt8>(std::min<uint16>(colours.m_nSkyBottomRed, 255u)),
        static_cast<RwUInt8>(std::min<uint16>(colours.m_nSkyBottomGreen, 255u)),
        static_cast<RwUInt8>(std::min<uint16>(colours.m_nSkyBottomBlue, 255u)),
        255
    };
    RwCameraClear(m_camera, &clearColor, rwCAMERACLEARIMAGE | rwCAMERACLEARZ);
    if (!RwCameraBeginUpdate(m_camera)) {
        CVisibilityPlugins::SetRenderWareCamera(mainCamera);
        renderStateGuard.Restore();
        LOGE("PHONE_CAM secondary camera begin failed");
        return false;
    }

    DefinedState();
    RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(true));
    RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(true));
    RwRenderStateSet(rwRENDERSTATESHADEMODE, RWRSTATE(rwSHADEMODEGOURAUD));
    RwRenderStateSet(rwRENDERSTATEFOGENABLE, RWRSTATE(false));

    RpWorldAddLight(Scene.m_pRpWorld, m_light);
    CPlayerPed* player = pGame ? pGame->FindPlayerPed() : nullptr;
    bool playerWasVisible = true;
    bool playerHiddenForRearPass = false;
    if (m_mode == MODE_REAR && player && player->m_pPed) {
        playerWasVisible = player->m_pPed->m_bIsVisible;
        player->m_pPed->m_bIsVisible = false;
        playerHiddenForRearPass = true;
    }
    if (m_lastDiagnosticTick == 0 || GetTickCount() - m_lastDiagnosticTick > 1000) {
        m_lastDiagnosticTick = GetTickCount();
        LOGI("PHONE_CAM pass mode=%d player=%p ped=%p clump=%p camera=%p raster=%p", 
             m_mode, player, player ? player->m_pPed : nullptr,
             player && player->m_pPed ? player->m_pPed->m_pRwClump : nullptr,
             m_camera, m_frameBuffer);
    }
    if (player && player->m_pPed) {
        // O ped real já existe no mundo. A rotina RenderEntity reproduz o
        // caminho usado pelo snapshot: PreRender, preparação de visibilidade e
        // RenderOneNonRoad, que termina no callback nativo do clump. Ela não
        // faz ScanWorld e não troca Scene.m_pRwCamera.
        RwFrame* secondaryFrame = RwCameraGetFrame(m_camera);
        RwMatrix* secondaryMatrix = RwFrameGetMatrix(secondaryFrame);
        if (m_mode == MODE_FRONT) {
            // Selfie real: o mundo vira fundo. O ped é desenhado depois com o
            // teste de profundidade desligado para nenhum objeto da cidade
            // atravessar/ficar sobre o rosto por causa do estado deixado por
            // um pipeline de prédio ou árvore.
            const int frontWorldDrawn = RenderWorldFromCameraFrustum(
                player->m_pPed, secondaryMatrix, 8192);
            const uint32_t frontWorldNow = GetTickCount();
            if (m_lastWorldDiagnosticTick == 0 ||
                frontWorldNow - m_lastWorldDiagnosticTick > 1000) {
                m_lastWorldDiagnosticTick = frontWorldNow;
                LOGI("PHONE_CAM front_world drawn=%d capacity=8192", frontWorldDrawn);
            }
            player->m_pPed->UpdateRpHAnim();
            player->m_pPed->UpdateRW();
            player->m_pPed->UpdateRwFrame();
            RwRenderStateSet(rwRENDERSTATEZTESTENABLE, RWRSTATE(false));
            RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, RWRSTATE(false));
            RenderEntity(player->m_pPed);
        } else {
            // A traseira é uma câmera real em primeira pessoa, apontada para
            // a frente. A frustum é calculada com a matriz desta câmera, não
            // com a câmera principal; portanto objetos fora da visão normal
            // também podem aparecer. O ped local é excluído.
            const int drawn = RenderWorldFromCameraFrustum(player->m_pPed,
                                                           secondaryMatrix);
            const uint32_t worldNow = GetTickCount();
            if (m_lastWorldDiagnosticTick == 0 ||
                worldNow - m_lastWorldDiagnosticTick > 1000) {
                m_lastWorldDiagnosticTick = worldNow;
                LOGI("PHONE_CAM secondary_world drawn=%d", drawn);
            }
        }
    }
    RpWorldRemoveLight(Scene.m_pRpWorld, m_light);

    // O readback precisa ocorrer antes do EndUpdate, enquanto o framebuffer
    // desta câmera ainda está vinculado pelo driver RenderWare.
    const bool readbackOk = ReadbackCurrentFrame();
    RwCameraEndUpdate(m_camera);
    if (playerHiddenForRearPass && player && player->m_pPed) {
        player->m_pPed->m_bIsVisible = playerWasVisible;
    }
    CVisibilityPlugins::SetRenderWareCamera(mainCamera);
    renderStateGuard.Restore();
    return readbackOk;
}

bool CPhoneSecondaryCamera::ReadbackCurrentFrame()
{
    constexpr int width = kSecondaryWidth;
    constexpr int height = kSecondaryHeight;
    constexpr size_t pixelBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;

    m_readbackPixels.assign(pixelBytes, 0u);
    if (!emu_glReadPixels) {
        LOGE("PHONE_CAM emu_glReadPixels unavailable");
        m_readbackPixels.clear();
        return false;
    }

    // Não usar glReadPixels/glGetIntegerv diretamente aqui. Esta função é
    // chamada a partir do loop do jogo, enquanto o GTA encaminha as operações
    // OpenGL pelo RenderQueue para o GraphicsThread/contexto correto.
    emu_glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                     m_readbackPixels.data());

    size_t nonZeroSamples = 0;
    size_t nonClearSamples = 0;
    const size_t sampleStride = 16u;
    for (size_t i = 0; i + 2u < m_readbackPixels.size(); i += sampleStride) {
        const bool nonZero = m_readbackPixels[i] != 0u ||
                             m_readbackPixels[i + 1u] != 0u ||
                             m_readbackPixels[i + 2u] != 0u;
        if (nonZero) {
            ++nonZeroSamples;
        }
        if (m_readbackPixels[i] != 0u || m_readbackPixels[i + 1u] != 0u ||
            m_readbackPixels[i + 2u] != 0u) {
            ++nonClearSamples;
        }
    }

    if (m_lastRawDiagnosticTick == 0 ||
        GetTickCount() - m_lastRawDiagnosticTick > 1000) {
        m_lastRawDiagnosticTick = GetTickCount();
        const RwUInt8* firstPixel = m_readbackPixels.data();
        LOGI("PHONE_CAM emu_readpixels queue=1 firstRGBA=%u,%u,%u,%u nonZero=%zu/%zu nonClear=%zu/%zu",
             firstPixel[0], firstPixel[1], firstPixel[2], firstPixel[3],
             nonZeroSamples, m_readbackPixels.size() / sampleStride,
             nonClearSamples, m_readbackPixels.size() / sampleStride);
    }

    return true;
}

bool CPhoneSecondaryCamera::WriteFrameToRaw()
{
    constexpr int width = kSecondaryWidth;
    constexpr int height = kSecondaryHeight;
    constexpr size_t rowBytes = static_cast<size_t>(width) * 4u;
    constexpr size_t expectedBytes = rowBytes * static_cast<size_t>(height);

    if (!m_framePath[0] || m_readbackPixels.size() != expectedBytes) {
        LOGE("PHONE_CAM raw unavailable path=%d readbackBytes=%zu expected=%zu",
             m_framePath[0] != '\0', m_readbackPixels.size(), expectedBytes);
        return false;
    }

    char temporaryPath[sizeof(m_framePath)]{};
    std::snprintf(temporaryPath, sizeof(temporaryPath), "%s.tmp", m_framePath);
    FILE* output = std::fopen(temporaryPath, "wb");
    if (!output) {
        LOGE("PHONE_CAM raw open failed path=%s", temporaryPath);
        return false;
    }

    const uint32_t magic = 0x31574152; // "RAW1"
    const uint32_t rawWidth = static_cast<uint32_t>(width);
    const uint32_t rawHeight = static_cast<uint32_t>(height);
    bool written = std::fwrite(&magic, sizeof(magic), 1, output) == 1 &&
                   std::fwrite(&rawWidth, sizeof(rawWidth), 1, output) == 1 &&
                   std::fwrite(&rawHeight, sizeof(rawHeight), 1, output) == 1;

    size_t nonZeroSamples = 0;
    size_t samples = 0;
    // O readback OpenGL e o Bitmap Java usam origens verticais diferentes.
    // A traseira já foi calibrada com a ordem invertida; a frontal precisa da
    // ordem oposta para não aparecer de cabeça para baixo.
    // Neste pipeline o Bitmap recebe a imagem verticalmente invertida tanto
    // na frontal quanto na traseira; a frontal estava sem a inversão e ficou
    // de cabeça para baixo. A ordem da traseira permanece a mesma.
    const bool flipVertical = m_mode == MODE_FRONT || m_mode == MODE_REAR;
    for (int row = 0; written && row < height; ++row) {
        const int y = flipVertical ? (height - 1 - row) : row;
        const RwUInt8* sourceRow = m_readbackPixels.data() +
                                   static_cast<size_t>(y) * rowBytes;
        written = std::fwrite(sourceRow, rowBytes, 1, output) == 1;
        for (size_t x = 0; x + 2u < rowBytes; x += 16u) {
            ++samples;
            if (sourceRow[x] != 0u || sourceRow[x + 1u] != 0u || sourceRow[x + 2u] != 0u) {
                ++nonZeroSamples;
            }
        }
    }
    std::fflush(output);
    const int closeResult = std::fclose(output);
    if (written && closeResult == 0) {
        written = std::rename(temporaryPath, m_framePath) == 0;
    }

    if (m_lastWriterDiagnosticTick == 0 ||
        GetTickCount() - m_lastWriterDiagnosticTick > 1000) {
        m_lastWriterDiagnosticTick = GetTickCount();
        LOGI("PHONE_CAM raw width=%d height=%d nonZeroSamples=%zu/%zu written=%d",
             width, height, nonZeroSamples, samples, written);
    }
    if (!written) {
        std::remove(temporaryPath);
        LOGE("PHONE_CAM raw write failed path=%s temp=%s", m_framePath, temporaryPath);
    }
    return written;
}

void CPhoneSecondaryCamera::Tick()
{
    const int requestedMode = m_requestedMode.load();
    if (requestedMode != m_mode) {
        m_mode = requestedMode;
        if (m_mode == MODE_OFF) {
            m_lastFrameTick = 0;
            return;
        }
        if (!Initialize()) {
            m_mode = MODE_OFF;
            return;
        }
        m_lastFrameTick = 0;
    }

    if (m_mode == MODE_OFF || !Initialize()) {
        return;
    }

    const uint32_t now = GetTickCount();
    if (m_lastFrameTick != 0 && now - m_lastFrameTick < 100) {
        return;
    }
    m_lastFrameTick = now;

    const bool renderOk = RenderSecondaryPass();
    bool rawOk = false;
    if (renderOk) {
        rawOk = WriteFrameToRaw();
    }
    if (m_lastWriterDiagnosticTick == 0 || now - m_lastWriterDiagnosticTick > 1000) {
        m_lastWriterDiagnosticTick = now;
        LOGI("PHONE_CAM tick renderOk=%d rawOk=%d mode=%d", renderOk, rawOk, m_mode);
    }
}
