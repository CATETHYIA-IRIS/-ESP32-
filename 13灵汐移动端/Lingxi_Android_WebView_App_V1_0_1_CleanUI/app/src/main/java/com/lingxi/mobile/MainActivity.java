
package com.lingxi.mobile;

import android.Manifest;
import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.SharedPreferences;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.os.Build;
import android.os.Bundle;
import android.view.Gravity;
import android.view.View;
import android.view.ViewGroup;
import android.webkit.CookieManager;
import android.webkit.PermissionRequest;
import android.webkit.WebChromeClient;
import android.webkit.WebSettings;
import android.webkit.WebView;
import android.webkit.WebViewClient;
import android.widget.Button;
import android.widget.EditText;
import android.widget.FrameLayout;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.PopupMenu;
import android.widget.ScrollView;
import android.widget.TextView;
import android.widget.Toast;

public class MainActivity extends Activity {
    private static final int REQ_AUDIO = 1001;
    private static final String PREF_NAME = "lingxi_app_config";
    private static final String KEY_LOCAL_URL = "local_url";
    private static final String KEY_REMOTE_URL = "remote_url";
    private static final String DEFAULT_LOCAL_URL = "http://192.168.4.1";

    private SharedPreferences prefs;
    private WebView webView;
    private FrameLayout webRoot;
    private String currentMode = "home";
    private PermissionRequest pendingPermissionRequest;

    private int dp(float v) { return (int)(v * getResources().getDisplayMetrics().density + 0.5f); }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        prefs = getSharedPreferences(PREF_NAME, MODE_PRIVATE);
        showHomePage();
    }

    private String getLocalUrl() { return prefs.getString(KEY_LOCAL_URL, DEFAULT_LOCAL_URL); }
    private String getRemoteUrl() { return prefs.getString(KEY_REMOTE_URL, ""); }

    private void saveUrls(String localUrl, String remoteUrl) {
        prefs.edit()
                .putString(KEY_LOCAL_URL, normalizeUrl(localUrl, true))
                .putString(KEY_REMOTE_URL, normalizeUrl(remoteUrl, false))
                .apply();
    }

    private String normalizeUrl(String raw, boolean localDefault) {
        String url = raw == null ? "" : raw.trim();
        if (url.isEmpty()) return localDefault ? DEFAULT_LOCAL_URL : "";
        if (!url.startsWith("http://") && !url.startsWith("https://")) url = "http://" + url;
        return url;
    }

    private GradientDrawable bg(int color, float radius) {
        GradientDrawable g = new GradientDrawable();
        g.setColor(color);
        g.setCornerRadius(dp(radius));
        return g;
    }

    private GradientDrawable strokeBg(int color, int stroke, float radius) {
        GradientDrawable g = bg(color, radius);
        g.setStroke(dp(1), stroke);
        return g;
    }

    private GradientDrawable pageBg() {
        return new GradientDrawable(GradientDrawable.Orientation.TL_BR,
                new int[]{Color.rgb(232,247,255), Color.rgb(245,251,255), Color.rgb(213,239,255)});
    }

    private void showHomePage() {
        currentMode = "home";
        FrameLayout root = new FrameLayout(this);
        root.setBackground(pageBg());

        ScrollView scroll = new ScrollView(this);
        scroll.setFillViewport(true);
        root.addView(scroll, new FrameLayout.LayoutParams(-1, -1));

        LinearLayout outer = new LinearLayout(this);
        outer.setOrientation(LinearLayout.VERTICAL);
        outer.setGravity(Gravity.CENTER);
        outer.setPadding(dp(18), dp(24), dp(18), dp(24));
        scroll.addView(outer, new ScrollView.LayoutParams(-1, -1));

        LinearLayout card = new LinearLayout(this);
        card.setOrientation(LinearLayout.VERTICAL);
        card.setPadding(dp(24), dp(28), dp(24), dp(28));
        card.setBackground(strokeBg(Color.argb(242,255,255,255), Color.argb(120,14,165,233), 28));
        outer.addView(card, new LinearLayout.LayoutParams(-1, -2));

        ImageView logo = new ImageView(this);
        logo.setImageResource(getResources().getIdentifier("lingxi_logo", "drawable", getPackageName()));
        logo.setScaleType(ImageView.ScaleType.CENTER_CROP);
        LinearLayout.LayoutParams logoLp = new LinearLayout.LayoutParams(dp(112), dp(112));
        logoLp.setMargins(0, 0, 0, dp(24));
        card.addView(logo, logoLp);

        TextView title = new TextView(this);
        title.setText("灵汐全屋智能控制中心");
        title.setTextColor(Color.rgb(15,35,68));
        title.setTextSize(27);
        title.setTypeface(Typeface.DEFAULT_BOLD);
        title.setLineSpacing(dp(2), 1.0f);
        card.addView(title, new LinearLayout.LayoutParams(-1, -2));

        TextView gap = new TextView(this);
        card.addView(gap, new LinearLayout.LayoutParams(-1, dp(22)));

        card.addView(entryButton("本地访问", true));
        card.addView(entryButton("远程访问", false));

        Button settings = new Button(this);
        settings.setText("地址设置");
        settings.setAllCaps(false);
        settings.setTextSize(16);
        settings.setTextColor(Color.rgb(15,76,129));
        settings.setBackground(strokeBg(Color.rgb(240,249,255), Color.argb(150,14,165,233), 18));
        settings.setOnClickListener(v -> showAddressDialog());
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, dp(54));
        lp.setMargins(0, dp(14), 0, 0);
        card.addView(settings, lp);

        setContentView(root);
    }

    private View entryButton(String text, boolean local) {
        TextView b = new TextView(this);
        b.setText((local ? "🏠  " : "☁  ") + text);
        b.setTextSize(22);
        b.setTextColor(Color.rgb(10,47,88));
        b.setTypeface(Typeface.DEFAULT_BOLD);
        b.setGravity(Gravity.CENTER_VERTICAL);
        b.setPadding(dp(18), 0, dp(18), 0);
        b.setBackground(strokeBg(Color.WHITE, Color.argb(120,14,165,233), 18));
        b.setClickable(true);
        LinearLayout.LayoutParams lp = new LinearLayout.LayoutParams(-1, dp(78));
        lp.setMargins(0, dp(12), 0, 0);
        b.setLayoutParams(lp);
        b.setOnClickListener(v -> {
            if (local) {
                openWeb(getLocalUrl(), "local");
            } else {
                String remote = getRemoteUrl();
                if (remote == null || remote.trim().isEmpty()) {
                    Toast.makeText(this, "请先填写远程地址", Toast.LENGTH_SHORT).show();
                    showAddressDialog();
                } else {
                    openWeb(remote, "remote");
                }
            }
        });
        return b;
    }

    private void openWeb(String url, String mode) {
        currentMode = mode;
        buildWebPage();
        setContentView(webRoot);
        webView.loadUrl(normalizeUrl(url, mode.equals("local")));
    }

    private void buildWebPage() {
        webRoot = new FrameLayout(this);
        webView = new WebView(this);
        setupWebView(webView);
        webRoot.addView(webView, new FrameLayout.LayoutParams(-1, -1));

        TextView menuBtn = new TextView(this);
        menuBtn.setText("⋮");
        menuBtn.setTextColor(Color.WHITE);
        menuBtn.setTextSize(26);
        menuBtn.setGravity(Gravity.CENTER);
        menuBtn.setTypeface(Typeface.DEFAULT_BOLD);
        menuBtn.setBackground(bg(Color.argb(160,25,40,54), 2));
        FrameLayout.LayoutParams mlp = new FrameLayout.LayoutParams(dp(52), dp(52));
        mlp.gravity = Gravity.TOP | Gravity.RIGHT;
        mlp.setMargins(0, dp(8), dp(8), 0);
        webRoot.addView(menuBtn, mlp);
        menuBtn.setOnClickListener(v -> showMenu(menuBtn));
    }

    private void setupWebView(WebView wv) {
        WebSettings s = wv.getSettings();
        s.setJavaScriptEnabled(true);
        s.setDomStorageEnabled(true);
        s.setDatabaseEnabled(true);
        s.setUseWideViewPort(true);
        s.setLoadWithOverviewMode(true);
        s.setSupportZoom(true);
        s.setBuiltInZoomControls(true);
        s.setDisplayZoomControls(false);
        s.setCacheMode(WebSettings.LOAD_DEFAULT);
        s.setMediaPlaybackRequiresUserGesture(false);
        s.setAllowFileAccess(true);
        s.setAllowContentAccess(true);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            s.setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);
        }
        wv.setWebViewClient(new WebViewClient());
        wv.setWebChromeClient(new WebChromeClient() {
            @Override
            public void onPermissionRequest(PermissionRequest request) {
                boolean audio = false;
                for (String r : request.getResources()) {
                    if (PermissionRequest.RESOURCE_AUDIO_CAPTURE.equals(r)) audio = true;
                }
                if (audio) {
                    if (hasAudioPermission()) request.grant(new String[]{PermissionRequest.RESOURCE_AUDIO_CAPTURE});
                    else {
                        pendingPermissionRequest = request;
                        requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO}, REQ_AUDIO);
                    }
                } else request.deny();
            }
        });
    }

    private void showMenu(View anchor) {
        PopupMenu menu = new PopupMenu(this, anchor);
        menu.getMenu().add("灵汐");
        menu.getMenu().add("重新加载");
        menu.getMenu().add("返回启动页");
        menu.getMenu().add("修改中控地址");
        menu.getMenu().add("修改远程地址");
        menu.getMenu().add("检查麦克风权限");
        menu.getMenu().add("清除 WebView 缓存");
        menu.getMenu().add("网络状态提示");
        menu.getMenu().add("关于灵汐");

        menu.setOnMenuItemClickListener(item -> {
            String t = item.getTitle().toString();
            if ("重新加载".equals(t) && webView != null) webView.reload();
            else if ("返回启动页".equals(t)) showHomePage();
            else if ("修改中控地址".equals(t) || "修改远程地址".equals(t)) showAddressDialog();
            else if ("检查麦克风权限".equals(t)) checkMicPermission();
            else if ("清除 WebView 缓存".equals(t)) clearWebCache();
            else if ("网络状态提示".equals(t)) showNetworkStatus();
            else if ("关于灵汐".equals(t)) showAboutDialog();
            return true;
        });
        menu.show();
    }

    private void showAddressDialog() {
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(dp(18), dp(10), dp(18), 0);

        EditText local = new EditText(this);
        local.setHint("本地地址，例如 http://192.168.4.1");
        local.setSingleLine(true);
        local.setText(getLocalUrl());
        layout.addView(local, new LinearLayout.LayoutParams(-1, -2));

        EditText remote = new EditText(this);
        remote.setHint("远程地址，例如 https://xxx.github.io/xxx/");
        remote.setSingleLine(true);
        remote.setText(getRemoteUrl());
        layout.addView(remote, new LinearLayout.LayoutParams(-1, -2));

        new AlertDialog.Builder(this)
                .setTitle("地址设置")
                .setView(layout)
                .setPositiveButton("保存", (d, w) -> {
                    saveUrls(local.getText().toString(), remote.getText().toString());
                    Toast.makeText(this, "地址已保存", Toast.LENGTH_SHORT).show();
                })
                .setNegativeButton("取消", null)
                .show();
    }

    private boolean hasAudioPermission() {
        return Build.VERSION.SDK_INT < Build.VERSION_CODES.M ||
                checkSelfPermission(Manifest.permission.RECORD_AUDIO) == PackageManager.PERMISSION_GRANTED;
    }

    private void checkMicPermission() {
        if (hasAudioPermission()) Toast.makeText(this, "麦克风权限已授权", Toast.LENGTH_SHORT).show();
        else requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO}, REQ_AUDIO);
    }

    private void clearWebCache() {
        if (webView != null) {
            webView.clearCache(true);
            webView.clearHistory();
        }
        CookieManager.getInstance().removeAllCookies(null);
        CookieManager.getInstance().flush();
        Toast.makeText(this, "WebView 缓存已清除", Toast.LENGTH_SHORT).show();
    }

    private void showNetworkStatus() {
        ConnectivityManager cm = (ConnectivityManager)getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo info = cm == null ? null : cm.getActiveNetworkInfo();
        String msg = (info != null && info.isConnected()) ? "当前网络可用：" + info.getTypeName() : "当前未检测到可用网络";
        new AlertDialog.Builder(this).setTitle("网络状态").setMessage(msg).setPositiveButton("确定", null).show();
    }

    private void showAboutDialog() {
        new AlertDialog.Builder(this).setTitle("关于灵汐").setMessage("灵汐\n版本：1.0.1").setPositiveButton("确定", null).show();
    }

    @Override
    public void onBackPressed() {
        if ("home".equals(currentMode)) super.onBackPressed();
        else if (webView != null && webView.canGoBack()) webView.goBack();
        else showHomePage();
    }

    @Override
    public void onRequestPermissionsResult(int code, String[] permissions, int[] results) {
        super.onRequestPermissionsResult(code, permissions, results);
        if (code == REQ_AUDIO) {
            boolean ok = results.length > 0 && results[0] == PackageManager.PERMISSION_GRANTED;
            if (ok) {
                Toast.makeText(this, "麦克风权限已授权", Toast.LENGTH_SHORT).show();
                if (pendingPermissionRequest != null) {
                    pendingPermissionRequest.grant(new String[]{PermissionRequest.RESOURCE_AUDIO_CAPTURE});
                    pendingPermissionRequest = null;
                }
            } else {
                Toast.makeText(this, "麦克风权限未授权", Toast.LENGTH_LONG).show();
                if (pendingPermissionRequest != null) {
                    pendingPermissionRequest.deny();
                    pendingPermissionRequest = null;
                }
            }
        }
    }

    @Override
    protected void onDestroy() {
        if (webView != null) webView.destroy();
        super.onDestroy();
    }
}
