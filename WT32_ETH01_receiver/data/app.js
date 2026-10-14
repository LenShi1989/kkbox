'use strict';

const $  = (id) => document.getElementById(id);
let TOKEN = localStorage.getItem('kkbox-rx-token') || '';
let ws = null;

/* ---------- 通用 API ---------- */
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
  let data = null;
  try { data = JSON.parse(txt); } catch (e) { data = txt; }
  if (!r.ok) throw new Error((data && data.err) || ('HTTP ' + r.status));
  return data;
}

/* ---------- Toast ---------- */
let toastTimer = null;
function toast(msg, kind = 'ok') {
  const t = $('toast');
  t.textContent = msg;
  t.className = 'toast ' + kind;
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => t.classList.add('hidden'), 2600);
}

/* ---------- 主題切換 (明亮 / 暗黑 / 玻璃) ---------- */
const THEME_KEY = 'kkbox-rx-theme';
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
  e.preventDefault();
  $('lg-err').textContent = '';
  try {
    const d = await fetch('/api/login', {
      method: 'POST',
      headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
      body: new URLSearchParams({ user: $('lg-user').value, pass: $('lg-pass').value }).toString()
    });
    const j = await d.json();
    if (!d.ok) { $('lg-err').textContent = j.err || '登入失敗'; return; }
    TOKEN = j.token || '';
    localStorage.setItem('kkbox-rx-token', TOKEN);
    start();
  } catch (err) {
    $('lg-err').textContent = '無法連線';
  }
});

$('logout').addEventListener('click', async () => {
  try { await api('/api/logout', { method: 'POST' }); } catch (e) {}
  localStorage.removeItem('kkbox-rx-token');
  TOKEN = '';
  showLogin();
});

/* ---------- 導覽 ---------- */
$('nav').addEventListener('click', (e) => {
  const a = e.target.closest('a[data-page]');
  if (!a) return;
  document.querySelectorAll('#nav a').forEach(x => x.classList.toggle('active', x === a));
  document.querySelectorAll('.page').forEach(p => p.classList.toggle('active', p.id === 'page-' + a.dataset.page));
  if (a.dataset.page === 'network') loadNet();
  if (a.dataset.page === 'pid') loadPid();
  if (a.dataset.page === 'user') loadUser();
});

/* ---------- 狀態 ---------- */
function fmtUptime(s) {
  const d = Math.floor(s / 86400), h = Math.floor(s % 86400 / 3600),
        m = Math.floor(s % 3600 / 60), ss = s % 60;
  return (d ? d + 'd ' : '') + h + 'h ' + m + 'm ' + ss + 's';
}

async function loadStatus() {
  let s;
  try { s = await api('/api/status'); } catch (e) { return; }
  $('s-fw').textContent = s.fw + ' ' + s.ver;
  $('s-uptime').textContent = fmtUptime(s.uptime);
  $('s-heap').textContent = (s.heap / 1024).toFixed(1) + ' KB';
  const alive = s.kk.age < 1500;
  $('s-kk').textContent = alive ? '正常' : '無訊號';
  $('s-kk').style.color = alive ? 'var(--ok)' : 'var(--muted)';

  $('s-eth-link').textContent = s.eth.link ? '已連線' : '未連線';
  $('s-eth-link').style.color = s.eth.link ? 'var(--ok)' : 'var(--muted)';
  $('s-eth-ip').textContent = s.eth.ip;
  $('s-eth-spd').textContent = s.eth.speed ? s.eth.speed + ' Mbps' : '-';

  $('s-wifi-c').textContent = s.wifi.connected ? '已連線' : '未連線';
  $('s-wifi-c').style.color = s.wifi.connected ? 'var(--ok)' : 'var(--muted)';
  $('s-wifi-ssid').textContent = s.wifi.ssid || '-';
  $('s-wifi-ip').textContent = s.wifi.ip;
  $('s-wifi-rssi').textContent = s.wifi.connected ? s.wifi.rssi + ' dBm' : '-';

  const dot = $('net-dot');
  dot.classList.toggle('on', s.eth.link || s.wifi.connected);

  if (s.ap && s.ap.active) {
    $('wifi-scan-state').textContent = 'SoftAP: ' + s.ap.ssid + ' (' + s.ap.ip + ')';
  }
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
}

async function loadFs() {
  let f;
  try { f = await api('/api/fs'); } catch (e) { return; }
  const pct = f.total ? (f.used / f.total * 100) : 0;
  $('fs-usage').textContent = (f.used / 1024).toFixed(1) + ' / ' + (f.total / 1024).toFixed(1) + ' KB (' + pct.toFixed(0) + '%)';
  $('fs-bar').style.width = pct.toFixed(1) + '%';
  const tb = $('fs-files').querySelector('tbody');
  tb.innerHTML = '';
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
    try {
      const m = JSON.parse(ev.data);
      if (m.gyro) applyTelemetry(m);
    } catch (e) {}
  };
  ws.onclose = () => { ws = null; setTimeout(() => { if (!$('app').classList.contains('hidden')) connectWs(); }, 3000); };
}

/* ---------- 網路設定 ---------- */
let scanned = false;
$('wifi-scan').addEventListener('click', async () => {
  $('wifi-scan-state').textContent = '掃描中...';
  try {
    const list = await api('/api/scan');
    const sel = $('wifi-list');
    sel.innerHTML = '';
    list.sort((a, b) => b.rssi - a.rssi).forEach(n => {
      const o = document.createElement('option');
      o.value = n.ssid;
      o.textContent = n.ssid + '  (' + n.rssi + ' dBm' + (n.enc ? ', 加密' : '') + ')';
      sel.appendChild(o);
    });
    $('wifi-scan-state').textContent = list.length + ' 個網路';
    scanned = true;
  } catch (e) { $('wifi-scan-state').textContent = '掃描失敗'; }
});
$('wifi-list').addEventListener('change', () => { $('wifi-ssid').value = $('wifi-list').value; });

$('wifi-save').addEventListener('click', async () => {
  const ssid = $('wifi-ssid').value || $('wifi-list').value;
  if (!ssid) { toast('請輸入或選擇 SSID', 'err'); return; }
  try {
    await api('/api/wifi', { method: 'POST', body: { ssid, pass: $('wifi-pass').value } });
    toast('已儲存, 正在連線...');
    setTimeout(loadStatus, 4000);
  } catch (e) { toast('失敗: ' + e.message, 'err'); }
});
$('wifi-clear').addEventListener('click', async () => {
  try { await api('/api/wifi', { method: 'POST', body: { action: 'clear' } }); toast('已清除 WiFi 設定'); }
  catch (e) { toast('失敗', 'err'); }
});

async function loadNet() {
  let n;
  try { n = await api('/api/net'); } catch (e) { return; }
  document.querySelector('input[name=dhcp][value="' + (n.dhcp ? 1 : 0) + '"]').checked = true;
  $('net-ip').value = n.ip;
  $('net-gw').value = n.gw;
  $('net-mask').value = n.mask;
  toggleStatic();
}
function toggleStatic() {
  const dhcp = document.querySelector('input[name=dhcp]:checked').value === '1';
  $('static-fields').style.display = dhcp ? 'none' : 'block';
}
document.querySelectorAll('input[name=dhcp]').forEach(r => r.addEventListener('change', toggleStatic));
$('net-save').addEventListener('click', async () => {
  const dhcp = document.querySelector('input[name=dhcp]:checked').value;
  try {
    await api('/api/net', { method: 'POST', body: { dhcp, ip: $('net-ip').value, gw: $('net-gw').value, mask: $('net-mask').value } });
    toast('已儲存，裝置將重新啟動');
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

/* ---------- 使用者 ---------- */
async function loadUser() {
  let u;
  try { u = await api('/api/user'); } catch (e) { return; }
  $('u-user').value = u.user;
  $('u-token').textContent = u.token;
}
$('u-copy').addEventListener('click', () => {
  navigator.clipboard.writeText($('u-token').textContent).then(() => toast('已複製 Token'));
});
$('u-save').addEventListener('click', async () => {
  try {
    await api('/api/user', { method: 'POST', body: { user: $('u-user').value, pass: $('u-pass').value, cur: $('u-cur').value } });
    toast('已儲存');
    $('u-pass').value = ''; $('u-cur').value = '';
  } catch (e) { toast('儲存失敗: ' + e.message, 'err'); }
});

/* ---------- 啟動 ---------- */
function start() {
  $('login').classList.add('hidden');
  $('app').classList.remove('hidden');
  loadStatus();
  loadFs();
  connectWs();
  setInterval(loadStatus, 3000);
  setInterval(loadFs, 15000);
}

(async function init() {
  try {
    const ok = await fetch('/api/status', { credentials: 'same-origin', headers: TOKEN ? { 'X-Auth': TOKEN } : {} });
    if (ok.ok) { start(); } else { showLogin(); }
  } catch (e) { showLogin(); }
})();
