#ifndef RC_SETTINGS_H
#define RC_SETTINGS_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

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

#endif
