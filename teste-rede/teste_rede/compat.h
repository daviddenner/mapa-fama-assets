// Diferenças entre os chips num lugar só: ESP8266 (ESP-12, NodeMCU), ESP32 e
// ESP32-C3. O resto do programa chama estas funções e não precisa saber em
// qual chip está rodando.
#pragma once
#include <Arduino.h>

#include "analise.h"

#if defined(ESP8266)
// ---------------------------------------------------------------- ESP8266
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <lwip/raw.h>  // ICMP (pacotes crus)
#include <sntp.h>      // servidor de hora informado pelo DHCP

typedef ESP8266WebServer ServidorWeb;
#define NOME_CHIP "ESP8266"
inline int64_t agoraUs() { return (int64_t)micros64(); }

// O ESP8266 não tem a biblioteca Preferences: guarda cada valor num arquivinho.
class Preferences {
 public:
  bool begin(const char *, bool = false) { return true; }
  String getString(const char *k, const String &padrao = String()) {
    File f = LittleFS.open(caminho(k), "r");
    if (!f) return padrao;
    const String s = f.readString();
    f.close();
    return s;
  }
  size_t putString(const char *k, const char *v) {
    File f = LittleFS.open(caminho(k), "w");
    if (!f) return 0;
    const size_t n = f.print(v);
    f.close();
    return n;
  }
  bool remove(const char *k) { return LittleFS.remove(caminho(k)); }
  uint32_t getUInt(const char *k, uint32_t padrao = 0) {
    const String s = getString(k);
    return s.length() ? (uint32_t)strtoul(s.c_str(), NULL, 10) : padrao;
  }
  size_t putUInt(const char *k, uint32_t v) {
    char b[12];
    snprintf(b, sizeof b, "%lu", (unsigned long)v);
    return putString(k, b);
  }
  uint8_t getUChar(const char *k, uint8_t padrao = 0) { return (uint8_t)getUInt(k, padrao); }
  size_t putUChar(const char *k, uint8_t v) { return putUInt(k, v); }

 private:
  static String caminho(const char *k) { return String("/cfg_") + k; }
};

inline bool montarArquivos() { return LittleFS.begin() || (LittleFS.format() && LittleFS.begin()); }
inline void wifiNomeNaRede(const char *n) { WiFi.hostname(n); }
inline void wifiSemEconomia() { WiFi.setSleepMode(WIFI_NONE_SLEEP); }
inline void wifiPotencia(uint8_t meiosDbm) { WiFi.setOutputPower(meiosDbm ? meiosDbm / 2.0f : 20.5f); }
inline const char *segurancaDaRede(int i) { return nomeSeguranca8266(WiFi.encryptionType(i)); }

inline void pedirNtpAoDhcp() { sntp_servermode_dhcp(1); }
inline IPAddress ntpInformadoPeloDhcp() {
  const ip_addr_t *a = sntp_getserver(0);
  return a ? IPAddress(*a) : IPAddress();
}

inline bool conectarTcp(WiFiClient &c, const IPAddress &ip, uint16_t porta, uint32_t ms) {
  c.setTimeout(ms);
  return c.connect(ip, porta);
}
inline bool conectarTcp(WiFiClient &c, const char *host, uint16_t porta, uint32_t ms) {
  c.setTimeout(ms);
  return c.connect(host, porta);
}

// HTTPS só para ler o cabeçalho. O ESP8266 precisa de ~22 KB livres para o
// HTTPS normal; com menos, tenta blocos pequenos (se o servidor aceitar).
inline bool conectarTls(WiFiClientSecure &s, const char *host, uint16_t porta, const char *&erro) {
  s.setInsecure();
  s.setTimeout(8000);
  if (ESP.getFreeHeap() < 30000 || ESP.getMaxFreeBlockSize() < 18000) {
    if (!WiFiClientSecure::probeMaxFragmentLength(host, porta, 1024)) {
      erro = "pouca memoria livre para HTTPS neste chip (pulei)";
      return false;
    }
    s.setBufferSizes(1024, 1024);
  }
  if (!s.connect(host, porta)) {
    erro = "nao conectou (porta 443 bloqueada ou falha no TLS)";
    return false;
  }
  return true;
}

inline bool escutarMulticast(WiFiUDP &u, const IPAddress &grupo, uint16_t porta) {
  return u.beginMulticast(WiFi.localIP(), grupo, porta) != 0;
}

#else
// ------------------------------------------------------- ESP32 e ESP32-C3
#include <LittleFS.h>
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>
#include <lwip/sockets.h>  // ICMP (socket cru)

#include "esp_sntp.h"
#include "esp_timer.h"

typedef WebServer ServidorWeb;
#define NOME_CHIP ESP.getChipModel()
inline int64_t agoraUs() { return esp_timer_get_time(); }

inline bool montarArquivos() { return LittleFS.begin(true); }
inline void wifiNomeNaRede(const char *n) { WiFi.setHostname(n); }
inline void wifiSemEconomia() { WiFi.setSleep(false); }
inline void wifiPotencia(uint8_t meiosDbm) {
  wifi_power_t p = WIFI_POWER_19_5dBm;
  if (meiosDbm == 17) p = WIFI_POWER_8_5dBm;
  if (meiosDbm == 22) p = WIFI_POWER_11dBm;
  if (meiosDbm == 30) p = WIFI_POWER_15dBm;
  WiFi.setTxPower(p);
}
inline const char *segurancaDaRede(int i) { return nomeSeguranca((int)WiFi.encryptionType(i)); }

inline void pedirNtpAoDhcp() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_sntp_servermode_dhcp(true);
#else
  sntp_servermode_dhcp(1);
#endif
}
inline IPAddress ntpInformadoPeloDhcp() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  const ip_addr_t *a = esp_sntp_getserver(0);
#else
  const ip_addr_t *a = sntp_getserver(0);
#endif
  if (a == NULL || !IP_IS_V4(a)) return IPAddress((uint32_t)0);
  return IPAddress((uint32_t)ip4_addr_get_u32(ip_2_ip4(a)));
}

inline bool conectarTcp(WiFiClient &c, const IPAddress &ip, uint16_t porta, uint32_t ms) {
  return c.connect(ip, porta, ms);
}
inline bool conectarTcp(WiFiClient &c, const char *host, uint16_t porta, uint32_t ms) {
  return c.connect(host, porta, ms);
}

inline bool conectarTls(WiFiClientSecure &s, const char *host, uint16_t porta, const char *&erro) {
  s.setInsecure();  // só lemos o cabeçalho; não enviamos dado nenhum
  s.setHandshakeTimeout(8);
  if (!s.connect(host, porta, 6000)) {
    erro = "nao conectou (porta 443 bloqueada ou falha no TLS)";
    return false;
  }
  return true;
}

inline bool escutarMulticast(WiFiUDP &u, const IPAddress &grupo, uint16_t porta) {
  return u.beginMulticast(grupo, porta) != 0;
}
#endif
