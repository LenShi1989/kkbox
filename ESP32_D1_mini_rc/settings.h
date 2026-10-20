#ifndef RC_SETTINGS_H
#define RC_SETTINGS_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_mac.h>
#include "config.h"

/* SoftAP SSID = 前綴 + MAC 後 4 碼 (例: KKBOX-RC-A1B2) */
static inline String apSsid() {
  uint8_t mac[6] = {0};
  /* 直接讀 eFuse 基礎 MAC, 避免 WiFi 驅動尚未啟動時 macAddress() 回傳 0 */
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[40];
  snprintf(buf, sizeof(buf), "%s%02X%02X", AP_SSID_PREFIX, mac[4], mac[5]);
  return String(buf);
}

/* BLE 裝置名稱 = 前綴 + MAC 後 4 碼 (例: KKBOX-RC-A1B2) */
static inline String btDevName() {
  uint8_t mac[6] = {0};
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char buf[40];
  snprintf(buf, sizeof(buf), "%s%02X%02X", BT_NAME_PREFIX, mac[4], mac[5]);
  return String(buf);
}

struct Settings {
  /* 登入帳密 */
  String user;
  String pass;

  /* WiFi 站台 */
  String ssid;
  String wpass;

  /* 接收機 (WT32-ETH01) 連線 */
  String rxHost;
  uint16_t rxPort;
  String rxToken;

  /* 藍牙 (BLE) 連線 */
  uint8_t channel;      /* 0 = WiFi, 1 = 藍牙 */
  String  btAddr;       /* 已配對接收機位址 (自動重連用) */
  String  btName;       /* 已配對接收機名稱 (顯示用) */

  /* PID (本機保存; 下傳飛控板需經目前通道連線) */
  float rollKp, rollKi, rollKd;
  float pitchKp, pitchKi, pitchKd;
  float yawKp, yawKi, yawKd;
};

static Preferences gPrefs;
static Settings    gSet;

static String randomHex(size_t n) {
  static const char* hex = "0123456789abcdef";
  String s;
  for (size_t i = 0; i < n; i++) s += hex[random(16)];
  return s;
}

static inline void settingsLoad() {
  gPrefs.begin("kkbox-rc", false);
  gSet.user    = gPrefs.getString("user", DEF_USER);
  gSet.pass    = gPrefs.getString("pass", DEF_PASS);
  gSet.ssid    = gPrefs.getString("ssid", "");
  gSet.wpass   = gPrefs.getString("wpass", "");
  gSet.rxHost  = gPrefs.getString("rxhost", "");
  gSet.rxPort  = gPrefs.getUShort("rxport", DEF_RX_PORT);
  gSet.rxToken = gPrefs.getString("rxtoken", "");
  gSet.channel = gPrefs.getUChar("btch", DEF_BT_CHANNEL);
  gSet.btAddr  = gPrefs.getString("btaddr", "");
  gSet.btName  = gPrefs.getString("btname", "");
  gSet.rollKp  = gPrefs.getFloat("rkp", DEF_ROLL_KP);
  gSet.rollKi  = gPrefs.getFloat("rki", DEF_ROLL_KI);
  gSet.rollKd  = gPrefs.getFloat("rkd", DEF_ROLL_KD);
  gSet.pitchKp = gPrefs.getFloat("pkp", DEF_PITCH_KP);
  gSet.pitchKi = gPrefs.getFloat("pki", DEF_PITCH_KI);
  gSet.pitchKd = gPrefs.getFloat("pkd", DEF_PITCH_KD);
  gSet.yawKp   = gPrefs.getFloat("ykp", DEF_YAW_KP);
  gSet.yawKi   = gPrefs.getFloat("yki", DEF_YAW_KI);
  gSet.yawKd   = gPrefs.getFloat("ykd", DEF_YAW_KD);
}

static inline void settingsSaveWifi() {
  gPrefs.putString("ssid", gSet.ssid);
  gPrefs.putString("wpass", gSet.wpass);
}
static inline void settingsSaveLink() {
  gPrefs.putString("rxhost", gSet.rxHost);
  gPrefs.putUShort("rxport", gSet.rxPort);
  gPrefs.putString("rxtoken", gSet.rxToken);
}
static inline void settingsSaveUser() {
  gPrefs.putString("user", gSet.user);
  gPrefs.putString("pass", gSet.pass);
}
static inline void settingsSaveBt() {
  gPrefs.putUChar("btch", gSet.channel);
  gPrefs.putString("btaddr", gSet.btAddr);
  gPrefs.putString("btname", gSet.btName);
}
static inline void settingsSavePid() {
  gPrefs.putFloat("rkp", gSet.rollKp);  gPrefs.putFloat("rki", gSet.rollKi);  gPrefs.putFloat("rkd", gSet.rollKd);
  gPrefs.putFloat("pkp", gSet.pitchKp); gPrefs.putFloat("pki", gSet.pitchKi); gPrefs.putFloat("pkd", gSet.pitchKd);
  gPrefs.putFloat("ykp", gSet.yawKp);   gPrefs.putFloat("yki", gSet.yawKi);   gPrefs.putFloat("ykd", gSet.yawKd);
}

#endif
