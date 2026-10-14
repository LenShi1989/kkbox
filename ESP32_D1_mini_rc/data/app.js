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
  if (a.dataset.page === 'user') loadUser();
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
  const up = s.link.up && s.link.age < 2000;
  $('s-link').textContent = s.link.up ? (up ? '已連線' : '連線中') : '未連線';
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
  if (s.link.up && up) { $('joy-state').textContent = '已連線接收機'; }
  else if (s.link.up) { $('joy-state').textContent = '連線中...'; }
  else $('joy-state').textContent = '未連線';
}

function applyTelemetry(t) {
  if (!t || !t.gyro) return;
  $('t-armed').textContent = t.armed ? '已解鎖' : '上鎖';
  $('t-armed').style.color = t.armed ? 'var(--warn)' : 'var(--muted)';
  $('t-thr').textContent = t.set ? t.set.thr : '-';
  $('t-gr').textContent = t.gyro.roll.toFixed(1);
  $('t-gp').textContent = t.gyro.pitch.toFixed(1);
  $('t-gy').textContent = t.gyro.yaw.toFixed(1);
  $('t-motor').textContent = (t.motor || []).join(' ');
  $('joy-state').textContent = t.armed ? '已解鎖 (ARMED)' : '已連線 (待命)';
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
  ws.onmessage = (ev) => {
    try { const m = JSON.parse(ev.data); if (m.gyro) applyTelemetry(m); } catch (e) {}
  };
  ws.onclose = () => { ws = null; setTimeout(() => { if (!$('app').classList.contains('hidden')) connectWs(); }, 3000); };
}
function sendCmd(obj) {
  const s = JSON.stringify(obj);
  if (ws && ws.readyState === 1) ws.send(s);
  else api('/api/command', { method: 'POST', body: { cmd: s } }).catch(() => {});
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
  try {
    await api('/api/wifi', { method: 'POST', body: { ssid: $('wifi-ssid').value, pass: $('wifi-pass').value } });
    toast('已儲存，嘗試連線');
  } catch (e) { toast('儲存失敗', 'err'); }
});
$('wifi-clear').addEventListener('click', async () => {
  try { await api('/api/wifi', { method: 'POST', body: { action: 'clear' } }); toast('已清除'); }
  catch (e) { toast('失敗', 'err'); }
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

$('btn-arm').addEventListener('click', () => sendCmd({ cmd: 'arm' }));
$('btn-disarm').addEventListener('click', () => sendCmd({ cmd: 'disarm' }));
$('btn-cal').addEventListener('click', () => sendCmd({ cmd: 'calibrate' }));
document.querySelectorAll('[data-led]').forEach(b =>
  b.addEventListener('click', () => sendCmd({ cmd: 'led', mode: b.dataset.led })));

/* 連續傳送 (20Hz) */
setInterval(() => {
  if (!dirty) return;
  sendCmd({ cmd: 'throttle', value: joyThr });
  sendCmd({ cmd: 'setpoints', roll: joyRoll, pitch: joyPitch, yaw: joyYaw });
  dirty = false;
}, 50);

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
