#include "jniutil.h"
#include "game/game.h"
extern CGame *pGame;

CJavaWrapper::CJavaWrapper(JNIEnv *env, jobject activity)
{
    this->activity = env->NewGlobalRef(activity);

    jclass clas = env->GetObjectClass(activity);
    if(!clas)
    {
        FLog("no clas");
        return;
    }

    s_showTab = env->GetMethodID(clas, "showTab", "()V");
    s_hideTab = env->GetMethodID(clas, "hideTab", "()V");
    s_clearTab = env->GetMethodID(clas, "clearTab", "()V");
    s_setTab = env->GetMethodID(clas, "setTab", "(ILjava/lang/String;II)V");

    s_showLoadingScreen = env->GetMethodID(clas, "showLoadingScreen", "()V");
    s_hideLoadingScreen = env->GetMethodID(clas, "hideLoadingScreen", "()V");
    s_updateLoadingStatus = env->GetMethodID(clas, "updateLoadingStatus", "(Ljava/lang/String;)V");
    s_skipToAuthenticating = env->GetMethodID(clas, "skipToAuthenticating", "()V");
    s_startLoadingSequence = env->GetMethodID(clas, "startLoadingSequence", "()V");
    s_onGameReady = env->GetMethodID(clas, "onGameReady", "()V");
    s_onNetworkReady = env->GetMethodID(clas, "onNetworkReady", "()V");
    s_setMusicPlaylist = env->GetMethodID(clas, "setMusicPlaylist", "([Ljava/lang/String;Ljava/lang/String;)V");
    s_stopLoadingMusic = env->GetMethodID(clas, "stopLoadingMusic", "()V");

    s_setPauseState = env->GetMethodID(clas, "setPauseState", "(Z)V");

    s_ShowDialog = env->GetMethodID(clas, "showDialog", "(II[B[B[B[B)V");

		s_showInputLayout = env->GetMethodID(clas, "showKeyboard", "()V");
	    s_hideInputLayout = env->GetMethodID(clas, "hideKeyboard", "()V");
	    s_setNexionHudLogoVisible = env->GetMethodID(clas, "setNexionHudLogoVisible", "(Z)V");

	    s_exitGame = env->GetMethodID(clas, "exitGame", "()V");
	    s_goToLauncher = env->GetMethodID(clas, "goToLauncher", "()V");

    s_showEditObject = env->GetMethodID(clas, "showEditObject", "()V");
    s_hideEditObject = env->GetMethodID(clas, "hideEditObject", "()V");

    s_showBank = env->GetMethodID(clas, "showBank", "(Ljava/lang/String;)V");
    s_updateBank = env->GetMethodID(clas, "updateBank", "(Ljava/lang/String;)V");
    s_hideBank = env->GetMethodID(clas, "hideBank", "()V");
    s_showLogin = env->GetMethodID(clas, "showLogin", "(Ljava/lang/String;)V");
    s_authResult = env->GetMethodID(clas, "authResult", "(Ljava/lang/String;)V");
    s_hideLogin = env->GetMethodID(clas, "hideLogin", "()V");
    s_showPhone = env->GetMethodID(clas, "showPhoneFromNative", "()V");
    s_hidePhone = env->GetMethodID(clas, "hidePhoneFromNative", "()V");
    s_togglePhone = env->GetMethodID(clas, "togglePhoneFromNative", "()V");

    s_showBankInteract = env->GetMethodID(clas, "showBankInteract", "(I)V");
    s_hideBankInteract = env->GetMethodID(clas, "hideBankInteract", "()V");
    s_sendGameDataToFlutter = env->GetMethodID(clas, "sendGameDataToFlutter", "(Ljava/lang/String;Ljava/lang/String;)V");

    env->DeleteLocalRef(clas);
}

void CJavaWrapper::ShowBank(const char* jsonData)
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env || !s_showBank) return;

    jstring jData = env->NewStringUTF(jsonData ? jsonData : "{}");
    env->CallVoidMethod(activity, s_showBank, jData);
    env->DeleteLocalRef(jData);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::UpdateBank(const char* jsonData)
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env || !s_updateBank) return;

    jstring jData = env->NewStringUTF(jsonData ? jsonData : "{}");
    env->CallVoidMethod(activity, s_updateBank, jData);
    env->DeleteLocalRef(jData);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::ShowLogin(const char* jsonData)
{
    JNIEnv* env; javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env || !s_showLogin) return;
    jstring data = env->NewStringUTF(jsonData ? jsonData : "{}");
    env->CallVoidMethod(activity, s_showLogin, data);
    env->DeleteLocalRef(data); EXCEPTION_CHECK(env);
}

void CJavaWrapper::AuthResult(const char* jsonData)
{
    JNIEnv* env; javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env || !s_authResult) return;
    jstring data = env->NewStringUTF(jsonData ? jsonData : "{}");
    env->CallVoidMethod(activity, s_authResult, data);
    env->DeleteLocalRef(data); EXCEPTION_CHECK(env);
}

void CJavaWrapper::HideLogin()
{
    JNIEnv* env; javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env || !s_hideLogin) return;
    env->CallVoidMethod(activity, s_hideLogin); EXCEPTION_CHECK(env);
}

void CJavaWrapper::HideBank()
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env || !s_hideBank) return;

    env->CallVoidMethod(activity, s_hideBank);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::ShowPhone()
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env || !s_showPhone) return;

    env->CallVoidMethod(activity, s_showPhone);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::HidePhone()
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env || !s_hidePhone) return;

    env->CallVoidMethod(activity, s_hidePhone);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::TogglePhone()
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env || !s_togglePhone) return;

    env->CallVoidMethod(activity, s_togglePhone);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::ShowBankInteract(int alpha)

{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env || !s_showBankInteract) return;

    env->CallVoidMethod(activity, s_showBankInteract, (jint)alpha);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::HideBankInteract()
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env || !s_hideBankInteract) return;

    env->CallVoidMethod(activity, s_hideBankInteract);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::SendGameDataToFlutter(const char* location, const char* time)
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env || !s_sendGameDataToFlutter) return;

    jstring jLocation = env->NewStringUTF(location);
    jstring jTime = env->NewStringUTF(time);
    env->CallVoidMethod(activity, s_sendGameDataToFlutter, jLocation, jTime);
    env->DeleteLocalRef(jLocation);
    env->DeleteLocalRef(jTime);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::ShowKeyboard()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_showInputLayout);
    EXCEPTION_CHECK(p);
}


void CJavaWrapper::HideKeyboard()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_hideInputLayout);
    EXCEPTION_CHECK(p);
}

#include "../gui/gui.h"
extern UI* pUI;

void CJavaWrapper::SetKeyboardOffset(int offsetY)
{
    if (pUI) pUI->setKeyboardOffset(offsetY);
}

void CJavaWrapper::SetNexionHudLogoVisible(bool visible)
{
    JNIEnv* env = nullptr;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env || !this->s_setNexionHudLogoVisible) return;

    env->CallVoidMethod(this->activity, this->s_setNexionHudLogoVisible, visible ? JNI_TRUE : JNI_FALSE);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::ShowLoadingScreen()

{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_showLoadingScreen);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::HideLoadingScreen()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_hideLoadingScreen);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::UpdateLoadingStatus(const char* status)
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env) return;

    jstring jstatus = env->NewStringUTF(status);
    env->CallVoidMethod(activity, s_updateLoadingStatus, jstatus);
    env->DeleteLocalRef(jstatus);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::SkipToAuthenticating()
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env) return;

    env->CallVoidMethod(activity, s_skipToAuthenticating);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::StartLoadingSequence()
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env) return;

    env->CallVoidMethod(activity, s_startLoadingSequence);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::OnGameReady()
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env) return;

    env->CallVoidMethod(activity, s_onGameReady);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::OnNetworkReady()
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env) return;

    env->CallVoidMethod(activity, s_onNetworkReady);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::SetMusicPlaylist(const char* tracks[], int trackCount, const char* title)
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env) return;

    // Create Java String array
    jclass stringClass = env->FindClass("java/lang/String");
    jobjectArray jTracks = env->NewObjectArray(trackCount, stringClass, nullptr);
    for (int i = 0; i < trackCount; i++) {
        jstring jTrack = env->NewStringUTF(tracks[i]);
        env->SetObjectArrayElement(jTracks, i, jTrack);
        env->DeleteLocalRef(jTrack);
    }

    jstring jTitle = env->NewStringUTF(title);
    env->CallVoidMethod(activity, s_setMusicPlaylist, jTracks, jTitle);
    env->DeleteLocalRef(jTracks);
    env->DeleteLocalRef(jTitle);
    env->DeleteLocalRef(stringClass);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::StopLoadingMusic()
{
    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);
    if (!env) return;

    env->CallVoidMethod(activity, s_stopLoadingMusic);
    EXCEPTION_CHECK(env);
}

void CJavaWrapper::SetPauseState(bool pause)
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_setPauseState, pause);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::SetTab(int id, char* names, int score, int pings)
{
    JNIEnv* global_env;
    javaVM->GetEnv((void**)&global_env, JNI_VERSION_1_6);

	if (!global_env)
	{
		LOGI("No env");
		return;
	}

	jclass strClass = global_env->FindClass("java/lang/String");
	jmethodID ctorID = global_env->GetMethodID(strClass, "<init>", "([BLjava/lang/String;)V");
	jstring encoding = global_env->NewStringUTF("UTF-8");

	jbyteArray bytes = global_env->NewByteArray(strlen(names));
	global_env->SetByteArrayRegion(bytes, 0, strlen(names), (jbyte*)names);
	jstring str1 = (jstring)global_env->NewObject(strClass, ctorID, bytes, encoding);

    global_env->CallVoidMethod(activity, s_setTab, id, str1, score, pings);

    EXCEPTION_CHECK(global_env);
}

void CJavaWrapper::ShowTab()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_showTab);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::HideTab()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_hideTab);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::ClearTab()
{
    JNIEnv* p;
    javaVM->GetEnv((void**)&p, JNI_VERSION_1_6);
    p->CallVoidMethod(activity, s_clearTab);
    EXCEPTION_CHECK(p);
}

void CJavaWrapper::ShowDialog(int dialogStyle, int dialogID, char* title, char* text, char* button1, char* button2)
{
	JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

	if (!env)
	{
		FLog("No env");
		return;
	}

	std::string sTitle(title);
	std::string sText(text);
	std::string sButton1(button1);
	std::string sButton2(button2);

	jbyteArray jstrTitle = as_byte_array((unsigned char*)sTitle.c_str(), sTitle.length());
	jbyteArray jstrText = as_byte_array((unsigned char*)sText.c_str(), sText.length());
	jbyteArray jstrButton1 = as_byte_array((unsigned char*)sButton1.c_str(), sButton1.length());
	jbyteArray jstrButton2 = as_byte_array((unsigned char*)sButton2.c_str(), sButton2.length());

	env->CallVoidMethod(activity, s_ShowDialog, dialogID, dialogStyle, jstrTitle, jstrText, jstrButton1, jstrButton2);

	EXCEPTION_CHECK(env);
}

void CJavaWrapper::exitGame() {

    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (!env)
    {
        FLog("No env");
        return;
    }

    env->CallVoidMethod(this->activity, this->s_exitGame);
}

void CJavaWrapper::GoToLauncher() {

    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (!env)
    {
        FLog("No env");
        return;
    }

    env->CallVoidMethod(this->activity, this->s_goToLauncher);
}

void CJavaWrapper::ShowEditObject() {

    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (!env)
    {
        FLog("No env");
        return;
    }

    env->CallVoidMethod(this->activity, this->s_showEditObject);
}

void CJavaWrapper::HideEditObject() {

    JNIEnv* env;
    javaVM->GetEnv((void**)&env, JNI_VERSION_1_6);

    if (!env)
    {
        FLog("No env");
        return;
    }

    env->CallVoidMethod(this->activity, this->s_hideEditObject);
}
