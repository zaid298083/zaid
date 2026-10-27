#include <jni.h>
#include <pthread.h>
#include <atomic>
#include <syscall.h>

#include "main.h"
#include "game/game.h"
#include "game/PhoneSecondaryCamera.h"
#include "game/BrlTextureShader.h"

#include "net/netgame.h"
#include "gui/gui.h"
#include "gui/nexioncontrols.h"

#include "playertags.h"
#include "audiostream.h"
#include "java/jniutil.h"
#include <dlfcn.h>
#include <sys/mman.h>
#include <inttypes.h>
#include "StackTrace.h"

// voice
#include "voice_new/Plugin.h"

#include "vendor/armhook/patch.h"
#include "vendor/str_obfuscator/str_obfuscator.hpp"

#include "settings.h"

#include "crashlytics.h"

/*
Peerapol Unarak
*/

JavaVM* javaVM;

char* g_pszStorage = nullptr;

UI* pUI = nullptr;
CGame *pGame = nullptr;

CNetGame *pNetGame = nullptr;
CPlayerTags* pPlayerTags = nullptr;
CSnapShotHelper* pSnapShotHelper = nullptr;
CAudioStream* pAudioStream = nullptr;
CJavaWrapper* pJavaWrapper = nullptr;
CSettings* pSettings = nullptr;
//CVoice* pVoice = nullptr;

MaterialTextGenerator* pMaterialTextGenerator = nullptr;

bool bDebug = false;
bool bGameInited = false;
bool bNetworkInited = false;

uintptr_t g_libGTASA = 0x00;
uintptr_t g_libSAMP = 0x00;

void ApplyGlobalPatches();
void ApplyPatches_level0();
void ApplyMultiTouchPatches();
void InstallGlobalHooks();
void InstallSpecialHooks();
void InitRenderWareFunctions();
void FLog(const char* fmt, ...);
//void MyLog(const char* fmt, ...);

int work = 0;

void ReadSettingFile()
{
    /*char path[255] = { 0 };
    //sprintf(path, "%ssamp.set", g_pszStorage);
    sprintf(path, "%sNickName.ini", g_pszStorage);

    FILE* fp = fopen(path, "r");
    if (fp == NULL) return;

    char buf[1024];

    // nickname
    if (fgets(buf, 1024, fp) != NULL) {
        buf[strcspn(buf, "\n\r")] = 0;
        strcpy(g_nick, buf);
    }

    fclose(fp);*/

    pSettings = new CSettings();

    firebase::crashlytics::SetUserId(pSettings->Get().szNickName);
}

int hashing(const char* str) {
    int hashing = 5381;
    int c;
    while (c = *str++) {
        hashing = ((hashing << 5) + hashing) + c; /* hash * 33 + c */
        if (hashing < 0) hashing = 100;
    }
    if (hashing < 0) hashing = 100;
    return hashing;
}

void DoDebugLoop()
{
    // ...
}

void DoDebugStuff()
{
    // ...

    RwMatrix mat = pGame->FindPlayerPed()->m_pPed->GetMatrix().ToRwMatrix();

    for (int i = 0; i < 100; i++)
    {
        CPlayerPed* ped = pGame->NewPlayer(i, mat.pos.x + i, mat.pos.y, mat.pos.z, 0.0f, false, false);
        //ped->SetCollisionChecking(false);
        //ped->SetGravityProcessing(false);
    }
}
struct sigaction act_old;
struct sigaction act1_old;
struct sigaction act2_old;
struct sigaction act3_old;

extern int g_iLastProcessedSkinCollision, g_iLastProcessedEntityCollision, g_iLastRenderedObject;
extern uintptr_t g_dwLastRetAddrCrash;
void handler(int signum, siginfo_t *info, void* contextPtr)
{
    ucontext* context = (ucontext_t*)contextPtr;

    if (act_old.sa_sigaction)
    {
        act_old.sa_sigaction(signum, info, contextPtr);
    }

    if(info->si_signo == SIGSEGV)
    {
        FLog("SIGSEGV | Fault address: 0x%x", info->si_addr);

        PRINT_CRASH_STATES(context);

        CStackTrace::printBacktrace();
    }

    return;
}

void handler1(int signum, siginfo_t *info, void* contextPtr)
{
    ucontext* context = (ucontext_t*)contextPtr;

    if (act1_old.sa_sigaction)
    {
        act1_old.sa_sigaction(signum, info, contextPtr);
    }

    if(info->si_signo == SIGABRT)
    {
        FLog("SIGABRT | Fault address: 0x%x", info->si_addr);

        PRINT_CRASH_STATES(context);

        CStackTrace::printBacktrace();
    }

    return;
}

void handler2(int signum, siginfo_t *info, void* contextPtr)
{
    ucontext* context = (ucontext_t*)contextPtr;

    if (act2_old.sa_sigaction)
    {
        act2_old.sa_sigaction(signum, info, contextPtr);
    }

    if(info->si_signo == SIGFPE)
    {
        FLog("SIGFPE | Fault address: 0x%x", info->si_addr);

        PRINT_CRASH_STATES(context);

        CStackTrace::printBacktrace();
    }

    return;
}

void handler3(int signum, siginfo_t *info, void* contextPtr)
{
    ucontext* context = (ucontext_t*)contextPtr;

    if (act3_old.sa_sigaction)
    {
        act3_old.sa_sigaction(signum, info, contextPtr);
    }

    if(info->si_signo == SIGBUS)
    {
        FLog("SIGBUS | Fault address: 0x%x", info->si_addr);

        PRINT_CRASH_STATES(context);

        CStackTrace::printBacktrace();
    }

    return;
}

void RestartGameConnection()
{
    // O jogador escolheu reiniciar: volta para a tela inicial do launcher.
    // NÃO escondemos o painel de "CONEXÃO PERDIDA" aqui: o processo vai ser
    // encerrado (System.exit) no Java e o painel continua cobrindo a tela até
    // o app sair — assim o mundo do GTA não aparece atrás da launcher.
    if (pJavaWrapper)
    {
        pJavaWrapper->GoToLauncher();
    }
}

void DoInitStuff()
{
    if (bGameInited == false)
    {
        // Start the loading sequence on Java side
        if (pJavaWrapper) {
            pJavaWrapper->StartLoadingSequence();
        }

        pPlayerTags = new CPlayerTags();
        pSnapShotHelper = new CSnapShotHelper();
        pMaterialTextGenerator = new MaterialTextGenerator();
        pAudioStream = new CAudioStream();
        pAudioStream->Initialize();

        pUI->splashscreen()->setVisible(false);
        pUI->chat()->setVisible(true);
        pUI->buttonpanel()->setVisible(true);

        pGame->Initialize();
        pGame->SetMaxStats();
        pGame->ToggleThePassingOfTime(false);

        // Notify Java that game resources are ready
        if (pJavaWrapper) {
            pJavaWrapper->OnGameReady();
        }

        // voice
        /*LogVoice("[dbg:samp:load] : module loading...");

        for (const auto& loadCallback : Samp::loadCallbacks) {
            if (loadCallback != nullptr) {
                loadCallback();
            }
        }

        Samp::loadStatus = true;*/

        LogVoice("[dbg:samp:load] : module loaded");

        if (bDebug)
        {
            CCamera& TheCamera = *reinterpret_cast<CCamera*>(g_libGTASA + (VER_x32 ? 0x00951FA8 : 0xBBA8D0));
            //TheCamera.Restore();
            CCamera::SetBehindPlayer();
                        // Mesmo no caminho de depuração, nunca restaurar a camada HUD/mapa nativa.
            pGame->DisplayHUD(false);

            pGame->EnableClock(false);

            DoDebugStuff();
        }

        bGameInited = true;
    }

    if (!bNetworkInited && !bDebug)
    {
        // Notify Java that network connection is starting
        if (pJavaWrapper) {
            pJavaWrapper->OnNetworkReady();
        }

        //ReadSettingFile();

        pNetGame = new CNetGame("188.127.241.74", 1596, pSettings->Get().szNickName, pSettings->Get().szPassword);
        bNetworkInited = true;

        FLog("DoInitStuff end");
    }
}

extern "C" {
static std::string AuthJniString(JNIEnv* env, jstring value)
{
    if (!value) return {};
    const char* chars = env->GetStringUTFChars(value, nullptr);
    if (!chars) return {};
    std::string result(chars);
    env->ReleaseStringUTFChars(value, chars);
    return result;
}

JNIEXPORT void JNICALL Java_com_nexion_online_SAMP_nativeOnAuthLogin(JNIEnv* env, jclass, jstring email, jstring password)
{
    if (!pNetGame) return;
    const std::string e = AuthJniString(env, email), p = AuthJniString(env, password);
    if (!e.empty() && !p.empty()) pNetGame->SendAuthLogin(e.c_str(), p.c_str());
}

JNIEXPORT void JNICALL Java_com_nexion_online_SAMP_nativeOnAuthRegister(JNIEnv* env, jclass, jstring fullname, jstring email, jstring password)
{
    if (!pNetGame) return;
    const std::string n = AuthJniString(env, fullname), e = AuthJniString(env, email), p = AuthJniString(env, password);
    if (!n.empty() && !e.empty() && !p.empty()) pNetGame->SendAuthRegister(n.c_str(), e.c_str(), p.c_str());
}

JNIEXPORT void JNICALL Java_com_nexion_online_SAMP_initializeSAMP(JNIEnv *pEnv, jobject thiz)
{
    pJavaWrapper = new CJavaWrapper(pEnv, thiz);

}
JNIEXPORT void JNICALL Java_com_nexion_online_GTASA_onApplicationPaused(JNIEnv *pEnv, jobject thiz)
{
    NexionControls::Reset();
}

JNIEXPORT void JNICALL Java_com_nexion_online_SAMP_onInputEnd(JNIEnv *pEnv, jobject thiz, jbyteArray str)
{
    if(pUI)
    {
        pUI->keyboard()->sendForGB(pEnv, thiz, str);
    }
}
JNIEXPORT void JNICALL Java_com_nexion_online_SAMP_onEventBackPressed(JNIEnv *pEnv, jobject thiz)
{
    if(pSettings)
    {
        if(pSettings->Get().iAndroidKeyboard)
            pJavaWrapper->HideKeyboard();
    }
}

JNIEXPORT void JNICALL Java_com_nexion_online_SAMP_nativeSetKeyboardOffset(JNIEnv *pEnv, jobject thiz, jint offset)
{
    if(pUI)
    {
        pUI->setKeyboardOffset((int)offset);
    }
}

JNIEXPORT void JNICALL Java_com_nexion_online_SAMP_nativeUpdateKeyboardInput(JNIEnv *pEnv, jobject thiz, jstring text)
{
    if(!pUI || !pUI->keyboard() || !pUI->keyboard()->input()) return;

    const char* chars = pEnv->GetStringUTFChars(text, nullptr);
    if(!chars) return;

    std::string input(chars);
    pUI->keyboard()->input()->setInputString(input);

    pEnv->ReleaseStringUTFChars(text, chars);
}
// ============================================================
// WebUI BANCO: ponte JS -> nativo -> servidor (packet 251)
// ============================================================
JNIEXPORT void JNICALL Java_com_nexion_online_SAMP_nativeOnBankAction(JNIEnv* pEnv, jclass thiz, jstring actionType, jstring dataJson, jint source)
{
    if (!pNetGame) return;

    const char* szAction = pEnv->GetStringUTFChars(actionType, nullptr);
    const char* szJson = pEnv->GetStringUTFChars(dataJson, nullptr);
    if (!szAction || !szJson) {
        if (szAction) pEnv->ReleaseStringUTFChars(actionType, szAction);
        if (szJson) pEnv->ReleaseStringUTFChars(dataJson, szJson);
        return;
    }

    // Parse mínimo do JSON recebido do JS (campos conhecidos)
    int amount = 0, pin = 0, target = 0;
    const char* pAmount = strstr(szJson, "\"amount\"");
    const char* pPin = strstr(szJson, "\"pin\"");
    const char* pTarget = strstr(szJson, "\"target\"");
    if (pAmount) sscanf(pAmount, "%*[^0-9]%d", &amount);
    if (pPin) sscanf(pPin, "%*[^0-9-]%d", &pin);
    if (pTarget) sscanf(pTarget, "%*[^0-9]%d", &target);

    if (!strcmp(szAction, "register_pin")) {
        pNetGame->SendBankRegisterPin(pin);
    } else if (!strcmp(szAction, "verify_pin")) {
        pNetGame->SendBankVerifyPin(pin);
    } else if (!strcmp(szAction, "withdraw")) {
        pNetGame->SendBankTransaction(0, amount, target, pin, (uint8_t)source);
    } else if (!strcmp(szAction, "deposit")) {
        pNetGame->SendBankTransaction(1, amount, target, pin, (uint8_t)source);
    } else if (!strcmp(szAction, "transfer")) {
        pNetGame->SendBankTransaction(2, amount, target, pin, (uint8_t)source);
    }

    FLog("[BANK] action=%s amount=%d pin=%d target=%d", szAction, amount, pin, target);

    pEnv->ReleaseStringUTFChars(actionType, szAction);
    pEnv->ReleaseStringUTFChars(dataJson, szJson);
}

JNIEXPORT void JNICALL Java_com_nexion_online_SAMP_nativeOnBankClose(JNIEnv* pEnv, jclass thiz)
{
    if (pJavaWrapper) {
        pJavaWrapper->HideBank();
    }
}

JNIEXPORT void JNICALL Java_com_nexion_online_SAMP_nativeOnPhoneClosed(JNIEnv* pEnv, jclass thiz)
{
    gPhoneSecondaryCamera.SetMode(CPhoneSecondaryCamera::MODE_OFF);
}

JNIEXPORT void JNICALL Java_com_nexion_online_SAMP_nativeSetGtaCameraMode(
        JNIEnv* pEnv, jclass thiz, jint mode)
{
    gPhoneSecondaryCamera.SetMode(static_cast<int>(mode));
}

JNIEXPORT void JNICALL Java_com_nexion_online_SAMP_nativeAdjustGtaCamera(
        JNIEnv* pEnv, jclass thiz, jfloat dx, jfloat dy)
{
    gPhoneSecondaryCamera.AdjustView(static_cast<float>(dx), static_cast<float>(dy));
}

JNIEXPORT jstring JNICALL Java_com_nexion_online_SAMP_nativeCaptureSecondaryCameraFrame(
        JNIEnv* pEnv, jobject thiz)
{
    const char* path = gPhoneSecondaryCamera.GetFramePath();
    if (!path || !path[0] || access(path, R_OK) != 0) {
        return nullptr;
    }
    return pEnv->NewStringUTF(path);
}

// Botão flutuante clicado: pede pro servidor abrir o banco
JNIEXPORT void JNICALL Java_com_nexion_online_SAMP_nativeOnBankInteract(JNIEnv* pEnv, jclass thiz)
{
    if (pNetGame) {
        pNetGame->SendBankOpenRequest();
    }
}

JNIEXPORT void JNICALL Java_com_nexion_online_ui_dialog_DialogManager_sendDialogResponse(JNIEnv* pEnv, jobject thiz, jint i3, jint i, jint i2, jbyteArray str)
{
    jboolean isCopy = true;

    jbyte* pMsg = pEnv->GetByteArrayElements(str, &isCopy);
    jsize length = pEnv->GetArrayLength(str);

    std::string szStr((char*)pMsg, length);

    if(pNetGame) {
        pNetGame->SendDialogResponse(i, i3, i2, (char*)szStr.c_str());
        //pGame->FindPlayerPed()->TogglePlayerControllableWithoutLock(true);
    }

    pEnv->ReleaseByteArrayElements(str, pMsg, JNI_ABORT);
}
}

void MainLoop()
{
    if (pGame->bIsGameExiting) return;
    DoInitStuff();

    // O passe secundário roda fora do hook do renderer; nesse ponto ele não
    // interrompe a rotina que desenha a cidade principal.
    gPhoneSecondaryCamera.Tick();

    // Reaplica as texturas de clothes (pele/rosto/cabelo/roupas) no ped local
    // e demais entidades — o GTA mobile recria materiais ao streamar.
    BrlTextureShader::Process();

    if (bDebug) {
        DoDebugLoop();
    }

    if (pNetGame) {
        pNetGame->Process();
    }

    if (pAudioStream) {
        pAudioStream->Process();
    }
}

void InitGui()
{
    // new voice
    Plugin::OnPluginLoad();
    Plugin::OnSampLoad();

    std::string font_path = string_format("%sSAMP/fonts/%s", g_pszStorage, FONT_NAME);
    pUI = new UI(ImVec2(RsGlobal->maximumWidth, RsGlobal->maximumHeight), font_path.c_str());
    pUI->initialize();
    pUI->performLayout();
}

#include "game/multitouch.h"
#include "armhook/patch.h"
#include "util/CUtil.h"
void SetUpGLHooks();
jint JNI_OnLoad(JavaVM* vm, void* reserved)
{
    javaVM = vm;
    LOGI("SA-MP library loaded! Build time: " __DATE__ " " __TIME__);

    g_libGTASA = CUtil::FindLib("libGTASA.so");
    if (g_libGTASA == 0x00) {
        LOGE("libGTASA.so address was not found! ");
        return JNI_VERSION_1_6;
    }

    g_libSAMP = CUtil::FindLib("libsamp.so");
    if (g_libSAMP == 0x00) {
        LOGE("libsamp.so address was not found! ");
        return JNI_VERSION_1_6;
    }

    firebase::crashlytics::Initialize();

    uintptr_t libgtasa = CUtil::FindLib("libGTASA.so");
    uintptr_t libsamp = CUtil::FindLib("libsamp.so");
    uintptr_t libc = CUtil::FindLib("libc.so");

    FLog("libGTASA.so: 0x%x", libgtasa);
    FLog("libsamp.so: 0x%x", libsamp);
    FLog("libc.so: 0x%x", libc);

    char str[100];

    sprintf(str, "0x%x", libgtasa);
    firebase::crashlytics::SetCustomKey("libGTASA.so", str);

    sprintf(str, "0x%x", libsamp);
    firebase::crashlytics::SetCustomKey("libsamp.so", str);

    sprintf(str, "0x%x", libc);
    firebase::crashlytics::SetCustomKey("libc.so", str);

    CHook::InitHookStuff();
    InstallSpecialHooks();
    ApplyPatches_level0();
    //SetUpGLHooks();
    InitRenderWareFunctions();
    MultiTouch::initialize();

    pGame = new CGame();

    //pVoice = new CVoice();
    //pVoice->Initialize(VOICE_FREQUENCY, CODEC_FREQUENCY, VOICE_SENDRRATE);

    //pthread_t thread;
    //pthread_create(&thread, 0, Init, 0);

    struct sigaction act;
    act.sa_sigaction = handler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &act, &act_old);

    struct sigaction act1;
    act1.sa_sigaction = handler1;
    sigemptyset(&act1.sa_mask);
    act1.sa_flags = SA_SIGINFO;
    sigaction(SIGABRT, &act1, &act1_old);

    struct sigaction act2;
    act2.sa_sigaction = handler2;
    sigemptyset(&act2.sa_mask);
    act2.sa_flags = SA_SIGINFO;
    sigaction(SIGFPE, &act2, &act2_old);

    struct sigaction act3;
    act3.sa_sigaction = handler3;
    sigemptyset(&act3.sa_mask);
    act3.sa_flags = SA_SIGINFO;
    sigaction(SIGBUS, &act3, &act3_old);

    return JNI_VERSION_1_6;
}

uint32_t GetTickCount()
{
    return CTimer::m_snTimeInMillisecondsNonClipped;
}

void FLog(const char* fmt, ...)
{
    char buffer[0xFF];
    static FILE* flLog = nullptr;
    const char* pszStorage = g_pszStorage;


    if (flLog == nullptr && pszStorage != nullptr)
    {
        sprintf(buffer, "%s/samp_log.txt", pszStorage);
        //LOGI("buffer: %s", buffer);
        flLog = fopen(buffer, "a");
    }

    memset(buffer, 0, sizeof(buffer));

    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, arg);
    va_end(arg);

    LOGI("%s", buffer);
    firebase::crashlytics::Log(buffer);

    if (flLog == nullptr) return;
    fprintf(flLog, "%s\n", buffer);
    fflush(flLog);

    return;
}

void ChatLog(const char* fmt, ...)
{
    char buffer[0xFF];
    static FILE* flLog = nullptr;
    const char* pszStorage = g_pszStorage;


    if (flLog == nullptr && pszStorage != nullptr)
    {
        sprintf(buffer, "%s/chat_log.txt", pszStorage);
        flLog = fopen(buffer, "a");
    }

    memset(buffer, 0, sizeof(buffer));

    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, arg);
    va_end(arg);

    if (flLog == nullptr) return;
    fprintf(flLog, "%s\n", buffer);
    fflush(flLog);

    return;
}

void MyLog(const char* fmt, ...)
{
    char buffer[0xFF];
    static FILE* flLog = nullptr;
    const char* pszStorage = g_pszStorage;


    if (flLog == nullptr && pszStorage != nullptr)
    {
        sprintf(buffer, "%s/samp_log.txt", pszStorage);
        //LOGI("buffer: %s", buffer);
        flLog = fopen(buffer, "a");
    }

    memset(buffer, 0, sizeof(buffer));

    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, arg);
    va_end(arg);

    if (flLog == nullptr) return;
    fprintf(flLog, "%s\n", buffer);
    fflush(flLog);

    return;
}

void MyLog2(const char* fmt, ...)
{
    char buffer[0xFF];
    static FILE* flLog = nullptr;
    const char* pszStorage = g_pszStorage;


    if (flLog == nullptr && pszStorage != nullptr)
    {
        sprintf(buffer, "%s/samp_log.txt", pszStorage);
        //LOGI("buffer: %s", buffer);
        flLog = fopen(buffer, "a");
    }

    memset(buffer, 0, sizeof(buffer));

    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, arg);
    va_end(arg);

    if (pUI) pUI->chat()->addDebugMessage(buffer);

    if (flLog == nullptr) return;
    fprintf(flLog, "%s\n", buffer);
    fflush(flLog);
    return;
}

void LogVoice(const char* fmt, ...)
{
    char buffer[0xFF];
    static FILE* flLog = nullptr;
    const char* pszStorage = g_pszStorage;

    if (flLog == nullptr && pszStorage != nullptr)
    {
        sprintf(buffer, "%sSAMP/%s", pszStorage, SV::kLogFileName);
        flLog = fopen(buffer, "w");
    }

    memset(buffer, 0, sizeof(buffer));

    va_list arg;
    va_start(arg, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, arg);
    va_end(arg);

    __android_log_write(ANDROID_LOG_INFO, "AXL", buffer);

    if (flLog == nullptr) return;
    fprintf(flLog, "%s\n", buffer);
    fflush(flLog);

    return;
}
