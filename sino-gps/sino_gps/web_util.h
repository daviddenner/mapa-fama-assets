// Funções puras da página (testáveis no PC): MD5, login "Digest" e JSON.
//
// Login Digest (RFC 7616): o navegador nunca manda a senha, só um código
// calculado a partir dela. O "nonce" (desafio) é assinado com um segredo
// sorteado a cada vez que o sino liga, então vários aparelhos e vários pedidos
// ao mesmo tempo funcionam, e depois de reiniciar o navegador refaz o login
// sozinho (stale=TRUE), sem perguntar a senha de novo.
#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ======================================================================= MD5
struct Md5 {
  uint32_t a, b, c, d;
  uint64_t total;
  uint8_t bloco[64];
  uint8_t n;
};

inline uint32_t md5Rot(uint32_t x, int c) { return (x << c) | (x >> (32 - c)); }

inline void md5Bloco(Md5 &m, const uint8_t *p) {
  static const uint32_t K[64] = {
      0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee, 0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
      0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be, 0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
      0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa, 0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
      0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed, 0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
      0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c, 0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
      0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05, 0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
      0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039, 0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
      0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1, 0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391};
  static const uint8_t R[64] = {7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
                                5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20, 5, 9,  14, 20,
                                4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
                                6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21};
  uint32_t w[16];
  for (int i = 0; i < 16; i++)
    w[i] = (uint32_t)p[i * 4] | (uint32_t)p[i * 4 + 1] << 8 | (uint32_t)p[i * 4 + 2] << 16 |
           (uint32_t)p[i * 4 + 3] << 24;
  uint32_t a = m.a, b = m.b, c = m.c, d = m.d;
  for (int i = 0; i < 64; i++) {
    uint32_t f;
    int g;
    if (i < 16) {
      f = (b & c) | (~b & d);
      g = i;
    } else if (i < 32) {
      f = (d & b) | (~d & c);
      g = (5 * i + 1) % 16;
    } else if (i < 48) {
      f = b ^ c ^ d;
      g = (3 * i + 5) % 16;
    } else {
      f = c ^ (b | ~d);
      g = (7 * i) % 16;
    }
    const uint32_t t = d;
    d = c;
    c = b;
    b = b + md5Rot(a + f + K[i] + w[g], R[i]);
    a = t;
  }
  m.a += a;
  m.b += b;
  m.c += c;
  m.d += d;
}

inline void md5Iniciar(Md5 &m) {
  m.a = 0x67452301;
  m.b = 0xefcdab89;
  m.c = 0x98badcfe;
  m.d = 0x10325476;
  m.total = 0;
  m.n = 0;
}

inline void md5Somar(Md5 &m, const void *dados, size_t tam) {
  const uint8_t *p = (const uint8_t *)dados;
  m.total += tam;
  while (tam--) {
    m.bloco[m.n++] = *p++;
    if (m.n == 64) {
      md5Bloco(m, m.bloco);
      m.n = 0;
    }
  }
}

inline void md5SomarTexto(Md5 &m, const char *s) { md5Somar(m, s, strlen(s)); }

// Termina e escreve o resultado em hexadecimal (32 letras + '\0').
inline void md5Hex(Md5 &m, char saida[33]) {
  const uint64_t bits = m.total * 8;
  const uint8_t um = 0x80, zero = 0;
  md5Somar(m, &um, 1);
  while (m.n != 56) md5Somar(m, &zero, 1);
  uint8_t tam[8];
  for (int i = 0; i < 8; i++) tam[i] = (uint8_t)(bits >> (8 * i));
  md5Somar(m, tam, 8);
  const uint32_t r[4] = {m.a, m.b, m.c, m.d};
  for (int i = 0; i < 16; i++) snprintf(saida + 2 * i, 3, "%02x", (unsigned)((r[i / 4] >> (8 * (i % 4))) & 0xFF));
}

// MD5 de vários pedaços separados por ':' (como o Digest usa).
inline void md5DePartes(char saida[33], const char *a, const char *b, const char *c = nullptr,
                        const char *d = nullptr, const char *e = nullptr, const char *f = nullptr) {
  Md5 m;
  md5Iniciar(m);
  const char *partes[6] = {a, b, c, d, e, f};
  for (int i = 0; i < 6 && partes[i]; i++) {
    if (i) md5Somar(m, ":", 1);
    md5SomarTexto(m, partes[i]);
  }
  md5Hex(m, saida);
}

// ============================================================ login Digest
#define DIGEST_REINO "Sino IFMS"
#define DIGEST_VALIDADE_S (12UL * 3600UL)  // depois disto o navegador refaz o login sozinho

// nonce = hora de emissão (8 hex) + assinatura (16 hex)
inline void gerarNonce(const char *segredo, uint32_t agoraS, char nonce[25]) {
  char tempo[9], assin[33];
  snprintf(tempo, sizeof tempo, "%08lx", (unsigned long)agoraS);
  md5DePartes(assin, segredo, tempo);
  snprintf(nonce, 25, "%s%.16s", tempo, assin);
}

inline bool nonceValido(const char *nonce, const char *segredo, uint32_t agoraS) {
  if (strlen(nonce) != 24) return false;
  char tempo[9];
  memcpy(tempo, nonce, 8);
  tempo[8] = 0;
  char *fim;
  const uint32_t emitido = (uint32_t)strtoul(tempo, &fim, 16);
  if (*fim) return false;
  char esperado[25];
  gerarNonce(segredo, emitido, esperado);
  if (strcmp(esperado, nonce) != 0) return false;
  return (uint32_t)(agoraS - emitido) <= DIGEST_VALIDADE_S;
}

// Cabeçalho WWW-Authenticate do desafio.
inline void desafioDigest(const char *segredo, uint32_t agoraS, bool vencido, char *saida, size_t tam) {
  char nonce[25];
  gerarNonce(segredo, agoraS, nonce);
  snprintf(saida, tam, "Digest realm=\"" DIGEST_REINO "\", qop=\"auth\", algorithm=MD5, nonce=\"%s\", opaque=\"sino\"%s",
           nonce, vencido ? ", stale=TRUE" : "");
}

// Pega um parâmetro (nome=valor ou nome="valor") do cabeçalho Authorization.
inline bool parametroDigest(const char *cab, const char *nome, char *valor, size_t tam) {
  const size_t n = strlen(nome);
  for (const char *p = cab; (p = strstr(p, nome)) != nullptr; p += n) {
    const bool inicio = (p == cab) || p[-1] == ' ' || p[-1] == ',' || p[-1] == '\t';
    if (!inicio || p[n] != '=') continue;
    const char *v = p + n + 1;
    size_t k = 0;
    if (*v == '"') {
      v++;
      while (*v && *v != '"' && k < tam - 1) valor[k++] = *v++;
      if (*v != '"') return false;
    } else {
      while (*v && *v != ',' && *v != ' ' && k < tam - 1) valor[k++] = *v++;
    }
    valor[k] = 0;
    return true;
  }
  return false;
}

enum ResultadoDigest { DIGEST_OK = 0, DIGEST_NEGADO = 1, DIGEST_VENCIDO = 2 };

// Confere o cabeçalho Authorization de um pedido.
// 'caminho' = caminho pedido sem a parte "?..." (ex.: "/api/estado").
inline int verificarDigest(const char *cab, const char *metodo, const char *caminho, const char *usuario,
                           const char *senha, const char *segredo, uint32_t agoraS) {
  if (strncmp(cab, "Digest ", 7) != 0) return DIGEST_NEGADO;
  cab += 7;
  char user[40], reino[40], nonce[40], uri[200], resp[40], qop[16], nc[16], cnonce[80];
  if (!parametroDigest(cab, "username", user, sizeof user) || !parametroDigest(cab, "realm", reino, sizeof reino) ||
      !parametroDigest(cab, "nonce", nonce, sizeof nonce) || !parametroDigest(cab, "uri", uri, sizeof uri) ||
      !parametroDigest(cab, "response", resp, sizeof resp))
    return DIGEST_NEGADO;
  if (strcmp(user, usuario) != 0 || strcmp(reino, DIGEST_REINO) != 0) return DIGEST_NEGADO;
  const size_t nCaminho = strcspn(uri, "?");
  if (nCaminho != strlen(caminho) || strncmp(uri, caminho, nCaminho) != 0) return DIGEST_NEGADO;

  char ha1[33], ha2[33], esperado[33];
  md5DePartes(ha1, usuario, DIGEST_REINO, senha);
  md5DePartes(ha2, metodo, uri);
  const bool comQop = parametroDigest(cab, "qop", qop, sizeof qop);
  if (comQop) {
    if (strcmp(qop, "auth") != 0 || !parametroDigest(cab, "nc", nc, sizeof nc) ||
        !parametroDigest(cab, "cnonce", cnonce, sizeof cnonce))
      return DIGEST_NEGADO;
    md5DePartes(esperado, ha1, nonce, nc, cnonce, qop, ha2);
  } else {
    md5DePartes(esperado, ha1, nonce, ha2);
  }
  // compara sem parar no primeiro caractere diferente
  uint8_t dif = (uint8_t)(strlen(resp) != 32);
  for (int i = 0; i < 32 && resp[i]; i++) dif |= (uint8_t)(resp[i] ^ esperado[i]);
  if (dif) return DIGEST_NEGADO;
  return nonceValido(nonce, segredo, agoraS) ? DIGEST_OK : DIGEST_VENCIDO;
}

// ===================================================================== JSON
// Copia texto para dentro de aspas JSON, escapando o que precisa.
inline void jsonTexto(char *dst, size_t tam, const char *src) {
  size_t k = 0;
  for (; *src && k + 7 < tam; src++) {
    const unsigned char c = (unsigned char)*src;
    if (c == '"' || c == '\\') {
      dst[k++] = '\\';
      dst[k++] = (char)c;
    } else if (c < 0x20) {
      k += snprintf(dst + k, tam - k, "\\u%04x", c);
    } else {
      dst[k++] = (char)c;
    }
  }
  dst[k] = 0;
}
