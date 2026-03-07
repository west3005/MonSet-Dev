#include "runtime_config.hpp"
#include "debug_uart.hpp"

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {
#include "ff.h"
}

static RuntimeConfig g_cfg;
RuntimeConfig& Cfg() { return g_cfg; }

// -----------------------------------------------------------------------------
// Вспомогательные функции
// -----------------------------------------------------------------------------
static void copyStr(char* dst, size_t dstSz, const char* src) {
  if (!dst || dstSz == 0) return;
  if (!src) { dst[0] = 0; return; }
  std::strncpy(dst, src, dstSz - 1);
  dst[dstSz - 1] = 0;
}

// -----------------------------------------------------------------------------
// Заполнение дефолтами из config.hpp
// -----------------------------------------------------------------------------
void RuntimeConfig::setDefaultsFromConfig() {
  complex_enabled = false;

  copyStr(metric_id,  sizeof(metric_id),  Config::METRIC_ID);
  copyStr(complex_id, sizeof(complex_id), Config::COMPLEX_ID);

  copyStr(server_url,      sizeof(server_url),      Config::SERVER_URL);
  copyStr(server_auth_b64, sizeof(server_auth_b64), Config::SERVER_AUTH);

  eth_mode = (Config::NET_MODE == Config::NetMode::DHCP)
             ? EthMode::Dhcp
             : EthMode::Static;

  std::memcpy(w5500_mac, Config::W5500_MAC, 6);
  std::memcpy(eth_ip,    Config::NET_IP,    4);
  std::memcpy(eth_sn,    Config::NET_SN,    4);
  std::memcpy(eth_gw,    Config::NET_GW,    4);
  std::memcpy(eth_dns,   Config::NET_DNS,   4);

  copyStr(gsm_apn,  sizeof(gsm_apn),  Config::GSM_APN);
  copyStr(gsm_user, sizeof(gsm_user), Config::GSM_APN_USER);
  copyStr(gsm_pass, sizeof(gsm_pass), Config::GSM_APN_PASS);

  poll_interval_sec   = Config::POLL_INTERVAL_SEC;
  send_interval_polls = Config::SEND_INTERVAL_POLLS;

  modbus_slave     = Config::MODBUS_SLAVE;
  modbus_func      = Config::MODBUS_FUNC_CODE;
  modbus_start_reg = Config::MODBUS_START_REG;
  modbus_num_regs  = Config::MODBUS_NUM_REGS;

  sensor_zero_level = Config::SENSOR_ZERO_LEVEL;
  sensor_divider    = Config::SENSOR_DIVIDER;

  ntp_enabled   = true;
  copyStr(ntp_host, sizeof(ntp_host), "pool.ntp.org");
  ntp_resync_sec = 86400;
}

// -----------------------------------------------------------------------------
// Валидация
// -----------------------------------------------------------------------------
bool RuntimeConfig::validateAndFix() {
  bool ok = true;

  if (metric_id[0] == 0)  ok = false;
  if (server_url[0] == 0) ok = false;

  if (poll_interval_sec == 0)      poll_interval_sec = 1;
  if (send_interval_polls == 0)    send_interval_polls = 1;

  if (modbus_slave == 0)           modbus_slave = 1;
  if (modbus_func != 3 && modbus_func != 4) modbus_func = 4;
  if (modbus_num_regs == 0)        modbus_num_regs = 2;
  if (modbus_num_regs > 30)        modbus_num_regs = 30;

  if (sensor_divider == 0.0f)      sensor_divider = 1000.0f;

  if (ntp_resync_sec < 3600)       ntp_resync_sec = 3600;
  if (ntp_host[0] == 0)            copyStr(ntp_host, sizeof(ntp_host), "pool.ntp.org");

  return ok;
}

// -----------------------------------------------------------------------------
// Минималистичный JSON-парсер
// -----------------------------------------------------------------------------
static const char* skipWs(const char* p) {
  while (p && *p && std::isspace((unsigned char)*p)) p++;
  return p;
}

static const char* findKey(const char* json, const char* key) {
  if (!json || !key) return nullptr;

  char pat[80];
  std::snprintf(pat, sizeof(pat), "\"%s\"", key);

  const char* p = std::strstr(json, pat);
  if (!p) return nullptr;

  p += std::strlen(pat);
  p = skipWs(p);
  if (*p != ':') return nullptr;
  p++;
  return skipWs(p);
}

static bool jsonGetString(const char* json, const char* key, char* out, size_t outSz) {
  const char* p = findKey(json, key);
  if (!p) return false;
  if (*p != '"') return false;

  p++;
  size_t i = 0;
  while (*p && *p != '"' && (i + 1) < outSz) out[i++] = *p++;
  out[i] = 0;
  return (*p == '"');
}

static bool jsonGetU32(const char* json, const char* key, uint32_t& out) {
  const char* p = findKey(json, key);
  if (!p) return false;
  out = (uint32_t)std::strtoul(p, nullptr, 10);
  return true;
}

static bool jsonGetU16(const char* json, const char* key, uint16_t& out) {
  const char* p = findKey(json, key);
  if (!p) return false;
  out = (uint16_t)std::strtoul(p, nullptr, 10);
  return true;
}

static bool jsonGetU8(const char* json, const char* key, uint8_t& out) {
  const char* p = findKey(json, key);
  if (!p) return false;
  out = (uint8_t)std::strtoul(p, nullptr, 10);
  return true;
}

static bool jsonGetBool(const char* json, const char* key, bool& out) {
  const char* p = findKey(json, key);
  if (!p) return false;
  if (std::strncmp(p, "true", 4) == 0)  { out = true;  return true; }
  if (std::strncmp(p, "false", 5) == 0) { out = false; return true; }
  return false;
}

static bool jsonGetF32(const char* json, const char* key, float& out) {
  const char* p = findKey(json, key);
  if (!p) return false;
  out = std::strtof(p, nullptr);
  return true;
}

static bool parseIpv4Str(const char* s, uint8_t out[4]) {
  if (!s) return false;
  unsigned a=0,b=0,c=0,d=0;
  if (std::sscanf(s, "%u.%u.%u.%u", &a,&b,&c,&d) != 4) return false;
  if (a>255||b>255||c>255||d>255) return false;
  out[0]=(uint8_t)a; out[1]=(uint8_t)b; out[2]=(uint8_t)c; out[3]=(uint8_t)d;
  return true;
}

static bool jsonGetIpv4(const char* json, const char* key, uint8_t out[4]) {
  char tmp[32];
  if (!jsonGetString(json, key, tmp, sizeof(tmp))) return false;
  return parseIpv4Str(tmp, out);
}

static bool parseMacStr(const char* s, uint8_t out[6]) {
  if (!s) return false;
  unsigned m[6]{};
  if (std::sscanf(s, "%x:%x:%x:%x:%x:%x", &m[0],&m[1],&m[2],&m[3],&m[4],&m[5]) != 6) return false;
  for (int i=0;i<6;i++) if (m[i] > 0xFF) return false;
  for (int i=0;i<6;i++) out[i] = (uint8_t)m[i];
  return true;
}

static bool jsonGetMac(const char* json, const char* key, uint8_t out[6]) {
  char tmp[32];
  if (!jsonGetString(json, key, tmp, sizeof(tmp))) return false;
  return parseMacStr(tmp, out);
}

static void ftoa6(float x, char* out, size_t outSz) {
  if (!out || outSz < 4) return;

  bool neg = (x < 0.0f);
  if (neg) x = -x;

  uint32_t ip   = (uint32_t)x;
  float    frac = x - (float)ip;
  uint32_t fp   = (uint32_t)(frac * 1000000.0f + 0.5f);
  if (fp >= 1000000UL) { ip += 1; fp = 0; }

  if (neg) std::snprintf(out, outSz, "-%lu.%06lu", (unsigned long)ip, (unsigned long)fp);
  else     std::snprintf(out, outSz,  "%lu.%06lu", (unsigned long)ip, (unsigned long)fp);
}

// -----------------------------------------------------------------------------
// loadFromJson
// -----------------------------------------------------------------------------
bool RuntimeConfig::loadFromJson(const char* json, size_t len) {
  if (!json || len < 2) return false;

  RuntimeConfig tmp;
  tmp.setDefaultsFromConfig();

  (void)jsonGetBool(json,   "complex_enabled", tmp.complex_enabled);
  (void)jsonGetString(json, "metric_id",       tmp.metric_id,   sizeof(tmp.metric_id));
  (void)jsonGetString(json, "complex_id",      tmp.complex_id,  sizeof(tmp.complex_id));

  (void)jsonGetString(json, "server_url",      tmp.server_url,      sizeof(tmp.server_url));
  (void)jsonGetString(json, "server_auth_b64", tmp.server_auth_b64, sizeof(tmp.server_auth_b64));

  char ethMode[16]{};
  if (jsonGetString(json, "eth_mode", ethMode, sizeof(ethMode))) {
    if (std::strcmp(ethMode, "dhcp")   == 0) tmp.eth_mode = EthMode::Dhcp;
    if (std::strcmp(ethMode, "static") == 0) tmp.eth_mode = EthMode::Static;
  }

  (void)jsonGetMac (json, "w5500_mac", tmp.w5500_mac);
  (void)jsonGetIpv4(json, "eth_ip",    tmp.eth_ip);
  (void)jsonGetIpv4(json, "eth_sn",    tmp.eth_sn);
  (void)jsonGetIpv4(json, "eth_gw",    tmp.eth_gw);
  (void)jsonGetIpv4(json, "eth_dns",   tmp.eth_dns);

  (void)jsonGetString(json, "gsm_apn",  tmp.gsm_apn,  sizeof(tmp.gsm_apn));
  (void)jsonGetString(json, "gsm_user", tmp.gsm_user, sizeof(tmp.gsm_user));
  (void)jsonGetString(json, "gsm_pass", tmp.gsm_pass, sizeof(tmp.gsm_pass));

  (void)jsonGetU32(json, "poll_interval_sec",   tmp.poll_interval_sec);
  (void)jsonGetU32(json, "send_interval_polls", tmp.send_interval_polls);

  (void)jsonGetU8 (json, "modbus_slave",     tmp.modbus_slave);
  (void)jsonGetU8 (json, "modbus_func",      tmp.modbus_func);
  (void)jsonGetU16(json, "modbus_start_reg", tmp.modbus_start_reg);
  (void)jsonGetU16(json, "modbus_num_regs",  tmp.modbus_num_regs);

  (void)jsonGetF32(json, "sensor_zero_level", tmp.sensor_zero_level);
  (void)jsonGetF32(json, "sensor_divider",    tmp.sensor_divider);

  (void)jsonGetBool  (json, "ntp_enabled",    tmp.ntp_enabled);
  (void)jsonGetString(json, "ntp_host",       tmp.ntp_host, sizeof(tmp.ntp_host));
  (void)jsonGetU32   (json, "ntp_resync_sec", tmp.ntp_resync_sec);

  tmp.validateAndFix();
  *this = tmp;
  return true;
}

// -----------------------------------------------------------------------------
// loadFromSd
// -----------------------------------------------------------------------------
bool RuntimeConfig::loadFromSd(const char* filename) {
  // Сначала жёсткие дефолты из config.hpp
  setDefaultsFromConfig();

  FIL f;
  FRESULT fr = f_open(&f, filename, FA_READ);
  if (fr != FR_OK) {
    DBG.warn("CFG: %s not found -> defaults", filename);
    validateAndFix();
    return false;
  }

  static char buf[2048];
  UINT br = 0;
  fr = f_read(&f, buf, sizeof(buf) - 1, &br);
  f_close(&f);

  if (fr != FR_OK || br == 0) {
    DBG.error("CFG: read fail fr=%d br=%u", (int)fr, (unsigned)br);
    validateAndFix();
    return false;
  }

  buf[br] = 0;
  if (!loadFromJson(buf, (size_t)br)) {
    DBG.error("CFG: JSON parse fail -> defaults");
    setDefaultsFromConfig();
    validateAndFix();
    return false;
  }

  DBG.info("CFG: loaded from %s (%u bytes)", filename, (unsigned)br);
  return true;
}

// -----------------------------------------------------------------------------
// saveToSd
// -----------------------------------------------------------------------------
bool RuntimeConfig::saveToSd(const char* filename) const {
  char macStr[24];
  std::snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                w5500_mac[0], w5500_mac[1], w5500_mac[2],
                w5500_mac[3], w5500_mac[4], w5500_mac[5]);

  auto ipStr = [](const uint8_t ip[4], char* out, size_t outSz) {
    std::snprintf(out, outSz, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  };

  char ip[16], sn[16], gw[16], dns[16];
  ipStr(eth_ip,  ip,  sizeof(ip));
  ipStr(eth_sn,  sn,  sizeof(sn));
  ipStr(eth_gw,  gw,  sizeof(gw));
  ipStr(eth_dns, dns, sizeof(dns));

  const char* ethModeStr = (eth_mode == EthMode::Dhcp) ? "dhcp" : "static";

  char zStr[48], dStr[48];
  ftoa6(sensor_zero_level, zStr, sizeof(zStr));
  ftoa6(sensor_divider,    dStr, sizeof(dStr));

  char json[1400]; // достаточно для компактного JSON без pretty-print
  int n = std::snprintf(
    json, sizeof(json),
    "{\"v\":%lu,"
    "\"complex_enabled\":%s,"
    "\"metric_id\":\"%s\","
    "\"complex_id\":\"%s\","
    "\"server_url\":\"%s\","
    "\"server_auth_b64\":\"%s\","
    "\"eth_mode\":\"%s\","
    "\"w5500_mac\":\"%s\","
    "\"eth_ip\":\"%s\","
    "\"eth_sn\":\"%s\","
    "\"eth_gw\":\"%s\","
    "\"eth_dns\":\"%s\","
    "\"gsm_apn\":\"%s\","
    "\"gsm_user\":\"%s\","
    "\"gsm_pass\":\"%s\","
    "\"poll_interval_sec\":%lu,"
    "\"send_interval_polls\":%lu,"
    "\"modbus_slave\":%u,"
    "\"modbus_func\":%u,"
    "\"modbus_start_reg\":%u,"
    "\"modbus_num_regs\":%u,"
    "\"sensor_zero_level\":%s,"
    "\"sensor_divider\":%s,\"ntp_enabled\":%s,"
    "\"ntp_host\":\"%s\","
    "\"ntp_resync_sec\":%lu"
    "}\n",
    (unsigned long)VERSION,
    complex_enabled ? "true" : "false",
    metric_id,
    complex_id,
    server_url,
    server_auth_b64,
    ethModeStr,
    macStr,
    ip, sn, gw, dns,
    gsm_apn,
    gsm_user,
    gsm_pass,
    (unsigned long)poll_interval_sec,
    (unsigned long)send_interval_polls,
    (unsigned)modbus_slave,
    (unsigned)modbus_func,
    (unsigned)modbus_start_reg,
    (unsigned)modbus_num_regs,
    zStr,
    dStr,
    ntp_enabled ? "true" : "false",
    ntp_host,
    (unsigned long)ntp_resync_sec
  );

  if (n <= 0 || n >= (int)sizeof(json)) {
    DBG.error("CFG: save JSON overflow");
    return false;
  }

  FIL f;
  FRESULT fr = f_open(&f, filename, FA_CREATE_ALWAYS | FA_WRITE);
  if (fr != FR_OK) {
    DBG.error("CFG: open for write fail fr=%d", (int)fr);
    return false;
  }

  UINT bw = 0;
  fr = f_write(&f, json, (UINT)std::strlen(json), &bw);
  f_close(&f);

  if (fr != FR_OK || bw == 0) {
    DBG.error("CFG: write fail fr=%d bw=%u", (int)fr, (unsigned)bw);
    return false;
  }

  DBG.info("CFG: saved %s (%u bytes)", filename, (unsigned)bw);
  return true;
}

// -----------------------------------------------------------------------------
// log
// -----------------------------------------------------------------------------
void RuntimeConfig::log() const {
  DBG.info("CFG: metric_id=%s", metric_id);
  DBG.info("CFG: complex_enabled=%d complex_id=%s",
           complex_enabled ? 1 : 0, complex_id);
  DBG.info("CFG: server_url=%s", server_url);
  DBG.info("CFG: eth_mode=%s", (eth_mode == EthMode::Dhcp) ? "dhcp" : "static");
  DBG.info("CFG: eth_ip=%u.%u.%u.%u",
           eth_ip[0], eth_ip[1], eth_ip[2], eth_ip[3]);
  DBG.info("CFG: poll=%lu sec, send_each=%lu polls",
           (unsigned long)poll_interval_sec,
           (unsigned long)send_interval_polls);
  DBG.info("CFG: modbus slave=%u fc=%u reg=%u n=%u",
           (unsigned)modbus_slave, (unsigned)modbus_func,
           (unsigned)modbus_start_reg, (unsigned)modbus_num_regs);
  DBG.info("CFG: zero=%.3f divider=%.3f",
           (double)sensor_zero_level, (double)sensor_divider);
  DBG.info("CFG: ntp=%d host=%s period=%lu sec",
           ntp_enabled ? 1 : 0, ntp_host, (unsigned long)ntp_resync_sec);
}
