#include "../main.h"
#include "../game/game.h"
#include "../vendor/armhook/patch.h"
#include <dlfcn.h>
#include "vehicleColoursTable.h"
#include "../settings.h"
extern CSettings* pSettings;

VehicleAudioPropertiesStruct VehicleAudioProperties[20000];
#include "game.h"
#include "World.h"
#include "net/netgame.h"

extern CGame* pGame;

static std::unordered_map<std::string, std::vector<char>> g_fileCache;

void CacheFile(const char* filename)
{
    if(g_fileCache.find(filename) != g_fileCache.end()) return;

    FILE* f = fopen(filename, "rb");
    if(f)
    {
        fseek(f, 0, SEEK_END);
        long size = ftell(f);
        fseek(f, 0, SEEK_SET);

        std::vector<char> buffer(size);
        fread(buffer.data(), 1, size, f);
        fclose(f);

        g_fileCache[filename] = std::move(buffer);
    }
}

void readVehiclesAudioSettings()
{
    char vehicleModel[50];
    int16_t pIndex = 0;

    char buffer[0xFF];
    sprintf(buffer, "%sSAMP/vehicleAudioSettings.cfg", g_pszStorage);

    CacheFile(buffer);

    if(g_fileCache.find(buffer) == g_fileCache.end())
    {
        //Log("Cannot read vehicleAudioSettings.cfg");
        return;
    }

    // Zero VehicleAudioProperties
    memset(VehicleAudioProperties, 0x00, sizeof(VehicleAudioProperties));

    VehicleAudioPropertiesStruct CurrentVehicleAudioProperties;
    memset(&CurrentVehicleAudioProperties, 0x0, sizeof(VehicleAudioPropertiesStruct));

    // Leitura do cache
    const auto& fileData = g_fileCache[buffer];
    std::stringstream ss(std::string(fileData.begin(), fileData.end()));
    std::string line;

    while(std::getline(ss, line))
    {
        if (strncmp(line.c_str(), ";the end", 8) == 0)
            break;

        if (line[0] == ';')
            continue;

        sscanf(line.c_str(), "%s %d %d %d %d %f %f %d %f %d %d %d %d %f",
               vehicleModel,
               &CurrentVehicleAudioProperties.VehicleType,
               &CurrentVehicleAudioProperties.EngineOnSound,
               &CurrentVehicleAudioProperties.EngineOffSound,
               &CurrentVehicleAudioProperties.field_4,
               &CurrentVehicleAudioProperties.field_5,
               &CurrentVehicleAudioProperties.field_6,
               &CurrentVehicleAudioProperties.HornTon,
               &CurrentVehicleAudioProperties.HornHigh,
               &CurrentVehicleAudioProperties.DoorSound,
               &CurrentVehicleAudioProperties.RadioNum,
               &CurrentVehicleAudioProperties.RadioType,
               &CurrentVehicleAudioProperties.field_14,
               &CurrentVehicleAudioProperties.field_16);

        ((void (*)(const char* thiz, int16_t* a2))(g_libGTASA + 0x385E38 + 1))(vehicleModel, &pIndex);
        memcpy(&VehicleAudioProperties[pIndex-400], &CurrentVehicleAudioProperties, sizeof(VehicleAudioPropertiesStruct));
    }
}

void ApplyFPSPatch(uint8_t fps)
{
    uint8_t targetFPS = 120;

#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x005E49E0, (uintptr_t)&targetFPS, 1);
    CHook::WriteMemory(g_libGTASA + 0x005E492E, (uintptr_t)&targetFPS, 1);
#else
    CHook::WriteMemory(g_libGTASA + 0x70A38C + 1, &targetFPS, 1);
    CHook::WriteMemory(g_libGTASA + 0x70A43C + 1, &targetFPS, 1);
    CHook::WriteMemory(g_libGTASA + 0x70A458 + 1, &targetFPS, 1);
#endif

    FLog("New fps limit = %d", targetFPS);
}

void DisableAutoAim()
{
    CHook::RET("_ZN10CPlayerPed22FindWeaponLockOnTargetEv"); // CPed::FindWeaponLockOnTarget
    CHook::RET("_ZN10CPlayerPed26FindNextWeaponLockOnTargetEP7CEntityb"); // CPed::FindNextWeaponLockOnTarget
    CHook::RET("_ZN4CPed21SetWeaponLockOnTargetEP7CEntity"); // CPed::SetWeaponLockOnTarget
}

void ApplySAMPPatchesInGame()
{
    FLog("Installing patches (ingame)..");

    /* Desbloqueio do mapa */
    // CTheZones::ZonesVisited[100]
    memset((void*)(g_libGTASA + (VER_x32 ? 0x0098D252 : 0xC1BF92)), 1, 100);
    // CTheZones::ZonesRevealed
    *(uint32_t*)(g_libGTASA + (VER_x32 ? 0x0098D2B8 : 0xC1BFF8)) = 100;

    // CPlayerPed::CPlayerPed task fix
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x004C36E2, (uintptr_t)"\xE0", 1);
#else
    CHook::WriteMemory(g_libGTASA + 0x5C0BC4, (uintptr_t)"\x34\x00\x80\x52", 4);
#endif
    // radar draw blips
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x0043FE5A : 0x52522C), 2);
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x004409AE : 0x525E14), 2);

    // no vehicle audio processing
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x00553E96 : 0x674610), 2);
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x00561AC2 : 0x682C1C), 2);
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x0056BED4 : 0x68DD0C), 2);

    // Disable in-game radio
    CHook::RET("_ZN20CAERadioTrackManager7ServiceEi");

    // mapa no menu
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x2ABA08 : 0x36A6E8), 2); // texto da legenda do mapa
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x2ABA14 : 0x36A6F8), 2); // ícones da legenda
    CHook::NOP(g_libGTASA + (VER_x32 ? 0x2AB4A6 : 0x36A190), 2); // nome da região

    // Mapa original do GTA (tela de mapa do menu de pausa) totalmente desativado.
    // Ao minimizar o app, o GTA mobile inicializa e renderiza o mapa grande do
    // menu de pausa (Menu_MapInitPause/Update/Render). Com essas rotinas
    // desativadas, nada do mapa original aparece ao voltar do segundo plano.
    CHook::RET("_Z17Menu_MapInitPausev"); // Menu_MapInitPause()
    CHook::RET("_Z14Menu_MapUpdatef");    // Menu_MapUpdate(float)
    CHook::RET("_Z14Menu_MapRenderv");    // Menu_MapRender()

    // Menu de pausa inteiro do GTA mobile desativado. O `MobileMenu::InitForPause`
    // é o ponto que empilha a tela do menu de pausa (mapa + opções/config). Ao
    // minimizar o app, a camada de emulação chama essa função; no retorno, o
    // jogo só volta ao gameplay se a pilha de telas do menu estiver vazia
    // (OS_ApplicationEvent checa isso antes de chamar EndUserPause). Com o
    // InitForPause morto, nenhuma tela de menu é empilhada -> o jogo volta
    // direto ao gameplay no resume, sem tela preta de config.
    CHook::RET("_ZN10MobileMenu12InitForPauseEv"); // MobileMenu::InitForPause()
}

int32_t CWorld__FindPlayerSlotWithPedPointer(CPedGTA* pPlayersPed)
{
    // Otimização - busca rápida
    static CPedGTA* lastPed = nullptr;
    static int32_t lastIndex = -1;

    if(lastPed == pPlayersPed && lastIndex != -1)
        return lastIndex;

    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        if(CWorld::Players[i].m_pPed == pPlayersPed)
        {
            lastPed = pPlayersPed;
            lastIndex = i;
            return i;
        }
    }

    lastPed = nullptr;
    lastIndex = -1;
    return -1;
}

void ApplyPatches_level0()
{
    FLog("ApplyPatches_level0");

    CHook::Write(g_libGTASA + (VER_x32 ? 0x006783C0 : 0x84E7A8), &CWorld::Players);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00679B5C : 0x8516D8), &CWorld::PlayerInFocus);

    CHook::Redirect("_ZN6CWorld28FindPlayerSlotWithPedPointerEPv", &CWorld__FindPlayerSlotWithPedPointer);

// fix aplha raster
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x001AE8DE, (uintptr_t)"\x01\x22", 2);
#else
    CHook::WriteMemory(g_libGTASA + 0x23FDE0, (uintptr_t)"\x22\x00\x80\x52", 4);
#endif

    CHook::RET("_ZN8CClothes4InitEv"); // CClothes::Init()
    CHook::RET("_ZN8CClothes13RebuildPlayerEP10CPlayerPedb"); // CClothes::RebuildPlayer

    CHook::RET("_ZNK35CPedGroupDefaultTaskAllocatorRandom20AllocateDefaultTasksEP9CPedGroupP4CPed"); // AllocateDefaultTasks
    CHook::RET("_ZN6CGlass4InitEv"); // CGlass::Init
    CHook::RET("_ZN8CGarages17Init_AfterRestartEv"); // CGarages::Init_AfterRestart
    CHook::RET("_ZN6CGangs10InitialiseEv"); // CGangs::Initialise
    CHook::RET("_ZN5CHeli9InitHelisEv"); // CHeli::InitHelis(void)
    CHook::RET("_ZN11CFileLoader10LoadPickupEPKc"); // CFileLoader::LoadPickup
    CHook::RET("_ZN14CLoadingScreen15DisplayPCScreenEv"); // Loading screen

    CHook::RET("_ZN10CSkidmarks6UpdateEv"); // CSkidmarks::Update
    CHook::RET("_ZN10CSkidmarks6RenderEv"); // CSkidmarks::Render

    CHook::RET("_ZN14SurfaceInfos_c16CreatesWheelDustEj"); // SurfaceInfos_c::CreatesWheelDust
    CHook::RET("_ZN14SurfaceInfos_c17CreatesWheelSprayEj"); // SurfaceInfos_c::CreatesWheelSpray

    CHook::RET("_ZN4Fx_c12AddWheelDustEP8CVehicle7CVectorhf"); // Fx_c::AddWheelDust
    CHook::RET("_ZN4Fx_c13AddWheelSprayEP8CVehicle7CVectorhhf"); // Fx_c::AddWheelSpray
}

void ApplyGlobalPatches()
{
    FLog("Installing patches..");

    CHook::RET("_ZN17CVehicleModelInfo17SetCarCustomPlateEv"); // default plate

    // CHook::RET("_Z16SaveGameForPause10eSaveTypesPc");

#if VER_x32
    // ícones pretos
    CHook::WriteMemory(g_libGTASA + 0x00442120, (uintptr_t)"\x2C\xE0", 2); // B 0x44217c
    CHook::WriteMemory(g_libGTASA + 0x0044217C, (uintptr_t)"\x30\x46", 2); // mov r0, r6
#else
    // ícones pretos
    CHook::WriteMemory(g_libGTASA + 0x52737C, (uintptr_t)"\x1E\x00\x00\x14", 4); // B 0x5273F4
    CHook::WriteMemory(g_libGTASA + 0x5273F4, (uintptr_t)"\xE1\x03\x14\x2A", 4); // mov w1, w20

    // crash legend
    CHook::NOP(g_libGTASA + 0x36A690, 1);
#endif

    CHook::RET("_ZN12CAudioEngine16StartLoadingTuneEv"); // Sound loading screen

    // DefaultPCSaveFileName
    char* DefaultPCSaveFileName = (char*)(g_libGTASA + (VER_x32 ? 0x006B012C : 0x88CB08));
    memcpy(DefaultPCSaveFileName, "GTASAMP", 8);

#if VER_x32
    CHook::NOP(g_libGTASA + 0x003F61B6, 2);    // CCoronas::RenderSunReflection crash
    CHook::NOP(g_libGTASA + 0x00584884, 2);    // não dar arma ao sair do carro     ( taco, escopeta e etc. )
    CHook::NOP(g_libGTASA + 0x00584850, 2);    // não dar arma ao sair do carro    ( taco, escopeta e etc. )
#else
    CHook::NOP(g_libGTASA + 0x004D8700, 1);  // CCoronas::RenderSunReflection crash
    CHook::NOP(g_libGTASA + 0x006A852C, 1);  // não dar arma ao sair do carro   ( taco, escopeta e etc. )
    CHook::NOP(g_libGTASA + 0x006A84E0, 1);  // não dar arma ao sair do carro  ( taco, escopeta e etc. )
#endif

    CHook::RET("_ZN17CVehicleRecording4LoadEP8RwStreamii"); // CVehicleRecording::Load

    CHook::RET("_ZN18CMotionBlurStreaks6UpdateEv");
    CHook::RET("_ZN7CCamera16RenderMotionBlurEv");

    CHook::RET("_ZN11CPlayerInfo17FindObjectToStealEP4CPed"); // Crash
    CHook::RET("_ZN26CAEGlobalWeaponAudioEntity21ServiceAmbientGunFireEv");    // CAEGlobalWeaponAudioEntity::ServiceAmbientGunFire
    CHook::RET("_ZN30CWidgetRegionSteeringSelection4DrawEv"); // CWidgetRegionSteeringSelection::Draw
    CHook::RET("_ZN23CTaskSimplePlayerOnFoot18PlayIdleAnimationsEP10CPlayerPed");    // CTaskSimplePlayerOnFoot::PlayIdleAnimations
    CHook::RET("_ZN13CCarEnterExit17SetPedInCarDirectEP4CPedP8CVehicleib");    // CCarEnterExit::SetPedInCarDirect
    CHook::RET("_ZN6CRadar10DrawLegendEiii"); // CRadar::DrawLgegend
    CHook::RET("_ZN6CRadar19AddBlipToLegendListEhi"); // CRadar::AddBlipToLegendList

    CHook::RET("_ZN11CAutomobile35CustomCarPlate_BeforeRenderingStartEP17CVehicleModelInfo"); // CAutomobile::CustomCarPlate_BeforeRenderingStart
    CHook::RET("_ZN11CAutomobile33CustomCarPlate_AfterRenderingStopEP17CVehicleModelInfo"); // CAutomobile::CustomCarPlate_AfterRenderingStop
    CHook::RET("_ZN7CCamera8CamShakeEffff"); // CCamera::CamShake - TREMOR DA CÂMERA REMOVIDO
    CHook::RET("_ZN7CEntity23PreRenderForGlassWindowEv"); // CEntity::PreRenderForGlassWindow
    CHook::RET("_ZN8CMirrors16RenderReflBufferEb"); // CMirrors::RenderReflBuffer
    CHook::RET("_ZN4CHud23DrawBustedWastedMessageEv"); // CHud::DrawBustedWastedMessage // WASTED (desativado)

    // CGameLogic::ForceDeathRestart — o GTA chama esta função quando o timer da
    // sequência de morte expira (~4s) para reviver/teleportar o jogador. Desativada
    // para a tela de morte Nexion: o jogador permanece caído até o respawn manual
    // (botão "Renascer"), que reativa o ped pelo cliente (localplayer.cpp) antes
    // do Spawn() completar o respawn.
    {
        void* pForceDeathRestart = dlsym(CHook::lib, "_ZN10CGameLogic17ForceDeathRestartEv");
        if (pForceDeathRestart)
        {
            CHook::RET((uintptr_t)pForceDeathRestart);
        }
        else
        {
            FLog("NexionDeath: simbolo CGameLogic::ForceDeathRestart nao encontrado");
        }
    }

    // CCamera::Fade — sem o fade, a tela não escurece ("apagão") durante a morte;
    // o jogador vê o personagem caído e a tela de morte o tempo todo.
    {
        void* pCameraFade = dlsym(CHook::lib, "_ZN7CCamera4FadeEfs");
        if (pCameraFade)
        {
            CHook::RET((uintptr_t)pCameraFade);
        }
        else
        {
            FLog("NexionDeath: simbolo CCamera::Fade nao encontrado");
        }
    }

    CHook::RET("_ZN4CHud14SetHelpMessageEPKcPtbbbj"); // CHud::SetHelpMessage
    CHook::RET("_ZN4CHud24SetHelpMessageStatUpdateEhtff"); // CHud::SetHelpMessageStatUpdate
    CHook::RET("_ZN6CCheat16ProcessCheatMenuEv"); // CCheat::ProcessCheatMenu
    CHook::RET("_ZN6CCheat13ProcessCheatsEv"); // CCheat::ProcessCheats
    CHook::RET("_ZN6CCheat16AddToCheatStringEc"); // CCheat::AddToCheatString
    CHook::RET("_ZN6CCheat12WeaponCheat1Ev"); // CCheat::WeaponCheat1
    CHook::RET("_ZN6CCheat12WeaponCheat2Ev"); // CCheat::WeaponCheat2
    CHook::RET("_ZN6CCheat12WeaponCheat3Ev"); // CCheat::WeaponCheat3
    CHook::RET("_ZN6CCheat12WeaponCheat4Ev"); // CCheat::WeaponCheat4
    CHook::RET("_ZN8CGarages14TriggerMessageEPcsts"); // CGarages::TriggerMessage

    CHook::RET("_ZN11CPopulation6AddPedE8ePedTypejRK7CVectorb"); // CPopulation::AddPed
    CHook::RET("_ZN6CPlane27DoPlaneGenerationAndRemovalEv"); // CPlane::DoPlaneGenerationAndRemoval

    CHook::RET("_ZN10CEntryExit19GenerateAmbientPedsERK7CVector"); // CEntryExit::GenerateAmbientPeds
    CHook::RET("_ZN8CCarCtrl31GenerateOneEmergencyServicesCarEj7CVector"); // CCarCtrl::GenerateOneEmergencyServicesCar
    CHook::RET("_ZN11CPopulation17AddPedAtAttractorEiP9C2dEffect7CVectorP7CEntityi"); // CPopulation::AddPedAtAttractor crash. wtf stuff?

    CHook::RET("_ZN7CDarkel26RegisterCarBlownUpByPlayerEP8CVehiclei"); // CDarkel__RegisterCarBlownUpByPlayer_hook
    CHook::RET("_ZN7CDarkel25ResetModelsKilledByPlayerEi"); // CDarkel__ResetModelsKilledByPlayer_hook
    CHook::RET("_ZN7CDarkel25QueryModelsKilledByPlayerEii"); // CDarkel__QueryModelsKilledByPlayer_hook
    CHook::RET("_ZN7CDarkel27FindTotalPedsKilledByPlayerEi"); // CDarkel__FindTotalPedsKilledByPlayer_hook
    CHook::RET("_ZN7CDarkel20RegisterKillByPlayerEPK4CPed11eWeaponTypebi"); // CDarkel__RegisterKillByPlayer_hook

#if VER_x32
    CHook::NOP(g_libGTASA + (VER_2_1 ? 0x0040BF26 : 0x3AC8B2), 2);     // CMessages::AddBigMessage from CPlayerInfo::KillPlayer

    CHook::NOP(g_libGTASA + 0x004C5902, 2);  // CCamera::ClearPlayerWeaponMode from CPedSamp::ClearWeaponTarget
    CHook::NOP(g_libGTASA + (VER_2_1 ? 0x003F395E : 0x39840A), 2);    // CStreaming::Shutdown from CGame::Shutdown

    CHook::WriteMemory(g_libGTASA + 0x003F4138, "\x03", 1); // RE3: Fix R* optimization that prevents peds to spawn
#else
    CHook::NOP(g_libGTASA + 0x5C3258, 1);  // CCamera::ClearPlayerWeaponMode from CPlayerPed::ClearWeaponTarget
    CHook::WriteMemory(g_libGTASA + 0x4D644C, "\x1F\x0D\x00\x71", 4); // RE3: Fix R* optimization that prevents peds to spawn
#endif

    CHook::RET("_ZN10CPlayerPed14AnnoyPlayerPedEb"); // CPedSamp::AnnoyPlayerPed
    CHook::RET("_ZN11CPopulation15AddToPopulationEffff");    // CPopulation::AddToPopulation

    CHook::RET("_ZN23CAEPedSpeechAudioEntity11AddSayEventEisjfhhh"); // CPed::Say

    CHook::RET("_ZN10CPedGroups7ProcessEv"); // CPedGroups::Process
    CHook::RET("_ZN21CPedGroupIntelligence7ProcessEv"); // CPedGroupIntelligence::Process
    CHook::RET("_ZN19CPedGroupMembership9SetLeaderEP4CPed"); // CPedGroupMembership::SetLeader
    CHook::RET("_ZN21CPedGroupIntelligence5FlushEv"); // CPedGroupIntelligence::Flush

    // Disabling Shadow Call Lock
    // CHook::RET("_ZN22CRealTimeShadowManager4InitEv"); // CRealTimeShadowManager::Init
    // CHook::RET("_ZN22CRealTimeShadowManager6UpdateEv"); // CRealTimeShadowManager::Update
    // CHook::RET("_ZN22CRealTimeShadowManager20ReturnRealTimeShadowEP15CRealTimeShadow"); // CRealTimeShadowManager::ReturnRealTimeShadow from ~CPhysical
    // CHook::RET("_ZN8CShadows19RenderStaticShadowsEb"); // CShadows::RenderStaticShadows
    // CHook::RET("_ZN8CMirrors16BeforeMainRenderEv"); // CMirrors::BeforeMainRender(void)
    // CHook::RET("_ZN8CMirrors17RenderReflectionsEv");

    CHook::RET("_ZN8CCarCtrl18GenerateRandomCarsEv");
}

// Preloading textures
void PreloadTextures()
{
    FLog("Preloading textures for optimization...");

    // Preloading of used textures
    const char* commonTextures[] = {
            "radar", "hud", "fonts", "vehicle", "ped", "weapon"
    };

    for(const auto& tex : commonTextures)
    {
        // Preloading textures
    }
}

// Clearing the cache
void ClearFileCache()
{
    g_fileCache.clear();
    FLog("File cache cleared");
}

void InstallVehicleEngineLightPatches()
{
    // tipo um fix para as lanternas traseiras
    CHook::WriteMemory(g_libGTASA + 0x591272, (uintptr_t)"\x02", 1);
    CHook::WriteMemory(g_libGTASA + 0x59128E, (uintptr_t)"\x02", 1);
}