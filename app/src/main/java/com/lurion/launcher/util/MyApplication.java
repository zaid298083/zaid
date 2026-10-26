package com.nexion.launcher.util;

import android.app.Application;
import android.util.Log;

import com.google.firebase.FirebaseApp;
import com.google.firebase.analytics.FirebaseAnalytics;

public class MyApplication extends Application {
    private FirebaseAnalytics mFirebaseAnalytics;

    @Override
    public void onCreate() {
        super.onCreate();

        FirebaseApp.initializeApp(this);
        mFirebaseAnalytics = FirebaseAnalytics.getInstance(this);

        setUserProperties();
    }

    private void setUserProperties() {
        try {
            mFirebaseAnalytics.setUserProperty("app_version", "0.8.2.1");
            mFirebaseAnalytics.setUserProperty("device_architecture", "ARMx64");
            mFirebaseAnalytics.setUserProperty("build_type", "release");
        } catch (Exception e) {
            Log.e("Application", "Error setting user properties: " + e.getMessage());
        }
    }

    public FirebaseAnalytics getFirebaseAnalytics() {
        return mFirebaseAnalytics;
    }
}