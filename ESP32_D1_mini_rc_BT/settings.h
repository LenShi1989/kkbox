#ifndef RC_BT_SETTINGS_H
#define RC_BT_SETTINGS_H

/* =============================================================
 * NVS 設定儲存 (藍牙 Server 名稱/位址、ID、搖桿校正值)
 * ============================================================= */

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

struct Settings {
  /* 藍牙 Server: 名稱優先, 若 serverMac 非空則以位址直連 */
  String serverName;
  String serverMac;
  /* 編號 */
  uint8_t rcId;
  uint8_t flyId;
  /* 搖桿校正 (ADC 原始值 min/max), 0 值代表未校正 */
  int tmin, tmax, ymin, ymax, pmin, pmax, rmin, rmax;
};
static Settings gSet;

static Preferences gPrefs;

static void settingsDefaults() {
  gSet.serverName = DEF_BT_SERVER_NAME;
  gSet.serverMac  = "";
  gSet.rcId       = DEF_RC_ID;
  gSet.flyId      = DEF_FLY_ID;
  /* 未校正時先用滿刻度 (0~4095), 待首次校正覆寫 */
  gSet.tmin = 0; gSet.tmax = 4095;
  gSet.ymin = 0; gSet.ymax = 4095;
  gSet.pmin = 0; gSet.pmax = 4095;
  gSet.rmin = 0; gSet.rmax = 4095;
}

static void settingsLoad() {
  settingsDefaults();
  gPrefs.begin("rcbt", false);
  gSet.serverName = gPrefs.getString("srvn", DEF_BT_SERVER_NAME).c_str();
  gSet.serverMac  = gPrefs.getString("srvm", "").c_str();
  gSet.rcId       = gPrefs.getUChar("rcid", DEF_RC_ID);
  gSet.flyId      = gPrefs.getUChar("fyid", DEF_FLY_ID);
  gSet.tmin = (int)gPrefs.getInt("tmin", 0);    gSet.tmax = (int)gPrefs.getInt("tmax", 4095);
  gSet.ymin = (int)gPrefs.getInt("ymin", 0);    gSet.ymax = (int)gPrefs.getInt("ymax", 4095);
  gSet.pmin = (int)gPrefs.getInt("pmin", 0);    gSet.pmax = (int)gPrefs.getInt("pmax", 4095);
  gSet.rmin = (int)gPrefs.getInt("rmin", 0);    gSet.rmax = (int)gPrefs.getInt("rmax", 4095);
}

static void settingsSaveBt() {
  gPrefs.putString("srvn", gSet.serverName);
  gPrefs.putString("srvm", gSet.serverMac);
}
static void settingsSaveIds() {
  gPrefs.putUChar("rcid", gSet.rcId);
  gPrefs.putUChar("fyid", gSet.flyId);
}
static void settingsSaveCal() {
  gPrefs.putInt("tmin", gSet.tmin); gPrefs.putInt("tmax", gSet.tmax);
  gPrefs.putInt("ymin", gSet.ymin); gPrefs.putInt("ymax", gSet.ymax);
  gPrefs.putInt("pmin", gSet.pmin); gPrefs.putInt("pmax", gSet.pmax);
  gPrefs.putInt("rmin", gSet.rmin); gPrefs.putInt("rmax", gSet.rmax);
}

#endif
