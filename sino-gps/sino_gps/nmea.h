// Leitor mínimo de sentenças NMEA "RMC" do GPS (sem bibliotecas externas).
// Exemplo: $GPRMC,083559.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A*57
//                 hora UTC  fix                                       data
#pragma once
#include <stdint.h>
#include <string.h>
#include "tempo.h"

struct LeituraRMC {
  bool comFix;      // status 'A' = posição/hora confirmadas pelos satélites
  DataHora utc;     // diaSemana não é preenchido aqui
  uint16_t fracMs;  // fração de segundo informada (normalmente 0)
};

class LeitorNMEA {
 public:
  uint32_t sentencasOk = 0;
  uint32_t errosChecksum = 0;

  // Entregue cada caractere recebido do GPS junto com millis() do momento.
  // Retorna true quando acabou de chegar uma RMC íntegra (checksum ok);
  // nesse caso 'rmc' é preenchida e 'inicioMs' recebe o millis() do '$'.
  bool alimentar(char c, uint32_t agoraMs, LeituraRMC &rmc, uint32_t &inicioMs) {
    if (c == '$') {
      ativo_ = true;
      len_ = 0;
      inicio_ = agoraMs;
      return false;
    }
    if (!ativo_) return false;
    if (c == '\r' || c == '\n') {
      ativo_ = false;
      buf_[len_] = 0;
      if (analisar(rmc)) {
        inicioMs = inicio_;
        return true;
      }
      return false;
    }
    if (len_ >= sizeof(buf_) - 1) {  // linha longa demais: descarta
      ativo_ = false;
      return false;
    }
    buf_[len_++] = c;
    return false;
  }

 private:
  char buf_[100];
  uint8_t len_ = 0;
  bool ativo_ = false;
  uint32_t inicio_ = 0;

  static int hex(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;
  }
  static bool digitos(const char *s, uint8_t n) {
    for (uint8_t i = 0; i < n; i++)
      if (s[i] < '0' || s[i] > '9') return false;
    return true;
  }
  static uint8_t dois(const char *s) { return (s[0] - '0') * 10 + (s[1] - '0'); }

  bool analisar(LeituraRMC &rmc) {
    char *ast = strchr(buf_, '*');
    if (!ast || hex(ast[1]) < 0 || hex(ast[2]) < 0) return false;
    uint8_t soma = 0;
    for (char *p = buf_; p < ast; ++p) soma ^= (uint8_t)*p;
    if (soma != (uint8_t)((hex(ast[1]) << 4) | hex(ast[2]))) {
      errosChecksum++;
      return false;
    }
    sentencasOk++;
    *ast = 0;

    // "GPRMC", "GNRMC", "GLRMC"...: 2 letras do sistema + "RMC"
    if (strncmp(buf_ + 2, "RMC,", 4) != 0) return false;

    const char *campo[13];
    uint8_t n = 0;
    campo[n++] = buf_;
    for (char *p = buf_; *p && n < 13; ++p) {
      if (*p == ',') {
        *p = 0;
        campo[n++] = p + 1;
      }
    }
    if (n < 10) return false;

    const char *h = campo[1];  // hhmmss.ss
    const char *d = campo[9];  // ddmmaa
    if (strlen(h) < 6 || !digitos(h, 6) || strlen(d) != 6 || !digitos(d, 6)) return false;

    rmc.utc.hora = dois(h);
    rmc.utc.min = dois(h + 2);
    rmc.utc.seg = dois(h + 4);
    rmc.utc.dia = dois(d);
    rmc.utc.mes = dois(d + 2);
    rmc.utc.ano = 2000 + dois(d + 4);
    rmc.utc.diaSemana = 0;

    rmc.fracMs = 0;
    if (h[6] == '.') {
      uint16_t escala = 100;
      for (const char *p = h + 7; *p >= '0' && *p <= '9' && escala > 0; ++p, escala /= 10)
        rmc.fracMs += (*p - '0') * escala;
    }
    rmc.comFix = (campo[2][0] == 'A');
    return dataHoraValida(rmc.utc);
  }
};
