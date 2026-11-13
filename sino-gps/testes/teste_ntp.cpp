// Testes da leitura de respostas NTP do sino (hora pela rede). Rodam no PC:
//   g++ -std=c++17 -Wall -I../sino_gps teste_ntp.cpp -o teste_ntp && ./teste_ntp
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "ntp.h"

static int falhas = 0;
#define CHECA(cond)                                          \
  do {                                                       \
    if (!(cond)) {                                           \
      std::printf("FALHOU linha %d: %s\n", __LINE__, #cond); \
      falhas++;                                              \
    }                                                        \
  } while (0)

static void gravar(uint8_t *p, int64_t us) {  // µs desde 1970 -> carimbo NTP
  const uint32_t s = (uint32_t)(us / 1000000LL + NTP_1900_PARA_1970);
  const uint32_t f = (uint32_t)(((uint64_t)(us % 1000000LL) << 32) / 1000000ULL);
  for (int i = 0; i < 4; i++) p[i] = s >> (24 - 8 * i);
  for (int i = 0; i < 4; i++) p[4 + i] = f >> (24 - 8 * i);
}

int main() {
  const int64_t t2026 = 1793620800LL * 1000000LL;  // 2026-11-02 12:00:00 UTC
  uint8_t c[8];
  gravar(c, t2026 + 250000);
  CHECA(ntpParaUs(c) == t2026 + 250000);

  const uint8_t marca[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  uint8_t r[48] = {0};
  r[0] = 0x24;  // LI 0, versão 4, modo 4 (servidor)
  r[1] = 2;
  memcpy(r + 24, marca, 8);
  gravar(r + 32, t2026);
  gravar(r + 40, t2026 + 30);
  RespostaNtp rn;
  CHECA(analisarNtp(r, 48, marca, rn) == NTP_OK && rn.estrato == 2);
  CHECA(analisarNtp(r, 40, marca, rn) == NTP_CURTA);
  const uint8_t outra[8] = {0};
  CHECA(analisarNtp(r, 48, outra, rn) == NTP_NAO_E_NOSSA);
  uint8_t r2[48];
  memcpy(r2, r, 48);
  r2[0] = 0x23;
  CHECA(analisarNtp(r2, 48, marca, rn) == NTP_NAO_E_RESPOSTA);
  memcpy(r2, r, 48);
  r2[1] = 0;
  CHECA(analisarNtp(r2, 48, marca, rn) == NTP_KOD);
  memcpy(r2, r, 48);
  r2[0] = 0xE4;
  CHECA(analisarNtp(r2, 48, marca, rn) == NTP_NAO_SINCRONIZADO);
  memcpy(r2, r, 48);
  memset(r2 + 40, 0, 8);
  CHECA(analisarNtp(r2, 48, marca, rn) == NTP_SEM_HORA);
  CHECA(strcmp(textoProblemaNtp(NTP_KOD), "servidor recusou") == 0);

  // relógio local 5 s atrás da hora certa; ida 20 ms, servidor 1 ms, volta 20 ms
  const int64_t D = 5000000, t1 = 777000;
  int64_t desloc, atraso;
  calcularNtp(t1, t1 + 41000, t1 + 20000 + D, t1 + 21000 + D, desloc, atraso);
  CHECA(desloc == D && atraso == 40000);
  // caminho torto (ida 35 ms, volta 5 ms): o erro fica dentro de +-atraso/2
  calcularNtp(t1, t1 + 41000, t1 + 35000 + D, t1 + 36000 + D, desloc, atraso);
  CHECA(llabs(desloc - D) <= atraso / 2);

  // próximo segundo cheio: relógio local em 1000,3 s; hora certa = local + 2,9 s
  uint32_t ep;
  int64_t falta;
  proximoSegundoCheio(1000300000LL, 2900000LL, ep, falta);
  CHECA(ep == 1004 && falta == 800000);  // 1003,2 s agora -> 1004 s em 0,8 s
  proximoSegundoCheio(5000000LL, t2026, ep, falta);  // exatamente na virada
  CHECA(ep == 1793620806UL && falta == 1000000);

  if (falhas == 0) std::printf("OK: todos os testes passaram\n");
  return falhas ? 1 : 0;
}
