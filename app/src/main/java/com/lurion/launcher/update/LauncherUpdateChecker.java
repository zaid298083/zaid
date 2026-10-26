package com.nexion.launcher.update;

import android.content.Context;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.util.Log;

import com.google.gson.Gson;
import com.nexion.launcher.api.Api;

import java.io.BufferedReader;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.HttpURLConnection;
import java.net.URL;

public class LauncherUpdateChecker {
    private static final String TAG = "LauncherUpdateChecker";
    private static final String LAUNCHER_UPDATE_JSON_URL = Api.LAUNCHER_UPDATE_URL;

    public interface UpdateCheckListener {
        void onUpdateAvailable(UpdateInfo updateInfo);
        void onNoUpdateAvailable();
        void onError(String errorMessage);
    }

    public static void checkForUpdate(Context context, UpdateCheckListener listener) {
        new Thread(() -> {
            try {
                URL url = new URL(LAUNCHER_UPDATE_JSON_URL);
                HttpURLConnection connection = (HttpURLConnection) url.openConnection();
                connection.setRequestMethod("GET");
                connection.setConnectTimeout(10000);
                connection.setReadTimeout(10000);

                int responseCode = connection.getResponseCode();
                if (responseCode == HttpURLConnection.HTTP_OK) {
                    InputStream inputStream = connection.getInputStream();
                    BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));
                    StringBuilder response = new StringBuilder();
                    String line;

                    while ((line = reader.readLine()) != null) {
                        response.append(line);
                    }

                    reader.close();
                    inputStream.close();

           
                    Gson gson = new Gson();
                    UpdateInfo updateInfo = gson.fromJson(response.toString(), UpdateInfo.class);

                    Log.d(TAG, "Current version: " + getCurrentVersion(context) + ", New version: " + updateInfo.getVersion());

                    if (updateInfo.isUpdate() && isNewVersionAvailable(context, updateInfo.getVersion())) {
                        listener.onUpdateAvailable(updateInfo);
                    } else {
                        listener.onNoUpdateAvailable();
                    }

                } else {
                    listener.onError("HTTP error: " + responseCode);
                }

                connection.disconnect();

            } catch (Exception e) {
                Log.e(TAG, "Error checking launcher update", e);
                listener.onError(e.getMessage());
            }
        }).start();
    }

    private static boolean isNewVersionAvailable(Context context, String newVersion) {
        try {
            String currentVersion = getCurrentVersion(context);
            return compareBuildVersions(newVersion, currentVersion) > 0;
        } catch (Exception e) {
            Log.e(TAG, "Error comparing versions", e);
            return false;
        }
    }

    private static String getCurrentVersion(Context context) {
        try {
            PackageInfo pInfo = context.getPackageManager().getPackageInfo(context.getPackageName(), 0);
            return pInfo.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            return "build_8.11.25";
        }
    }

    /**
     * Compara versões no formato build_8.11.25
     * Retorna:
     * > 0 se version1 for mais nova que version2
     * = 0 se as versões forem iguais
     * < 0 se version1 for mais antiga que version2
     */
    private static int compareBuildVersions(String version1, String version2) {
        try {
            String v1 = version1.replace("build_", "");
            String v2 = version2.replace("build_", "");

            String[] v1Parts = v1.split("\\.");
            String[] v2Parts = v2.split("\\.");

            int length = Math.max(v1Parts.length, v2Parts.length);
            for (int i = 0; i < length; i++) {
                int num1 = i < v1Parts.length ? Integer.parseInt(v1Parts[i]) : 0;
                int num2 = i < v2Parts.length ? Integer.parseInt(v2Parts[i]) : 0;

                if (num1 != num2) {
                    return num1 - num2;
                }
            }
            return 0;
        } catch (Exception e) {
            Log.e(TAG, "Error parsing versions: " + version1 + " vs " + version2, e);
            return 0;
        }
    }
}