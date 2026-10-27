package com.nexion.launcher.api;

public class Api {
    private static final String BASE_URL = "https://pride-russia-game.ru";
    private static final String DATA_TEST_URL = BASE_URL + "/data_test";

    public static final String GAME_CACHE_ZIP =
        "https://drive.usercontent.google.com/download?id=1nrwaHgLplRAk7X72yrERlH8qyRPnvGok&export=download&confirm=t";

    public static final String UPDATE_JSON = DATA_TEST_URL + "/update.json";
    public static final String SERVERS_JSON = DATA_TEST_URL + "/servers.json";

    /*
     * Diretório externo específico do aplicativo. Em Android 11+ ele pode ser
     * gravado sem READ/WRITE/MANAGE_EXTERNAL_STORAGE.
     */
    public static final String STORAGE_ROOT = "/storage/emulated/0/Android/data/com.nexion.online/files";
    public static final String GAME_CACHE_PATH = STORAGE_ROOT + "/";
    public static final String GAME_INSTALL_PATH = GAME_CACHE_PATH + "downloand/";
    public static final String GAME_TEXDB_PATH = GAME_CACHE_PATH + "texdb/";

    public static final String FTEAM_FILE = GAME_CACHE_PATH + "fteam.txt";
    public static final String VERSION_FILE = GAME_CACHE_PATH + "version.txt";

    public static final String TELEGRAM_URL = "https://t.me/priderussiacrmp";
    public static final String VK_URL = "https://vk.com/priderussiacrmp";
    public static final String DISCORD_URL = "https://discord.gg/xMbQNRUJ";
    public static final String INSTAGRAM_URL = "https://www.instagram.com/nexion.online?igsh=MTI3MjVjcDIxOHJtNA==";
    public static final String YOUTUBE_URL = "https://www.youtube.com/@nexiononline.oficial";

    public static final String CLIENT_LOCK_URL = BASE_URL + "/lock.json";

    // Endpoint HTTPS próprio para verificar atualizações do launcher
    public static final String LAUNCHER_UPDATE_URL =
        "https://updates.nexionoficial.online/launcher_update.json";

    public static String getUpdateUrl(String version) {
        return BASE_URL + "/data/update.zip";
    }

    public static boolean isStorageAvailable() {
        java.io.File gameDir = new java.io.File(GAME_CACHE_PATH);
        if (!gameDir.exists() && !gameDir.mkdirs()) {
            return false;
        }
        return gameDir.isDirectory() && gameDir.canWrite();
    }
}