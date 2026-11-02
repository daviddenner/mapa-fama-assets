// Configuração guardada na memória flash do ESP (EEPROM): horários, senhas e
// rede da instituição. Sobrevive a falta de energia e a regravar o firmware.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "horarios.h"

const uint32_t MARCA_CONFIG = 0x53494E31;  // "SIN1": muda se o formato mudar

struct Configuracao {
  uint32_t marca;
  uint8_t nHorarios;
  Toque horarios[MAX_HORARIOS];
  char apSenha[65];     // senha da rede própria do sino
  char adminSenha[33];  // senha da página
  char instNome[33];    // rede da instituição (vazio = não usa)
  char instSenha[65];
  uint32_t crc;         // confere se o que está na memória não corrompeu
};

inline uint32_t crc32(const uint8_t *d, size_t n) {
  uint32_t c = 0xFFFFFFFFUL;
  while (n--) {
    c ^= *d++;
    for (int k = 0; k < 8; k++) c = (c >> 1) ^ (0xEDB88320UL & (0UL - (c & 1)));
  }
  return ~c;
}

inline uint32_t crcConfig(const Configuracao &c) {
  return crc32((const uint8_t *)&c, offsetof(Configuracao, crc));
}

inline bool configValida(const Configuracao &c) {
  return c.marca == MARCA_CONFIG && c.nHorarios <= MAX_HORARIOS && c.crc == crcConfig(c) &&
         memchr(c.apSenha, 0, sizeof c.apSenha) && memchr(c.adminSenha, 0, sizeof c.adminSenha) &&
         memchr(c.instNome, 0, sizeof c.instNome) && memchr(c.instSenha, 0, sizeof c.instSenha);
}

inline void selarConfig(Configuracao &c) {
  c.marca = MARCA_CONFIG;
  c.crc = crcConfig(c);
}

inline void copiarTexto(char *dst, size_t tam, const char *src) {
  strncpy(dst, src, tam - 1);
  dst[tam - 1] = 0;
}
