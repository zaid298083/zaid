package com.nexion.online.ui;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.KeyEvent;
import android.view.View;
import android.view.inputmethod.EditorInfo;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.core.view.ViewCompat;
import androidx.core.view.WindowInsetsCompat;

import com.joom.paranoid.Obfuscate;

import java.util.ArrayList;

@Obfuscate
public class CustomKeyboard {

    public interface InputListener
    {
        public void OnInputEnd(String str);
    }

    private LinearLayout mInputLayout = null;
    private EditText mInputEt = null;

    private Activity mContext = null;
    private boolean mIsShowing = false;

    private String mSavedInput = null;

    private final int mMaxHistory = 20;
    private ArrayList<String> mInputHistory = null;
    private int mCurrentHistoryMessage = 0;

    public boolean IsShowing()
    {
        return mIsShowing;
    }

    public CustomKeyboard(Activity act)
    {
        mContext = act;

        mInputHistory = new ArrayList<String>();

        // EditText — captura o input do teclado Android
        mInputEt = new EditText(mContext);
        mInputEt.setBackgroundColor(Color.TRANSPARENT);
        mInputEt.setTextColor(Color.TRANSPARENT);
        mInputEt.setTextSize(1);
        mInputEt.setSingleLine(true);
        mInputEt.setCursorVisible(false);
        mInputEt.setImeOptions(EditorInfo.IME_ACTION_DONE | EditorInfo.IME_FLAG_NO_EXTRACT_UI);
        mInputEt.setHint("");
        mInputEt.setAlpha(0.0f);

        // Container posicionado no canto inferior (mesmo que invisível)
        mInputLayout = new LinearLayout(mContext);
        mInputLayout.setOrientation(LinearLayout.HORIZONTAL);
        mInputLayout.setBackgroundColor(Color.TRANSPARENT);
        mInputLayout.setVisibility(View.GONE);

        LinearLayout.LayoutParams etParams = new LinearLayout.LayoutParams(1, 1);
        mInputLayout.addView(mInputEt, etParams);

        FrameLayout.LayoutParams frameParams = new FrameLayout.LayoutParams(1, 1);
        frameParams.gravity = android.view.Gravity.BOTTOM | android.view.Gravity.START;
        mContext.addContentView(mInputLayout, frameParams);

        // Detecta altura do teclado via WindowInsets (funciona com adjustNothing)
        ViewCompat.setOnApplyWindowInsetsListener(mInputEt, (v, insets) -> {
            int imeHeight = insets.getInsets(WindowInsetsCompat.Type.ime()).bottom;
            if (imeHeight > 0) {
                setKeyboardOffsetNative(imeHeight);
            } else {
                setKeyboardOffsetNative(0);
            }
            return insets;
        });

        // Envia o texto digitado pro C++ a cada letra, pra atualizar o chat ImGui
        mInputEt.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence s, int start, int count, int after) {}

            @Override
            public void onTextChanged(CharSequence s, int start, int before, int count) {}

            @Override
            public void afterTextChanged(Editable s) {
                try {
                    ((com.nexion.online.SAMP) mContext).updateKeyboardInput(s.toString());
                } catch (Exception ignored) {}
            }
        });

        mInputEt.setOnEditorActionListener(new EditText.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView textView, int i, KeyEvent keyEvent)
            {
                if(i== EditorInfo.IME_ACTION_DONE || i == EditorInfo.IME_ACTION_NEXT)
                {
                    Editable editableText = mInputEt.getText();
                    if(editableText != null)
                    {
                        String str = editableText.toString();
                        mInputEt.setText("");
                        OnInputEnd(str);
                    }
                }
                return false;
            }
        });

        HideInputLayout();
    }

    public void onHeightChanged(int height)
    {
        FrameLayout.LayoutParams params = (FrameLayout.LayoutParams) mInputLayout.getLayoutParams();
        params.setMargins(0, 0 , 0, height);
        mInputLayout.setLayoutParams(params);
    }

    public void ShowInputLayout()
    {
        mIsShowing = true;

        mInputLayout.setVisibility(View.VISIBLE);
        mInputEt.setText("");
        mInputEt.requestFocus();
        mInputEt.setFocusable(true);
        mInputEt.setFocusableInTouchMode(true);
        mInputEt.requestFocusFromTouch();

        // Força o teclado Android a aparecer
        mInputEt.postDelayed(() -> {
            InputMethodManager imm = (InputMethodManager) mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
            if (imm != null) {
                imm.showSoftInput(mInputEt, InputMethodManager.SHOW_FORCED);
            }
            // Fallback: se WindowInsets não detectar, usa estimativa
            mInputEt.postDelayed(() -> {
                if (m_keyboardOffset == 0) {
                    int screenHeight = mContext.getResources().getDisplayMetrics().heightPixels;
                    int estimatedKeyboardHeight = (int)(screenHeight * 0.38f);
                    setKeyboardOffsetNative(estimatedKeyboardHeight);
                }
            }, 500);
        }, 200);

        mCurrentHistoryMessage = 0;

        if(mSavedInput != null)
        {
            mInputEt.setText(mSavedInput);
            mInputEt.setSelection(mInputEt.getText().length());
        }
    }

    private int m_keyboardOffset = 0;

    public void HideInputLayout()
    {
        mCurrentHistoryMessage = 0;
        m_keyboardOffset = 0;

        if(mInputEt.getEditableText() != null)
        {
            mSavedInput = mInputEt.getEditableText().toString();
        }

        if(mContext.getCurrentFocus() != null)
        {
            InputMethodManager inputMethodManager = (InputMethodManager) mContext.getSystemService(Context.INPUT_METHOD_SERVICE);
            inputMethodManager.hideSoftInputFromWindow(mContext.getCurrentFocus().getWindowToken(), 0);
        }
        mInputLayout.setVisibility(View.GONE);

        // Reseta o offset do chat
        setKeyboardOffsetNative(0);

        // Limpa o buffer do teclado C++
        try {
            ((com.nexion.online.SAMP) mContext).updateKeyboardInput("");
        } catch (Exception ignored) {}

        mIsShowing = false;
    }

    private void setKeyboardOffsetNative(int offset) {
        m_keyboardOffset = offset;
        try {
            // mContext já é a instância do SAMP
            ((com.nexion.online.SAMP) mContext).setKeyboardOffset(offset);
        } catch (Exception ignored) {}
    }

    private void OnInputEnd(String str)
    {
        if(mInputHistory.size() >= mMaxHistory)
        {
            mInputHistory.remove(mInputHistory.size() - 1);
        }
        mInputHistory.add(0, str);

        InputListener listener = (InputListener)mContext;

        listener.OnInputEnd(str);
    }
}

