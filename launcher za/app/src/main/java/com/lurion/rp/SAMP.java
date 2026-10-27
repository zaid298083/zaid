package com.nexion.online;

import android.graphics.Bitmap;
import android.os.Bundle;
import android.util.Base64;
import android.util.DisplayMetrics;
import android.util.Log;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.provider.MediaStore;
import android.view.Gravity;
import android.view.KeyEvent;
import android.view.SurfaceView;
import android.view.View;

import android.view.ViewGroup;
import android.widget.FrameLayout;
import android.webkit.JavascriptInterface;
import android.webkit.WebView;
import android.webkit.WebSettings;
import android.webkit.PermissionRequest;
import android.graphics.Color;

import java.io.ByteArrayOutputStream;
import java.io.FileInputStream;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.Map;

import io.flutter.embedding.android.FlutterFragment;
import io.flutter.embedding.android.RenderMode;
import io.flutter.embedding.android.TransparencyMode;
import io.flutter.embedding.engine.FlutterEngine;
import io.flutter.embedding.engine.FlutterEngineCache;
import io.flutter.embedding.engine.dart.DartExecutor;
import io.flutter.plugin.common.MethodChannel;

import com.google.firebase.crashlytics.FirebaseCrashlytics;
import com.nexion.online.ui.CustomKeyboard;
import com.nexion.online.ui.LoadingScreen;
import com.nexion.online.ui.dialog.DialogManager;
import android.os.Handler;

// @Obfuscate // This line is commented out to prevent method renaming/removal
public class SAMP extends GTASA implements CustomKeyboard.InputListener, HeightProvider.HeightListener {
    private static final String TAG = "SAMP";

    private static native void nativeOnAuthLogin(String email, String password);
    private static native void nativeOnAuthRegister(String fullname, String email, String password);
    private static SAMP instance;

    private DialogManager mDialog;

    private CustomKeyboard mKeyboard;
    private HeightProvider mHeightProvider;

    private LoadingScreen mLoadingScreen;
    private boolean mMusicStarted = false;

    // =====================
    // WebUI (sistemas HTML estilo NUI)
    // =====================
    private WebView mBankWebView;
    private boolean mBankPageReady = false;
    private boolean mBankRequestFromFlutter = false; // true = pedido veio do celular
    private boolean mBankOpenOnFlutter = false; // true = celular esta com banco aberto
    private WebView mAuthWebView;
    private boolean mAuthPageReady = false;
    private WebView mPhoneCefWebView;
    private boolean mPhoneCefPageReady = false;
    private static final int PHONE_CAMERA_REQUEST = 7101;
    private static final int PHONE_VIDEO_REQUEST = 7102;

    // =====================
    // Flutter Phone
    // =====================
    private static final String PHONE_ENGINE_ID = "lurion_phone_engine";
    private static final String PHONE_CHANNEL = "com.nexion.online/phone";
    private FlutterEngine mPhoneEngine;
    private FlutterFragment mPhoneFragment;
    private FrameLayout mPhoneContainer;
    private MethodChannel mPhoneChannel;
    private boolean mPhoneVisible = false;

    // Botão flutuante de interação (aparece ao chegar perto do banco)
    private android.widget.LinearLayout mInteractButton = null;

    private Handler mHandler = new Handler();

    public static SAMP getInstance() {
        return instance;
    }

    private void showTab()
    {

    }

    private void hideTab()
    {

    }

    private void setTab(int id, String name, int score, int ping)
    {

    }

    private void clearTab()
    {

    }

    /**
     * Este método é chamado pelo código nativo (C++) para exibir a tela de carregamento.
     * Nós o executamos na thread principal da UI para que as animações funcionem corretamente.
     */
    private void showLoadingScreen()
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mLoadingScreen != null) {
                    mLoadingScreen.show();
                }
            }
        });
    }

    /**
     * Pula direto para "Autenticando..." quando o C++ termina a inicialização.
     */
    private void skipToAuthenticating()
    {
        if (mLoadingScreen != null) {
            mLoadingScreen.skipToAuthenticating();
        }
    }

    /**
     * Chamado quando o C++ termina de carregar os recursos do jogo.
     */
    private void onGameReady()
    {
        if (mLoadingScreen != null) {
            mLoadingScreen.onGameReady();
        }
    }

    /**
     * Chamado quando o C++ inicia a conexão de rede.
     */
    private void onNetworkReady()
    {
        if (mLoadingScreen != null) {
            mLoadingScreen.onNetworkReady();
        }
    }

    /**
     * Este método é chamado pelo código nativo (C++) para ocultar a tela de carregamento.
     */
    private void hideLoadingScreen()
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mLoadingScreen != null) {
                    mLoadingScreen.stopMusic();
                    mLoadingScreen.hide();
                }
            }
        });
    }

    /**
     * Atualiza o texto de status do carregamento. Chamado pelo código nativo (C++).
     */
    private void updateLoadingStatus(final String status)
    {
        if (mLoadingScreen != null) {
            mLoadingScreen.updateStatus(status);
        }
    }

    /**
     * Inicia a sequência automática de status de carregamento.
     */
    private void startLoadingSequence()
    {
        if (mLoadingScreen != null) {
            mLoadingScreen.startLoadingSequence();
        }
    }

    /**
     * Configura a playlist de música e começa a tocar. Chamado pelo código nativo (C++).
     */
    private void setMusicPlaylist(final String[] tracks, final String title)
    {
        if (mLoadingScreen != null) {
            mLoadingScreen.setPlaylist(tracks, title);
        }
    }

    /**
     * Para a música da tela de carregamento. Chamado pelo código nativo (C++).
     */
    private void stopLoadingMusic()
    {
        if (mLoadingScreen != null) {
            mLoadingScreen.stopMusic();
        }
    }

    public void setPauseState(boolean pause) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (pause) {
                    mDialog.hideWithoutReset();
                }
                else {
                    if(mDialog.isShow)
                        mDialog.showWithOldContent();
                }
            }
        });
    }

    // ============================================================
    // BANCO (WebUI) — chamado pelo código nativo (C++)
    // ============================================================

    /**
     * Cria o WebView do banco uma única vez (transparente, sobreposto ao jogo).
     */
    private void ensureBankWebView()
    {
        if (mBankWebView != null) return;

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mBankWebView = new WebView(SAMP.this);
                WebSettings settings = mBankWebView.getSettings();
                settings.setJavaScriptEnabled(true);
                settings.setDomStorageEnabled(true);
                settings.setLoadWithOverviewMode(true);
                settings.setUseWideViewPort(true);
                mBankWebView.setBackgroundColor(Color.TRANSPARENT);
                mBankWebView.setLayerType(android.view.View.LAYER_TYPE_HARDWARE, null);
                mBankWebView.setVisibility(android.view.View.GONE);
                mBankWebView.setWebChromeClient(new android.webkit.WebChromeClient());

                // Ponte JS -> nativo: window.SampBank.action(...) / window.SampBank.close()
                mBankWebView.addJavascriptInterface(new BankBridge(), "SampBank");

                // Avisa o JS quando a página terminar de carregar
                mBankWebView.setWebViewClient(new android.webkit.WebViewClient() {
                    @Override
                    public void onPageFinished(android.webkit.WebView view, String url) {
                        mBankPageReady = true;
                    }
                });

                FrameLayout params = new FrameLayout(SAMP.this);
                addContentView(params, new ViewGroup.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT));
                params.addView(mBankWebView, new FrameLayout.LayoutParams(
                        ViewGroup.LayoutParams.MATCH_PARENT,
                        ViewGroup.LayoutParams.MATCH_PARENT));

                mBankWebView.loadUrl("file:///android_asset/bank/index.html");
            }
        });
    }

    /**
     * Chamado pelo C++ quando o servidor envia os dados do banco (RPC BANK_OPEN).
     * Mostra o WebView e repassa o JSON pra interface.
     */
    public void showBank(final String jsonData)
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mBankRequestFromFlutter) {
                    // Pedido veio do celular -> só manda pro Flutter, NÃO abre WebView
                    mBankRequestFromFlutter = false;
                    mBankOpenOnFlutter = true;
                    sendBankDataToFlutter(jsonData, true);
                } else {
                    mBankOpenOnFlutter = false;
                    // Pedido veio do botão físico -> só abre WebView
                    ensureBankWebViewInner();
                    mBankWebView.setVisibility(android.view.View.VISIBLE);
                    mBankWebView.requestFocus();
                    execInBank("BankApp.openFromNative(" + safeJson(jsonData) + ");");
                }
            }
        });
    }

    /**
     * Chamado pelo C++ com atualizações do servidor (saldo, histórico, erros).
     */
    public void updateBank(final String jsonData)
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mBankOpenOnFlutter) {
                    // Celular com banco aberto -> manda pro Flutter
                    sendBankDataToFlutter(jsonData, false);
                } else {
                    // Sem celular -> manda pro WebView
                    execInBank("BankApp.onUpdateFromNative(" + safeJson(jsonData) + ");");
                }
            }
        });
    }

    /**
     * Envia dados do banco para o Flutter via MethodChannel.
     * @param jsonData JSON com saldo, histórico, etc.
     * @param isOpen true se o banco acabou de abrir, false se é atualização
     */
    private void sendBankDataToFlutter(final String jsonData, final boolean isOpen)
    {
        if (mPhoneChannel == null) return;
        try {
            String method = isOpen ? "onBankData" : "onBankUpdate";
            mPhoneChannel.invokeMethod(method, jsonData);
        } catch (Exception e) {
            // Flutter pode não estar rodando, ignora silenciosamente
        }
    }

    /**
     * Envia dados do jogo (localização, horário) para o Flutter.
     * Chamado pelo C++ periodicamente.
     */
    public void sendGameDataToFlutter(final String location, final String time)
    {
        if (mPhoneChannel == null) return;
        try {
            java.util.Map<String, String> data = new java.util.HashMap<>();
            data.put("location", location);
            data.put("time", time);
            mPhoneChannel.invokeMethod("onGameData", data);
        } catch (Exception e) {
            // Flutter pode não estar rodando, ignora silenciosamente
        }
    }

    /**
     * Esconde a UI do banco e devolve o controle ao jogo.
     */
    public void hideBank()
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mBankOpenOnFlutter = false;
                if (mBankWebView != null) {
                    execInBank("BankApp.closeFromNative();");
                    mBankWebView.setVisibility(android.view.View.GONE);
                }
            }
        });
    }

    // ============================================================
    // BOTÃO FLUTUANTE DE INTERAÇÃO — chamado pelo nativo quando o
    // jogador se aproxima de um ponto de interação (banco, etc.)
    // alpha 0-255: aparece gradualmente conforme ele chega mais perto
    // ============================================================

    private void ensureInteractButton()
    {
        if (mInteractButton != null) return;

        float density = getResources().getDisplayMetrics().density;

        mInteractButton = new android.widget.LinearLayout(SAMP.this);
        mInteractButton.setOrientation(android.widget.LinearLayout.VERTICAL);
        mInteractButton.setGravity(android.view.Gravity.CENTER);
        mInteractButton.setPadding((int)(10 * density), (int)(10 * density), (int)(10 * density), (int)(10 * density));
        mInteractButton.setClickable(true);

        // PNG do botão (assets/bank/nexionl_interaction_button.png)
        try {
            java.io.InputStream is = getAssets().open("bank/nexionl_interaction_button.png");
            android.graphics.Bitmap bmp = android.graphics.BitmapFactory.decodeStream(is);
            is.close();
            if (bmp != null) {
                android.widget.ImageView icon = new android.widget.ImageView(SAMP.this);
                icon.setImageBitmap(bmp);
                int iconPx = (int)(64 * density);
                icon.setLayoutParams(new android.widget.LinearLayout.LayoutParams(iconPx, iconPx));
                mInteractButton.addView(icon);
            }
        } catch (Exception e) {
            Log.e(TAG, "Falha ao carregar PNG do botão de interação: " + e.getMessage());
        }

        // Texto "Acessar"
        android.widget.TextView label = new android.widget.TextView(SAMP.this);
        label.setText("Acessar");
        label.setTextColor(0xFFFFFFFF);
        label.setTextSize(14);
        label.setTypeface(null, android.graphics.Typeface.BOLD);
        label.setShadowLayer(4, 1, 1, 0xFF000000);
        android.widget.LinearLayout.LayoutParams lp = new android.widget.LinearLayout.LayoutParams(
                android.widget.LinearLayout.LayoutParams.WRAP_CONTENT,
                android.widget.LinearLayout.LayoutParams.WRAP_CONTENT);
        lp.topMargin = (int)(6 * density);
        label.setLayoutParams(lp);
        mInteractButton.addView(label);

        mInteractButton.setVisibility(android.view.View.GONE);
        mInteractButton.setAlpha(0f);

        // Clique -> pede pro servidor abrir a interface do banco
        mInteractButton.setOnClickListener(new android.view.View.OnClickListener() {
            @Override
            public void onClick(android.view.View v) {
                mBankRequestFromFlutter = false; // pedido veio do botão físico
                try {
                    nativeOnBankInteract();
                } catch (UnsatisfiedLinkError | Exception e) {
                    Log.e(TAG, "nativeOnBankInteract falhou: " + e.getMessage());
                }
            }
        });

        FrameLayout params = new FrameLayout(SAMP.this);
        addContentView(params, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        params.setClickable(false);

        FrameLayout.LayoutParams btnLp = new FrameLayout.LayoutParams(
                FrameLayout.LayoutParams.WRAP_CONTENT,
                FrameLayout.LayoutParams.WRAP_CONTENT,
                android.view.Gravity.BOTTOM | android.view.Gravity.CENTER_HORIZONTAL);
        btnLp.bottomMargin = (int)(90 * density); // acima da HUD do jogo
        params.addView(mInteractButton, btnLp);
    }

    /** Chamado pelo C++: mostra/ajusta o botão com o nível de proximidade. */
    public void showBankInteract(final int alpha)
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                // Banco aberto? O botão fica escondido atrás da interface
                if (mBankWebView != null && mBankWebView.getVisibility() == android.view.View.VISIBLE) return;
                ensureInteractButton();
                if (mInteractButton.getVisibility() != android.view.View.VISIBLE)
                    mInteractButton.setVisibility(android.view.View.VISIBLE);
                mInteractButton.animate()
                        .alpha(Math.max(0f, Math.min(1f, alpha / 255f)))
                        .setDuration(300)
                        .start();
            }
        });
    }

    /** Chamado pelo C++: some com o botão (jogador se afastou). */
    public void hideBankInteract()
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (mInteractButton == null) return;
                mInteractButton.animate()
                        .alpha(0f)
                        .setDuration(250)
                        .withEndAction(new Runnable() {
                            @Override
                            public void run() {
                                if (mInteractButton != null)
                                    mInteractButton.setVisibility(android.view.View.GONE);
                            }
                        }).start();
            }
        });
    }

    private void ensureBankWebViewInner()
    {
        if (mBankWebView == null) {
            // fallback caso showBank chegue antes de garantir a criação
            try {
                ensureBankWebView();
            } catch (Exception ignored) {}
        }
    }

    private void execInBank(String js)
    {
        final String script = js;
        mHandler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (mBankWebView != null && mBankPageReady) {
                    mBankWebView.evaluateJavascript(script, null);
                } else {
                    // página ainda carregando: tenta de novo em breve
                    mHandler.postDelayed(this, 120);
                }
            }
        }, 0);
    }

    /** Garante que o payload vira um objeto JSON literal válido dentro de evaluateJavascript. */
    private static String safeJson(String s)
    {
        if (s == null || s.trim().isEmpty()) return "{}";
        return s.trim();
    }

    /**
     * Ponte exposta ao JavaScript como window.SampBank.
     * O JS chama action("withdraw", "{...}") e o nativo envia pro servidor via packet 251.
     */
    public class BankBridge
    {
        @JavascriptInterface
        public void action(String actionType, String dataJson)
        {
            try {
                nativeOnBankAction(actionType, dataJson == null ? "{}" : dataJson, 0); // source=0: caixa eletronico
            } catch (UnsatisfiedLinkError | Exception e) {
                Log.e(TAG, "BankBridge.action falhou: " + e.getMessage());
            }
        }

        @JavascriptInterface
        public void close()
        {
            try {
                nativeOnBankClose();
            } catch (UnsatisfiedLinkError | Exception e) {
                Log.e(TAG, "BankBridge.close falhou: " + e.getMessage());
            }
        }
    }

    public void exitGame(){
        FirebaseCrashlytics.getInstance().setCrashlyticsCollectionEnabled(false);

        finishAndRemoveTask();
        System.exit(0);
    }

    /**
     * Chamado pelo código nativo (C++) quando o jogador toca em
     * "CLIQUE AQUI PARA REINICIAR" na tela de conexão perdida.
     * Volta para a tela inicial do launcher.
     */
    public void goToLauncher(){
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    // Esconde a superficie do jogo e deixa a janela preta ANTES de
                    // abrir a launcher: impede que o ultimo frame do GTA (ainda
                    // renderizando) apareca atras da tela inicial.
                    try {
                        android.view.SurfaceView surfaceView = GetSurfaceView();
                        if (surfaceView != null) {
                            surfaceView.setVisibility(android.view.View.GONE);
                        }
                        getWindow().setBackgroundDrawable(new android.graphics.drawable.ColorDrawable(android.graphics.Color.BLACK));
                    } catch (Exception ignored) {
                    }

                    // Traz a MainActivity (tela inicial do launcher) para o topo.
                    Intent launcherIntent = new Intent(SAMP.this, com.nexion.launcher.activity.MainActivity.class);
                    launcherIntent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP | Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_NEW_TASK);
                    startActivity(launcherIntent);

                    // Mata o processo IMEDIATAMENTE, antes que o onDestroy rode o
                    // cleanup nativo do GTASA (systemCleanup -> cleanup() destrói o
                    // mutex da fila de eventos EGL enquanto a thread NVEvent ainda
                    // espera eventos -> pthread_mutex_lock on destroyed mutex ->
                    // SIGABRT). O exitGame() original nunca crashava justamente
                    // porque faz System.exit(0) antes do cleanup. O Android
                    // restaura a MainActivity (launcher) quando o processo voltar.
                    finishAndRemoveTask();
                    System.exit(0);
                } catch (Exception e) {
                    Log.e(TAG, "Erro ao voltar para o launcher: " + e.getMessage());
                }
            }
        });
    }

    public void showDialog(int dialogId, int dialogTypeId, byte[] bArr, byte[] bArr2, byte[] bArr3, byte[] bArr4) {
        final String caption = new String(bArr);
        final String content = new String(bArr2);
        final String leftBtnText = new String(bArr3);
        final String rightBtnText = new String(bArr4);
        runOnUiThread(() -> { this.mDialog.show(dialogId, dialogTypeId, caption, content, leftBtnText, rightBtnText); });
    }

    public void showEditObject()
    {
        Log.d("SAMP_JNI_FIX", "showEditObject() called from native code. Doing nothing.");
    }

    public void hideEditObject()
    {
        Log.d("SAMP_JNI_FIX", "hideEditObject() called from native code. Doing nothing.");
    }

    // Native functions that need empty counterparts in Java
    private native void onInputEnd(byte[] str);

    // Empty methods to satisfy native calls
    private void showKeyboard()
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                Log.d("AXL", "showKeyboard()");
                mKeyboard.ShowInputLayout();
            }
        });
    }

    private void hideKeyboard()
    {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                mKeyboard.HideInputLayout();
            }
        });
    }

    public void OnInputEnd(String str) {
        // This function is potentially called by a UI element, which then calls the native onInputEnd
        byte[] toReturn = str.getBytes();

        try {
            onInputEnd(toReturn);
        }
        catch (UnsatisfiedLinkError e5) {
            Log.e(TAG, e5.getMessage());
        }
    }


        private void ensurePhoneFlutter() {
        if (mPhoneContainer != null) return;

        if (mAndroidUI == null) {
            Log.e(TAG, "ui_layout ainda não foi inicializado; telefone não será exibido");
            return;
        }

        mPhoneContainer = new FrameLayout(this);
        mPhoneContainer.setId(View.generateViewId());
        mPhoneContainer.setVisibility(View.GONE);
        mPhoneContainer.setBackgroundColor(Color.TRANSPARENT);
        mPhoneContainer.setClipChildren(true);
        mPhoneContainer.setClipToPadding(true);
        DisplayMetrics displayMetrics = getResources().getDisplayMetrics();
        int screenHeight = mAndroidUI.getHeight() > 0
                ? mAndroidUI.getHeight()
                : displayMetrics.heightPixels;
        int phoneHeight = Math.round(screenHeight * 0.96f);
        int phoneWidth = Math.round(phoneHeight * 814f / 1716f);
        FrameLayout.LayoutParams phoneLayoutParams = new FrameLayout.LayoutParams(
                phoneWidth,
                phoneHeight,
                Gravity.RIGHT | Gravity.CENTER_VERTICAL
        );
        phoneLayoutParams.rightMargin = 12;
        mPhoneContainer.setLayoutParams(phoneLayoutParams);
        mAndroidUI.addView(mPhoneContainer, phoneLayoutParams);
        mPhoneContainer.bringToFront();

        mPhoneEngine = FlutterEngineCache.getInstance().get(PHONE_ENGINE_ID);
        if (mPhoneEngine == null) {
            mPhoneEngine = new FlutterEngine(this);
            mPhoneEngine.getNavigationChannel().setInitialRoute("/");
            mPhoneEngine.getDartExecutor().executeDartEntrypoint(
                    DartExecutor.DartEntrypoint.createDefault()
            );
            FlutterEngineCache.getInstance().put(PHONE_ENGINE_ID, mPhoneEngine);
        }

        mPhoneChannel = new MethodChannel(
                mPhoneEngine.getDartExecutor().getBinaryMessenger(),
                PHONE_CHANNEL
        );
        mPhoneChannel.setMethodCallHandler((call, result) -> {
            if ("closePhone".equals(call.method)) {
                hidePhoneFromNative();
                result.success(null);
            } else if ("setGtaCameraMode".equals(call.method)) {
                int mode = call.arguments instanceof Number
                        ? ((Number) call.arguments).intValue()
                        : 0;
                try {
                    nativeSetGtaCameraMode(mode);
                    result.success(null);
                } catch (UnsatisfiedLinkError e) {
                    Log.e(TAG, "setGtaCameraMode indisponível", e);
                    result.error("NATIVE_CAMERA", e.getMessage(), null);
                }
            } else if ("captureGtaCamera".equals(call.method)) {
                captureGtaCamera(result);
            } else if ("requestBankData".equals(call.method)) {
                // Flutter pediu dados do banco -> manda packet 251 rpcID 100 pro servidor
                mBankRequestFromFlutter = true; // marca que veio do celular
                try {
                    nativeOnBankInteract();
                    result.success(null);
                } catch (UnsatisfiedLinkError e) {
                    mBankRequestFromFlutter = false;
                    Log.e(TAG, "requestBankData indisponível", e);
                    result.error("NATIVE_BANK", e.getMessage(), null);
                }
            } else if ("sendBankAction".equals(call.method)) {
                // Flutter quer enviar uma ação do banco -> manda pro nativo
                Object args = call.arguments;
                if (args instanceof Map) {
                    Map<?, ?> map = (Map<?, ?>) args;
                    String action = map.containsKey("action") ? String.valueOf(map.get("action")) : "";
                    String dataJson = map.containsKey("data") ? String.valueOf(map.get("data")) : "{}";
                    try {
                        nativeOnBankAction(action, dataJson, 1); // source=1: celular
                        result.success(null);
                    } catch (UnsatisfiedLinkError e) {
                        Log.e(TAG, "sendBankAction indisponível", e);
                        result.error("NATIVE_BANK", e.getMessage(), null);
                    }
                } else {
                    result.error("INVALID_ARGS", "Map esperado", null);
                }
            } else {
                result.notImplemented();
            }
        });

        mPhoneFragment = FlutterFragment.withCachedEngine(PHONE_ENGINE_ID)
                .renderMode(RenderMode.texture)
                .transparencyMode(TransparencyMode.transparent)
                .build();
        getSupportFragmentManager()
                .beginTransaction()
                .add(mPhoneContainer.getId(), mPhoneFragment, "lurion_phone")
                .commit();
    }

    private void captureGtaCamera(MethodChannel.Result result) {
        final String framePath;
        try {
            framePath = nativeCaptureSecondaryCameraFrame();
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Câmera secundária indisponível", e);
            result.error("NATIVE_CAMERA", e.getMessage(), null);
            return;
        }

        if (framePath == null || framePath.isEmpty()) {
            Log.e(TAG, "PHONE_CAM_ANDROID frame path unavailable");
            result.error("SECONDARY_FRAME_NOT_READY", "O raster secundário ainda não produziu um frame", null);
            return;
        }
        Log.i(TAG, "PHONE_CAM_ANDROID path=" + framePath);

        try (FileInputStream input = new FileInputStream(framePath);
             ByteArrayOutputStream output = new ByteArrayOutputStream()) {
            ByteArrayOutputStream rawOutput = new ByteArrayOutputStream();
            byte[] readBuffer = new byte[8192];
            int readCount;
            while ((readCount = input.read(readBuffer)) != -1) {
                rawOutput.write(readBuffer, 0, readCount);
            }
            byte[] raw = rawOutput.toByteArray();
            Log.i(TAG, "PHONE_CAM_ANDROID rawBytes=" + raw.length);
            if (raw.length < 12) {
                result.error("SECONDARY_FRAME_INVALID", "Cabeçalho RAW1 incompleto", null);
                return;
            }

            ByteBuffer header = ByteBuffer.wrap(raw).order(ByteOrder.LITTLE_ENDIAN);
            int magic = header.getInt();
            int width = header.getInt();
            int height = header.getInt();
            long expectedSize = 12L + (long) width * height * 4L;
            Log.i(TAG, "PHONE_CAM_ANDROID header magic=0x" + Integer.toHexString(magic)
                    + " width=" + width + " height=" + height + " expected=" + expectedSize);
            if (magic != 0x31574152 || width <= 0 || height <= 0 ||
                    expectedSize > raw.length || expectedSize > Integer.MAX_VALUE) {
                result.error("SECONDARY_FRAME_INVALID", "Cabeçalho ou tamanho RAW1 inválido", null);
                return;
            }

            int[] pixels = new int[width * height];
            int sourceOffset = 12;
            for (int y = 0; y < height; y++) {
                int targetRow = y * width;
                for (int x = 0; x < width; x++) {
                    // O nativo grava GL_RGBA/UNSIGNED_BYTE. A ordem correta
                    // para o Bitmap ARGB_8888 é R, G, B, A; não BGRA.
                    int r = raw[sourceOffset++] & 0xFF;
                    int g = raw[sourceOffset++] & 0xFF;
                    int b = raw[sourceOffset++] & 0xFF;
                    int a = raw[sourceOffset++] & 0xFF;
                    if (a == 0) a = 255;
                    pixels[targetRow + x] = (a << 24) | (r << 16) | (g << 8) | b;
                }
            }

            Bitmap bitmap = Bitmap.createBitmap(pixels, width, height, Bitmap.Config.ARGB_8888);
            boolean compressed = bitmap.compress(Bitmap.CompressFormat.PNG, 100, output);
            bitmap.recycle();
            byte[] encodedPng = output.toByteArray();
            Log.i(TAG, "PHONE_CAM_ANDROID bitmap compressed=" + compressed
                    + " pngBytes=" + encodedPng.length);
            result.success(Base64.encodeToString(encodedPng, Base64.NO_WRAP));
        } catch (IOException | RuntimeException e) {
            Log.e(TAG, "Falha ao ler frame RAW da câmera secundária", e);
            result.error("SECONDARY_FRAME_READ_FAILED", e.getMessage(), null);
        }
    }

    private void ensurePhoneCef() {
        if (mPhoneCefWebView != null) return;
        mPhoneCefWebView = new WebView(this);
        WebSettings settings = mPhoneCefWebView.getSettings();
        settings.setJavaScriptEnabled(true);
        settings.setDomStorageEnabled(true);
        settings.setAllowFileAccess(true);
        settings.setAllowContentAccess(true);
        settings.setLoadWithOverviewMode(false);
        settings.setUseWideViewPort(false);
        settings.setMediaPlaybackRequiresUserGesture(false);
        settings.setAllowFileAccessFromFileURLs(true);
        settings.setAllowUniversalAccessFromFileURLs(true);
        mPhoneCefWebView.setBackgroundColor(Color.TRANSPARENT);
        mPhoneCefWebView.setLayerType(View.LAYER_TYPE_HARDWARE, null);
        mPhoneCefWebView.setVisibility(View.GONE);
        mPhoneCefWebView.setWebChromeClient(new android.webkit.WebChromeClient() {
            @Override public void onPermissionRequest(final PermissionRequest request) {
                runOnUiThread(() -> request.grant(request.getResources()));
            }
        });
        mPhoneCefWebView.addJavascriptInterface(new PhoneCefBridge(), "SampPhone");
        mPhoneCefWebView.setWebViewClient(new android.webkit.WebViewClient() {
            @Override
            public void onPageFinished(WebView view, String url) {
                mPhoneCefPageReady = true;
            }
        });
        FrameLayout phoneLayer = new FrameLayout(this);
        phoneLayer.setBackgroundColor(Color.TRANSPARENT);
        addContentView(phoneLayer, new ViewGroup.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
        FrameLayout.LayoutParams phoneParams = new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT);
        phoneLayer.addView(mPhoneCefWebView, phoneParams);
        mPhoneCefWebView.setFocusable(true);
        mPhoneCefWebView.setFocusableInTouchMode(true);
        mPhoneCefWebView.loadUrl("file:///android_asset/phone/index.html");
    }

    public void showPhoneFromNative() {
        runOnUiThread(() -> {
            ensurePhoneCef();
            if (mPhoneCefWebView == null) return;
            mPhoneCefWebView.setVisibility(View.VISIBLE);
            mPhoneCefWebView.bringToFront();
            mPhoneCefWebView.requestFocus();
            mPhoneVisible = true;
        });
    }

    public void hidePhoneFromNative() {
        runOnUiThread(() -> {
            if (mPhoneCefWebView != null) mPhoneCefWebView.setVisibility(View.GONE);
            if (mPhoneContainer != null) {
                mPhoneContainer.setVisibility(View.GONE);
            }
            mPhoneVisible = false;
            try {
                nativeOnPhoneClosed();
            } catch (UnsatisfiedLinkError e) {
                Log.e(TAG, "nativeOnPhoneClosed indisponível", e);
            }
        });
    }

    public void togglePhoneFromNative() {
        if (mPhoneVisible) hidePhoneFromNative();
        else showPhoneFromNative();
    }

    private void notifyPhoneJavascript(String script) {
        runOnUiThread(() -> {
            if (mPhoneCefWebView != null) mPhoneCefWebView.evaluateJavascript(script, null);
        });
    }

    private static float phonePayloadFloat(String data, String key, float fallback) {
        if (data == null) return fallback;
        String prefix = key + "=";
        for (String part : data.split(";")) {
            if (!part.startsWith(prefix)) continue;
            try { return Float.parseFloat(part.substring(prefix.length())); }
            catch (NumberFormatException ignored) { return fallback; }
        }
        return fallback;
    }

    private String readGtaCameraDataUrl() {
        final String framePath;
        try {
            framePath = nativeCaptureSecondaryCameraFrame();
        } catch (UnsatisfiedLinkError e) {
            Log.e(TAG, "Câmera interna indisponível", e);
            return "";
        }
        if (framePath == null || framePath.isEmpty()) return "";
        try (FileInputStream input = new FileInputStream(framePath);
             ByteArrayOutputStream rawOutput = new ByteArrayOutputStream()) {
            byte[] buffer = new byte[8192];
            int count;
            while ((count = input.read(buffer)) != -1) rawOutput.write(buffer, 0, count);
            byte[] raw = rawOutput.toByteArray();
            if (raw.length < 12) return "";
            ByteBuffer header = ByteBuffer.wrap(raw).order(ByteOrder.LITTLE_ENDIAN);
            int magic = header.getInt();
            int width = header.getInt();
            int height = header.getInt();
            long expected = 12L + (long) width * height * 4L;
            if (magic != 0x31574152 || width <= 0 || height <= 0 || expected > raw.length || expected > Integer.MAX_VALUE) return "";
            int[] pixels = new int[width * height];
            int offset = 12;
            for (int y = 0; y < height; y++) {
                for (int x = 0; x < width; x++) {
                    int r = raw[offset++] & 0xFF;
                    int g = raw[offset++] & 0xFF;
                    int b = raw[offset++] & 0xFF;
                    int a = raw[offset++] & 0xFF;
                    if (a == 0) a = 255;
                    pixels[y * width + x] = (a << 24) | (r << 16) | (g << 8) | b;
                }
            }
            Bitmap bitmap = Bitmap.createBitmap(pixels, width, height, Bitmap.Config.ARGB_8888);
            ByteArrayOutputStream png = new ByteArrayOutputStream();
            boolean ok = bitmap.compress(Bitmap.CompressFormat.PNG, 100, png);
            bitmap.recycle();
            return ok ? "data:image/png;base64," + Base64.encodeToString(png.toByteArray(), Base64.NO_WRAP) : "";
        } catch (IOException | RuntimeException e) {
            Log.e(TAG, "Falha ao converter frame da câmera interna", e);
            return "";
        }
    }

    private void captureGtaCameraForPhone() {
        notifyPhoneJavascript("window.PhoneApp?.cameraNativeStatus('capturing','','')");
        new Thread(() -> {
            String dataUrl = readGtaCameraDataUrl();
            if (dataUrl.isEmpty()) {
                notifyPhoneJavascript("window.PhoneApp?.cameraNativeStatus('error','','Frame da câmera ainda não está pronto')");
                return;
            }
            String safe = dataUrl.replace("\\", "\\\\").replace("'", "\\'");
            notifyPhoneJavascript("window.PhoneApp?.cameraNativeStatus('captured','" + safe + "','" + safe + "');window.PhoneApp?.galleryNativeStatus('" + safe + "','image/png')");
        }, "phone-gta-camera-capture").start();
    }

    private void launchPhoneCamera(boolean video) {
        try {
            Intent intent = new Intent(video ? MediaStore.ACTION_VIDEO_CAPTURE : MediaStore.ACTION_IMAGE_CAPTURE);
            if (video) intent.putExtra(MediaStore.EXTRA_VIDEO_QUALITY, 1);
            startActivityForResult(intent, video ? PHONE_VIDEO_REQUEST : PHONE_CAMERA_REQUEST);
        } catch (RuntimeException e) {
            Log.e(TAG, "Camera indisponível", e);
            notifyPhoneJavascript("window.PhoneApp?.cameraNativeStatus('error','','Câmera indisponível')");
        }
    }

    public class PhoneCefBridge {
        @JavascriptInterface public void close() { hidePhoneFromNative(); }
        @JavascriptInterface public void action(String action, String data) {
            if ("camera_close".equals(action)) {
                try { nativeSetGtaCameraMode(0); } catch (UnsatisfiedLinkError ignored) { }
            } else if ("camera_mode".equals(action)) {
                String mode = data == null ? "" : data;
                try { nativeSetGtaCameraMode(mode.contains("mode=selfie") ? 1 : 2); }
                catch (UnsatisfiedLinkError e) { Log.e(TAG, "Modo câmera interna indisponível", e); }
            } else if ("camera_adjust".equals(action)) {
                float dx = phonePayloadFloat(data, "dx", 0.0f);
                float dy = phonePayloadFloat(data, "dy", 0.0f);
                try { nativeAdjustGtaCamera(dx, dy); } catch (UnsatisfiedLinkError ignored) { }
            }
        }
        @JavascriptInterface public void startVoice(String phone) { }
        @JavascriptInterface public void stopVoice() { }
        @JavascriptInterface public void cancelVoice() { }
        @JavascriptInterface public void setCallVoice(boolean enabled) { }
        @JavascriptInterface public void keepAudioAlive() { }
        @JavascriptInterface public void stopAudio() { }
        @JavascriptInterface public void playAudio(String url) { }
        @JavascriptInterface public void setSportfyAudioVolume(double volume) { }
        @JavascriptInterface public void stopSportfyAudio() { }
        @JavascriptInterface public void stopSportfyAmbientAudio() { }
        @JavascriptInterface public void updateSportfyAmbientAudio(String id, boolean playing) { }
        @JavascriptInterface public void prefetchSportfyAudio(String url) { }
        @JavascriptInterface public void searchSportfy(String query) { }
        @JavascriptInterface public String getSportfyVehicleAudioState() { return "{}"; }
        @JavascriptInterface public void capturePhoto(String mode, String callback) { }
        @JavascriptInterface public void capturePhoto(int left, int top, int width, int height, int viewportWidth, int viewportHeight) { captureGtaCameraForPhone(); }
        @JavascriptInterface public void captureVideo() { launchPhoneCamera(true); }
        @JavascriptInterface public String resolveMedia(String url) { return url == null ? "" : url; }
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {

        Log.i(TAG, "**** onCreate");
        super.onCreate(savedInstanceState);

        // Pré-carrega o Flutter enquanto o jogo termina de inicializar.
        // O container permanece GONE até o servidor enviar /phone.
        ensurePhoneFlutter();

        //if(!SignatureChecker.isSignatureValid(this, getPackageName()))
        //{
        //Toast.makeText(this, "Use original launcher! No remake", Toast.LENGTH_LONG).show();
        //return;
        //}

        //mHeightProvider = new HeightProvider(this);

        mDialog = new DialogManager(this);

        mKeyboard = new CustomKeyboard(this);

        mLoadingScreen = new LoadingScreen(this);

        // --- A solução final está aqui ---
        // Assim que a activity é criada, exibimos a tela de carregamento.
        // Não esperamos mais pela chamada vinda do C++.
        showLoadingScreen();

        // Inicia a música de fundo automaticamente
        // A playlist será configurada pelo C++ quando os recursos estiverem prontos
        // Por enquanto, apenas atualizamos o status inicial
        if (mLoadingScreen != null) {
            mLoadingScreen.updateStatus(LoadingScreen.STATUS_RESOURCES);
        }

        instance = this;

        // Prepara o WebView do banco com antecedência (evita delay ao abrir)
        ensureBankWebView();

        try {
            initializeSAMP();
        } catch (UnsatisfiedLinkError e5) {
            Log.e(TAG, e5.getMessage());
        }



    }

    private native void initializeSAMP();
    private native void nativeOnBankInteract();



    @Override
    public void onStart() {
        Log.i(TAG, "**** onStart");
        super.onStart();
    }

    @Override
    public void onRestart() {
        Log.i(TAG, "**** onRestart");
        super.onRestart();
    }

    @Override
    public void onResume() {
        Log.i(TAG, "**** onResume");
        super.onResume();
        //mHeightProvider.init(view);
    }

    @Override
    public void onPostResume() {
        super.onPostResume();
        if (mPhoneFragment != null) {
            mPhoneFragment.onPostResume();
        }
    }

    @Override
    protected void onNewIntent(Intent intent) {
        super.onNewIntent(intent);
        if (mPhoneFragment != null) {
            mPhoneFragment.onNewIntent(intent);
        }
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (mPhoneFragment != null) {
            mPhoneFragment.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == PHONE_CAMERA_REQUEST || requestCode == PHONE_VIDEO_REQUEST) {
            if (resultCode != RESULT_OK || data == null) {
                notifyPhoneJavascript("window.PhoneApp?.cameraNativeStatus('error','','Captura cancelada')");
            } else if (requestCode == PHONE_CAMERA_REQUEST && data.getExtras() != null && data.getExtras().get("data") instanceof Bitmap) {
                try {
                    Bitmap bitmap = (Bitmap) data.getExtras().get("data");
                    ByteArrayOutputStream output = new ByteArrayOutputStream();
                    bitmap.compress(Bitmap.CompressFormat.JPEG, 92, output);
                    String dataUrl = "data:image/jpeg;base64," + Base64.encodeToString(output.toByteArray(), Base64.NO_WRAP);
                    String safe = dataUrl.replace("\\", "\\\\").replace("'", "\\'");
                    notifyPhoneJavascript("window.PhoneApp?.cameraNativeStatus('captured','" + safe + "','" + safe + "');window.PhoneApp?.galleryNativeStatus('" + safe + "','image/jpeg')");
                } catch (RuntimeException e) {
                    notifyPhoneJavascript("window.PhoneApp?.cameraNativeStatus('error','','Falha ao salvar foto')");
                }
            } else if (requestCode == PHONE_VIDEO_REQUEST && data.getData() != null) {
                String uri = data.getData().toString().replace("\\", "\\\\").replace("'", "\\'");
                notifyPhoneJavascript("window.PhoneApp?.galleryNativeStatus('" + uri + "','video/mp4');window.PhoneApp?.cameraNativeStatus('error','','Vídeo salvo na galeria')");
            }
        }
        if (mPhoneFragment != null) {
            mPhoneFragment.onActivityResult(requestCode, resultCode, data);
        }
    }

    @Override
    public void onUserLeaveHint() {
        super.onUserLeaveHint();
        if (mPhoneFragment != null) {
            mPhoneFragment.onUserLeaveHint();
        }
    }

    @Override
    public void onTrimMemory(int level) {
        super.onTrimMemory(level);
        if (mPhoneFragment != null) {
            mPhoneFragment.onTrimMemory(level);
        }
    }

    public native void onEventBackPressed();

    @Override
    public void onBackPressed() {
        if (mPhoneVisible) {
            hidePhoneFromNative();
            return;
        }
        super.onBackPressed();
        onEventBackPressed();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK && mPhoneVisible) {
            hidePhoneFromNative();
            return true;
        }
        if(keyCode == KeyEvent.KEYCODE_BACK)
        {
            onEventBackPressed();
        }
        return super.onKeyDown(keyCode, event);
    }

    @Override
    public void onPause() {
        Log.i(TAG, "**** onPause");
        super.onPause();
    }

    @Override
    public void onStop() {
        Log.i(TAG, "**** onStop");
        super.onStop();
    }

    @Override
    public void onDestroy() {
        Log.i(TAG, "**** onDestroy");
        if (mPhoneVisible) {
            try {
                nativeOnPhoneClosed();
            } catch (UnsatisfiedLinkError e) {
                Log.e(TAG, "nativeOnPhoneClosed indisponível", e);
            }
            mPhoneVisible = false;
        }
        if (mLoadingScreen != null) {
            mLoadingScreen.releaseMusic();
        }
        super.onDestroy();
    }

    @Override
    public void onHeightChanged(int orientation, int height) {
        //mDialog.onHeightChanged(height);
    }

    public void setKeyboardOffset(int offset) {
        nativeSetKeyboardOffset(offset);
    }

    private native void nativeSetKeyboardOffset(int offset);

    public void updateKeyboardInput(String text) {
        nativeUpdateKeyboardInput(text);
    }

    private native void nativeUpdateKeyboardInput(String text);

    // Ponte WebUI banco (JS -> nativo)
    private static native void nativeOnBankAction(String actionType, String dataJson, int source);
    private static native void nativeOnBankClose();
    private static native void nativeOnPhoneClosed();
    private static native void nativeSetGtaCameraMode(int mode);
    private static native void nativeAdjustGtaCamera(float dx, float dy);
    private static native String nativeCaptureSecondaryCameraFrame();

    // =====================
    // WebUI de autenticação (pacote Login)
    // =====================
    private void ensureAuthWebView() {
        if (mAuthWebView != null) return;
        runOnUiThread(new Runnable() {
            @Override public void run() {
                if (mAuthWebView != null) return;
                mAuthWebView = new WebView(SAMP.this);
                WebSettings settings = mAuthWebView.getSettings();
                settings.setJavaScriptEnabled(true);
                settings.setDomStorageEnabled(true);
                settings.setLoadWithOverviewMode(true);
                settings.setUseWideViewPort(true);
                mAuthWebView.setBackgroundColor(Color.BLACK);
                mAuthWebView.setLayerType(View.LAYER_TYPE_HARDWARE, null);
                mAuthWebView.setVisibility(View.GONE);
                mAuthWebView.setWebChromeClient(new android.webkit.WebChromeClient());
                mAuthWebView.addJavascriptInterface(new AuthBridge(), "AuthBridge");
                mAuthWebView.setWebViewClient(new android.webkit.WebViewClient() {
                    @Override public void onPageFinished(WebView view, String url) { mAuthPageReady = true; }
                });
                FrameLayout host = new FrameLayout(SAMP.this);
                addContentView(host, new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
                host.addView(mAuthWebView, new FrameLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
                mAuthWebView.loadUrl("file:///android_asset/login/index.html");
            }
        });
    }

    public void showLogin(final String jsonData) {
        runOnUiThread(new Runnable() {
            @Override public void run() {
                ensureAuthWebView();
                if (mAuthWebView == null) return;
                mAuthWebView.setVisibility(View.VISIBLE);
                mAuthWebView.bringToFront();
                final String payload = (jsonData == null || jsonData.isEmpty()) ? "{}" : jsonData;
                execInAuth("window.AuthOpen && AuthOpen(" + payload + ");");
            }
        });
    }

    public void authResult(final String jsonData) {
        runOnUiThread(new Runnable() {
            @Override public void run() {
                execInAuth("window.AuthResult && AuthResult(" + (jsonData == null ? "{}" : jsonData) + ");");
            }
        });
    }

    public void hideLogin() {
        runOnUiThread(new Runnable() {
            @Override public void run() {
                if (mAuthWebView != null) mAuthWebView.setVisibility(View.GONE);
            }
        });
    }

    private void execInAuth(final String script) {
        mHandler.postDelayed(new Runnable() {
            @Override public void run() {
                if (mAuthWebView != null && mAuthPageReady) mAuthWebView.evaluateJavascript(script, null);
                else if (mAuthWebView != null) mHandler.postDelayed(this, 120);
            }
        }, 0);
    }

    public class AuthBridge {
        @JavascriptInterface public void login(String email, String password) {
            try { nativeOnAuthLogin(email == null ? "" : email.trim(), password == null ? "" : password); }
            catch (Throwable e) { Log.e(TAG, "AuthBridge.login falhou", e); }
        }
        // Entrada só com código: reaproveita o packet de login (e-mail = "code:<codigo>")
        @JavascriptInterface public void code(String code) {
            try { String c = code == null ? "" : code.trim(); nativeOnAuthLogin("code:" + c, c); }
            catch (Throwable e) { Log.e(TAG, "AuthBridge.code falhou", e); }
        }
        @JavascriptInterface public void register(String fullname, String email, String password) {
            try { nativeOnAuthRegister(fullname == null ? "" : fullname.trim(), email == null ? "" : email.trim(), password == null ? "" : password); }
            catch (Throwable e) { Log.e(TAG, "AuthBridge.register falhou", e); }
        }
    }

}

