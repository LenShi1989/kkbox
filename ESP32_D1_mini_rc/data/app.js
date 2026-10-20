'use strict';

const $ = (id) => document.getElementById(id);
let TOKEN = localStorage.getItem('kkbox-rc-token') || '';
let ws = null;

/* ---------- API ---------- */
async function api(path, opts = {}) {
  const o = { method: opts.method || 'GET', credentials: 'same-origin', headers: {} };
  if (TOKEN) o.headers['X-Auth'] = TOKEN;
  if (opts.body) {
    o.headers['Content-Type'] = 'application/x-www-form-urlencoded';
    o.body = new URLSearchParams(opts.body).toString();
  }
  const r = await fetch(path, o);
  if (r.status === 401) { showLogin(); throw new Error('unauthorized'); }
  const txt = await r.text();
  let data; try { data = JSON.parse(txt); } catch (e) { data = txt; }
  if (!r.ok) throw new Error((data && data.err) || ('HTTP ' + r.status));
  return data;
}

let toastTimer = null;
function toast(msg, kind = 'ok') {
  const t = $('toast'); t.textContent = msg; t.className = 'toast ' + kind;
  clearTimeout(toastTimer); toastTimer = setTimeout(() => t.classList.add('hidden'), 2600);
}

/* ---------- 彈窗 ---------- */
function showModal(title, msg) {
  $('modal-title').textContent = title;
  $('modal-msg').textContent = msg;
  $('modal').classList.remove('hidden');
}
$('modal-ok').addEventListener('click', () => $('modal').classList.add('hidden'));

/* ---------- 主題切換 (明亮 / 暗黑 / 玻璃) ---------- */
const THEME_KEY = 'kkbox-rc-theme';
function setTheme(t) {
  document.documentElement.dataset.theme = t;
  try { localStorage.setItem(THEME_KEY, t); } catch (e) {}
  document.querySelectorAll('.theme-btn').forEach(b =>
    b.classList.toggle('active', b.dataset.theme === t));
}
document.querySelectorAll('.theme-btn').forEach(b =>
  b.addEventListener('click', () => setTheme(b.dataset.theme)));
setTheme((function () { try { return localStorage.getItem(THEME_KEY) || 'dark'; } catch (e) { return 'dark'; } })());

/* ---------- Sidebar (漢堡選單) ---------- */
const menuBtn = $('menu-toggle');
const backdrop = $('backdrop');
const isDesktop = () => window.matchMedia('(min-width:1025px)').matches;
let wasDesktop = isDesktop();
function setNav(open) {
  document.body.classList.toggle('nav-open', open);
  menuBtn.setAttribute('aria-expanded', String(isDesktop() ? !open : open));
}
menuBtn.addEventListener('click', () =>
  setNav(!document.body.classList.contains('nav-open')));
backdrop.addEventListener('click', () => setNav(false));
$('nav').addEventListener('click', () => { if (!isDesktop()) setNav(false); });
window.addEventListener('resize', () => {
  const d = isDesktop();
  if (d !== wasDesktop) { wasDesktop = d; setNav(false); }
});
setNav(false);

/* ---------- 登入 ---------- */
function showLogin() {
  $('app').classList.add('hidden');
  $('login').classList.remove('hidden');
  if (ws) { try { ws.close(); } catch (e) {} ws = null; }
}
$('login-form').addEventListener('submit', async (e) => {
  e.preventDefault(); $('lg-err').textContent = '';
  try {
    const d = await fetch('/api/login', { method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: new URLSearchParams({ user: $('lg-user').value, pass: $('lg-pass').value }).toString() });
    const j = await d.json();
    if (!d.ok) { $('lg-err').textContent = j.err || '登入失敗'; return; }
    TOKEN = j.token || ''; localStorage.setItem('kkbox-rc-token', TOKEN); start();
  } catch (e) { $('lg-err').textContent = '無法連線'; }
});
$('logout').addEventListener('click', async () => {
  try { await api('/api/logout', { method: 'POST' }); } catch (e) {}
  localStorage.removeItem('kkbox-rc-token'); TOKEN = ''; showLogin();
});

/* ---------- 導覽 ---------- */
$('nav').addEventListener('click', (e) => {
  const a = e.target.closest('a[data-page]'); if (!a) return;
  document.querySelectorAll('#nav a').forEach(x => x.classList.toggle('active', x === a));
  document.querySelectorAll('.page').forEach(p => p.classList.toggle('active', p.id === 'page-' + a.dataset.page));
  if (a.dataset.page === 'network') loadNet();
  if (a.dataset.page === 'bt') loadBt();
  if (a.dataset.page === 'pid') loadPid();
  if (a.dataset.page === 'user') loadUser();
  if (a.dataset.page === 'esc') loadEsc();
});

/* ---------- 狀態 ---------- */
function fmtUptime(s) {
  const d = Math.floor(s / 86400), h = Math.floor(s % 86400 / 3600),
        m = Math.floor(s % 3600 / 60), ss = s % 60;
  return (d ? d + 'd ' : '') + h + 'h ' + m + 'm ' + ss + 's';
}
async function loadStatus() {
  let s; try { s = await api('/api/status'); } catch (e) { return; }
  $('s-fw').textContent = s.fw + ' ' + s.ver;
  $('s-uptime').textContent = fmtUptime(s.uptime);
  $('s-heap').textContent = (s.heap / 1024).toFixed(1) + ' KB';
  const bt = s.chan === 1;
  const up = bt ? !!s.btUp : (s.link.up && s.link.age < 2000);
  const linked = bt ? !!s.btUp : s.link.up;
  $('s-link').textContent = (bt ? '藍牙：' : 'WiFi：') + (linked ? (up ? '已連線' : '連線中') : '未連線');
  $('s-link').style.color = up ? 'var(--ok)' : 'var(--muted)';
  $('net-dot').classList.toggle('on', s.wifi.connected);

  $('s-wifi-c').textContent = s.wifi.connected ? '已連線' : '未連線';
  $('s-wifi-c').style.color = s.wifi.connected ? 'var(--ok)' : 'var(--muted)';
  $('s-wifi-ssid').textContent = s.wifi.ssid || '-';
  $('s-wifi-ip').textContent = s.wifi.ip;
  $('s-wifi-rssi').textContent = s.wifi.connected ? s.wifi.rssi + ' dBm' : '-';

  if (s.ap && s.ap.active) $('s-ap').textContent = s.ap.ssid + ' (' + s.ap.ip + ')';
  else $('s-ap').textContent = '-';

  const st = $('joy-state');
  if (linked && up) { $('joy-state').textContent = '已連線接收機'; }
  else if (linked) { $('joy-state').textContent = '連線中...'; }
  else $('joy-state').textContent = '未連線';

  /* 飛控最後回應: 走 HTTP 輪詢, 瀏覽器 WS 中斷時仍可確認命令結果 */
  if ((s.kkAck || '') !== lastAckPoll) {
    lastAckPoll = s.kkAck || '';
    const e = $('kk-ack');
    if (e) e.textContent = s.kkAck || '-';
    try { const a = JSON.parse(s.kkAck); if (a && a.ack) onAck(a.ack); } catch (err) {}
  }
}

function applyTelemetry(t) {
  if (!t || !t.gyro) return;
  armedNow = !!t.armed;
  $('t-armed').textContent = t.armed ? '已解鎖' : '上鎖';
  $('t-armed').style.color = t.armed ? 'var(--warn)' : 'var(--muted)';
  $('t-thr').textContent = t.set ? t.set.thr : '-';
  $('t-gr').textContent = t.gyro.roll.toFixed(1);
  $('t-gp').textContent = t.gyro.pitch.toFixed(1);
  $('t-gy').textContent = t.gyro.yaw.toFixed(1);
  $('t-motor').textContent = (t.motor || []).join(' ');
  $('joy-state').textContent = t.armed ? '已解鎖 (ARMED)' : '已連線 (待命)';
  if (t.hold !== undefined) setHold(!!t.hold);
  loadEsc();
}

async function loadFs() {
  let f; try { f = await api('/api/fs'); } catch (e) { return; }
  const pct = f.total ? (f.used / f.total * 100) : 0;
  $('fs-usage').textContent = (f.used / 1024).toFixed(1) + ' / ' + (f.total / 1024).toFixed(1) + ' KB (' + pct.toFixed(0) + '%)';
  $('fs-bar').style.width = pct.toFixed(1) + '%';
  const tb = $('fs-files').querySelector('tbody'); tb.innerHTML = '';
  (f.files || []).forEach(x => {
    const tr = document.createElement('tr');
    tr.innerHTML = '<td>' + x.name + '</td><td>' + x.size + ' B</td>';
    tb.appendChild(tr);
  });
}

/* ---------- WebSocket ---------- */
function connectWs() {
  const proto = location.protocol === 'https:' ? 'wss:' : 'ws:';
  const url = proto + '//' + location.host + '/ws' + (TOKEN ? '?token=' + encodeURIComponent(TOKEN) : '');
  try { ws = new WebSocket(url); } catch (e) { return; }
  ws.onopen = () => setWs('已建立 (即時)', true);
  ws.onmessage = (ev) => {
    try {
      const m = JSON.parse(ev.data);
      if (m.gyro) applyTelemetry(m);
      else if (m.ack) onAck(m.ack);
      else if (m.err) onLinkErr(m.err);
      else if (m.evt === 'failsafe') onLinkErr('飛控 failsafe 已自動上鎖');
    } catch (e) {}
  };
  ws.onclose = () => {
    setWs('中斷 (改 REST 送出, 無即時 ack)', false);
    ws = null;
    setTimeout(() => { if (!$('app').classList.contains('hidden')) connectWs(); }, 3000);
  };
}
function setWs(txt, ok) {
  const e = $('ws-state');
  if (!e) return;
  e.textContent = txt;
  e.style.color = ok ? 'var(--ok)' : 'var(--warn)';
}
let txCount = 0;
function sendCmd(obj) {
  const s = JSON.stringify(obj);
  txCount++;
  const e = $('tx-info');
  if (e) e.textContent = txCount + ' 筆 (最後: ' + (obj.cmd || '?') + ')';
  if (ws && ws.readyState === 1) ws.send(s);
  else api('/api/command', { method: 'POST', body: { cmd: s } }).catch((e) => onLinkErr(e.message));
}

/* ---------- 命令回應追蹤: KK 板遙測傳輸中 RX 短暫全盲, 單次命令需重送 ---------- */
const pendingCmd = {};
let lastErrAt = 0;
let lastAckPoll = '';
function onAck(cmd) {
  const st = pendingCmd[cmd];
  if (!st) return;   /* 週期性命令 (throttle/setpoints) 的回應不提示 */
  clearTimeout(st.timer); delete pendingCmd[cmd];
  toast('✓ ' + cmd + ' 飛控已回應', 'ok');
}
function onLinkErr(msg) {
  const now = Date.now();
  if (now - lastErrAt < 3000) return;   /* 節流: 週期命令失敗不洗版 */
  lastErrAt = now;
  toast('✗ ' + msg, 'err');
}
function sendOnce(obj, tries = 3) {
  const cmd = obj.cmd;
  const old = pendingCmd[cmd];
  if (old) clearTimeout(old.timer);
  const st = { n: 0, timer: 0 };
  pendingCmd[cmd] = st;
  (function go() {
    sendCmd(obj);
    st.n++;
    st.timer = setTimeout(() => {
      if (st.n < tries) { go(); return; }
      if (pendingCmd[cmd] === st) delete pendingCmd[cmd];
      toast('✗ ' + cmd + ' 無回應 (' + tries + ' 次)', 'err');
    }, 250);
  })();
}

/* ---------- 網路設定 ---------- */
$('wifi-scan').addEventListener('click', async () => {
  $('wifi-scan-state').textContent = '掃描中...';
  try {
    const list = await api('/api/scan');
    const sel = $('wifi-list'); sel.innerHTML = '';
    list.sort((a, b) => b.rssi - a.rssi).forEach(n => {
      const o = document.createElement('option');
      o.value = n.ssid;
      o.textContent = n.ssid + '  (' + n.rssi + ' dBm' + (n.enc ? ', 加密' : '') + ')';
      sel.appendChild(o);
    });
    $('wifi-scan-state').textContent = list.length + ' 個網路';
  } catch (e) { $('wifi-scan-state').textContent = '掃描失敗'; }
});
$('wifi-list').addEventListener('change', (e) => { $('wifi-ssid').value = e.target.value; });
$('wifi-save').addEventListener('click', async () => {
  $('wifi-scan-state').textContent = '連線中…';
  toast('連線中，請稍候…');
  try {
    const res = await api('/api/wifi', { method: 'POST', body: { ssid: $('wifi-ssid').value, pass: $('wifi-pass').value } });
    const msg = '已連線，IP 位置：' + res.ip + '（系統將重新啟動）';
    $('wifi-scan-state').textContent = msg;
    toast(msg, 'ok');
  } catch (e) { toast('連線失敗: ' + e.message, 'err'); }
});
$('wifi-clear').addEventListener('click', async () => {
  try {
    await api('/api/wifi', { method: 'POST', body: { action: 'clear' } });
    showModal('已刪除', '連線設定已刪除，系統將重新啟動…');
  } catch (e) { toast('清除失敗', 'err'); }
});

async function loadNet() {
  let l; try { l = await api('/api/link'); } catch (e) { return; }
  $('lk-host').value = l.host; $('lk-port').value = l.port; $('lk-token').value = l.token;
}
$('lk-save').addEventListener('click', async () => {
  try {
    await api('/api/link', { method: 'POST', body: { host: $('lk-host').value, port: $('lk-port').value, token: $('lk-token').value } });
    toast('已儲存，重新連線接收機');
  } catch (e) { toast('儲存失敗', 'err'); }
});

/* ---------- PID ---------- */
async function loadPid() {
  let p;
  try { p = await api('/api/pid'); } catch (e) { return; }
  for (const ax of ['roll', 'pitch', 'yaw']) {
    $('pid-' + ax + '-kp').value = p[ax].kp;
    $('pid-' + ax + '-ki').value = p[ax].ki;
    $('pid-' + ax + '-kd').value = p[ax].kd;
  }
}
$('pid-save').addEventListener('click', async () => {
  try {
    for (const ax of ['roll', 'pitch', 'yaw']) {
      await api('/api/pid', { method: 'POST', body: {
        axis: ax, kp: $('pid-' + ax + '-kp').value,
        ki: $('pid-' + ax + '-ki').value, kd: $('pid-' + ax + '-kd').value } });
    }
    toast('PID 已儲存並下傳飛控板');
  } catch (e) { toast('儲存失敗: ' + e.message, 'err'); }
});

/* ---------- 藍牙 (BLE) 連線 ---------- */
let btPoll = null;
let btDevs = [];

function renderBt(d) {
  $('bt-up').textContent = d.up ? '已連線' : '未連線';
  $('bt-up').style.color = d.up ? 'var(--ok)' : 'var(--muted)';
  $('bt-paired').textContent = d.addr ? (d.name || d.addr) + ' (' + d.addr + ')' : '無';
  $('bt-name').textContent = d.name || '-';
  $('bt-rx').textContent = d.rx || 0;
  $('bt-err').textContent = d.err || '';

  $('bt-chan-state').textContent = '目前：' + (d.chan === 1 ? '藍牙' : 'WiFi');
  $('bt-chan-ble').classList.toggle('danger', d.chan === 1);
  $('bt-chan-wifi').classList.toggle('danger', d.chan !== 1);

  if (d.scanning) {
    $('bt-scan-state').textContent = '掃描中... ' + (d.devs ? d.devs.length : 0) + ' 台';
  } else {
    $('bt-scan-state').textContent = (d.devs ? d.devs.length : 0) + ' 台裝置';
  }
  const sel = $('bt-list');
  const keep = sel.value;
  btDevs = d.devs || [];
  sel.innerHTML = '';
  btDevs.sort((a, b) => b.rssi - a.rssi).forEach(n => {
    const o = document.createElement('option');
    o.value = n.addr;
    o.textContent = (n.name || n.addr) + '  (' + n.rssi + ' dBm)';
    sel.appendChild(o);
  });
  if (keep) sel.value = keep;
}

async function loadBt() {
  let d; try { d = await api('/api/bt'); } catch (e) { return; }
  renderBt(d);
  if (d.scanning) {
    if (!btPoll) btPoll = setInterval(loadBt, 1000);
  } else if (btPoll) {
    clearInterval(btPoll); btPoll = null;
  }
}

$('bt-scan').addEventListener('click', async () => {
  try { await api('/api/bt', { method: 'POST', body: { action: 'scan' } }); loadBt(); }
  catch (e) { toast('掃描失敗: ' + e.message, 'err'); }
});
$('bt-stop').addEventListener('click', async () => {
  try { await api('/api/bt', { method: 'POST', body: { action: 'stop' } }); } catch (e) {}
  if (btPoll) { clearInterval(btPoll); btPoll = null; }
  loadBt();
});
$('bt-connect').addEventListener('click', async () => {
  const addr = $('bt-list').value;
  if (!addr) { toast('請先掃描並選擇裝置', 'err'); return; }
  const dev = btDevs.find(x => x.addr === addr) || {};
  try {
    await api('/api/bt', { method: 'POST', body: { action: 'connect', addr: addr, name: dev.name || '' } });
    toast('正在連線配對…');
    if (btPoll) { clearInterval(btPoll); btPoll = null; }
    setTimeout(loadBt, 1500); setTimeout(loadBt, 4000);
  } catch (e) { toast('連線失敗: ' + e.message, 'err'); }
});
$('bt-disconnect').addEventListener('click', async () => {
  try { await api('/api/bt', { method: 'POST', body: { action: 'disconnect' } }); loadBt(); }
  catch (e) { toast('斷線失敗', 'err'); }
});
$('bt-clear').addEventListener('click', async () => {
  try { await api('/api/bt', { method: 'POST', body: { action: 'clear' } }); loadBt(); toast('已清除配對'); }
  catch (e) { toast('清除失敗', 'err'); }
});
$('bt-chan-wifi').addEventListener('click', () => setChan(0));
$('bt-chan-ble').addEventListener('click', () => setChan(1));
async function setChan(ch) {
  try { await api('/api/bt', { method: 'POST', body: { action: 'channel', ch: ch } }); loadBt(); loadStatus(); toast('通道：' + (ch ? '藍牙' : 'WiFi')); }
  catch (e) { toast('切換失敗', 'err'); }
}

/* ---------- 使用者 ---------- */
async function loadUser() {
  let u; try { u = await api('/api/user'); } catch (e) { return; }
  $('u-user').value = u.user;
}
$('u-save').addEventListener('click', async () => {
  try {
    await api('/api/user', { method: 'POST', body: { user: $('u-user').value, pass: $('u-pass').value, cur: $('u-cur').value } });
    toast('已儲存'); $('u-pass').value = ''; $('u-cur').value = '';
  } catch (e) { toast('儲存失敗: ' + e.message, 'err'); }
});

/* ---------- 模擬搖桿 ---------- */
const RANGE = 200;          /* 角速率滿刻度 deg/s */
let joyRoll = 0, joyPitch = 0, joyYaw = 0, joyThr = 1000, dirty = false;
let armedNow = false;
let holdOn = false;
const pad = $('pad'), knob = $('knob');
let dragging = false;

function setKnob(nx, ny) {
  const r = pad.clientWidth * 0.5 - 28;
  knob.style.left = (pad.clientWidth / 2 + nx * r) + 'px';
  knob.style.top  = (pad.clientHeight / 2 + ny * r) + 'px';
}
function padMove(clientX, clientY) {
  const rect = pad.getBoundingClientRect();
  let nx = (clientX - rect.left - rect.width / 2) / (rect.width / 2 - 28);
  let ny = (clientY - rect.top - rect.height / 2) / (rect.height / 2 - 28);
  const len = Math.hypot(nx, ny);
  if (len > 1) { nx /= len; ny /= len; }
  setKnob(nx, ny);
  joyRoll  = Math.round(nx * RANGE);
  joyPitch = Math.round(-ny * RANGE);
  $('joy-roll').textContent = joyRoll;
  $('joy-pitch').textContent = joyPitch;
  dirty = true;
}
pad.addEventListener('pointerdown', (e) => { dragging = true; pad.setPointerCapture(e.pointerId); padMove(e.clientX, e.clientY); });
pad.addEventListener('pointermove', (e) => { if (dragging) padMove(e.clientX, e.clientY); });
pad.addEventListener('pointerup',   () => { dragging = false; });
pad.addEventListener('pointercancel', () => { dragging = false; });

function resetPad() {
  joyRoll = 0; joyPitch = 0; joyYaw = 0; joyThr = 1000;
  setKnob(0, 0);
  $('joy-roll').textContent = '0'; $('joy-pitch').textContent = '0';
  $('joy-yaw').value = 0; $('joy-yaw-v').textContent = '0';
  $('joy-thr').value = 1000; $('joy-thr-v').textContent = '1000';
}
$('btn-reset').addEventListener('click', () => { resetPad(); dirty = true; });

$('joy-thr').addEventListener('input', (e) => { joyThr = +e.target.value; $('joy-thr-v').textContent = joyThr; dirty = true; });
$('joy-yaw').addEventListener('input', (e) => { joyYaw = +e.target.value; $('joy-yaw-v').textContent = joyYaw; dirty = true; });

$('btn-arm').addEventListener('click', () => sendOnce({ cmd: 'arm' }));
$('btn-disarm').addEventListener('click', () => sendOnce({ cmd: 'disarm' }));
$('btn-cal').addEventListener('click', () => sendOnce({ cmd: 'calibrate' }));

/* 定高 (油門保持): 鎖定飛控板當前油門, 無高度感測器 */
function setHold(on) {
  holdOn = on;
  const b = $('btn-hold');
  if (b) { b.textContent = '定高：' + (on ? '開' : '關'); b.classList.toggle('danger', on); }
}
$('btn-hold').addEventListener('click', () => {
  setHold(!holdOn);
  sendOnce({ cmd: 'hold', on: holdOn ? 1 : 0 });
});
document.querySelectorAll('[data-led]').forEach(b =>
  b.addEventListener('click', () => sendOnce({ cmd: 'led', mode: b.dataset.led })));

/* 連續傳送 (20Hz) */
setInterval(() => {
  if (!dirty) return;
  sendCmd({ cmd: 'throttle', value: joyThr });
  sendCmd({ cmd: 'setpoints', roll: joyRoll, pitch: joyPitch, yaw: joyYaw });
  dirty = false;
}, 50);

/* 解鎖中每 500ms 送 ping 保活: KK 板 2 秒無命令即 failsafe 上鎖,
 * 否則搖桿靜止時 (無 input 事件) 會被誤判失聯而斷馬達 */
setInterval(() => {
  if (armedNow) sendCmd({ cmd: 'ping' });
}, 500);

/* ---------- 馬達校正 (ESC 行程校正) 導引 ---------- */
function loadEsc() {
  const e = $('esc-armed');
  if (!e) return;
  e.textContent = armedNow ? '已解鎖 (請先上鎖)' : '上鎖 (可校正)';
  e.style.color = armedNow ? 'var(--warn)' : 'var(--ok)';
}
$('esc-safe').addEventListener('click', () => sendOnce({ cmd: 'disarm' }));

let escTimerId = null;
$('esc-timer').addEventListener('click', () => {
  if (escTimerId) { clearInterval(escTimerId); escTimerId = null; }
  sendOnce({ cmd: 'disarm' });
  let n = 3;
  $('esc-count').textContent = n + ' 秒';
  escTimerId = setInterval(() => {
    n--;
    if (n <= 0) {
      clearInterval(escTimerId); escTimerId = null;
      $('esc-count').textContent = '開始！推最高油門';
      toast('開始！把油門推至最高', 'ok');
    } else {
      $('esc-count').textContent = n + ' 秒';
    }
  }, 1000);
});

/* ---------- 啟動 ---------- */
function start() {
  $('login').classList.add('hidden');
  $('app').classList.remove('hidden');
  resetPad();
  loadStatus(); loadFs(); connectWs();
  setInterval(loadStatus, 3000);
  setInterval(loadFs, 15000);
}

(async function init() {
  try {
    const ok = await fetch('/api/status', { credentials: 'same-origin', headers: TOKEN ? { 'X-Auth': TOKEN } : {} });
    if (ok.ok) start(); else showLogin();
  } catch (e) { showLogin(); }
})();
