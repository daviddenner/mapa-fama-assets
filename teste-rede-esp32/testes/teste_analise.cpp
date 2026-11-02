// Testes das funções de análise do teste de rede. Rodam no PC, sem ESP32:
//   g++ -std=c++17 -Wall -I../teste_rede teste_analise.cpp -o teste && ./teste
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "analise.h"

static int falhas = 0;
#define CHECA(cond)                                          \
  do {                                                       \
    if (!(cond)) {                                           \
      std::printf("FALHOU linha %d: %s\n", __LINE__, #cond); \
      falhas++;                                              \
    }                                                        \
  } while (0)

static void gravarNtp(uint8_t *p, int64_t us) {  // µs desde 1970 -> carimbo NTP
  const uint32_t s = (uint32_t)(us / 1000000LL + NTP_1900_PARA_1970);
  const uint32_t f = (uint32_t)(((uint64_t)(us % 1000000LL) << 32) / 1000000ULL);
  for (int i = 0; i < 4; i++) p[i] = s >> (24 - 8 * i);
  for (int i = 0; i < 4; i++) p[4 + i] = f >> (24 - 8 * i);
}

int main() {
  // ------------------------------------------------------------------ NTP
  const int64_t t2026 = 1793620800LL * 1000000LL;  // 2026-11-02 12:00:00 UTC
  uint8_t c[8];
  gravarNtp(c, t2026 + 500000);
  CHECA(ntpParaUs(c) == t2026 + 500000);
  gravarNtp(c, t2026 + 123456);
  CHECA(llabs(ntpParaUs(c) - (t2026 + 123456)) <= 1);

  const uint8_t marca[8] = {1, 2, 3, 4, 5, 6, 7, 8};
  uint8_t r[48] = {0};
  r[0] = 0x24;  // LI 0, versão 4, modo 4 (servidor)
  r[1] = 2;
  r[12] = 200; r[13] = 160; r[14] = 7; r[15] = 186;
  memcpy(r + 24, marca, 8);
  gravarNtp(r + 32, t2026);
  gravarNtp(r + 40, t2026 + 20);
  RespostaNtp rn;
  CHECA(analisarNtp(r, 48, marca, rn) == NTP_OK);
  CHECA(rn.estrato == 2 && rn.versao == 4 && llabs(rn.enviadoUs - (t2026 + 20)) <= 1);
  char ref[20];
  textoRefid(rn, ref, sizeof ref);
  CHECA(strcmp(ref, "200.160.7.186") == 0);
  CHECA(analisarNtp(r, 47, marca, rn) == NTP_CURTA);
  const uint8_t outra[8] = {9, 9, 9, 9, 9, 9, 9, 9};
  CHECA(analisarNtp(r, 48, outra, rn) == NTP_NAO_E_NOSSA);
  uint8_t r2[48];
  memcpy(r2, r, 48);
  r2[0] = 0x23;  // modo 3 = cliente
  CHECA(analisarNtp(r2, 48, marca, rn) == NTP_NAO_E_RESPOSTA);
  memcpy(r2, r, 48);
  r2[1] = 0;  // estrato 0 = kiss-of-death
  memcpy(r2 + 12, "DENY", 4);
  CHECA(analisarNtp(r2, 48, marca, rn) == NTP_KOD);
  memcpy(r2, r, 48);
  r2[0] = 0xE4;  // LI 3 = relógio do servidor não sincronizado
  CHECA(analisarNtp(r2, 48, marca, rn) == NTP_NAO_SINCRONIZADO);
  memcpy(r2, r, 48);
  memset(r2 + 40, 0, 8);
  CHECA(analisarNtp(r2, 48, marca, rn) == NTP_SEM_HORA);
  memcpy(r2, r, 48);
  r2[1] = 1;
  memcpy(r2 + 12, "GPS\0", 4);
  CHECA(analisarNtp(r2, 48, marca, rn) == NTP_OK);
  textoRefid(rn, ref, sizeof ref);
  CHECA(strcmp(ref, "GPS") == 0);

  // Ida 30 µs, servidor leva 20 µs, volta 30 µs; servidor está D à frente.
  const int64_t D = 5000000, t1 = 1000;
  const int64_t T2 = t1 + 30 + D, T3 = t1 + 50 + D, t4 = t1 + 80;
  int64_t desloc, atraso;
  calcularNtp(t1, t4, T2, T3, desloc, atraso);
  CHECA(desloc == D && atraso == 60);
  // caminho assimétrico: erro fica dentro de +-atraso/2
  calcularNtp(t1, t1 + 100, t1 + 90 + D, t1 + 90 + D, desloc, atraso);
  CHECA(llabs(desloc - D) <= atraso / 2);

  // ----------------------------------------------------------------- HTTP
  uint32_t e;
  CHECA(lerDataHttp("Mon, 02 Nov 2026 14:00:00 GMT", e) && e == 1793628000UL);
  CHECA(lerDataHttp("02 nov 2026 14:00:00 GMT", e) && e == 1793628000UL);
  CHECA(!lerDataHttp("Mon, 31 Feb 2026 14:00:00 GMT", e));
  CHECA(!lerDataHttp("ontem", e));
  CHECA(lerDataHttp("Sat, 01 Jan 2000 00:00:05 GMT", e) && e == 946684805UL);  // roteador sem hora

  RespostaHttp h;
  limparHttp(h);
  lerLinhaHttp("HTTP/1.1 302 Found", true, h);
  lerLinhaHttp("date: Mon, 02 Nov 2026 14:00:00 GMT", false, h);
  lerLinhaHttp("LOCATION:  https://portal.ifms.edu.br/login?mac=aa ", false, h);
  lerLinhaHttp("Server: nginx", false, h);
  lerLinhaHttp("X-Outro: 1", false, h);
  CHECA(h.status == 302 && h.temData && h.dataEpoch == 1793628000UL);
  CHECA(strcmp(h.local, "https://portal.ifms.edu.br/login?mac=aa") == 0);
  CHECA(strcmp(h.servidor, "nginx") == 0);
  limparHttp(h);
  lerLinhaHttp("SSH-2.0-OpenSSH", true, h);
  CHECA(h.status == -1);

  char t[80];
  CHECA(extrairTitulo("<html><head><TITLE lang=pt>\n  Login  -\tIFMS </title></head>", t, sizeof t) &&
        strcmp(t, "Login - IFMS") == 0);
  CHECA(!extrairTitulo("<html><body>sem titulo</body>", t, sizeof t));
  CHECA(!extrairTitulo("<title>cortado", t, sizeof t));

  Url u;
  CHECA(lerUrl("http://10.0.0.1/", u) && !u.https && u.porta == 80 && !strcmp(u.host, "10.0.0.1") &&
        !strcmp(u.caminho, "/"));
  CHECA(lerUrl("https://portal.ifms.edu.br:8443/login?x=1#topo", u) && u.https && u.porta == 8443 &&
        !strcmp(u.host, "portal.ifms.edu.br") && !strcmp(u.caminho, "/login?x=1"));
  CHECA(lerUrl("10.0.0.1", u) && u.porta == 80 && !strcmp(u.caminho, "/"));
  CHECA(lerUrl("http://h?x=1", u) && !strcmp(u.caminho, "/?x=1"));
  CHECA(lerUrl("HTTPS://H/", u) && u.https && u.porta == 443);
  CHECA(!lerUrl("http://h:0/", u));
  CHECA(!lerUrl("http://h:abc/", u));
  CHECA(!lerUrl("http:///x", u));
  CHECA(mesmoHost("http://1.1.1.1/", "https://1.1.1.1/"));
  CHECA(!mesmoHost("http://1.1.1.1/", "https://portal.ifms.edu.br/login"));
  CHECA(!mesmoHost("http://1.1.1.1/", "/relativo"));

  // Date: servidor D à frente; pedidos de 12 ms; resposta gerada no meio.
  const int64_t Dd = 1793628000LL * 1000000LL + 337000;  // hora do servidor - relógio local
  AmostraData a[60];
  int n = 0;
  for (int64_t envio = 5000; n < 60; envio += 12000, n++) {
    const int64_t gerada = envio + 6000;
    a[n].envioUs = envio;
    a[n].recebUs = envio + 12000;
    a[n].segundo = (uint32_t)((gerada + Dd) / 1000000LL);
  }
  int64_t des, inc;
  deslocDeUmaAmostra(a[0], des, inc);
  CHECA(llabs(des - Dd) <= inc);
  CHECA(acharVirada(a, n, des, inc));
  CHECA(llabs(des - Dd) <= inc && inc <= 12000);
  CHECA(!acharVirada(a, 1, des, inc));

  // --------------------------------------------------------------- outros
  CHECA(ipPrivado(10, 0) && ipPrivado(172, 16) && ipPrivado(172, 31) && ipPrivado(192, 168));
  CHECA(ipPrivado(100, 64) && ipPrivado(169, 254) && !ipPrivado(172, 32) && !ipPrivado(200, 160));
  CHECA(contemSemCaixa("CloudFlare-nginx", "cloudflare") && !contemSemCaixa("nginx", "cloudflare"));
  CHECA(strstr(motivoQueda(202), "MAC") && strstr(motivoQueda(15), "SENHA"));
  CHECA(strstr(motivoQueda(201), "nao encontrada") && strcmp(motivoQueda(999), "outro motivo") == 0);
  CHECA(!strcmp(nomeSeguranca(0), "aberta") && strstr(nomeSeguranca(5), "Enterprise"));

  if (falhas == 0) std::printf("OK: todos os testes passaram\n");
  return falhas ? 1 : 0;
}
