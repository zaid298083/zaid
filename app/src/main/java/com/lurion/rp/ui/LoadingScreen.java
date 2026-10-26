package com.nexion.online.ui;

import android.app.Activity;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Handler;
import android.os.Looper;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ImageButton;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.VideoView;

import androidx.constraintlayout.widget.ConstraintLayout;

import com.nexion.online.R;

import java.io.File;
import java.io.IOException;

public class LoadingScreen {

    private Activity activity;
    private View mainLayout;
    private TextView loadingStatusText;
    private TextView musicTitle;
    private ImageButton btnPlayPause;
    private ImageButton btnPrev;
    private ImageButton btnNext;
    private SeekBar musicSeekbar;
    private VideoView connectingVideo;

    private MediaPlayer mediaPlayer;
    private boolean isPlaying = false;
    private boolean isPrepared = false;
    private Handler handler;

    // Music playlist
    private String[] playlist = {};
    private int currentTrackIndex = 0;

    // Loading status steps
    public static final String STATUS_RESOURCES = "Carregando Recursos...";
    public static final String STATUS_CONNECTING = "Conectando ao Servidor...";
    public static final String STATUS_AUTHENTICATING = "Autenticando...";

    // Minimum display times (milliseconds)
    private static final long MIN_RESOURCES_TIME = 3000;   // 3 seconds
    private static final long MIN_CONNECTING_TIME = 2500;  // 2.5 seconds

    // Loading state machine
    private int loadingState = 0; // 0=resources, 1=connecting, 2=authenticating, 3=done
    private boolean loadingStarted = false;
    private boolean gameReady = false;  // C++ finished game init
    private boolean networkReady = false; // C++ finished network init
    private long stateStartTime = 0;
    private String pendingNextStatus = null;

    public LoadingScreen(Activity activity) {
        this.activity = activity;
        this.handler = new Handler(Looper.getMainLooper());

        mainLayout = activity.getLayoutInflater().inflate(R.layout.loadingscreen, null);
        activity.addContentView(mainLayout, new ConstraintLayout.LayoutParams(-1, -1));

        // Initialize views
        loadingStatusText = mainLayout.findViewById(R.id.loadingStatusText);
        musicTitle = mainLayout.findViewById(R.id.musicTitle);
        btnPlayPause = mainLayout.findViewById(R.id.btnPlayPause);
        btnPrev = mainLayout.findViewById(R.id.btnPrev);
        btnNext = mainLayout.findViewById(R.id.btnNext);
        musicSeekbar = mainLayout.findViewById(R.id.musicSeekbar);
        connectingVideo = mainLayout.findViewById(R.id.connectingVideo);

        setupConnectingVideo();
        // Setup music controls
        setupMusicControls();
    }

    private void setupConnectingVideo() {
        if (connectingVideo == null) return;
        View oldBackground = mainLayout.findViewById(R.id.loadingBackground);
        if (oldBackground != null) {
            // O vídeo é 16:9 e pode deixar barras laterais em telas ultrawide;
            // nunca deixe a arte antiga aparecer nessas áreas.
            oldBackground.setVisibility(View.GONE);
        }
        Uri videoUri = Uri.parse("android.resource://" + activity.getPackageName() + "/" + R.raw.connecting_video);
        connectingVideo.setVideoURI(videoUri);
        connectingVideo.setOnPreparedListener(mp -> {
            mp.setLooping(true);
            mp.setVolume(1.0f, 1.0f);
            connectingVideo.start();
        });
        connectingVideo.setOnErrorListener((mp, what, extra) -> true);
        connectingVideo.start();
    }

    private AudioManager audioManager;
    private int savedVolume = -1;

    private void setupMusicControls() {
        // Setup volume control
        audioManager = (AudioManager) activity.getSystemService(Activity.AUDIO_SERVICE);
        if (musicSeekbar != null && audioManager != null) {
            int maxVolume = audioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);
            musicSeekbar.setMax(maxVolume);
            // Save original volume to restore later
            savedVolume = audioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
            // Set volume to 70% for loading screen (not too loud)
            int targetVolume = (int) (maxVolume * 0.7);
            if (targetVolume < 1) targetVolume = 1;
            audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, targetVolume, 0);
            musicSeekbar.setProgress(targetVolume);
        } else if (musicSeekbar != null) {
            // Fallback if no audio manager
            musicSeekbar.setMax(15);
            musicSeekbar.setProgress(10);
        }

        btnPlayPause.setOnClickListener(v -> {
            if (isPlaying) {
                pauseMusic();
            } else {
                playMusic();
            }
        });

        btnPrev.setOnClickListener(v -> {
            if (playlist.length > 0) {
                currentTrackIndex = (currentTrackIndex - 1 + playlist.length) % playlist.length;
                playMusic();
            }
        });

        btnNext.setOnClickListener(v -> {
            if (playlist.length > 0) {
                currentTrackIndex = (currentTrackIndex + 1) % playlist.length;
                playMusic();
            }
        });

        musicSeekbar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser && audioManager != null) {
                    audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, progress, 0);
                }
            }
            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {}
            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {}
        });

        // No need to update seekbar for volume - it stays where user set it
    }

    /**
     * Starts the loading sequence. Shows "Carregando Recursos..." immediately.
     * The sequence progresses automatically with minimum display times.
     */
    public void startLoadingSequence() {
        if (loadingStarted) return;
        loadingStarted = true;
        loadingState = 0;
        stateStartTime = System.currentTimeMillis();
        gameReady = false;
        networkReady = false;

        updateStatusUI(STATUS_RESOURCES);

        // Auto-progress: after MIN_RESOURCES_TIME, move to "Conectando ao Servidor..."
        handler.postDelayed(() -> {
            if (loadingState == 0) {
                loadingState = 1;
                stateStartTime = System.currentTimeMillis();
                updateStatusUI(STATUS_CONNECTING);

                // Auto-progress: after MIN_CONNECTING_TIME, move to "Autenticando..."
                handler.postDelayed(() -> {
                    if (loadingState == 1) {
                        loadingState = 2;
                        stateStartTime = System.currentTimeMillis();
                        updateStatusUI(STATUS_AUTHENTICATING);
                    }
                }, MIN_CONNECTING_TIME);
            }
        }, MIN_RESOURCES_TIME);
    }

    /**
     * Called by C++ when game resources are loaded.
     * If minimum time has passed, advances to next status.
     * Otherwise, stores the pending transition.
     */
    public void onGameReady() {
        gameReady = true;

        // If we're still showing "Carregando Recursos..." and minimum time passed
        if (loadingState == 0) {
            long elapsed = System.currentTimeMillis() - stateStartTime;
            if (elapsed >= MIN_RESOURCES_TIME) {
                // Minimum time passed, advance immediately
                advanceToConnecting();
            } else {
                // Wait for minimum time, then advance
                long remaining = MIN_RESOURCES_TIME - elapsed;
                handler.postDelayed(() -> {
                    if (loadingState == 0) {
                        advanceToConnecting();
                    }
                }, remaining);
            }
        }
    }

    /**
     * Called by C++ when network connection is being established.
     * If minimum time has passed, advances to "Autenticando...".
     * Otherwise, stores the pending transition.
     */
    public void onNetworkReady() {
        networkReady = true;

        // If we're showing "Conectando ao Servidor..." and minimum time has passed
        if (loadingState == 1) {
            long elapsed = System.currentTimeMillis() - stateStartTime;
            if (elapsed >= MIN_CONNECTING_TIME) {
                // Minimum time passed, advance immediately
                advanceToAuthenticating();
            } else {
                // Wait for minimum time, then advance
                long remaining = MIN_CONNECTING_TIME - elapsed;
                handler.postDelayed(() -> {
                    if (loadingState == 1) {
                        advanceToAuthenticating();
                    }
                }, remaining);
            }
        }
        // If already at authenticating or later, do nothing
    }

    private void advanceToConnecting() {
        if (loadingState != 0) return;
        loadingState = 1;
        stateStartTime = System.currentTimeMillis();
        updateStatusUI(STATUS_CONNECTING);

        // Auto-progress after MIN_CONNECTING_TIME
        handler.postDelayed(() -> {
            if (loadingState == 1) {
                advanceToAuthenticating();
            }
        }, MIN_CONNECTING_TIME);

        // If network is already ready, schedule advance
        if (networkReady) {
            long elapsed = System.currentTimeMillis() - stateStartTime;
            if (elapsed >= MIN_CONNECTING_TIME) {
                advanceToAuthenticating();
            } else {
                long remaining = MIN_CONNECTING_TIME - elapsed;
                handler.postDelayed(() -> {
                    if (loadingState == 1) {
                        advanceToAuthenticating();
                    }
                }, remaining);
            }
        }
    }

    private void advanceToAuthenticating() {
        if (loadingState != 1) return;
        loadingState = 2;
        stateStartTime = System.currentTimeMillis();
        updateStatusUI(STATUS_AUTHENTICATING);
    }

    /**
     * Legacy method - now handled by onGameReady/onNetworkReady.
     * Kept for backward compatibility.
     */
    public void skipToAuthenticating() {
        onNetworkReady();
    }

    /**
     * Updates the status text on the UI thread (internal).
     */
    private void updateStatusUI(final String status) {
        if (activity == null) return;
        activity.runOnUiThread(() -> {
            if (loadingStatusText != null) {
                loadingStatusText.setText(status);
            }
        });
    }

    public void playMusic() {
        if (playlist.length == 0) return;

        try {
            if (mediaPlayer != null) {
                mediaPlayer.release();
                isPrepared = false;
            }

            String path = playlist[currentTrackIndex];
            mediaPlayer = new MediaPlayer();
            mediaPlayer.setAudioStreamType(AudioManager.STREAM_MUSIC);
            mediaPlayer.setLooping(true);

            if (path.startsWith("file://")) {
                mediaPlayer.setDataSource(activity, Uri.parse(path));
            } else if (path.startsWith("android.resource://")) {
                mediaPlayer.setDataSource(activity, Uri.parse(path));
            } else {
                File file = new File(path);
                if (file.exists()) {
                    mediaPlayer.setDataSource(file.getAbsolutePath());
                } else {
                    // Try loading from assets
                    try {
                        android.content.res.AssetFileDescriptor afd = activity.getAssets().openFd(path);
                        mediaPlayer.setDataSource(afd.getFileDescriptor(), afd.getStartOffset(), afd.getLength());
                        afd.close();
                    } catch (IOException e) {
                        // Fallback: try as raw path
                        mediaPlayer.setDataSource(path);
                    }
                }
            }

            mediaPlayer.setOnPreparedListener(mp -> {
                isPrepared = true;
                mp.start();
                isPlaying = true;
                btnPlayPause.setImageResource(android.R.drawable.ic_media_pause);
            });

            mediaPlayer.setOnCompletionListener(mp -> {
                isPlaying = false;
                btnPlayPause.setImageResource(android.R.drawable.ic_media_play);
            });

            mediaPlayer.setOnErrorListener((mp, what, extra) -> {
                isPlaying = false;
                isPrepared = false;
                btnPlayPause.setImageResource(android.R.drawable.ic_media_play);
                return true;
            });

            mediaPlayer.prepareAsync();
        } catch (Exception e) {
            isPlaying = false;
            isPrepared = false;
        }
    }

    public void pauseMusic() {
        if (mediaPlayer != null && isPlaying) {
            mediaPlayer.pause();
            isPlaying = false;
            btnPlayPause.setImageResource(android.R.drawable.ic_media_play);
        }
    }

    public void stopMusic() {
        if (mediaPlayer != null) {
            if (isPlaying) mediaPlayer.stop();
            isPlaying = false;
            isPrepared = false;
            btnPlayPause.setImageResource(android.R.drawable.ic_media_play);
        }
    }

    public void releaseMusic() {
        if (mediaPlayer != null) {
            try {
                if (isPlaying) mediaPlayer.stop();
                mediaPlayer.release();
            } catch (Exception ignored) {}
            mediaPlayer = null;
            isPlaying = false;
            isPrepared = false;
        }
        // Restore original volume
        if (audioManager != null && savedVolume >= 0) {
            audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, savedVolume, 0);
        }
        if (handler != null) {
            handler.removeCallbacksAndMessages(null);
        }
    }

    public void updateStatus(final String status) {
        if (activity == null) return;
        activity.runOnUiThread(() -> {
            if (loadingStatusText != null) {
                loadingStatusText.setText(status);
            }
        });
    }

    public void setMusicTitle(final String title) {
        if (activity == null) return;
        activity.runOnUiThread(() -> {
            if (musicTitle != null) {
                musicTitle.setText(title);
            }
        });
    }

    public void setPlaylist(final String[] tracks, final String title) {
        this.playlist = tracks;
        this.currentTrackIndex = 0;
        if (title != null) setMusicTitle(title);
        if (tracks != null && tracks.length > 0) playMusic();
    }

    public void hide() {
        if (mainLayout != null) {
            mainLayout.setVisibility(View.INVISIBLE);
        }
        if (connectingVideo != null) {
            connectingVideo.stopPlayback();
        }
        // Restore original volume when hiding
        if (audioManager != null && savedVolume >= 0) {
            audioManager.setStreamVolume(AudioManager.STREAM_MUSIC, savedVolume, 0);
        }
    }

    public void show() {
        if (mainLayout != null) {
            mainLayout.setVisibility(View.VISIBLE);
            if (connectingVideo != null) {
                connectingVideo.start();
            }
            startLoadingSequence();
            // Auto-play loading music from assets
            if (!isPlaying && playlist.length == 0) {
                try {
                    playlist = new String[]{"music/loading_bg.opus"};
                    currentTrackIndex = 0;
                    playMusic();
                } catch (Exception e) {
                    // Music file not found or unsupported format
                }
            }
        }
    }

    public View getView() {
        return mainLayout;
    }
}
