// Leitura de respostas NTP (funções puras, testáveis no PC).
#pragma once
#include <stdint.h>
#include <string.h>

const uint32_t NTP_1900_PARA_1970 = 2208988800UL;

struct RespostaNtp {
  uint8_t li, versao, modo, estrato;
  int64_t recebidoUs;  // T2: hora do servidor ao receber o pedido (µs desde 1970)
  int64_t enviadoUs;   // T3: hora do servidor ao responder
};

enum ProblemaNtp {
  NTP_OK = 0,
  NTP_CURTA,             // resposta com menos de 48 bytes
  NTP_NAO_E_RESPOSTA,    // não veio no modo "servidor"
  NTP_NAO_E_NOSSA,       // não é resposta ao nosso pedido
  NTP_KOD,               // "kiss of death": o servidor recusou
  NTP_SEM_HORA,          // veio sem hora
  NTP_NAO_SINCRONIZADO,  // o próprio servidor diz que a hora dele não vale
};

inline const char *textoProblemaNtp(int p) {
  switch (p) {
    case NTP_CURTA: return "resposta incompleta";
    case NTP_NAO_E_RESPOSTA: return "resposta estranha";
    case NTP_NAO_E_NOSSA: return "resposta de outro pedido";
    case NTP_KOD: return "servidor recusou";
    case NTP_SEM_HORA: return "servidor sem hora";
    case NTP_NAO_SINCRONIZADO: return "relogio do servidor nao sincronizado";
    default: return "ok";
  }
}

inline uint32_t ntpU32(const uint8_t *p) {
  return (uint32_t)p[0] << 24 | (uint32_t)p[1] << 16 | (uint32_t)p[2] << 8 | p[3];
}

// Carimbo NTP de 64 bits (s desde 1900 + fração) -> µs desde 1970.
inline int64_t ntpParaUs(const uint8_t *p) {
  const int64_t s = (int64_t)ntpU32(p) - NTP_1900_PARA_1970;
  const uint64_t frac = ntpU32(p + 4);
  return s * 1000000LL + (int64_t)((frac * 1000000ULL) >> 32);
}

// Confere a resposta. 'marca' são os 8 bytes enviados no campo "transmit" do
// pedido; o servidor devolve iguais no campo "originate".
inline int analisarNtp(const uint8_t *r, int n, const uint8_t *marca, RespostaNtp &out) {
  memset(&out, 0, sizeof out);
  if (n < 48) return NTP_CURTA;
  out.li = r[0] >> 6;
  out.versao = (r[0] >> 3) & 7;
  out.modo = r[0] & 7;
  out.estrato = r[1];
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

// Cálculo padrão do NTP. t1/t4 = relógio local ao enviar/receber (µs);
// t2/t3 = hora do servidor ao receber/responder (µs desde 1970).
// desloc = hora certa - relógio local; a margem de erro é atraso/2.
inline void calcularNtp(int64_t t1, int64_t t4, int64_t t2, int64_t t3, int64_t &desloc, int64_t &atraso) {
  atraso = (t4 - t1) - (t3 - t2);
  if (atraso < 0) atraso = 0;
  desloc = ((t2 - t1) + (t3 - t4)) / 2;
}

// Próximo segundo cheio da hora certa, visto do relógio local:
// devolve o segundo (epoch UTC) e quantos µs faltam para ele.
inline void proximoSegundoCheio(int64_t agoraLocalUs, int64_t desloc, uint32_t &epoch, int64_t &faltaUs) {
  const int64_t utc = agoraLocalUs + desloc;
  epoch = (uint32_t)(utc / 1000000LL) + 1;
  faltaUs = (int64_t)epoch * 1000000LL - utc;
}
