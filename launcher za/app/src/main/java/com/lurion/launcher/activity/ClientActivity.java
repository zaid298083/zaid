package com.nexion.launcher.activity;

import android.os.Bundle;
import android.os.Handler;
import android.widget.TextView;
import androidx.appcompat.app.AppCompatActivity;
import org.json.JSONObject;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

import com.nexion.online.R;

public class ClientActivity extends AppCompatActivity {

    private TextView lockMessageText;
    private Handler handler;
    private static final String LOCK_URL = "https://pride-russia-game";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_client);

        lockMessageText = findViewById(R.id.lock_message);
        handler = new Handler();

        loadLockData();
    }

    private void loadLockData() {
        new Thread(() -> {
            try {
                String jsonString = downloadLockData();
                if (jsonString != null) {
                    JSONObject lockData = new JSONObject(jsonString);

                    boolean isLocked = lockData.getBoolean("locked");
                    String message = lockData.getString("message");

     
                    handler.post(() -> updateLockUI(message));
                } else {
                    handler.post(() -> showDefaultLockMessage());
                }
            } catch (Exception e) {
                e.printStackTrace();
                handler.post(() -> showDefaultLockMessage());
            }
        }).start();
    }

    private String downloadLockData() {
        try {
            java.net.URL url = new java.net.URL(LOCK_URL);
            java.net.HttpURLConnection connection = (java.net.HttpURLConnection) url.openConnection();
            connection.setRequestMethod("GET");
            connection.setConnectTimeout(10000);
            connection.setReadTimeout(10000);

            InputStream inputStream = connection.getInputStream();
            java.util.Scanner scanner = new java.util.Scanner(inputStream, StandardCharsets.UTF_8.name());
            scanner.useDelimiter("\\A");
            String result = scanner.hasNext() ? scanner.next() : "";
            scanner.close();
            return result;

        } catch (Exception e) {
            e.printStackTrace();
            return null;
        }
    }

    private void updateLockUI(String message) {
        try {
            lockMessageText.setText(message);
        } catch (Exception e) {
            e.printStackTrace();
            showDefaultLockMessage();
        }
    }

    private void showDefaultLockMessage() {
        lockMessageText.setText("Cliente temporariamente desativado. Tente novamente mais tarde.");
    }

    @Override
    public void onBackPressed() {
        moveTaskToBack(true);
    }
}