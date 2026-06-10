/*
 * 文件：03a_auth_core.ino
 * 标题：登录认证与管理员模块
 * 说明：负责管理员初始化、登录校验、会话状态和权限入口。
 * 可改：可改展示文案
 * 禁止：随意绕过认证和清空管理员逻辑。
 */

const char* AUTH_PREF_NS = "lingxi_auth";
const char* AUTH_COOKIE_NAME = "lx_token";
const char* AUTH_RESET_CODE = "LINGXI_RESET_ADMIN_36e3e338bc044ff4a35cec3fb8df5b3b"; 
const uint32_t AUTH_REMEMBER_SECONDS = 30UL * 24UL * 3600UL;

// 函数说明：页面输出函数，负责生成 WebUI 或设置页面内容。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String AuthHtmlEscape(String s) {
  s.replace("&", "&amp;");
  s.replace("<", "&lt;");
  s.replace(">", "&gt;");
  s.replace("\"", "&quot;");
  s.replace("'", "&#39;");
  return s;
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String AuthSha256Hex(String input) {
  unsigned char out[32];
  mbedtls_sha256_context ctx;
  mbedtls_sha256_init(&ctx);
  mbedtls_sha256_starts(&ctx, 0);
  mbedtls_sha256_update(&ctx, (const unsigned char*)input.c_str(), input.length());
  mbedtls_sha256_finish(&ctx, out);
  mbedtls_sha256_free(&ctx);
  const char* hex = "0123456789abcdef";
  String r;
  r.reserve(64);
  for (int i = 0; i < 32; ++i) {
    r += hex[(out[i] >> 4) & 0x0F];
    r += hex[out[i] & 0x0F];
  }
  return r;
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String AuthRandomHex(size_t bytes) {
  const char* hex = "0123456789abcdef";
  String r;
  r.reserve(bytes * 2);
  for (size_t i = 0; i < bytes; ++i) {
    uint8_t b = (uint8_t)(esp_random() & 0xFF);
    r += hex[(b >> 4) & 0x0F];
    r += hex[b & 0x0F];
  }
  return r;
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String AuthGetPrefString(const char* key, const char* fallback = "") {
  AuthPrefs.begin(AUTH_PREF_NS, true);
  String v = AuthPrefs.getString(key, fallback);
  AuthPrefs.end();
  return v;
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool AuthGetPrefBool(const char* key, bool fallback = false) {
  AuthPrefs.begin(AUTH_PREF_NS, true);
  bool v = AuthPrefs.getBool(key, fallback);
  AuthPrefs.end();
  return v;
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void AuthPutPrefString(const char* key, String value) {
  AuthPrefs.begin(AUTH_PREF_NS, false);
  AuthPrefs.putString(key, value);
  AuthPrefs.end();
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void AuthPutPrefBool(const char* key, bool value) {
  AuthPrefs.begin(AUTH_PREF_NS, false);
  AuthPrefs.putBool(key, value);
  AuthPrefs.end();
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool AuthAdminExists() {
  String user = AuthGetPrefString("user", "");
  String hash = AuthGetPrefString("hash", "");
  return user.length() > 0 && hash.length() > 0;
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String AuthCurrentUser() {
  return AuthGetPrefString("user", "灵汐管理员");
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String AuthPasswordHash(String user, String salt, String pass) {
  return AuthSha256Hex(user + "|" + salt + "|" + pass);
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String AuthCookieToken() {
  String cookie = server.header("Cookie");
  String mark = String(AUTH_COOKIE_NAME) + "=";
  int p = cookie.indexOf(mark);
  if (p < 0) return "";
  p += mark.length();
  int e = cookie.indexOf(';', p);
  if (e < 0) e = cookie.length();
  String t = cookie.substring(p, e);
  t.trim();
  return t;
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String AuthRequestToken() {
  String t = AuthCookieToken();
  if (t.length() == 0 && server.hasArg("token")) t = server.arg("token");
  t.trim();
  return t;
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool AuthIsValid() {
  if (!AuthAdminExists()) return false;
  String req = AuthRequestToken();
  if (req.length() < 16) return false;
  String saved = AuthGetPrefString("session", "");
  return saved.length() >= 16 && req == saved;
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void AuthSetCookie(String token, bool remember) {
  String cookie = String(AUTH_COOKIE_NAME) + "=" + token + "; Path=/; SameSite=Lax";
  if (remember) cookie += "; Max-Age=" + String(AUTH_REMEMBER_SECONDS);
  server.sendHeader("Set-Cookie", cookie);
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void AuthClearCookie() {
  server.sendHeader("Set-Cookie", String(AUTH_COOKIE_NAME) + "=; Path=/; Max-Age=0; SameSite=Lax");
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void AuthRedirect(String location) {
  server.sendHeader("Location", location);
  server.send(302, "text/plain; charset=utf-8", "Redirecting");
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool RequireAuth() {
  if (AuthIsValid()) return true;
  SendCorsHeader();
  server.send(401, "application/json", "{\"ok\":false,\"error\":\"unauthorized\",\"message\":\"请先登录灵汐中控\"}");
  return false;
}

// 函数说明：页面输出函数，负责生成 WebUI 或设置页面内容。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
String AuthPageShell(String title, String body) {
  String html;
  html.reserve(9000);
  html += "<!DOCTYPE html><html lang='zh-CN'><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0,user-scalable=no'>";
  html += "<title>" + AuthHtmlEscape(title) + "</title><style>";
  html += "*{box-sizing:border-box}body{margin:0;min-height:100vh;font-family:-apple-system,BlinkMacSystemFont,'Segoe UI','PingFang SC','Microsoft YaHei',Arial,sans-serif;background:linear-gradient(135deg,#eef8ff,#dff2ff 48%,#f8fcff);color:#12213a;display:flex;align-items:center;justify-content:center;padding:22px}.box{width:min(440px,94vw);background:rgba(255,255,255,.86);border:1px solid rgba(130,170,210,.35);border-radius:24px;box-shadow:0 24px 70px rgba(60,130,200,.22);padding:28px}.logo{width:58px;height:58px;border-radius:18px;background:linear-gradient(145deg,#071126,#1ca8ff);display:flex;align-items:center;justify-content:center;color:#fff;font-size:30px;box-shadow:0 12px 28px rgba(20,130,220,.25);margin-bottom:14px}.brand{font-size:28px;font-weight:900;letter-spacing:-.5px}.sub{color:#66809f;line-height:1.7;font-size:13px;margin:8px 0 18px}.field{margin:13px 0}.field label{display:block;font-size:13px;font-weight:800;color:#435672;margin-bottom:7px}.input{width:100%;height:44px;border:1px solid rgba(125,155,190,.42);border-radius:14px;background:#fff;padding:0 13px;font-size:15px;outline:0}.input:focus{border-color:#1aa7ff;box-shadow:0 0 0 3px rgba(26,167,255,.13)}.pwd{display:grid;grid-template-columns:1fr 48px;gap:8px}.eye{border:1px solid rgba(125,155,190,.42);border-radius:14px;background:#fff;position:relative;display:flex;align-items:center;justify-content:center}.eye:before{content:'';width:20px;height:12px;border:2px solid #7f8da3;border-radius:50%/60%;display:block}.eye:after{content:'';position:absolute;width:6px;height:6px;border-radius:50%;background:#7f8da3}.row{display:flex;justify-content:space-between;align-items:center;gap:12px;flex-wrap:wrap;margin-top:16px}.login-submit{display:flex;justify-content:center;margin-top:18px}.login-links{display:flex;justify-content:space-between;align-items:center;margin-top:18px}.btn{height:44px;border:0;border-radius:14px;padding:0 22px;font-weight:900;cursor:pointer;background:#12b76a;color:#fff}.btn.blue{background:#13a6f6}.btn.ghost{background:#fff;color:#20344f;border:1px solid rgba(125,155,190,.36)}.link{color:#158fe8;text-decoration:none;font-weight:850}.msg{border-radius:14px;padding:10px 12px;font-size:13px;margin:10px 0;background:#eef9ff;color:#15628d}.err{background:#fff2f2;color:#b3261e}.check{display:flex;align-items:center;gap:8px;color:#516982;font-size:13px;margin-top:12px}.foot{margin-top:18px;color:#89a0b9;font-size:12px;line-height:1.7}.warn{color:#d92d20;font-size:12px;margin-top:6px;min-height:16px}</style>";
  html += "<script>function tg(id,btn){var e=document.getElementById(id);if(!e)return;e.type=e.type==='password'?'text':'password';}function same(){var p=document.getElementById('pass'),p2=document.getElementById('pass2'),w=document.getElementById('warn');if(!p||!p2||!w)return true;if(p2.value&&p.value!==p2.value){w.innerText='两次输入的密码不一致';return false}w.innerText='';return true}</script>";
  html += "</head><body><div class='box'><div class='logo'>✦</div>" + body + "<div class='foot'>灵汐全屋智能控制中心 · 账号信息仅保存在中控本地配置区。</div></div></body></html>";
  return html;
}

// 函数说明：页面输出函数，负责生成 WebUI 或设置页面内容。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void SendAuthLoginPage(String message = "", String error = "") {
  String body;
  body += "<div class='brand'>灵汐全屋智能控制中心</div><p class='sub'>请输入本地管理员账号。首次使用请先注册；常用设备可勾选 30 天免登录。</p>";
  if (message.length()) body += "<div class='msg'>" + AuthHtmlEscape(message) + "</div>";
  if (error.length()) body += "<div class='msg err'>" + AuthHtmlEscape(error) + "</div>";
  body += "<form method='POST' action='/api/auth/login'><div class='field'><label>用户名</label><input class='input' name='user' autocomplete='username' placeholder='管理员用户名'></div>";
  body += "<div class='field'><label>密码</label><div class='pwd'><input class='input' id='loginPass' name='pass' type='password' autocomplete='current-password' placeholder='管理员密码'><button class='eye' type='button' onclick=\"tg('loginPass')\"></button></div></div>";
  body += "<label class='check'><input type='checkbox' name='remember' value='1' checked> 30 天内免登录</label>";
  body += "<div class='row'><button class='btn' type='submit'>登录</button><a class='link' href='/forgot'>找回密码</a><a class='link' href='/register'>注册</a></div></form>";
  server.send(200, "text/html; charset=utf-8", AuthPageShell("灵汐全屋智能控制中心", body));
}

// 函数说明：页面输出函数，负责生成 WebUI 或设置页面内容。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void SendAuthRegisterPage(String error = "") {
  String body;
  body += "<div class='brand'>注册</div><p class='sub'>欢迎使用灵汐全屋智能控制中心。这里创建的是本地管理员账号，不使用手机号或验证码。</p>";
  if (error.length()) body += "<div class='msg err'>" + AuthHtmlEscape(error) + "</div>";
  body += "<form method='POST' action='/api/auth/register' onsubmit='return same()'><div class='field'><label>用户名</label><input class='input' name='user' autocomplete='username' placeholder='例如 admin / 你的昵称'></div>";
  body += "<div class='field'><label>密码（至少 8 位）</label><div class='pwd'><input class='input' id='pass' name='pass' type='password' autocomplete='new-password' oninput='same()' placeholder='设置密码'><button class='eye' type='button' onclick=\"tg('pass')\"></button></div></div>";
  body += "<div class='field'><label>确认密码</label><div class='pwd'><input class='input' id='pass2' name='pass2' type='password' autocomplete='new-password' oninput='same()' placeholder='再次输入密码'><button class='eye' type='button' onclick=\"tg('pass2')\"></button></div><div class='warn' id='warn'></div></div>";
  body += "<div class='row'><button class='btn blue' type='submit'>注册</button><a class='link' href='/login'>返回登录</a></div></form>";
  server.send(200, "text/html; charset=utf-8", AuthPageShell("灵汐注册", body));
}

// 函数说明：页面输出函数，负责生成 WebUI 或设置页面内容。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleLoginPage() {
  if (AuthIsValid()) { AuthRedirect("/"); return; }
  if (server.hasArg("registered")) SendAuthLoginPage("注册成功，请使用刚才创建的账号登录。", "");
  else if (server.hasArg("logout")) SendAuthLoginPage("已退出登录。", "");
  else SendAuthLoginPage();
}

// 函数说明：页面输出函数，负责生成 WebUI 或设置页面内容。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleRegisterPage() {
  SendAuthRegisterPage();
}

// 函数说明：页面输出函数，负责生成 WebUI 或设置页面内容。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleForgotPasswordPage() {
  String body;
  body += "<div class='brand'>找回密码</div>";
  body += "<p class='sub'>灵汐采用本地管理员账号，账号密码保存在中控设备本地，不使用手机号、邮箱或验证码找回。</p>";
  body += "<div class='msg'>如果忘记管理员密码，请使用维护方式重置管理员：<br>1. 通过巴法云命令主题发送管理员重置维护码；<br>2. 或使用串口助手向中控发送管理员重置维护码。<br><br>维护码请查看最终烧录说明，由设备管理员保存。本页面不会显示维护码。</div>";
  body += "<div class='row'><a class='link' href='/login'>返回登录</a><a class='link' href='/register'>重新注册</a></div>";
  server.send(200, "text/html; charset=utf-8", AuthPageShell("灵汐找回密码", body));
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleAuthStatusApi() {
  SendCorsHeader();
  String json = "{";
  json += "\"ok\":true,";
  json += "\"initialized\":" + String(AuthAdminExists() ? "true" : "false") + ",";
  json += "\"authed\":" + String(AuthIsValid() ? "true" : "false") + ",";
  json += "\"user\":\"" + JsonEscape(AuthCurrentUser()) + "\"";
  json += "}";
  server.send(200, "application/json", json);
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleAuthRegisterApi() {
  String user = server.arg("user"); user.trim();
  String pass = server.arg("pass");
  String pass2 = server.arg("pass2");
  if (user.length() == 0) { SendAuthRegisterPage("用户名不能为空"); return; }
  if (pass.length() < 8) { SendAuthRegisterPage("密码至少 8 位"); return; }
  if (pass != pass2) { SendAuthRegisterPage("两次输入的密码不一致"); return; }
  if (AuthAdminExists()) { SendAuthRegisterPage("管理员账号已存在，请返回登录；如需重置请查看找回密码说明。"); return; }
  String salt = AuthRandomHex(8);
  String hash = AuthPasswordHash(user, salt, pass);
  AuthPrefs.begin(AUTH_PREF_NS, false);
  AuthPrefs.putString("user", user);
  AuthPrefs.putString("salt", salt);
  AuthPrefs.putString("hash", hash);
  AuthPrefs.putBool("first_done", false);
  AuthPrefs.putString("session", "");
  AuthPrefs.end();
  AddCenterLog("security", "管理员账号已创建：" + user);
  AuthRedirect("/login?registered=1");
}

// 函数说明：日志函数，负责记录系统运行和用户操作。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleAuthLoginApi() {
  if (!AuthAdminExists()) { AuthRedirect("/register"); return; }
  String user = server.arg("user"); user.trim();
  String pass = server.arg("pass");
  bool remember = server.hasArg("remember");
  String savedUser = AuthGetPrefString("user", "");
  String salt = AuthGetPrefString("salt", "");
  String hash = AuthGetPrefString("hash", "");
  if (user != savedUser || AuthPasswordHash(user, salt, pass) != hash) {
    AddCenterLog("security", "管理员登录失败：" + user);
    SendAuthLoginPage("", "用户名或密码不正确");
    return;
  }
  String token = AuthRandomHex(24);
  AuthPutPrefString("session", token);
  AuthSetCookie(token, remember);
  bool firstDone = AuthGetPrefBool("first_done", false);
  if (!firstDone) AuthPutPrefBool("first_done", true);
  AddCenterLog("security", "管理员登录成功：" + user + (remember ? " / 30天免登录" : ""));
  AuthRedirect(firstDone ? "/" : "/#settings");
}

// 函数说明：日志函数，负责记录系统运行和用户操作。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleAuthLogoutApi() {
  if (AuthIsValid()) AddCenterLog("security", "管理员退出登录：" + AuthCurrentUser());
  AuthPutPrefString("session", "");
  AuthClearCookie();
  AuthRedirect("/login?logout=1");
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleAuthChangePasswordApi() {
  if (!RequireAuth()) return;
  String oldPass = server.arg("old");
  String newPass = server.arg("pass");
  String newPass2 = server.arg("pass2");
  String user = AuthGetPrefString("user", "");
  String salt = AuthGetPrefString("salt", "");
  String hash = AuthGetPrefString("hash", "");
  if (newPass.length() < 8) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"密码至少 8 位\"}"); return; }
  if (newPass != newPass2) { server.send(400, "application/json", "{\"ok\":false,\"error\":\"两次输入的密码不一致\"}"); return; }
  if (AuthPasswordHash(user, salt, oldPass) != hash) { server.send(403, "application/json", "{\"ok\":false,\"error\":\"旧密码不正确\"}"); return; }
  String newSalt = AuthRandomHex(8);
  String newHash = AuthPasswordHash(user, newSalt, newPass);
  AuthPrefs.begin(AUTH_PREF_NS, false);
  AuthPrefs.putString("salt", newSalt);
  AuthPrefs.putString("hash", newHash);
  AuthPrefs.putString("session", "");
  AuthPrefs.end();
  AuthClearCookie();
  AddCenterLog("security", "管理员密码已修改：" + user);
  server.send(200, "application/json", "{\"ok\":true,\"message\":\"密码已修改，请重新登录\"}");
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool AuthResetAdminByMaintCode(const char* source, bool clearCookie) {
  AuthPrefs.begin(AUTH_PREF_NS, false);
  AuthPrefs.clear();
  AuthPrefs.end();

  if (clearCookie) {
    AuthClearCookie();
  }

  String src = source ? String(source) : String("unknown");
  AddCenterLog("security", "管理员账号已通过维护码重置，来源：" + src);
  Serial.println("[AUTH] Admin account has been reset by maintenance code.");
  Serial.print("[AUTH] Source: ");
  Serial.println(src);
  Serial.println("[AUTH] Reopen WebUI and register administrator again.");
  return true;
}

// 函数说明：控制函数，负责命令解析、场景执行或设备状态变更。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
bool TryAuthResetMaintCommand(String cmd, const char* source) {
  cmd.trim();
  if (cmd.length() == 0) return false;
  if (cmd != AUTH_RESET_CODE) return false;
  AuthResetAdminByMaintCode(source, false);
  return true;
}

// 函数说明：认证函数，负责管理员、登录和会话校验。
// 可改范围：仅在明确需求下调整内部逻辑；接口名和返回格式不要随意改变。
void HandleAuthResetMaintApi() {
  String code = server.arg("code");
  code.trim();
  if (code != AUTH_RESET_CODE) { server.send(403, "application/json", "{\"ok\":false,\"error\":\"维护码不正确\"}"); return; }
  AuthResetAdminByMaintCode("HTTP", true);
  server.send(200, "application/json", "{\"ok\":true,\"message\":\"管理员已重置，请重新注册\"}");
}


void HandleCmdApiAuth() { if (RequireAuth()) HandleCmdApi(); }
void HandleDeviceConfigSaveApiAuth() { if (RequireAuth()) HandleDeviceConfigSaveApi(); }
void HandleDeviceConfigResetApiAuth() { if (RequireAuth()) HandleDeviceConfigResetApi(); }
void HandleCloudConfigSaveApiAuth() { if (RequireAuth()) HandleCloudConfigSaveApi(); }
void HandleVoicePlayApiAuth() { if (RequireAuth()) HandleVoicePlayApi(); }
void HandleVoiceIoRecordApiAuth() { if (RequireAuth()) HandleVoiceIoRecordApi(); }
void HandleVoiceIoStartApiAuth() { if (RequireAuth()) HandleVoiceIoStartApi(); }
void HandleVoiceIoStopApiAuth() { if (RequireAuth()) HandleVoiceIoStopApi(); }
void HandleVoiceIoPlayLastApiAuth() { if (RequireAuth()) HandleVoiceIoPlayLastApi(); }
void HandleVoiceIoSetApiAuth() { if (RequireAuth()) HandleVoiceIoSetApi(); }
void HandleVoiceTaskClearApiAuth() { if (RequireAuth()) HandleVoiceTaskClearApi(); }
void HandleVoiceTaskMockTextApiAuth() { if (RequireAuth()) HandleVoiceTaskMockTextApi(); }
void HandleVoiceTaskMockCommandApiAuth() { if (RequireAuth()) HandleVoiceTaskMockCommandApi(); }
void HandleVoiceTaskMockReplyApiAuth() { if (RequireAuth()) HandleVoiceTaskMockReplyApi(); }
void HandleVoiceTaskAsrApiAuth() { if (RequireAuth()) HandleVoiceTaskAsrApi(); }
void HandleVoiceTaskParseApiAuth() { if (RequireAuth()) HandleVoiceTaskParseApi(); }
void HandleVoiceTaskAsrExecuteApiAuth() { if (RequireAuth()) HandleVoiceTaskAsrExecuteApi(); }
void HandleVoiceTaskDoubaoAsrApiAuth() { if (RequireAuth()) HandleVoiceTaskDoubaoAsrApi(); }
void HandleVoiceTaskDoubaoAiExecuteApiAuth() { if (RequireAuth()) HandleVoiceTaskDoubaoAiExecuteApi(); }
void HandleDoubaoAiTextApiAuth() { if (RequireAuth()) HandleDoubaoAiTextApi(); }
void HandleDoubaoTtsApiAuth() { if (RequireAuth()) HandleDoubaoTtsApi(); }

void HandleNodeDeleteApiAuth() { if (RequireAuth()) HandleNodeDeleteApi(); }
void HandleNodeAddApiAuth() { if (RequireAuth()) HandleNodeAddApi(); }
void HandleNodeRecheckApiAuth() { if (RequireAuth()) HandleNodeRecheckApi(); }
