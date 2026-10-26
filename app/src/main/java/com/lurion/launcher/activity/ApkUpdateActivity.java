package com.nexion.launcher.activity;

import android.Manifest;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.os.Looper;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Toast;

import androidx.core.app.ActivityCompat;
import androidx.core.app.NotificationCompat;
import androidx.core.app.NotificationManagerCompat;
import androidx.core.content.FileProvider;
import androidx.appcompat.app.AppCompatActivity;

import com.akexorcist.roundcornerprogressbar.RoundCornerProgressBar;
import com.nexion.online.R;
import com.nexion.launcher.util.Util;

import com.liulishuo.filedownloader.BaseDownloadTask;
import com.liulishuo.filedownloader.FileDownloadSampleListener;
import com.liulishuo.filedownloader.FileDownloader;

import java.io.File;

public class ApkUpdateActivity extends AppCompatActivity {
    private static final String CHANNEL_ID = "launcher_update_channel";
    private static final int NOTIFICATION_ID = 3;
    private static final int PERMISSION_REQUEST_CODE = 1003;
    private static final String APK_FILENAME = "launcher_update.apk";

    RoundCornerProgressBar progressbar;
    File folder;
    private NotificationManagerCompat notificationManager;
    private NotificationCompat.Builder notificationBuilder;
    private Handler mainHandler;
    private boolean isDownloadInProgress = false;
    private static boolean isToastShown = false;

    private String updateUrl;
    private String newVersion;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mainHandler = new Handler(Looper.getMainLooper());

        Intent intent = getIntent();
        if (intent != null && intent.hasExtra("update_url") && intent.hasExtra("new_version")) {
            updateUrl = intent.getStringExtra("update_url");
            newVersion = intent.getStringExtra("new_version");
        } else {
            safeShowToast("Erro: dados de atualização não recebidos");
            finish();
            return;
        }

 
        if (isVersionAlreadyInstalled(newVersion)) {
            safeShowToast("Esta versão do launcher já está instalada");
            finish();
            return;
        }

         
        if (isApkAlreadyDownloaded(newVersion)) {
            safeShowToast("A atualização já foi baixada, iniciando a instalação...");
            installUpdate(newVersion);
            return;
        }

        createNotificationChannel();
        notificationManager = NotificationManagerCompat.from(this);

        initializeUI();
    }

    

    private void initializeUI() {
        setContentView(R.layout.activity_apkupdate);
        progressbar = findViewById(R.id.progress_bar);

        FileDownloader.setup(this);
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

        showStartNotification();
        startDownload();
    }

    private void createNotificationChannel() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            NotificationChannel channel = new NotificationChannel(
                    CHANNEL_ID,
                    "Atualização do launcher",
                    NotificationManager.IMPORTANCE_LOW
            );
            channel.setDescription("Notificações de download de atualizações do launcher");

            NotificationManager manager = getSystemService(NotificationManager.class);
            if (manager != null) {
                manager.createNotificationChannel(channel);
            }
        }
    }

    private void showStartNotification() {
        Intent intent = new Intent(this, ApkUpdateActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        notificationBuilder = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setContentTitle("Atualização do launcher")
                .setContentText("Preparando o download...")
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentIntent(pendingIntent)
                .setPriority(NotificationCompat.PRIORITY_LOW)
                .setOngoing(true)
                .setOnlyAlertOnce(true)
                .setProgress(0, 0, true);

        if (notificationManager.areNotificationsEnabled()) {
            notificationManager.notify(NOTIFICATION_ID, notificationBuilder.build());
        }
    }

    private void updateNotification(String title, String text, int progress, int maxProgress, boolean indeterminate) {
        if (notificationBuilder != null) {
            notificationBuilder
                    .setContentTitle(title)
                    .setContentText(text)
                    .setProgress(maxProgress, progress, indeterminate);

            if (notificationManager.areNotificationsEnabled()) {
                notificationManager.notify(NOTIFICATION_ID, notificationBuilder.build());
            }
        }
    }

    private void showCompletionNotification() {
        Intent intent = new Intent(this, MainActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        NotificationCompat.Builder builder = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setContentTitle("Atualização concluída")
                .setContentText("Launcher atualizado com sucesso!")
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentIntent(pendingIntent)
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                .setAutoCancel(true);

        if (notificationManager.areNotificationsEnabled()) {
            notificationManager.notify(NOTIFICATION_ID + 1, builder.build());
        }
    }

    public void startDownload() {
        if (isDownloadInProgress) {
            return;
        }
        isDownloadInProgress = true;

        folder = getExternalFilesDir("updates");
        if (folder == null || (!folder.exists() && !folder.mkdirs())) {
            safeShowToast("Não foi possível criar a pasta de download");
            finish();
            return;
        }

        updateNotification("Baixando atualização", "Iniciando download...", 0, 100, true);

        createDownloadTask(updateUrl, folder.getPath(), newVersion).start();
    }

    private boolean isExternalStorageWritable() {
        String state = Environment.getExternalStorageState();
        return Environment.MEDIA_MOUNTED.equals(state);
    }

    private BaseDownloadTask createDownloadTask(String url, String path, String version) {
        String apkFileName = String.format(APK_FILENAME, version.replace("build_", "").replace(".", "_"));

        return FileDownloader.getImpl().create(url)
                .setPath(path + "/" + apkFileName)
                .setCallbackProgressTimes(100)
                .setMinIntervalUpdateSpeed(100)
                .setListener(new FileDownloadSampleListener() {

                    @Override
                    protected void pending(BaseDownloadTask task, int soFarBytes, int totalBytes) {
                        updateNotification("Baixando atualização", "Preparando o download...", 0, 100, true);
                    }

                    @Override
                    protected void progress(BaseDownloadTask task, int soFarBytes, int totalBytes) {
                        if (totalBytes <= 0) return;

                        long progressPercent = soFarBytes * 100L / totalBytes;

                        mainHandler.post(() -> {
                            RoundCornerProgressBar progressbar = findViewById(R.id.progress_bar);
                            TextView textprogress = findViewById(R.id.loading_percent);
                            TextView textloading = findViewById(R.id.loading_text);

                            if (textloading != null) {
                                textloading.setText("Baixando atualização do launcher...");
                            }
                            if (textprogress != null) {
                                textprogress.setText(Util.bytesIntoHumanReadable(soFarBytes) + " de " +
                                        Util.bytesIntoHumanReadable(totalBytes));
                            }
                            if (progressbar != null) {
                                progressbar.setProgress((int) progressPercent);
                            }
                        });

                        updateNotification("Baixando atualização do launcher",
                                "Download: " + Util.bytesIntoHumanReadable(soFarBytes) + " / " + Util.bytesIntoHumanReadable(totalBytes),
                                (int) progressPercent, 100, false);
                    }

                    @Override
                    protected void error(BaseDownloadTask task, Throwable e) {
                        isDownloadInProgress = false;
                        safeShowToast("Ocorreu um erro ao baixar a atualização");
                        updateNotification("Erro", "Erro no download", 0, 100, false);
                        finish();
                    }

                    @Override
                    protected void connected(BaseDownloadTask task, String etag, boolean isContinue, int soFarBytes, int totalBytes) {
                        updateNotification("Baixando atualização", "Conectado ao servidor", 0, 100, true);
                    }

                    @Override
                    protected void completed(BaseDownloadTask task) {
                        isDownloadInProgress = false;
                        mainHandler.post(() -> {
                            TextView textprogress = findViewById(R.id.loading_percent);
                            TextView textloading = findViewById(R.id.loading_text);

                            if (textloading != null) {
                                textloading.setText("Instalando atualização do launcher...");
                            }
                            if (textprogress != null) {
                                textprogress.setText("Concluído");
                            }
                        });

                        updateNotification("Atualização do launcher", "Download concluído, instalando...", 100, 100, false);
                        installUpdate(newVersion);
                    }
                });
    }

    public void installUpdate(String version){
        String apkFileName = String.format(APK_FILENAME, version.replace("build_", "").replace(".", "_"));
        File apkFile = new File(getExternalFilesDir("updates"), apkFileName);
        String apkPath = apkFile.getAbsolutePath();

        mainHandler.post(() -> {
            TextView textloading = findViewById(R.id.loading_text);
            if (textloading != null) {
                textloading.setText("Instalando atualização do launcher...");
            }
        });

        updateNotification("Instalação", "Instalando atualização do launcher...", 0, 0, true);

        if (apkFile.exists()) {
            Intent intent = new Intent(Intent.ACTION_VIEW);
            Uri apkUri;

            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
                apkUri = FileProvider.getUriForFile(ApkUpdateActivity.this,
                        getPackageName() + ".provider", apkFile);
                // A permissão precisa permanecer ativa para o instalador ler o APK.
                intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION);
            } else {
                apkUri = Uri.fromFile(apkFile);
            }

            intent.setDataAndType(apkUri, "application/vnd.android.package-archive");
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP |
                    Intent.FLAG_GRANT_READ_URI_PERMISSION);

            try {
                startActivity(intent);
                showCompletionNotification();

            
                mainHandler.postDelayed(() -> finish(), 1000);

            } catch (Exception e) {
                safeShowToast("Não foi possível abrir a instalação do APK: " + e.getMessage());
                finish();
            }
        } else {
            safeShowToast("Arquivo APK não encontrado: " + apkPath);
            finish();
        }
    }

    /**
     * Verifica se esta versão do launcher já está instalada
     */
    private boolean isVersionAlreadyInstalled(String newVersion) {
        try {
            String currentVersion = getCurrentVersion();
            return currentVersion.equals(newVersion);
        } catch (Exception e) {
            return false;
        }
    }

    /**
     * Verifica se o APK desta versão já foi baixado
     */
    private boolean isApkAlreadyDownloaded(String version) {
        String apkFileName = String.format(APK_FILENAME, version.replace("build_", "").replace(".", "_"));
        File apkFile = new File(getExternalFilesDir("updates"), apkFileName);
        return apkFile.exists();
    }

    /**
     * Obtém a versão atual do aplicativo
     */
    private String getCurrentVersion() {
        try {
            PackageInfo pInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
            return pInfo.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            return "unknown";
        }
    }

    private void safeShowToast(String message) {
        if (isToastShown) return;
        isToastShown = true;

        mainHandler.post(() -> {
            try {
                Toast.makeText(ApkUpdateActivity.this, message, Toast.LENGTH_LONG).show();
                mainHandler.postDelayed(() -> isToastShown = false, 3000);
            } catch (Exception e) {
                isToastShown = false;
            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if (isDownloadInProgress) {
            FileDownloader.getImpl().pauseAll();
        }
        if (notificationManager != null) {
            notificationManager.cancel(NOTIFICATION_ID);
        }
        if (mainHandler != null) {
            mainHandler.removeCallbacksAndMessages(null);
        }
    }
}