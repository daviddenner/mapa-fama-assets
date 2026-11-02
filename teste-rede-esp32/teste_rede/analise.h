// Funções puras do teste de rede (sem hardware), usadas pelo programa e pelos
// testes no PC: leitura de respostas NTP e HTTP, endereços e explicações.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "tempo.h"

// ======================================================================= NTP
const uint32_t NTP_1900_PARA_1970 = 2208988800UL;

struct RespostaNtp {
  uint8_t li, versao, modo, estrato;
  uint8_t refid[4];
  int64_t recebidoUs;  // T2: hora do servidor ao receber o pedido (µs desde 1970)
  int64_t enviadoUs;   // T3: hora do servidor ao responder
};

enum ProblemaNtp {
  NTP_OK = 0,
  NTP_CURTA,             // resposta com menos de 48 bytes
  NTP_NAO_E_RESPOSTA,    // não veio no modo "servidor"
  NTP_NAO_E_NOSSA,       // não é resposta ao nosso pedido
  NTP_KOD,               // "kiss of death": o servidor recusou (ex.: DENY, RATE)
  NTP_SEM_HORA,          // veio sem hora
  NTP_NAO_SINCRONIZADO,  // o próprio servidor diz que a hora dele não vale
};

inline const char *textoProblemaNtp(int p) {
  switch (p) {
    case NTP_CURTA: return "resposta incompleta";
    case NTP_NAO_E_RESPOSTA: return "resposta estranha (nao e de servidor NTP)";
    case NTP_NAO_E_NOSSA: return "resposta nao corresponde ao pedido";
    case NTP_KOD: return "servidor recusou (kiss-of-death)";
    case NTP_SEM_HORA: return "servidor respondeu sem hora";
    case NTP_NAO_SINCRONIZADO: return "servidor responde, mas o relogio dele NAO esta sincronizado";
    default: return "ok";
  }
}

inline uint32_t lerU32(const uint8_t *p) {
  return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

// Carimbo NTP de 64 bits (s desde 1900 + fração) -> µs desde 1970.
inline int64_t ntpParaUs(const uint8_t *p) {
  const int64_t s = (int64_t)lerU32(p) - NTP_1900_PARA_1970;
  const uint64_t frac = lerU32(p + 4);
  return s * 1000000LL + (int64_t)((frac * 1000000ULL) >> 32);
}

// Confere e lê a resposta. 'marca' são os 8 bytes que mandamos no campo
// "transmit" do pedido; o servidor devolve iguais no campo "originate".
inline int analisarNtp(const uint8_t *r, int n, const uint8_t *marca, RespostaNtp &out) {
  memset(&out, 0, sizeof out);
  if (n < 48) return NTP_CURTA;
  out.li = r[0] >> 6;
  out.versao = (r[0] >> 3) & 7;
  out.modo = r[0] & 7;
  out.estrato = r[1];
  memcpy(out.refid, r + 12, 4);
  if (out.modo != 4) return NTP_NAO_E_RESPOSTA;
  if (memcmp(r + 24, marca, 8) != 0) return NTP_NAO_E_NOSSA;
  if (out.estrato == 0) return NTP_KOD;
  bool zero = true;
  for (int i = 40; i < 48; i++)
    if (r[i]) zero = false;
  if (zero) return NTP_SEM_HORA;
  out.recebidoUs = ntpParaUs(r + 32);
  out.enviadoUs = ntpParaUs(r + 40);
  if (out.li == 3 || out.estrato >= 16) return NTP_NAO_SINCRONIZADO;
  return NTP_OK;
}

// Cálculo padrão do NTP. t1/t4 = relógio local ao enviar/receber;
// t2/t3 = hora do servidor ao receber/responder.
// desloc = hora do servidor - hora local; a incerteza é atraso/2.
inline void calcularNtp(int64_t t1, int64_t t4, int64_t t2, int64_t t3, int64_t &desloc,
                        int64_t &atraso) {
  atraso = (t4 - t1) - (t3 - t2);
  if (atraso < 0) atraso = 0;
  desloc = ((t2 - t1) + (t3 - t4)) / 2;
}

// "Referência" do servidor: estrato 1 = texto (GPS, PPS...); 2 ou mais = IP de quem ele segue.
inline void textoRefid(const RespostaNtp &r, char *dst, size_t tam) {
  if (r.estrato <= 1) {
    char t[5];
    for (int i = 0; i < 4; i++) t[i] = (r.refid[i] >= 32 && r.refid[i] < 127) ? (char)r.refid[i] : 0;
    t[4] = 0;
    snprintf(dst, tam, "%s", t);
  } else {
    snprintf(dst, tam, "%u.%u.%u.%u", r.refid[0], r.refid[1], r.refid[2], r.refid[3]);
  }
}

// ====================================================================== HTTP
struct RespostaHttp {
  int status;          // 0 = nada; -1 = respondeu mas não é HTTP
  bool temData;
  uint32_t dataEpoch;  // cabeçalho Date:, em UTC
  char data[40];
  char local[160];     // cabeçalho Location: (para onde redireciona)
  char servidor[48];   // cabeçalho Server:
  char titulo[80];     // <title> da página
};

inline void limparHttp(RespostaHttp &r) { memset(&r, 0, sizeof r); }

inline bool comecaCom(const char *s, const char *prefixo) {
  return strncasecmp(s, prefixo, strlen(prefixo)) == 0;
}

inline void copiarValor(char *dst, size_t tam, const char *src) {
  while (*src == ' ' || *src == '\t') src++;
  strncpy(dst, src, tam - 1);
  dst[tam - 1] = 0;
  size_t n = strlen(dst);
  while (n && (dst[n - 1] == '\r' || dst[n - 1] == ' ')) dst[--n] = 0;
}

// Data HTTP (RFC 9110): "Mon, 02 Nov 2026 14:00:00 GMT"
inline bool lerDataHttp(const char *s, uint32_t &epoch) {
  const char *virgula = strchr(s, ',');
  if (virgula) s = virgula + 1;
  int d, a, h, mi, se;
  char mes[4] = {0};
  if (sscanf(s, " %d %3s %d %d:%d:%d", &d, mes, &a, &h, &mi, &se) != 6) return false;
  static const char nomes[] = "JanFebMarAprMayJunJulAugSepOctNovDec";
  int m = 0;
  for (int i = 0; i < 12; i++)
    if (strncasecmp(mes, nomes + 3 * i, 3) == 0) m = i + 1;
  if (m == 0 || a < 2000 || a > 2099 || d < 1 || d > 31 || h < 0 || h > 23 || mi < 0 || mi > 59 ||
      se < 0 || se > 59)
    return false;
  const DataHora t = {(uint16_t)a, (uint8_t)m, (uint8_t)d, (uint8_t)h, (uint8_t)mi, (uint8_t)se, 0};
  if (!dataHoraValida(t)) return false;
  epoch = paraEpoch(t);
  return true;
}

// Uma linha do cabeçalho da resposta (sem o \r\n).
inline void lerLinhaHttp(const char *linha, bool primeira, RespostaHttp &r) {
  if (primeira) {  // "HTTP/1.1 302 Found"
    const char *espaco = strchr(linha, ' ');
    r.status = (comecaCom(linha, "HTTP/") && espaco) ? atoi(espaco + 1) : -1;
    return;
  }
  if (comecaCom(linha, "Date:")) {
    copiarValor(r.data, sizeof r.data, linha + 5);
    r.temData = lerDataHttp(r.data, r.dataEpoch);
  } else if (comecaCom(linha, "Location:")) {
    copiarValor(r.local, sizeof r.local, linha + 9);
  } else if (comecaCom(linha, "Server:")) {
    copiarValor(r.servidor, sizeof r.servidor, linha + 7);
  }
}

// Procura <title>...</title> num pedaço de HTML.
inline bool extrairTitulo(const char *html, char *dst, size_t tam) {
  const char *p = html;
  while (*p && strncasecmp(p, "<title", 6) != 0) p++;
  if (!*p) return false;
  p = strchr(p, '>');
  if (!p) return false;
  p++;
  const char *fim = p;
  while (*fim && strncasecmp(fim, "</title", 7) != 0) fim++;
  if (!*fim) return false;
  size_t n = 0;
  bool espaco = true;  // junta espaços e quebras de linha
  for (const char *q = p; q < fim && n < tam - 1; q++) {
    char c = *q;
    if (c == '\r' || c == '\n' || c == '\t') c = ' ';
    if (c == ' ' && espaco) continue;
    espaco = (c == ' ');
    dst[n++] = c;
  }
  while (n && dst[n - 1] == ' ') n--;
  dst[n] = 0;
  return n > 0;
}

struct Url {
  bool https;
  char host[96];
  uint16_t porta;
  char caminho[128];
};

// "http://host[:porta]/caminho" ou "https://..." (sem esquema = http).
inline bool lerUrl(const char *s, Url &u) {
  memset(&u, 0, sizeof u);
  u.porta = 80;
  if (comecaCom(s, "https://")) {
    u.https = true;
    u.porta = 443;
    s += 8;
  } else if (comecaCom(s, "http://")) {
    s += 7;
  }
  size_t n = 0;
  while (*s && *s != '/' && *s != ':' && *s != '?' && *s != '#') {
    if (n >= sizeof(u.host) - 1) return false;
    u.host[n++] = *s++;
  }
  u.host[n] = 0;
  if (n == 0) return false;
  if (*s == ':') {
    char *fim;
    const long p = strtol(s + 1, &fim, 10);
    if (fim == s + 1 || p <= 0 || p > 65535) return false;
    u.porta = (uint16_t)p;
    s = fim;
  }
  if (*s == '/') {
    snprintf(u.caminho, sizeof u.caminho, "%s", s);
  } else if (*s == '?') {
    snprintf(u.caminho, sizeof u.caminho, "/%s", s);
  } else {
    strcpy(u.caminho, "/");
  }
  char *cerquilha = strchr(u.caminho, '#');
  if (cerquilha) *cerquilha = 0;
  return true;
}

// Mesmo host? (para saber se um redirecionamento leva para outro lugar = portal)
inline bool mesmoHost(const char *urlA, const char *urlB) {
  Url a, b;
  if (!lerUrl(urlA, a) || !lerUrl(urlB, b)) return false;
  return strcasecmp(a.host, b.host) == 0;
}

// ------------------------------------------- Hora pelo cabeçalho Date (HTTP)
struct AmostraData {
  int64_t envioUs, recebUs;  // relógio local ao enviar o pedido / ao chegar a resposta
  uint32_t segundo;          // segundo informado no Date
};

// Uma resposta isolada: o Date só tem segundos inteiros, então a incerteza é
// de meio segundo mais metade do tempo de resposta.
inline void deslocDeUmaAmostra(const AmostraData &a, int64_t &desloc, int64_t &incerteza) {
  const int64_t meio = (a.envioUs + a.recebUs) / 2;
  desloc = (int64_t)a.segundo * 1000000LL + 500000LL - meio;
  incerteza = 500000LL + (a.recebUs - a.envioUs) / 2;
}

// Com pedidos seguidos, acha o momento em que o segundo do servidor virou:
// ele está entre o envio do último pedido com o segundo antigo e a chegada do
// primeiro com o segundo novo. Usa a virada com a janela mais estreita.
inline bool acharVirada(const AmostraData *a, int n, int64_t &desloc, int64_t &incerteza) {
  bool achou = false;
  int64_t melhor = 0;
  for (int i = 0; i + 1 < n; i++) {
    if (a[i + 1].segundo != a[i].segundo + 1) continue;
    const int64_t ini = a[i].envioUs, fim = a[i + 1].recebUs;
    if (fim < ini) continue;
    if (!achou || fim - ini < melhor) {
      achou = true;
      melhor = fim - ini;
      desloc = (int64_t)a[i + 1].segundo * 1000000LL - (ini + fim) / 2;
      incerteza = (fim - ini) / 2;
    }
  }
  return achou;
}

// ================================================================== Diversos
// 'agulha' aparece em 'palheiro', sem diferenciar maiúsculas?
inline bool contemSemCaixa(const char *palheiro, const char *agulha) {
  const size_t n = strlen(agulha);
  for (; *palheiro; palheiro++)
    if (strncasecmp(palheiro, agulha, n) == 0) return true;
  return n == 0;
}

inline bool ipPrivado(uint8_t a, uint8_t b) {
  return a == 10 || a == 127 || (a == 172 && b >= 16 && b <= 31) || (a == 192 && b == 168) ||
         (a == 100 && b >= 64 && b <= 127) || (a == 169 && b == 254);
}

// Códigos de desconexão do Wi-Fi do ESP32 (wifi_err_reason_t).
inline const char *motivoQueda(int r) {
  switch (r) {
    case 2: return "autenticacao expirou: o roteador nao aceitou o aparelho (comum com filtro de MAC)";
    case 3: return "o roteador desconectou o aparelho";
    case 4: return "associacao expirou (inatividade)";
    case 5: return "o roteador esta cheio (aparelhos demais)";
    case 6:
    case 7:
    case 9: return "o roteador recusou o aparelho (nao autenticado)";
    case 8: return "o roteador encerrou a conexao";
    case 13:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 24: return "configuracao de seguranca incompativel";
    case 14: return "falha de integridade (MIC)";
    case 15:
    case 16:
    case 204: return "falhou a troca de chaves: provavel SENHA ERRADA";
    case 23: return "falha 802.1X: a rede pede usuario e senha (tipo eduroam)";
    case 200: return "perdeu o sinal do roteador";
    case 201: return "rede nao encontrada (nome errado ou fora de alcance)";
    case 202: return "autenticacao recusada (filtro de MAC ou senha errada)";
    case 203: return "associacao recusada (filtro de MAC ou rede cheia)";
    case 205: return "falha na conexao";
    case 210:
    case 211: return "rede encontrada, mas com seguranca incompativel";
    case 212: return "sinal fraco demais";
    default: return "outro motivo";
  }
}

// Tipo de segurança da rede (wifi_auth_mode_t do ESP32).
inline const char *nomeSeguranca(int m) {
  switch (m) {
    case 0: return "aberta";
    case 1: return "WEP";
    case 2: return "WPA";
    case 3: return "WPA2";
    case 4: return "WPA/WPA2";
    case 5:
    case 10:
    case 14:
    case 15: return "Enterprise (usuario+senha)";
    case 6: return "WPA3";
    case 7: return "WPA2/WPA3";
    case 8: return "WAPI";
    case 9: return "OWE";
    default: return "outra";
  }
}
