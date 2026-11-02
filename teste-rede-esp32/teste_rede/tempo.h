// Funções de data/hora puras (cópia de sino-gps/sino_gps/tempo.h).
#pragma once
#include <stdint.h>

struct DataHora {
  uint16_t ano;
  uint8_t mes, dia, hora, min, seg;
  uint8_t diaSemana;  // 0 = domingo ... 6 = sábado (preenchido por deEpoch)
};

// Dias desde 01/01/1970 (algoritmo "days_from_civil" de Howard Hinnant).
inline int32_t diasDesde1970(int32_t a, uint32_t m, uint32_t d) {
  a -= (m <= 2);
  const int32_t era = (a >= 0 ? a : a - 399) / 400;
  const uint32_t aDaEra = (uint32_t)(a - era * 400);
  const uint32_t diaDoAno = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  const uint32_t diaDaEra = aDaEra * 365 + aDaEra / 4 - aDaEra / 100 + diaDoAno;
  return era * 146097 + (int32_t)diaDaEra - 719468;
}

inline bool dataHoraValida(const DataHora &t) {
  static const uint8_t diasNoMes[] = {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  return t.ano >= 2000 && t.ano <= 2099 && t.mes >= 1 && t.mes <= 12 && t.dia >= 1 &&
         t.dia <= diasNoMes[t.mes - 1] && t.hora < 24 && t.min < 60 && t.seg < 60;
}

// Segundos desde 01/01/1970 00:00:00.
inline uint32_t paraEpoch(const DataHora &t) {
  return (uint32_t)diasDesde1970(t.ano, t.mes, t.dia) * 86400UL + t.hora * 3600UL +
         t.min * 60UL + t.seg;
}

inline DataHora deEpoch(uint32_t e) {
  DataHora t;
  int32_t z = (int32_t)(e / 86400UL);
  const uint32_t resto = e % 86400UL;
  t.hora = resto / 3600;
  t.min = (resto / 60) % 60;
  t.seg = resto % 60;
  t.diaSemana = (uint8_t)((z + 4) % 7);  // 01/01/1970 foi quinta-feira

  z += 719468;
  const int32_t era = (z >= 0 ? z : z - 146096) / 146097;
  const uint32_t diaDaEra = (uint32_t)(z - era * 146097);
  const uint32_t aDaEra =
      (diaDaEra - diaDaEra / 1460 + diaDaEra / 36524 - diaDaEra / 146096) / 365;
  const uint32_t diaDoAno = diaDaEra - (365 * aDaEra + aDaEra / 4 - aDaEra / 100);
  const uint32_t mp = (5 * diaDoAno + 2) / 153;
  t.dia = diaDoAno - (153 * mp + 2) / 5 + 1;
  t.mes = mp < 10 ? mp + 3 : mp - 9;
  t.ano = (uint16_t)((int32_t)aDaEra + era * 400 + (t.mes <= 2));
  return t;
}

// Data de compilação (__DATE__ = "Nov  1 2026") em epoch. Serve de "data mínima
// plausível": um relógio que diga uma data anterior a esta está errado.
inline uint32_t epochDaCompilacao(const char *data) {
  static const char meses[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  DataHora t = {2025, 1, 1, 0, 0, 0, 0};
  for (uint8_t i = 0; i < 12; i++) {
    if (data[0] == meses[i * 3] && data[1] == meses[i * 3 + 1] && data[2] == meses[i * 3 + 2]) {
      t.mes = i + 1;
    }
  }
  t.dia = (data[4] == ' ' ? 0 : (data[4] - '0') * 10) + (data[5] - '0');
  t.ano = (data[7] - '0') * 1000 + (data[8] - '0') * 100 + (data[9] - '0') * 10 + (data[10] - '0');
  if (!dataHoraValida(t)) return 1735689600UL;  // 01/01/2025
  return paraEpoch(t) - 86400UL;                 // 1 dia de folga por causa do fuso
}

// Alguns módulos GPS antigos ou clones informam a data 1024 semanas (~19,6 anos)
// no passado por causa do "rollover" do número da semana GPS. Como 1024 semanas
// é um número inteiro de semanas, a hora e o dia da semana continuam certos;
// basta somar as 1024 semanas de volta. Retorna 0 se a data não for plausível.
const uint32_t SEGUNDOS_1024_SEMANAS = 1024UL * 7UL * 86400UL;

inline uint32_t corrigirDataGps(uint32_t epochGps, uint32_t epochMinimo) {
  if (epochGps >= epochMinimo) return epochGps;
  if (epochGps + SEGUNDOS_1024_SEMANAS >= epochMinimo) return epochGps + SEGUNDOS_1024_SEMANAS;
  return 0;
}
