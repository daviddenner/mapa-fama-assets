// Testes da lógica pura (datas, leitor NMEA, agenda). Rodam no PC, sem ESP:
//   g++ -std=c++17 -Wall -I../sino_gps teste_logica.cpp -o teste && ./teste
#include <cstdio>
#include <cstring>

#include "agenda.h"
#include "nmea.h"
#include "tempo.h"

static int falhas = 0;
#define CHECA(cond)                                                    \
  do {                                                                 \
    if (!(cond)) {                                                     \
      std::printf("FALHOU linha %d: %s\n", __LINE__, #cond);           \
      falhas++;                                                        \
    }                                                                  \
  } while (0)

static bool alimentarLinha(LeitorNMEA &l, const char *s, LeituraRMC &r) {
  bool ok = false;
  uint32_t ini = 0;
  for (const char *p = s; *p; ++p) ok |= l.alimentar(*p, 0, r, ini);
  ok |= l.alimentar('\r', 0, r, ini);
  l.alimentar('\n', 0, r, ini);
  return ok;
}

// Monta "$<corpo>*hh" com o checksum certo.
static void comChecksum(const char *corpo, char *saida) {
  uint8_t s = 0;
  for (const char *p = corpo; *p; ++p) s ^= (uint8_t)*p;
  std::sprintf(saida, "$%s*%02X", corpo, s);
}

int main() {
  // ---------------------------------------------------------------- datas
  DataHora t = {2026, 11, 1, 7, 0, 0, 0};
  CHECA(paraEpoch(t) == 1793516400UL);  // 2026-11-01 07:00:00 UTC
  t = deEpoch(1793516400UL);
  CHECA(t.ano == 2026 && t.mes == 11 && t.dia == 1 && t.hora == 7 && t.min == 0 && t.seg == 0);
  CHECA(t.diaSemana == 0);                  // domingo
  CHECA(deEpoch(0).diaSemana == 4);         // 1970-01-01 quinta
  CHECA(deEpoch(951782400UL).dia == 29);    // 2000-02-29
  for (uint32_t e = 946684800UL; e < 4102444800UL; e += 86400UL * 13 + 3607) {
    DataHora x = deEpoch(e);
    CHECA(dataHoraValida(x) && paraEpoch(x) == e);
  }
  DataHora inval = {2026, 2, 30, 0, 0, 0, 0};
  CHECA(!dataHoraValida(inval));
  CHECA(epochDaCompilacao("Nov  1 2026") == 1793491200UL - 86400UL);
  CHECA(epochDaCompilacao("Jan 15 2027") == paraEpoch({2027, 1, 15, 0, 0, 0, 0}) - 86400UL);

  // Rollover de 1024 semanas: hora e dia da semana iguais, data volta ao certo.
  const uint32_t minimo = epochDaCompilacao("Nov  1 2026");
  const uint32_t certo = 1793516400UL;
  CHECA(corrigirDataGps(certo, minimo) == certo);
  CHECA(corrigirDataGps(certo - SEGUNDOS_1024_SEMANAS, minimo) == certo);
  CHECA(deEpoch(certo - SEGUNDOS_1024_SEMANAS).diaSemana == deEpoch(certo).diaSemana);
  CHECA(corrigirDataGps(946684800UL, minimo) == 0);  // 2000-01-01: lixo

  // ----------------------------------------------------------------- NMEA
  LeitorNMEA l;
  LeituraRMC r;
  // Exemplo da documentação u-blox
  CHECA(alimentarLinha(
      l, "$GPRMC,083559.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A*57", r));
  CHECA(r.comFix && r.utc.hora == 8 && r.utc.min == 35 && r.utc.seg == 59);
  CHECA(r.utc.dia == 9 && r.utc.mes == 12 && r.utc.ano == 2002 && r.fracMs == 0);

  char linha[100];
  comChecksum("GNRMC,110000.50,A,2027.000,S,05437.000,W,0.0,0.0,011126,,,A,V", linha);
  CHECA(alimentarLinha(l, linha, r));  // NEO-M8 (GN) e NMEA 4.1 (campo extra)
  CHECA(r.comFix && r.utc.hora == 11 && r.fracMs == 500 && r.utc.ano == 2026);

  CHECA(!alimentarLinha(l, "$GPRMC,,V,,,,,,,,,,N*53", r));  // sem hora: ignorada
  comChecksum("GPRMC,123456.00,V,,,,,,,011126,,,N", linha);
  CHECA(alimentarLinha(l, linha, r) && !r.comFix);  // tem hora mas sem fix

  const uint32_t errosAntes = l.errosChecksum;
  CHECA(!alimentarLinha(
      l, "$GPRMC,083559.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A*58", r));
  CHECA(l.errosChecksum == errosAntes + 1);
  comChecksum("GPGGA,083559.00,4717.11437,N,00833.91522,E,1,08,1.01,499.6,M,48.0,M,,", linha);
  CHECA(!alimentarLinha(l, linha, r));  // outras sentenças são ignoradas
  // linha truncada no meio (GPS religado) seguida de uma boa
  CHECA(alimentarLinha(
      l, "$GPRMC,0835$GPRMC,083559.00,A,4717.11437,N,00833.91522,E,0.004,77.52,091202,,,A*57",
      r));

  // --------------------------------------------------------------- agenda
  const Toque tab[] = {
      {7, 0, SEG_A_SEX, 3000, 1},
      {9, 30, SEG | QUA, 2000, 2},
      {22, 35, SEX, 2000, 3},
  };
  const uint8_t n = sizeof(tab) / sizeof(tab[0]);
  // 2026-11-02 é segunda-feira
  const uint32_t seg0700 = paraEpoch({2026, 11, 2, 7, 0, 0, 0});
  uint32_t ultimo = 0;
  CHECA(verificarAgenda(tab, n, seg0700 - 1, ultimo, 5) == -1);
  CHECA(verificarAgenda(tab, n, seg0700, ultimo, 5) == 0);
  CHECA(verificarAgenda(tab, n, seg0700 + 1, ultimo, 5) == -1);  // não repete
  CHECA(verificarAgenda(tab, n, seg0700, ultimo, 5) == -1);      // relógio voltou: não repete
  ultimo = 0;
  CHECA(verificarAgenda(tab, n, seg0700 + 2, ultimo, 5) == 0);   // pulou o :00, ainda toca
  ultimo = 0;
  CHECA(verificarAgenda(tab, n, seg0700 + 5, ultimo, 5) == -1);  // fora da janela
  ultimo = 0;
  CHECA(verificarAgenda(tab, n, seg0700 - 86400, ultimo, 5) == -1);  // domingo
  ultimo = 0;
  CHECA(verificarAgenda(tab, n, seg0700 + 86400 + 9000, ultimo, 5) == -1);  // terça 09:30
  CHECA(verificarAgenda(tab, n, seg0700 + 2 * 86400 + 9000, ultimo, 5) == 1);  // quarta 09:30

  CHECA(proximoToque(tab, n, seg0700 - 60) == seg0700);
  CHECA(proximoToque(tab, n, seg0700) == seg0700 + 9000);  // segunda 09:30
  // sexta 22:35 -> próximo é segunda 07:00
  const uint32_t sex2235 = paraEpoch({2026, 11, 6, 22, 35, 0, 0});
  CHECA(proximoToque(tab, n, sex2235) == paraEpoch({2026, 11, 9, 7, 0, 0, 0}));
  CHECA(proximoToque(tab, 0, sex2235) == 0);

  if (falhas == 0) std::printf("OK: todos os testes passaram\n");
  return falhas ? 1 : 0;
}
