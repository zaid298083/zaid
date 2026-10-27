package com.nexion.launcher.activity;

import android.Manifest;
import android.app.AlertDialog;
import android.os.Bundle;
import android.os.Environment;
import android.os.Build;
import android.content.pm.PackageManager;
import android.os.Handler;

import android.util.Log;
import android.widget.*;

import androidx.appcompat.widget.AppCompatButton;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;
import androidx.core.content.FileProvider;

import android.content.Intent;
import android.net.Uri;

import android.view.View.OnClickListener;
import android.view.View;
import android.view.ViewGroup;
import android.view.WindowManager;
import android.view.animation.AnimationUtils;
import android.view.animation.Animation;

import com.nexion.online.R;
import com.nexion.launcher.api.Api;
import com.nexion.launcher.other.Preferences;
import com.nexion.launcher.update.LauncherUpdateChecker;
import com.nexion.launcher.update.UpdateInfo;
import com.nexion.launcher.util.Util;

import org.json.JSONObject;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.*;
import java.util.regex.Pattern;

public class MainActivity extends AppCompatActivity {

    private View launcherAuthScreen;
    private String selectedServer = "ТЕСТ";
    private String serverPort = "1596";
    private static final int PERMISSION_REQUEST_CODE = 1000;
    private boolean permissionsGranted = false;
    private boolean isInitialLoad = true;
    private static final int LOADER_REQUEST_CODE = 1001;
    private static final int UPDATE_REQUEST_CODE = 1003;
    private static final int LAUNCHER_UPDATE_REQUEST_CODE = 1004;
    private static boolean isToastShown = false;
    private boolean isClientLocked = false;
    private ImageButton discordButton;
    private ImageButton instagramButton;
    private ImageButton youtubeButton;

    private AppCompatButton playButton;
    private EditText nicknameInput;
    private VideoView preGameConnectingVideo;
    private boolean isCheckingUpdate = false;
    private String currentVersion = "1.0.0";
    private boolean isCheckingLauncherUpdate = false;
    private View launcherLoadingOverlay;
    private ProgressBar launcherLoadingProgress;
    private final android.os.Handler launcherLoadingHandler =
            new android.os.Handler(android.os.Looper.getMainLooper());
    private android.animation.ValueAnimator launcherLoadingProgressAnimator;
    private long launcherLoadingStartedAt;
    private boolean launcherLoadingFinishing;
    private boolean launcherUpdatePromptVisible;
    private boolean launcherUpdateDownloading;
    private boolean launcherInstallStarted;
    private UpdateInfo pendingLauncherUpdate;
    private Thread launcherUpdateThread;
    private File downloadedLauncherApk;
    private static final long MIN_LAUNCHER_LOADING_MILLIS = 4800L;
    private static final long LAUNCHER_DOWNLOAD_UI_INTERVAL_MILLIS = 150L;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setTheme(R.style.Theme_AppCompat_Light_NoActionBar_FullScreen);
        setContentView(R.layout.activity_main);

        // Modo imersivo: esconde barra de status e barra de navegação
        getWindow().getDecorView().setSystemUiVisibility(
                View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY
                | View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                | View.SYSTEM_UI_FLAG_LAYOUT_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN
                | View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
                | View.SYSTEM_UI_FLAG_FULLSCREEN);

        startLauncherLoadingScreen();
        setLauncherLoadingProgress(8);
        checkClientLockStatus();
    }

    private void startLauncherLoadingScreen() {
        launcherLoadingOverlay = findViewById(R.id.launcher_loading_overlay);
        launcherLoadingProgress = findViewById(R.id.launcher_loading_progress);
        launcherAuthScreen = findViewById(R.id.launcher_auth_screen);
        if (launcherAuthScreen != null)
            launcherAuthScreen.setVisibility(View.GONE);
        if (launcherLoadingOverlay == null || launcherLoadingProgress == null)
            return;
        launcherLoadingStartedAt = android.os.SystemClock.uptimeMillis();
        launcherLoadingFinishing = false;
        launcherLoadingOverlay.setVisibility(View.VISIBLE);
        launcherLoadingOverlay.setAlpha(1.0f);
        setLauncherHomeVisible(false);
        launcherLoadingOverlay.bringToFront();
        launcherLoadingOverlay.post(() -> launcherLoadingOverlay.bringToFront());
        launcherLoadingProgress.setMax(100);
        launcherLoadingProgress.setProgress(0);

        // Animação do logo vindo de cima
        View logo = findViewById(R.id.launcher_loading_logo);
        if (logo != null) {
            Animation logoAnimation = AnimationUtils.loadAnimation(this, R.anim.slide_from_top);
            logo.startAnimation(logoAnimation);
        }
    }

    private void setLauncherLoadingProgress(int progress) {
        if (launcherLoadingProgress == null)
            return;
        int target = Math.max(0, Math.min(100, progress));
        if (launcherLoadingProgressAnimator != null)
            launcherLoadingProgressAnimator.cancel();
        launcherLoadingProgressAnimator = android.animation.ValueAnimator.ofInt(
                launcherLoadingProgress.getProgress(), target);
        launcherLoadingProgressAnimator.setDuration(650L);
        launcherLoadingProgressAnimator.setInterpolator(new android.view.animation.DecelerateInterpolator());
        launcherLoadingProgressAnimator.addUpdateListener(animation ->
                launcherLoadingProgress.setProgress((Integer) animation.getAnimatedValue()));
        launcherLoadingProgressAnimator.start();
    }

    private void finishLauncherLoadingScreen() {
        if (launcherLoadingOverlay == null || launcherLoadingOverlay.getVisibility() != View.VISIBLE ||
                launcherLoadingFinishing)
            return;
        if (launcherUpdatePromptVisible || launcherUpdateDownloading || launcherInstallStarted) {
            launcherLoadingHandler.postDelayed(this::finishLauncherLoadingScreen, 500L);
            return;
        }
        long elapsed = android.os.SystemClock.uptimeMillis() - launcherLoadingStartedAt;
        if (elapsed < MIN_LAUNCHER_LOADING_MILLIS) {
            launcherLoadingHandler.postDelayed(this::finishLauncherLoadingScreen,
                    MIN_LAUNCHER_LOADING_MILLIS - elapsed);
            return;
        }
        launcherLoadingFinishing = true;
        setLauncherLoadingProgress(100);
        launcherLoadingHandler.postDelayed(() -> {
            if (launcherLoadingOverlay == null)
                return;
            launcherLoadingOverlay.animate()
                    .alpha(0.0f)
                    .setDuration(260L)
                    .withEndAction(() -> {
                        launcherLoadingOverlay.setVisibility(View.GONE);
                        launcherLoadingOverlay.setAlpha(1.0f);
                        showLauncherHome();
                    })
                    .start();
        }, 720L);
    }

    private void checkClientLockStatus() {
        new Thread(() -> {
            try {
                String jsonString = downloadLockData();
                if (jsonString != null) {
                    JSONObject lockData = new JSONObject(jsonString);
                    isClientLocked = lockData.getBoolean("locked");

                    runOnUiThread(() -> {
                        setLauncherLoadingProgress(38);
                        if (isClientLocked) {
                            Intent lockIntent = new Intent(MainActivity.this, ClientActivity.class);
                            startActivity(lockIntent);
                            finish();
                        } else {
                            initializeMainActivity();
                        }
                    });
                } else {
                    runOnUiThread(() -> {
                        setLauncherLoadingProgress(38);
                        initializeMainActivity();
                    });
                }
            } catch (Exception e) {
                runOnUiThread(() -> {
                    setLauncherLoadingProgress(38);
                    initializeMainActivity();
                });
            }
        }).start();
    }

    private String downloadLockData() {
        try {
            java.net.URL url = new java.net.URL(Api.CLIENT_LOCK_URL);
            java.net.HttpURLConnection connection = (java.net.HttpURLConnection) url.openConnection();
            connection.setRequestMethod("GET");
            connection.setConnectTimeout(5000);
            connection.setReadTimeout(5000);

            InputStream inputStream = connection.getInputStream();
            java.util.Scanner scanner = new java.util.Scanner(inputStream, StandardCharsets.UTF_8.name());
            scanner.useDelimiter("\\A");
            String result = scanner.hasNext() ? scanner.next() : "";
            scanner.close();
            return result;

        } catch (Exception e) {
            return null;
        }
    }

    private void initializeMainActivity() {
        setLauncherLoadingProgress(52);
        getWindow().setFlags(
                android.view.WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED,
                android.view.WindowManager.LayoutParams.FLAG_HARDWARE_ACCELERATED
        );

        Animation animation = null;
        try {
            animation = AnimationUtils.loadAnimation(this, R.anim.button_click);
        } catch (Exception e) {
            animation = new Animation() {};
        }

        loadCurrentVersion();
        createNecessaryDirectories();
        loadServerFromIni();
        replaceTestServerWithMoscow();
        checkAndRequestPermissions();
        initUIComponents(animation);
        initSocialButtons(animation);
        loadServerSelection();
        setLauncherLoadingProgress(82);

        new Handler().postDelayed(this::LoadNick, 100);
        new Handler().postDelayed(() -> setLauncherLoadingProgress(88), 160);
        new Handler().postDelayed(this::checkLauncherUpdate, 250);
        launcherLoadingHandler.postDelayed(this::finishLauncherLoadingScreen, 9000L);
    }

    private void setLauncherHomeVisible(boolean visible) {
        int state = visible ? View.VISIBLE : View.GONE;
        View social = findViewById(R.id.social_layout);
        View play = findViewById(R.id.button_play);
        if (social != null) social.setVisibility(state);
        if (play != null) play.setVisibility(state);
    }

    private void showLauncherHome() {
        if (launcherAuthScreen != null)
            launcherAuthScreen.setVisibility(View.GONE);
        setLauncherHomeVisible(true);
    }

    private void updatePlayButtonState() {
        if (playButton == null)
            return;
        boolean installed = IsGameInstalled();
        playButton.setText(installed ? "JOGAR" : "ATUALIZAR");
        playButton.setContentDescription(installed ? "Jogar" : "Atualizar jogo");
        playButton.setCompoundDrawablesRelativeWithIntrinsicBounds(
                installed ? R.drawable.launcher_play_small : R.drawable.launcher_update_small,
                0,
                0,
                0);
    }

    private void checkLauncherUpdate() {
        if (isCheckingLauncherUpdate) return;
        isCheckingLauncherUpdate = true;

        LauncherUpdateChecker.checkForUpdate(this, new LauncherUpdateChecker.UpdateCheckListener() {
            @Override
            public void onUpdateAvailable(UpdateInfo updateInfo) {
                isCheckingLauncherUpdate = false;
                runOnUiThread(() -> {
                    pendingLauncherUpdate = updateInfo;
                    launcherUpdatePromptVisible = true;
                    setLauncherLoadingProgress(96);
                    setLauncherLoadingText(
                            "Atualização disponível",
                            "Nova versão: " + updateInfo.getVersion());
                    showLauncherUpdateDialog(updateInfo);
                });
            }

            @Override
            public void onNoUpdateAvailable() {
                isCheckingLauncherUpdate = false;
                runOnUiThread(() -> {
                    launcherUpdatePromptVisible = false;
                    pendingLauncherUpdate = null;
                    setLauncherLoadingProgress(100);
                    restoreLauncherLoadingText();
                    finishLauncherLoadingScreen();
                });
            }

            @Override
            public void onError(String errorMessage) {
                isCheckingLauncherUpdate = false;
                runOnUiThread(() -> {
                    launcherUpdatePromptVisible = false;
                    pendingLauncherUpdate = null;
                    setLauncherLoadingProgress(100);
                    restoreLauncherLoadingText();
                    finishLauncherLoadingScreen();
                });
            }
        });
    }

    private void showLauncherUpdateDialog(UpdateInfo updateInfo) {
        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("Atualização do launcher disponível")
                .setMessage("Uma nova versão do launcher está disponível: " + updateInfo.getVersion() +
                        "\nAtualize para receber novos recursos e correções.")
                .setPositiveButton("Atualizar", (dialog, which) -> {
                    startLauncherUpdateActivity(updateInfo);
                })
                .setNegativeButton("Mais tarde", (dialog, which) -> {
                    launcherUpdatePromptVisible = false;
                    pendingLauncherUpdate = null;
                    restoreLauncherLoadingText();
                    finishLauncherLoadingScreen();
                })
                .setCancelable(false)
                .show();
    }

    private void startLauncherUpdateActivity(UpdateInfo updateInfo) {
        if (updateInfo == null || updateInfo.getUpdateUrl() == null ||
                updateInfo.getUpdateUrl().trim().isEmpty()) {
            launcherUpdatePromptVisible = false;
            safeShowToast("URL da atualização não configurada");
            restoreLauncherLoadingText();
            finishLauncherLoadingScreen();
            return;
        }

        launcherUpdatePromptVisible = false;
        launcherUpdateDownloading = true;
        pendingLauncherUpdate = updateInfo;
        setLauncherLoadingProgressDirect(0);
        setLauncherLoadingText("Baixando atualização... 0%", "0 B / tamanho desconhecido");

        File updateDirectory = new File(getFilesDir(), "launcher_updates");
        if (!updateDirectory.exists() && !updateDirectory.mkdirs()) {
            onLauncherDownloadFailed("Não foi possível preparar o armazenamento da atualização");
            return;
        }

        String safeVersion = updateInfo.getVersion() == null ? "latest" :
                updateInfo.getVersion().replaceAll("[^A-Za-z0-9_.-]", "_");
        downloadedLauncherApk = new File(updateDirectory, "launcher_" + safeVersion + ".apk");
        if (downloadedLauncherApk.exists() && !downloadedLauncherApk.delete()) {
            onLauncherDownloadFailed("Não foi possível substituir o APK antigo");
            return;
        }

        final File targetApk = downloadedLauncherApk;
        launcherUpdateThread = new Thread(() -> downloadLauncherApk(updateInfo.getUpdateUrl(), targetApk),
                "NexionLauncherUpdate");
        launcherUpdateThread.start();
    }

    private void downloadLauncherApk(String updateUrl, File targetApk) {
        java.net.HttpURLConnection connection = null;
        long downloadedBytes = 0L;
        long totalBytes = -1L;
        long lastUiUpdate = 0L;

        try {
            java.net.URL url = new java.net.URL(updateUrl);
            connection = (java.net.HttpURLConnection) url.openConnection();
            connection.setRequestMethod("GET");
            connection.setConnectTimeout(15000);
            connection.setReadTimeout(30000);
            connection.setInstanceFollowRedirects(true);
            connection.setRequestProperty("User-Agent", "Nexion-Online-Launcher");

            int responseCode = connection.getResponseCode();
            if (responseCode < 200 || responseCode >= 300) {
                throw new IOException("HTTP " + responseCode);
            }

            totalBytes = connection.getContentLengthLong();
            try (InputStream input = connection.getInputStream();
                 FileOutputStream output = new FileOutputStream(targetApk)) {
                byte[] buffer = new byte[64 * 1024];
                int count;
                while ((count = input.read(buffer)) != -1) {
                    output.write(buffer, 0, count);
                    downloadedBytes += count;

                    long now = android.os.SystemClock.uptimeMillis();
                    if (now - lastUiUpdate >= LAUNCHER_DOWNLOAD_UI_INTERVAL_MILLIS) {
                        lastUiUpdate = now;
                        int progress = totalBytes > 0
                                ? (int) Math.min(99L, downloadedBytes * 100L / totalBytes)
                                : 0;
                        long currentDownloaded = downloadedBytes;
                        long currentTotal = totalBytes;
                        runOnUiThread(() -> updateLauncherDownloadStatus(
                                progress, currentDownloaded, currentTotal));
                    }
                }
                output.flush();
            }

            final long completedBytes = downloadedBytes;
            final long completedTotal = totalBytes;
            runOnUiThread(() -> onLauncherDownloadCompleted(targetApk, completedBytes, completedTotal));
        } catch (Exception e) {
            try {
                if (targetApk.exists())
                    targetApk.delete();
            } catch (Exception ignored) {
            }
            String detail = e.getMessage() == null ? "erro desconhecido" : e.getMessage();
            runOnUiThread(() -> onLauncherDownloadFailed(detail));
        } finally {
            if (connection != null)
                connection.disconnect();
        }
    }

    private void updateLauncherDownloadStatus(int progress, long downloadedBytes, long totalBytes) {
        if (!launcherUpdateDownloading)
            return;
        String percent = totalBytes > 0 ? progress + "%" : "...";
        String total = totalBytes > 0 ? Util.bytesIntoHumanReadable(totalBytes) : "tamanho desconhecido";
        setLauncherLoadingProgressDirect(progress);
        setLauncherLoadingText(
                "Baixando atualização... " + percent,
                formatLauncherBytes(downloadedBytes) + " / " + total);
    }

    private void onLauncherDownloadCompleted(File apkFile, long downloadedBytes, long totalBytes) {
        if (!launcherUpdateDownloading)
            return;
        launcherUpdateDownloading = false;
        launcherInstallStarted = true;
        setLauncherLoadingProgressDirect(100);
        long displayTotal = totalBytes > 0 ? totalBytes : downloadedBytes;
        setLauncherLoadingText(
                "Download concluído",
                formatLauncherBytes(downloadedBytes) + " / " +
                        formatLauncherBytes(displayTotal));
        launcherLoadingHandler.postDelayed(() -> launchLauncherApkInstaller(apkFile), 900L);
    }

    private void onLauncherDownloadFailed(String detail) {
        launcherUpdateDownloading = false;
        launcherInstallStarted = false;
        launcherUpdatePromptVisible = false;
        setLauncherLoadingText("Falha no download", "Tente novamente mais tarde");
        safeShowToast("Não foi possível baixar a atualização: " + detail);
        launcherLoadingHandler.postDelayed(() -> {
            restoreLauncherLoadingText();
            finishLauncherLoadingScreen();
        }, 1300L);
    }

    private void launchLauncherApkInstaller(File apkFile) {
        if (apkFile == null || !apkFile.exists()) {
            onLauncherDownloadFailed("Arquivo APK não encontrado");
            return;
        }

        try {
            Intent installIntent = new Intent(Intent.ACTION_VIEW);
            Uri apkUri;
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                apkUri = FileProvider.getUriForFile(this, getPackageName() + ".provider", apkFile);
            } else {
                apkUri = Uri.fromFile(apkFile);
            }
            installIntent.setDataAndType(apkUri, "application/vnd.android.package-archive");
            installIntent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION |
                    Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
            launcherInstallStarted = true;
            startActivityForResult(installIntent, LAUNCHER_UPDATE_REQUEST_CODE);
        } catch (Exception e) {
            launcherInstallStarted = false;
            onLauncherDownloadFailed("não foi possível abrir a instalação");
        }
    }

    private String formatLauncherBytes(long bytes) {
        return bytes <= 0 ? "0 B" : Util.bytesIntoHumanReadable(bytes);
    }

    private void setLauncherLoadingText(String title, String info) {
        TextView titleView = findViewById(R.id.launcher_loading_title);
        TextView infoView = findViewById(R.id.launcher_loading_terms);
        if (titleView != null)
            titleView.setText(title);
        if (infoView != null) {
            infoView.setText(info);
            infoView.setTextSize(9f);
            infoView.setVisibility(View.VISIBLE);
        }
    }

    private void restoreLauncherLoadingText() {
        TextView titleView = findViewById(R.id.launcher_loading_title);
        TextView infoView = findViewById(R.id.launcher_loading_terms);
        if (titleView != null)
            titleView.setText("Carregando o jogo");
        if (infoView != null) {
            infoView.setText("Eu já li e concordo com os Termos e Política de Privacidade");
            infoView.setTextSize(6f);
            infoView.setVisibility(View.VISIBLE);
        }
    }

    private void setLauncherLoadingProgressDirect(int progress) {
        if (launcherLoadingProgress == null)
            return;
        if (launcherLoadingProgressAnimator != null)
            launcherLoadingProgressAnimator.cancel();
        launcherLoadingProgress.setProgress(Math.max(0, Math.min(100, progress)));
    }

    private void loadCurrentVersion() {
        try {
            File versionFile = new File(Api.VERSION_FILE);
            if (versionFile.exists()) {
                Scanner scanner = new Scanner(versionFile);
                if (scanner.hasNextLine()) {
                    currentVersion = scanner.nextLine().trim();
                }
                scanner.close();
            } else {
                createVersionFile();
            }
        } catch (Exception e) {
            currentVersion = "1.0.0";
            createVersionFile();
        }
    }

    private void createVersionFile() {
        try {
            File versionFile = new File(Api.VERSION_FILE);
            if (!versionFile.getParentFile().exists()) {
                versionFile.getParentFile().mkdirs();
            }
            java.io.FileWriter writer = new java.io.FileWriter(versionFile);
            writer.write(currentVersion);
            writer.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void checkAndRequestPermissions() {
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.RECORD_AUDIO},
                    PERMISSION_REQUEST_CODE);
            return;
        }

        permissionsGranted = true;
        initializeApp();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_CODE) {
            boolean allGranted = true;
            for (int result : grantResults) {
                if (result != PackageManager.PERMISSION_GRANTED) {
                    allGranted = false;
                    break;
                }
            }

            permissionsGranted = allGranted;

            if (!allGranted) {
                safeShowToast("Разрешение на микрофон не предоставлено. Голосовой чат будет недоступен.");
            }
            initializeApp();
        }
    }

    private void initializeApp() {
        isInitialLoad = false;
    }

    private void initUIComponents(Animation animation) {
        launcherAuthScreen = findViewById(R.id.launcher_auth_screen);
        nicknameInput = findViewById(R.id.nickname_input);
        playButton = findViewById(R.id.button_play);
        updatePlayButtonState();

        if (playButton != null) {
            playButton.setOnClickListener(new OnClickListener() {
                public void onClick(View v) {
                    if (animation != null) {
                        v.startAnimation(animation);
                    }
                    new android.os.Handler().postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            onClickPlay();
                        }
                    }, 200L);
                }
            });
        }
    }

    private void initSocialButtons(Animation animation) {
        discordButton = findViewById(R.id.button_discord);
        instagramButton = findViewById(R.id.button_instagram);
        youtubeButton = findViewById(R.id.button_youtube);

        if (discordButton != null) {
            discordButton.setOnClickListener(new OnClickListener() {
                public void onClick(View v) {
                    if (animation != null) {
                        v.startAnimation(animation);
                    }
                    new android.os.Handler().postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            openSocialLink(Api.DISCORD_URL);
                        }
                    }, 200L);
                }
            });
        }

        if (instagramButton != null) {
            instagramButton.setOnClickListener(new OnClickListener() {
                public void onClick(View v) {
                    if (animation != null) {
                        v.startAnimation(animation);
                    }
                    new android.os.Handler().postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            openSocialLink(Api.INSTAGRAM_URL);
                        }
                    }, 200L);
                }
            });
        }

        if (youtubeButton != null) {
            youtubeButton.setOnClickListener(new OnClickListener() {
                public void onClick(View v) {
                    if (animation != null) {
                        v.startAnimation(animation);
                    }
                    new android.os.Handler().postDelayed(new Runnable() {
                        @Override
                        public void run() {
                            openSocialLink(Api.YOUTUBE_URL);
                        }
                    }, 200L);
                }
            });
        }
    }

    private void openSocialLink(String url) {
        try {
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse(url));
            startActivity(intent);
        } catch (Exception e) {
            safeShowToast("Не удалось открыть ссылку");
        }
    }

    private void createNecessaryDirectories() {
        File gameDir = new File(Api.GAME_CACHE_PATH);
        if (!gameDir.exists()) {
            gameDir.mkdirs();
        }

        File sampDir = new File(Api.GAME_CACHE_PATH, "SAMP");
        if (!sampDir.exists()) {
            sampDir.mkdirs();
        }

        File texdbDir = new File(Api.GAME_TEXDB_PATH);
        if (!texdbDir.exists()) {
            texdbDir.mkdirs();
        }

        File filesDir = new File(Api.GAME_INSTALL_PATH);
        if (!filesDir.exists()) {
            filesDir.mkdirs();
        }

        File settingsFile = new File(sampDir, "settings.ini");
        if (!settingsFile.exists()) {
            try {
                writeIniValue(settingsFile, "client", "name", "");
                writeIniValue(settingsFile, "client", "server", "ТЕСТ");
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        File serverIniFile = new File(sampDir, "serv.ini");
        if (!serverIniFile.exists()) {
            try {
                writeIniValue(serverIniFile, "server", "name", "ТЕСТ");
                writeIniValue(serverIniFile, "server", "port", "1596");
            } catch (Exception e) {
                e.printStackTrace();
            }
        }
    }

    private void loadServerFromIni() {
        try {
            File serverIniFile = new File(Api.GAME_CACHE_PATH, "SAMP/serv.ini");
            if (serverIniFile.exists()) {
                String serverName = readIniValue(serverIniFile, "server", "name");
                String serverPortFromIni = readIniValue(serverIniFile, "server", "port");

                if (serverName != null && !serverName.isEmpty()) {
                    selectedServer = serverName;
                }
                if (serverPortFromIni != null && !serverPortFromIni.isEmpty()) {
                    serverPort = serverPortFromIni;
                }
            }

        } catch (Exception e) {
            selectedServer = "ТЕСТ";
            serverPort = "1596";
        }
    }

    private void loadServerSelection() {
        try {
            File serverIniFile = new File(Api.GAME_CACHE_PATH, "SAMP/serv.ini");
            if (serverIniFile.exists()) {
                String serverName = readIniValue(serverIniFile, "server", "name");
                String serverPortFromIni = readIniValue(serverIniFile, "server", "port");

                if (serverName != null && !serverName.isEmpty()) {
                    selectedServer = serverName;
                }
                if (serverPortFromIni != null && !serverPortFromIni.isEmpty()) {
                    serverPort = serverPortFromIni;
                }
            }

            File settingsFile = new File(Api.GAME_CACHE_PATH, "SAMP/settings.ini");
            if (settingsFile.exists()) {
                String savedServer = readIniValue(settingsFile, "client", "server");
                if (savedServer != null && !savedServer.isEmpty() && selectedServer == null) {
                    selectedServer = savedServer;
                }
            }

        } catch (Exception e) {
            selectedServer = "ТЕСТ";
            serverPort = "1596";
        }
    }

    private void replaceTestServerWithMoscow() {
        try {
            File settingsFile = new File(Api.GAME_CACHE_PATH, "SAMP/settings.ini");
            if (settingsFile.exists()) {
                String savedServer = readIniValue(settingsFile, "client", "server");
                if (savedServer != null && savedServer.equals("ТЕСТ")) {
                    writeIniValue(settingsFile, "client", "server", "ТЕСТ");
                    selectedServer = "ТЕСТ";
                }
            }

            File serverIniFile = new File(Api.GAME_CACHE_PATH, "SAMP/serv.ini");
            if (serverIniFile.exists()) {
                String serverName = readIniValue(serverIniFile, "server", "name");
                if (serverName != null && serverName.equals("ТЕСТ")) {
                    writeIniValue(serverIniFile, "server", "name", "ТЕСТ");
                    selectedServer = "ТЕСТ";
                }
            }

            if (selectedServer != null && selectedServer.equals("ТЕСТ")) {
                selectedServer = "ТЕСТ";
            }

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void saveServerSelection() {
        try {
            File settingsDir = getWritableGameCacheDir();
            if (!settingsDir.exists() && !settingsDir.mkdirs())
                throw new IOException("Não foi possível criar a pasta SAMP");
            if (!settingsDir.isDirectory())
                throw new IOException("A pasta SAMP não é um diretório");

            File f = new File(settingsDir, "settings.ini");
            writeIniValue(f, "client", "server", selectedServer);
            writeIniValue(f, "client", "name", Preferences.getNick());
        } catch (Exception e) {
            safeShowToast("Ошибка сохранения сервера: " + e.getMessage());
        }
    }

    private File getWritableGameCacheDir() {
        File externalFiles = getExternalFilesDir(null);
        if (externalFiles != null)
            return new File(externalFiles, "SAMP");
        return new File(Api.GAME_CACHE_PATH, "SAMP");
    }

    public void onClickPlay() {
        if (isCheckingUpdate) {
            return;
        }

        if (!permissionsGranted) {
            checkAndRequestPermissions();
            return;
        }

        if (selectedServer == null || selectedServer.isEmpty()) {
            return;
        }

        if (!IsGameInstalled()) {
            ToLoad();
            return;
        }

        String nickText = nicknameInput == null
                ? Preferences.getNick()
                : nicknameInput.getText().toString().trim();
        if (nickText.isEmpty()) {
            nickText = Preferences.getNick();
        }
        Preferences.setNick(nickText);
        saveNicknamePersistently(nickText);
        if (!checkValidNick(nickText)) {
            return;
        }

        saveServerSelection();

        // Exibe o vídeo antes da checagem de atualização, que pode levar
        // alguns segundos, evitando a tela congelada após tocar em Jogar.
        showImmediateConnectingVideo();
        checkForUpdatesBeforePlay();
    }

    private void checkForUpdatesBeforePlay() {
        isCheckingUpdate = true;

        new Thread(() -> {
            try {
                java.net.URL url = new java.net.URL(Api.UPDATE_JSON);
                java.net.HttpURLConnection connection = (java.net.HttpURLConnection) url.openConnection();
                connection.setRequestMethod("GET");
                connection.setConnectTimeout(5000);
                connection.setReadTimeout(5000);

                InputStream inputStream = connection.getInputStream();
                java.util.Scanner scanner = new java.util.Scanner(inputStream, StandardCharsets.UTF_8.name());
                scanner.useDelimiter("\\A");
                String result = scanner.hasNext() ? scanner.next() : "";
                scanner.close();

                JSONObject jsonObject = new JSONObject(result);
                boolean updateAvailable = jsonObject.getBoolean("update");
                String updateUrl = jsonObject.getString("update_url");
                String newVersion = jsonObject.getString("version");

                runOnUiThread(() -> {
                    isCheckingUpdate = false;

                    if (updateAvailable && !currentVersion.equals(newVersion)) {
                        startUpdateActivity(updateUrl, newVersion);
                    } else {
                        startGame();
                    }
                });

            } catch (Exception e) {
                runOnUiThread(() -> {
                    isCheckingUpdate = false;
                    startGame();
                });
            }
        }).start();
    }

    private void startUpdateActivity(String updateUrl, String newVersion) {
        try {
            Intent updateIntent = new Intent(MainActivity.this, UpdateActivity.class);
            updateIntent.putExtra("update_url", updateUrl);
            updateIntent.putExtra("new_version", newVersion);
            startActivityForResult(updateIntent, UPDATE_REQUEST_CODE);
        } catch (Exception e) {
            safeShowToast("Ошибка запуска обновления: " + e.getMessage());
            startGame();
        }
    }

    private void startGame() {
        try {
            hideImmediateConnectingVideo();
            Class<?> SAMPClass = Class.forName("com.nexion.online.SAMP");
            Intent gameIntent = new Intent(this, SAMPClass);
            startActivity(gameIntent);

        } catch (ClassNotFoundException e) {
            safeShowToast("Класс игры не найден. Запустите установку.");
            ToLoad();
        } catch (Exception e) {
            safeShowToast("Ошибка запуска игры: " + e.getMessage());
            ToLoad();
        }
    }

    private void showImmediateConnectingVideo() {
        if (preGameConnectingVideo != null) return;
        preGameConnectingVideo = new VideoView(this);
        preGameConnectingVideo.setBackgroundColor(0xFF000000);
        preGameConnectingVideo.setVideoURI(Uri.parse("android.resource://" + getPackageName() + "/" + R.raw.connecting_video));
        preGameConnectingVideo.setOnPreparedListener(mp -> {
            mp.setLooping(true);
            mp.setVolume(1.0f, 1.0f);
            preGameConnectingVideo.start();
        });
        preGameConnectingVideo.setOnErrorListener((mp, what, extra) -> true);
        addContentView(preGameConnectingVideo, new FrameLayout.LayoutParams(-1, -1));
        preGameConnectingVideo.bringToFront();
        preGameConnectingVideo.start();
    }

    private void hideImmediateConnectingVideo() {
        if (preGameConnectingVideo == null) return;
        preGameConnectingVideo.stopPlayback();
        ViewGroup parent = (ViewGroup) preGameConnectingVideo.getParent();
        if (parent != null) parent.removeView(preGameConnectingVideo);
        preGameConnectingVideo = null;
    }

    private boolean IsGameInstalled() {
        File fteamFile = new File(Api.FTEAM_FILE);
        if (fteamFile.exists() && checkFteamFile(fteamFile)) {
            return true;
        }

        File gameFile = new File(Api.GAME_TEXDB_PATH, "gta3.img");
        if (gameFile.exists() && gameFile.length() > 1000000) {
            return true;
        }

        return false;
    }

    private boolean checkFteamFile(File fteamFile) {
        try {
            if (fteamFile.exists() && fteamFile.length() > 0) {
                Scanner scanner = new Scanner(fteamFile);
                if (scanner.hasNextLine()) {
                    String firstLine = scanner.nextLine().trim();
                    scanner.close();
                    return firstLine.equals("created by fteam (ferkus team)");
                }
                scanner.close();
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    private void ToLoad() {
        try {
            Intent loaderIntent = new Intent(this, InstallActivity.class);
            startActivityForResult(loaderIntent, LOADER_REQUEST_CODE);
        } catch (Exception e) {
            safeShowToast("Ошибка запуска загрузчика: " + e.getMessage());
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);

        if (requestCode == LOADER_REQUEST_CODE) {
            if (resultCode == RESULT_OK) {
                safeShowToast("Игра успешно установлена!");
                loadCurrentVersion();
                updatePlayButtonState();

                new Handler().postDelayed(() -> {
                    updatePlayButtonState();
                    if (IsGameInstalled()) {
                        safeShowToast("Игра готова к запуску!");
                    } else {
                        safeShowToast("Игра не найдена после установки. Попробуйте еще раз.");
                    }
                }, 1000);
            } else {
                safeShowToast("Установка не завершена");
            }
        }

        if (requestCode == UPDATE_REQUEST_CODE) {
            if (resultCode == RESULT_OK) {
                loadCurrentVersion();
                safeShowToast("Atualização instalada com sucesso!");
            } else {
                safeShowToast("Atualização não instalada. Iniciando a versão atual...");
                startGame();
            }
        }

        if (requestCode == LAUNCHER_UPDATE_REQUEST_CODE) {
            launcherInstallStarted = false;
            final String expectedVersion = pendingLauncherUpdate == null ? null :
                    pendingLauncherUpdate.getVersion();
            launcherLoadingHandler.postDelayed(() -> {
                if (expectedVersion != null && isLauncherVersionInstalled(expectedVersion)) {
                    safeShowToast("Launcher atualizado. O aplicativo será reiniciado.");
                    restartApp();
                } else {
                    safeShowToast("Instalação cancelada. O launcher atual continuará aberto.");
                    restoreLauncherLoadingText();
                    finishLauncherLoadingScreen();
                }
            }, 700L);
        }
    }

    private boolean isLauncherVersionInstalled(String expectedVersion) {
        try {
            android.content.pm.PackageInfo packageInfo = getPackageManager()
                    .getPackageInfo(getPackageName(), 0);
            String installedVersion = packageInfo.versionName;
            if (installedVersion == null || expectedVersion == null)
                return false;
            return installedVersion.equals(expectedVersion) ||
                    installedVersion.replace("build_", "").equals(expectedVersion.replace("build_", ""));
        } catch (Exception e) {
            return false;
        }
    }

    private void restartApp() {
        Intent intent = getPackageManager().getLaunchIntentForPackage(getPackageName());
        if (intent != null) {
            intent.addFlags(Intent.FLAG_ACTIVITY_CLEAR_TOP);
            startActivity(intent);
            finish();
        }
    }

    public void InitLogic() {
        try {
            File f = new File(Api.GAME_CACHE_PATH, "SAMP/settings.ini");
            if (f.exists()) {
                String savedServer = readIniValue(f, "client", "server");
                if (savedServer != null && !savedServer.isEmpty()) {
                    selectedServer = savedServer;
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void LoadNick() {
        String nick = getSharedPreferences("duster_launcher", MODE_PRIVATE)
                .getString("nickname", null);
        if (nick == null || nick.trim().isEmpty()) {
            nick = Preferences.getNick();
        }
        if (nick == null || nick.trim().isEmpty()) {
            try {
                nick = readIniValue(new File(getWritableGameCacheDir(), "settings.ini"),
                        "client", "name");
            } catch (IOException ignored) {
                nick = null;
            }
        }
        if (nick == null || nick.trim().isEmpty()) {
            nick = "Player_Name";
        }
        nick = nick.trim();
        Preferences.setNick(nick);
        saveNicknamePersistently(nick);
        if (nicknameInput != null)
            nicknameInput.setText(nick);
    }

    private void saveNicknamePersistently(String nick) {
        if (nick == null || nick.trim().isEmpty()) return;
        getSharedPreferences("duster_launcher", MODE_PRIVATE)
                .edit().putString("nickname", nick.trim()).apply();
        try {
            writeIniValue(new File(getWritableGameCacheDir(), "settings.ini"),
                    "client", "name", nick.trim());
        } catch (IOException e) {
            Log.w("MainActivity", "Não foi possível sincronizar o nickname", e);
        }
    }

    private boolean checkValidNick(String nick) {
        if (nick == null || nick.trim().isEmpty()) {
            safeShowToast("Введите никнейм");
            return false;
        }

        if (nick.length() < 3 || nick.length() > 20) {
            safeShowToast("Ник должен быть от 3 до 20 символов");
            return false;
        }

        if (!Pattern.matches("^[a-zA-Z_]+$", nick)) {
            safeShowToast("Ник может содержать только буквы и подчеркивания");
            return false;
        }

        if (!nick.contains("_")) {
            safeShowToast("Ник должен содержать символ подчеркивания (_)");
            return false;
        }

        return true;
    }

    private void safeShowToast(String message) {
        if (isToastShown) return;
        isToastShown = true;

        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                try {
                    Toast.makeText(MainActivity.this, message, Toast.LENGTH_SHORT).show();
                    new Handler().postDelayed(() -> isToastShown = false, 2000);
                } catch (Exception e) {
                    isToastShown = false;
                }
            }
        });
    }

    public void setSelectedServer(String serverName, String port) {
        this.selectedServer = serverName;
        this.serverPort = port;

        try {
            File serverIniFile = new File(Api.GAME_CACHE_PATH, "SAMP/serv.ini");
            writeIniValue(serverIniFile, "server", "name", serverName);
            writeIniValue(serverIniFile, "server", "port", port);

            File settingsFile = new File(Api.GAME_CACHE_PATH, "SAMP/settings.ini");
            writeIniValue(settingsFile, "client", "server", serverName);

            safeShowToast("Сервер изменен на: " + serverName);

        } catch (Exception e) {
            safeShowToast("Ошибка сохранения сервера");
        }
    }

    private void writeIniValue(File iniFile, String section, String key, String value) throws IOException {
        if (!iniFile.exists()) {
            iniFile.getParentFile().mkdirs();
            iniFile.createNewFile();
        }

        List<String> lines = new ArrayList<>();
        boolean fileExists = iniFile.exists();

        if (fileExists) {
            try (Scanner scanner = new Scanner(iniFile)) {
                while (scanner.hasNextLine()) {
                    lines.add(scanner.nextLine());
                }
            }
        }

        boolean sectionFound = false;
        boolean keyFound = false;
        List<String> newLines = new ArrayList<>();

        for (int i = 0; i < lines.size(); i++) {
            String line = lines.get(i).trim();

            if (line.equals("[" + section + "]")) {
                sectionFound = true;
                newLines.add(line);

                boolean keyAdded = false;
                for (int j = i + 1; j < lines.size(); j++) {
                    String nextLine = lines.get(j).trim();
                    if (nextLine.startsWith("[") && nextLine.endsWith("]")) {
                        if (!keyAdded) {
                            newLines.add(key + " = " + value);
                            keyFound = true;
                            keyAdded = true;
                        }
                        newLines.add(nextLine);
                        i = j;
                        break;
                    } else if (nextLine.startsWith(key + " =")) {
                        newLines.add(key + " = " + value);
                        keyFound = true;
                        keyAdded = true;
                        i = j;
                    } else if (j == lines.size() - 1) {
                        newLines.add(nextLine);
                        if (!keyAdded) {
                            newLines.add(key + " = " + value);
                            keyFound = true;
                        }
                        i = j;
                    } else {
                        newLines.add(nextLine);
                        i = j;
                    }
                }
            } else {
                newLines.add(lines.get(i));
            }
        }

        if (!sectionFound) {
            newLines.add("[" + section + "]");
            newLines.add(key + " = " + value);
            keyFound = true;
        } else if (!keyFound) {
            List<String> finalLines = new ArrayList<>();
            boolean inTargetSection = false;
            boolean keyAdded = false;

            for (String line : newLines) {
                finalLines.add(line);
                if (line.equals("[" + section + "]")) {
                    inTargetSection = true;
                } else if (inTargetSection && !keyAdded) {
                    if (line.startsWith("[") && line.endsWith("]")) {
                        finalLines.add(finalLines.size() - 1, key + " = " + value);
                        keyAdded = true;
                    }
                }
            }

            if (!keyAdded) {
                finalLines.add(key + " = " + value);
            }

            newLines = finalLines;
        }

        try (FileOutputStream fos = new FileOutputStream(iniFile)) {
            for (String line : newLines) {
                fos.write((line + "\n").getBytes());
            }
        }
    }

    private String readIniValue(File iniFile, String section, String key) throws IOException {
        if (!iniFile.exists()) {
            return null;
        }

        try (Scanner scanner = new Scanner(iniFile)) {
            boolean inTargetSection = false;

            while (scanner.hasNextLine()) {
                String line = scanner.nextLine().trim();

                if (line.startsWith("[") && line.endsWith("]")) {
                    String currentSection = line.substring(1, line.length() - 1);
                    inTargetSection = currentSection.equals(section);
                } else if (inTargetSection && line.startsWith(key + " =")) {
                    return line.substring(key.length() + 3).trim();
                }
            }
        }

        return null;
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (!isInitialLoad) {
            LoadNick();
            loadServerSelection();
            updatePlayButtonState();
        }
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
    }

    @Override
    protected void onPause() {
        super.onPause();
    }

    @Override
    protected void onStop() {
        if (nicknameInput != null) {
            String nick = nicknameInput.getText().toString().trim();
            if (!nick.isEmpty()) {
                Preferences.setNick(nick);
                saveNicknamePersistently(nick);
            }
        }
        super.onStop();
    }
}
