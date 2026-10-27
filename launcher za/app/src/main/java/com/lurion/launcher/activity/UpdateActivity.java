package com.nexion.launcher.activity;

import android.Manifest;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Intent;
import android.content.pm.PackageManager;
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
import androidx.appcompat.app.AppCompatActivity;

import com.akexorcist.roundcornerprogressbar.RoundCornerProgressBar;
import com.nexion.online.R;
import com.nexion.launcher.api.Api;
import com.nexion.launcher.util.Util;

import com.liulishuo.filedownloader.BaseDownloadTask;
import com.liulishuo.filedownloader.FileDownloadSampleListener;
import com.liulishuo.filedownloader.FileDownloader;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;

public class UpdateActivity extends AppCompatActivity {
    private static final String CHANNEL_ID = "game_update_channel";
    private static final int NOTIFICATION_ID = 2;
    private static final int PERMISSION_REQUEST_CODE = 1002;

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
            returnToMainActivity();
            return;
        }

        createNotificationChannel();
        notificationManager = NotificationManagerCompat.from(this);

        initializeUI();
    }

    

    private void initializeUI() {
        setContentView(R.layout.activity_update);
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
                    "Atualização do jogo",
                    NotificationManager.IMPORTANCE_LOW
            );
            channel.setDescription("Notificações de download de atualizações do jogo");

            NotificationManager manager = getSystemService(NotificationManager.class);
            if (manager != null) {
                manager.createNotificationChannel(channel);
            }
        }
    }

    private void showStartNotification() {
        Intent intent = new Intent(this, UpdateActivity.class);
        intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TASK);
        PendingIntent pendingIntent = PendingIntent.getActivity(this, 0, intent,
                PendingIntent.FLAG_UPDATE_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        notificationBuilder = new NotificationCompat.Builder(this, CHANNEL_ID)
                .setContentTitle("Atualização do jogo")
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
                .setContentText("Jogo atualizado com sucesso!")
                .setSmallIcon(R.mipmap.ic_launcher)
                .setContentIntent(pendingIntent)
                .setPriority(NotificationCompat.PRIORITY_DEFAULT)
                .setAutoCancel(true);

        if (notificationManager.areNotificationsEnabled()) {
            notificationManager.notify(NOTIFICATION_ID + 1, builder.build());
        }
    }

    private void returnToMainActivity() {
        setResult(RESULT_CANCELED);
        finish();
    }

    private void returnToMainActivityWithSuccess() {
        setResult(RESULT_OK);
        finish();
    }

    private String formatBytes(long bytes) {
        return bytes <= 0 ? "0 MB" : Util.bytesIntoHumanReadable(bytes);
    }

    private void updateInstallProgress(String stage, String fileName, int progress,
                                       long downloadedBytes, long totalBytes) {
        mainHandler.post(() -> {
            TextView stageView = findViewById(R.id.loading_text);
            TextView fileView = findViewById(R.id.loading_file);
            TextView sizeView = findViewById(R.id.loading_percent);
            RoundCornerProgressBar bar = findViewById(R.id.progress_bar);

            if (stageView != null)
                stageView.setText(stage);
            if (fileView != null) {
                String visibleName = fileName == null || fileName.trim().isEmpty()
                        ? "preparando..." : fileName;
                fileView.setText("Baixando: " + visibleName + " (" +
                        Math.max(0, Math.min(100, progress)) + "%)");
            }
            if (sizeView != null) {
                sizeView.setText(formatBytes(downloadedBytes) + " de " +
                        formatBytes(totalBytes));
            }
            if (bar != null)
                bar.setProgress(Math.max(0, Math.min(100, progress)));
        });
    }

    private void updateExtractionProgress(String fileName, int progress,
                                          long extractedBytes, long totalBytes) {
        updateInstallProgress("Atualizando arquivos", fileName, progress,
                extractedBytes, totalBytes);
    }

    public void startDownload() {
        if (isDownloadInProgress) {
            return;
        }
        isDownloadInProgress = true;

        if (!Api.isStorageAvailable()) {
            safeShowToast("Armazenamento externo indisponível");
            returnToMainActivity();
            return;
        }

        folder = new File(Api.GAME_CACHE_PATH);
        if (!folder.exists() && !folder.mkdirs()) {
            safeShowToast("Não foi possível criar a pasta de download");
            returnToMainActivity();
            return;
        }

        updateInstallProgress("Atualizando arquivos", "update.zip", 0, 0, 0);
        updateNotification("Baixando atualização", "Iniciando download...", 0, 100, true);

        createDownloadTask(updateUrl, folder.getPath()).start();
    }

    private BaseDownloadTask createDownloadTask(String url, String path) {
        return FileDownloader.getImpl().create(url)
                .setPath(path + "/update.zip")
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
                        updateInstallProgress("Atualizando arquivos", "update.zip",
                                (int) progressPercent, soFarBytes, totalBytes);

                        updateNotification("Baixando atualização",
                                "Download: " + formatBytes(soFarBytes) + " / " + formatBytes(totalBytes),
                                (int) progressPercent, 100, false);
                    }

                    @Override
                    protected void error(BaseDownloadTask task, Throwable e) {
                        isDownloadInProgress = false;
                        safeShowToast("Ocorreu um erro ao baixar a atualização");
                        updateNotification("Erro", "Erro no download", 0, 100, false);
                        returnToMainActivity();
                    }

                    @Override
                    protected void connected(BaseDownloadTask task, String etag, boolean isContinue, int soFarBytes, int totalBytes) {
                        updateNotification("Baixando atualização", "Conectado ao servidor", 0, 100, true);
                    }

                    @Override
                    protected void completed(BaseDownloadTask task) {
                        isDownloadInProgress = false;
                        updateInstallProgress("Atualizando arquivos", "preparando arquivos", 0, 0, 0);
                        updateNotification("Atualização do jogo", "Download concluído, instalando...", 100, 100, false);
                        installUpdate();
                    }
                });
    }

    public void installUpdate(){
        final String inputFilePath = Api.GAME_CACHE_PATH + "/update.zip";
        final String outputPath = Api.GAME_CACHE_PATH;

        new Thread(() -> {
            try {
                File zipFile = new File(inputFilePath);
                if (!zipFile.exists()) {
                    throw new Exception("Arquivo ZIP não encontrado: " + inputFilePath);
                }

                File outputDir = new File(outputPath);
                if (!outputDir.exists() && !outputDir.mkdirs()) {
                    throw new Exception("Não foi possível criar o diretório de destino: " + outputPath);
                }

                long extractedTotalBytes = extractZipWithProgress(zipFile, outputDir);
                updateVersionFile(outputPath);
                if (zipFile.exists() && !zipFile.delete()) {
                    android.util.Log.w("UpdateActivity", "Não foi possível remover o update.zip após a extração");
                }

                mainHandler.post(() -> {
                    updateInstallProgress("Atualização concluída", "Download concluído", 100,
                            extractedTotalBytes, extractedTotalBytes);
                    updateNotification("Sucesso", "Atualização instalada", 100, 100, false);
                    afterUpdate();
                });
            } catch (Exception e) {
                mainHandler.post(() -> {
                    safeShowToast("Erro ao instalar a atualização: " + e.getMessage());
                    updateNotification("Erro", "Erro na instalação", 0, 100, false);
                    returnToMainActivity();
                });
            }
        }, "NexionCacheUpdateExtraction").start();
    }

    private long extractZipWithProgress(File zipFile, File outputDir) throws Exception {
        String outputRoot = outputDir.getCanonicalPath() + File.separator;
        List<ZipEntry> entries = new ArrayList<>();
        long totalBytes = 0L;

        try (ZipFile archive = new ZipFile(zipFile)) {
            java.util.Enumeration<? extends ZipEntry> enumeration = archive.entries();
            while (enumeration.hasMoreElements()) {
                ZipEntry entry = enumeration.nextElement();
                entries.add(entry);
                if (!entry.isDirectory() && entry.getSize() > 0)
                    totalBytes += entry.getSize();
            }

            long extractedBytes = 0L;
            int lastProgress = -1;
            for (ZipEntry entry : entries) {
                String entryName = entry.getName();
                File target = new File(outputDir, entryName);
                String targetPath = target.getCanonicalPath();
                if (!targetPath.equals(outputDir.getCanonicalPath()) &&
                        !targetPath.startsWith(outputRoot)) {
                    throw new IOException("Entrada ZIP inválida: " + entryName);
                }

                if (entry.isDirectory()) {
                    if (!target.exists() && !target.mkdirs())
                        throw new IOException("Não foi possível criar: " + targetPath);
                    continue;
                }

                File parent = target.getParentFile();
                if (parent != null && !parent.exists() && !parent.mkdirs())
                    throw new IOException("Não foi possível criar: " + parent.getAbsolutePath());

                try (InputStream input = new BufferedInputStream(archive.getInputStream(entry));
                     OutputStream output = new BufferedOutputStream(new FileOutputStream(target))) {
                    byte[] buffer = new byte[64 * 1024];
                    int count;
                    while ((count = input.read(buffer)) != -1) {
                        output.write(buffer, 0, count);
                        extractedBytes += count;
                        int progress = totalBytes > 0
                                ? (int) Math.min(99L, extractedBytes * 100L / totalBytes)
                                : 0;
                        if (progress != lastProgress) {
                            lastProgress = progress;
                            String visibleName = entryName.replace('\\', '/');
                            updateExtractionProgress(visibleName, progress,
                                    extractedBytes, totalBytes);
                            updateNotification("Extração", "Baixando: " + visibleName,
                                    progress, 100, false);
                        }
                    }
                    output.flush();
                }
            }

            updateExtractionProgress("concluindo arquivos", 100, totalBytes, totalBytes);
            updateNotification("Extração", "Arquivos extraídos", 100, 100, false);
            return totalBytes;
        }
    }

    private void updateVersionFile(String outputPath) {
        try {
            File versionFile = new File(outputPath, "version.txt");
            java.io.FileWriter writer = new java.io.FileWriter(versionFile);
            writer.write(newVersion);
            writer.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public void afterUpdate(){
        showCompletionNotification();
        safeShowToast("Jogo atualizado com sucesso!");
        mainHandler.postDelayed(() -> {
            returnToMainActivityWithSuccess();
        }, 1000);
    }

    private void safeShowToast(String message) {
        if (isToastShown) return;
        isToastShown = true;

        mainHandler.post(() -> {
            try {
                Toast.makeText(UpdateActivity.this, message, Toast.LENGTH_SHORT).show();
                mainHandler.postDelayed(() -> isToastShown = false, 2000);
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