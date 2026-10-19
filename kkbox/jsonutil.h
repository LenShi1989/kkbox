#ifndef KKBOX_JSONUTIL_H
#define KKBOX_JSONUTIL_H

#include <Arduino.h>
#include <stdlib.h>
#include <string.h>

/* 輕量 JSON 解析工具 (專為 ATmega168 的 1KB SRAM 設計, 不使用 ArduinoJson,
 * 不做動態記憶體配置)。僅支援本專案用的平面鍵值解析。
 * 輸出端 (遙測) 直接以 Serial.print(F("...")) 串流, 避免佔用 SRAM。
 */

/* 回傳 "key" 後、冒號後的值的起點; 找不到回傳 0 */
static const char* jFind(const char* json, const char* key) {
  char pat[24];
  snprintf(pat, sizeof pat, "\"%s\"", key);
  const char* p = strstr(json, pat);
  if (!p) return 0;
  p += strlen(pat);
  while (*p == ' ' || *p == '\t') p++;
  if (*p != ':') return 0;
  p++;
  while (*p == ' ' || *p == '\t') p++;
  return p;
}

static bool jHas(const char* json, const char* key) {
  return jFind(json, key) != 0;
}

static long jInt(const char* json, const char* key, long def) {
  const char* p = jFind(json, key);
  return p ? strtol(p, 0, 10) : def;
}

static float jFloat(const char* json, const char* key, float def) {
  const char* p = jFind(json, key);
  return p ? (float)strtod(p, 0) : def;
}

static bool jStr(const char* json, const char* key, char* out, size_t n) {
  const char* p = jFind(json, key);
  if (!p || *p != '"') return false;
  p++;
  size_t i = 0;
  while (*p && *p != '"' && i < n - 1) out[i++] = *p++;
  out[i] = 0;
  return true;
}

#endif
