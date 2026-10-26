package com.nexion.launcher.util;

import android.content.Context;
import android.content.res.AssetManager;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

public class ConfigValidator {
    public static void validateConfigFiles(Context context) {
        File externalFilesDir = context.getExternalFilesDir(null);
        File file = new File(externalFilesDir, "SAMP/settings.ini");
        if (!file.exists()) {
            file.getParentFile().mkdirs();
            copyAsset(context.getAssets(), "settings.ini", file.toString());
        }
                /*File file2 = new File(externalFilesDir, "gta_sa.set");
        if (!file2.exists()) {
            file2.getParentFile().mkdirs();
            copyAsset(context.getAssets(), "gta_sa.set", file2.toString());
        }*/
        installHudLogo(context);
    }

    public static void installHudLogo(Context context) {
        File externalFilesDir = context.getExternalFilesDir(null);
        if (externalFilesDir == null) {
            return;
        }

        File logoFile = new File(externalFilesDir, "SAMP/HUD/nexion_logo.png");
        File parent = logoFile.getParentFile();
        if (parent != null) {
            parent.mkdirs();
        }
        // Usa a imagem que fica em app/src/main/res/drawable/nexion_logo.png.
        // A copia e refeita ao abrir para garantir que a textura nativa sempre
        // receba a versao atual da logo Nexion.
        copyDrawable(context, "nexion_logo", logoFile);
    }

    private static boolean copyDrawable(Context context, String drawableName, File destination) {
        int resourceId = context.getResources().getIdentifier(
                drawableName, "drawable", context.getPackageName());
        if (resourceId == 0) {
            return false;
        }
        try (InputStream input = context.getResources().openRawResource(resourceId);
             FileOutputStream output = new FileOutputStream(destination, false)) {
            copyFile(input, output);
            output.flush();
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    static boolean copyAsset(AssetManager assetManager, String str, String str2) {

        try {
            InputStream open = assetManager.open(str);
            new File(str2).createNewFile();
            FileOutputStream fileOutputStream = new FileOutputStream(str2);
            copyFile(open, fileOutputStream);
            open.close();
            fileOutputStream.flush();
            fileOutputStream.close();
            return true;
        } catch (Exception e) {
            e.printStackTrace();
            return false;
        }
    }

    static void copyFile(InputStream inputStream, OutputStream outputStream) throws IOException {
        byte[] bArr = new byte[1024];
        while (true) {
            int read = inputStream.read(bArr);
            if (read != -1) {
                outputStream.write(bArr, 0, read);
            } else {
                return;
            }
        }
    }
}
