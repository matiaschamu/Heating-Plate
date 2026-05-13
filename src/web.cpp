#include "web.h"
#include "wifimgr.h"
#include "controller.h"
#include "FanBuzzer.h"

#include <WebServer.h>
#include <DNSServer.h>

static WebServer webServer(80);
static DNSServer dnsServer;
static bool      portalMode = false;

// ── Captive portal HTML (modo AP) ─────────────────────────────────────────────
static const char CAPTIVE_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Alien Tech - Configuración WiFi</title>
<style>
  body{font-family:system-ui,-apple-system,sans-serif;background:#0d0d12;color:#eee;margin:0;padding:24px;}
  .card{max-width:380px;margin:40px auto;background:#1a1a22;padding:24px;border-radius:12px;box-shadow:0 4px 20px rgba(0,0,0,.4);}
  h1{color:#4ac;font-size:22px;margin:0 0 4px;}
  p{color:#888;margin:0 0 24px;font-size:14px;}
  label{display:block;margin-top:14px;font-size:13px;color:#bbb;}
  input{width:100%;padding:11px;font-size:16px;margin-top:6px;box-sizing:border-box;background:#0d0d12;border:1px solid #333;color:#eee;border-radius:6px;}
  input:focus{outline:none;border-color:#4ac;}
  button{width:100%;padding:13px;margin-top:24px;font-size:16px;background:#4ac;border:none;color:#000;font-weight:600;cursor:pointer;border-radius:6px;}
  button:hover{background:#5bd;}
</style>
</head>
<body>
  <div class="card">
    <h1>Alien Tech</h1>
    <p>Configurá la red WiFi del calentador.</p>
    <form action="/save" method="POST">
      <label>Red (SSID)<input name="ssid" required maxlength="32" autocomplete="off"></label>
      <label>Contraseña<input name="pass" type="password" maxlength="63" autocomplete="off"></label>
      <button type="submit">Aceptar</button>
    </form>
  </div>
</body>
</html>
)HTML";

static const char SAVED_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Guardado</title>
<style>body{font-family:system-ui,sans-serif;background:#0d0d12;color:#eee;text-align:center;padding:60px 20px;}
h1{color:#4ac;}p{color:#888;}</style></head>
<body><h1>Configuración guardada</h1><p>El calentador se está reiniciando…</p></body></html>
)HTML";

static void handleCaptiveRoot() {
    webServer.send_P(200, "text/html", CAPTIVE_HTML);
}

static void handleCaptiveSave() {
    String ssid = webServer.arg("ssid");
    String pass = webServer.arg("pass");

    if (ssid.length() == 0) {
        webServer.sendHeader("Location", "/", true);
        webServer.send(302, "text/plain", "");
        return;
    }

    wifiSaveCredentials(ssid, pass);
    webServer.send_P(200, "text/html", SAVED_HTML);
    delay(1500);
    ESP.restart();
}

// Cualquier ruta desconocida (probes de iOS/Android/Windows) → redirige al form
static void handleCaptiveRedirect() {
    webServer.sendHeader("Location", String("http://") + wifiGetIP().toString() + "/", true);
    webServer.send(302, "text/plain", "");
}

// ── UI de control HTML (modo STA) ─────────────────────────────────────────────
static const char CONTROL_HTML[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Alien Tech — Control</title>
<style>
  body{font-family:system-ui,-apple-system,sans-serif;background:#0d0d12;color:#eee;margin:0;padding:20px;}
  .container{max-width:480px;margin:0 auto;}
  h1{color:#4ac;margin:0 0 4px;font-size:22px;}
  .subtitle{color:#666;font-size:13px;margin-bottom:20px;}
  .card{background:#1a1a22;padding:20px;border-radius:12px;margin-bottom:14px;box-shadow:0 4px 20px rgba(0,0,0,.4);}
  .label{font-size:12px;color:#888;text-transform:uppercase;letter-spacing:.5px;margin-bottom:6px;text-align:center;}
  .big-temp{font-size:54px;font-weight:300;color:#4ac;text-align:center;line-height:1;font-variant-numeric:tabular-nums;}
  .big-temp .unit{font-size:20px;color:#666;margin-left:4px;}
  .status{display:flex;gap:8px;justify-content:center;margin-top:14px;}
  .badge{padding:5px 11px;border-radius:6px;font-size:12px;font-weight:600;}
  .badge-on{background:#4ac;color:#000;}
  .badge-off{background:#333;color:#aaa;}
  .badge-warn{background:#fa4;color:#000;}
  .timer-display{font-size:36px;color:#fa4;text-align:center;font-variant-numeric:tabular-nums;margin-top:14px;}
  .duty-bar{height:2px;background:#222;border-radius:1px;margin:-12px -20px 14px;overflow:hidden;}
  .duty-fill{height:100%;background:linear-gradient(90deg,#4ac,#fa4);width:0;transition:width .4s ease;}
  .duty-text{text-align:center;font-size:13px;color:#777;letter-spacing:.6px;margin-top:6px;font-variant-numeric:tabular-nums;min-height:18px;}
  .row{display:flex;gap:12px;align-items:center;margin-bottom:14px;}
  .row:last-child{margin-bottom:0;}
  .row > label{flex:1;font-size:14px;color:#bbb;}
  input,select{padding:10px;font-size:15px;background:#0d0d12;border:1px solid #333;color:#eee;border-radius:6px;width:140px;text-align:right;font-family:inherit;}
  select{text-align:left;}
  input:focus,select:focus{outline:none;border-color:#4ac;}
  button{padding:14px;font-size:16px;border:none;border-radius:8px;cursor:pointer;font-weight:600;width:100%;font-family:inherit;}
  .btn-on{background:#4ac;color:#000;}
  .btn-on:hover{background:#5bd;}
  .btn-off{background:#a44;color:#fff;}
  .btn-off:hover{background:#b55;}
  .btn-reset{background:#222;color:#888;margin-top:10px;font-size:13px;padding:10px;}
  .btn-reset:hover{background:#333;}
  button:disabled{opacity:.4;cursor:not-allowed;}
</style>
</head>
<body>
<div class="container">
  <h1>Alien Tech</h1>
  <div class="subtitle">Control remoto</div>

  <div class="card">
    <div class="duty-bar"><div class="duty-fill" id="dutyFill"></div></div>
    <div class="label">Temperatura actual</div>
    <div class="big-temp"><span id="curTemp">--</span><span class="unit">°C</span></div>
    <div class="duty-text" id="dutyText"></div>
    <div class="status">
      <span class="badge" id="modeBadge">--</span>
      <span class="badge" id="outputBadge">--</span>
    </div>
    <div class="timer-display" id="timerDisplay" style="display:none;"></div>
  </div>

  <div class="card">
    <div class="row">
      <label>Modo</label>
      <select id="mode">
        <option value="0">OFF</option>
        <option value="1">Potencia</option>
        <option value="2">Potencia + Timer</option>
        <option value="3">Temperatura</option>
        <option value="4">Temperatura + Timer</option>
      </select>
    </div>
    <div class="row">
      <label>Temperatura objetivo (°C)</label>
      <input type="number" id="tempSp" min="0" max="650" step="5">
    </div>
    <div class="row">
      <label>Potencia (W)</label>
      <input type="number" id="powerSp" min="100" max="2000" step="100">
    </div>
    <div class="row">
      <label>Timer (MM:SS)</label>
      <input type="text" id="timerSp" placeholder="10:00" pattern="[0-9]+:[0-9]+">
    </div>
  </div>

  <div class="card">
    <button id="btnPower" class="btn-on">Encender</button>
    <button id="btnReset" class="btn-reset">Reiniciar equipo</button>
  </div>
</div>
<script>
const MODES = ['OFF','Potencia','Potencia+Timer','Temperatura','Temperatura+Timer'];
let state = null;
let editing = new Set();

function fmt(s){const m=Math.floor(s/60),x=s%60;return String(m).padStart(2,'0')+':'+String(x).padStart(2,'0');}
function parseT(t){const m=String(t).match(/^(\d+):(\d+)$/);if(!m)return null;return parseInt(m[1])*60+parseInt(m[2]);}

async function getState(){
  try{ const r=await fetch('/api/state'); state=await r.json(); render(); }catch(e){}
}
function render(){
  if(!state) return;
  document.getElementById('curTemp').textContent=state.curTemp.toFixed(1);
  const duty=state.resistorDuty||0;
  document.getElementById('dutyFill').style.width=duty+'%';
  document.getElementById('dutyText').textContent=state.outputOn?('salida '+duty+' %'):'';
  const mb=document.getElementById('modeBadge');
  mb.textContent=MODES[state.mode];
  mb.className='badge '+(state.mode===0?'badge-off':'badge-on');
  const ob=document.getElementById('outputBadge');
  ob.textContent=state.outputOn?'ACTIVO':'EN ESPERA';
  ob.className='badge '+(state.outputOn?'badge-warn':'badge-off');

  const td=document.getElementById('timerDisplay');
  if(state.timerSecs>0){td.style.display='block';td.textContent=fmt(state.timerSecs);}
  else td.style.display='none';

  if(!editing.has('mode')) document.getElementById('mode').value=state.mode;
  if(!editing.has('tempSp')) document.getElementById('tempSp').value=state.tempSetpoint;
  if(!editing.has('powerSp')) document.getElementById('powerSp').value=state.powerWatts;
  if(!editing.has('timerSp')) document.getElementById('timerSp').value=fmt(state.timerSetSecs);

  const btn=document.getElementById('btnPower');
  btn.disabled=(state.mode===0);
  if(state.outputOn){btn.textContent='Apagar';btn.className='btn-off';}
  else{btn.textContent='Encender';btn.className='btn-on';}
}
async function post(p,b){
  const r=await fetch(p,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:b});
  if(r.ok) getState();
}
function edit(id){ editing.add(id); clearTimeout(window['_t_'+id]); window['_t_'+id]=setTimeout(()=>editing.delete(id),2500); }

document.getElementById('mode').addEventListener('change',e=>{edit('mode');post('/api/mode','v='+e.target.value);});
document.getElementById('tempSp').addEventListener('input',()=>edit('tempSp'));
document.getElementById('tempSp').addEventListener('change',e=>{post('/api/temp','v='+e.target.value);editing.delete('tempSp');});
document.getElementById('powerSp').addEventListener('input',()=>edit('powerSp'));
document.getElementById('powerSp').addEventListener('change',e=>{post('/api/power','v='+e.target.value);editing.delete('powerSp');});
document.getElementById('timerSp').addEventListener('input',()=>edit('timerSp'));
document.getElementById('timerSp').addEventListener('change',e=>{const s=parseT(e.target.value);if(s!==null)post('/api/timer','v='+s);editing.delete('timerSp');});
document.getElementById('btnPower').addEventListener('click',()=>{if(state)post('/api/output','v='+(state.outputOn?0:1));});
document.getElementById('btnReset').addEventListener('click',()=>{if(confirm('¿Reiniciar el equipo?'))fetch('/api/reset',{method:'POST'});});

getState();
setInterval(getState,1000);
</script>
</body>
</html>
)HTML";

// ── Handlers de control ───────────────────────────────────────────────────────
static void handleControlRoot() {
    webServer.send_P(200, "text/html", CONTROL_HTML);
}

static void handleApiState() {
    AppState s;
    controllerGetState(s);

    String json;
    json.reserve(192);
    json  = "{\"mode\":";        json += (int)s.mode;
    json += ",\"outputOn\":";    json += s.outputOn ? "true" : "false";
    json += ",\"powerWatts\":";  json += s.powerWatts;
    json += ",\"tempSetpoint\":";json += s.tempSetpoint;
    json += ",\"curTemp\":";     json += String(s.currentTemp, 1);
    json += ",\"timerSetSecs\":";json += s.timerSetSecs;
    json += ",\"timerSecs\":";   json += s.timerSecs;
    json += ",\"resistorDuty\":";json += s.resistorDuty;
    json += "}";
    webServer.send(200, "application/json", json);
}

static bool argInt(const char* name, long& out) {
    if (!webServer.hasArg(name)) return false;
    String v = webServer.arg(name);
    if (v.length() == 0) return false;
    out = v.toInt();
    return true;
}

static void handleApiMode() {
    long v;
    if (!argInt("v", v) || v < 0 || v > 4) { webServer.send(400, "text/plain", "bad value"); return; }
    controllerSetMode((AppMode)v);
    buzzerBeep(30);
    webServer.send(200, "text/plain", "ok");
}

static void handleApiOutput() {
    long v;
    if (!argInt("v", v)) { webServer.send(400, "text/plain", "bad value"); return; }
    bool on = v != 0;
    bool ok = controllerSetOutputOn(on);
    if (ok) buzzerBeep(on ? 200 : 100);
    webServer.send(ok ? 200 : 409, "text/plain", ok ? "ok" : "mode is OFF");
}

static void handleApiTemp() {
    long v;
    if (!argInt("v", v) || v < 0 || v > 650) { webServer.send(400, "text/plain", "bad value"); return; }
    controllerSetTempSetpoint((uint16_t)v);
    buzzerBeep(20);
    webServer.send(200, "text/plain", "ok");
}

static void handleApiPower() {
    long v;
    if (!argInt("v", v) || v < 100 || v > 2000) { webServer.send(400, "text/plain", "bad value"); return; }
    controllerSetPowerWatts((uint16_t)v);
    buzzerBeep(20);
    webServer.send(200, "text/plain", "ok");
}

static void handleApiTimer() {
    long v;
    if (!argInt("v", v) || v < 5 || v > 99 * 60) { webServer.send(400, "text/plain", "bad value"); return; }
    controllerSetTimerSecs((uint32_t)v);
    buzzerBeep(20);
    webServer.send(200, "text/plain", "ok");
}

static void handleApiReset() {
    buzzerBeep(300);
    webServer.send(200, "text/plain", "ok");
    delay(500);
    ESP.restart();
}

// ── API pública ───────────────────────────────────────────────────────────────
void webStartControlUI() {
    portalMode = false;
    webServer.on("/",            HTTP_GET,  handleControlRoot);
    webServer.on("/api/state",   HTTP_GET,  handleApiState);
    webServer.on("/api/mode",    HTTP_POST, handleApiMode);
    webServer.on("/api/output",  HTTP_POST, handleApiOutput);
    webServer.on("/api/temp",    HTTP_POST, handleApiTemp);
    webServer.on("/api/power",   HTTP_POST, handleApiPower);
    webServer.on("/api/timer",   HTTP_POST, handleApiTimer);
    webServer.on("/api/reset",   HTTP_POST, handleApiReset);
    webServer.begin();
}

void webStartCaptivePortal() {
    portalMode = true;
    dnsServer.start(53, "*", wifiGetIP());   // DNS hijack: cualquier dominio → AP
    webServer.on("/",     HTTP_GET,  handleCaptiveRoot);
    webServer.on("/save", HTTP_POST, handleCaptiveSave);
    webServer.onNotFound(handleCaptiveRedirect);
    webServer.begin();
}

void webHandle() {
    if (portalMode) dnsServer.processNextRequest();
    webServer.handleClient();
}
