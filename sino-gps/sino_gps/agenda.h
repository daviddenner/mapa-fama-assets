// Decide se um horário da tabela deve tocar agora (lógica pura, testável no PC).
#pragma once
#include <stdio.h>
#include <string.h>

#include "horarios.h"
#include "tempo.h"

// Chamar uma vez por segundo com a hora local. Retorna o índice do horário que
// deve tocar ou -1. 'ultimoMinuto' (epochLocal / 60 do último toque) impede que
// o mesmo minuto toque duas vezes, mesmo se o relógio for corrigido para trás.
inline int verificarAgenda(const Toque *tabela, uint8_t n, uint32_t epochLocal,
                           uint32_t &ultimoMinuto, uint8_t janelaS) {
  const DataHora t = deEpoch(epochLocal);
  const uint32_t minuto = epochLocal / 60;
  if (t.seg >= janelaS || minuto == ultimoMinuto) return -1;
  for (uint8_t i = 0; i < n; i++) {
    const Toque &h = tabela[i];
    if (h.hora == t.hora && h.minuto == t.min && (h.dias & (1 << t.diaSemana))) {
      ultimoMinuto = minuto;
      return i;
    }
  }
  return -1;
}

// Epoch (hora local) do próximo toque depois de 'epochLocal', ou 0 se a tabela
// não tiver nenhum. Usado só para mostrar no status.
inline uint32_t proximoToque(const Toque *tabela, uint8_t n, uint32_t epochLocal) {
  const uint32_t inicioDoDia = epochLocal - epochLocal % 86400UL;
  const uint8_t diaSemana = deEpoch(epochLocal).diaSemana;
  uint32_t melhor = 0;
  for (uint8_t i = 0; i < n; i++) {
    for (uint8_t d = 0; d <= 7; d++) {
      if (!(tabela[i].dias & (1 << ((diaSemana + d) % 7)))) continue;
      const uint32_t e =
          inicioDoDia + d * 86400UL + tabela[i].hora * 3600UL + tabela[i].minuto * 60UL;
      if (e > epochLocal) {
        if (melhor == 0 || e < melhor) melhor = e;
        break;
      }
    }
  }
  return melhor;
}

// ------------------------------------------------- tabela <-> texto
// Formato usado pela página e pelo comando serial: um horário por item,
// separados por ';' ou quebra de linha:  HH:MM,dias,duracao_ms,vezes
//   ex.: "07:00,62,5000,1;09:30,62,2000,2"   (62 = SEG a SEX, ver horarios.h)
// Retorna quantos horários leu (já em ordem de hora) ou -1 com 'erro' preenchido.
inline int lerTabela(const char *txt, Toque *saida, uint8_t max, uint32_t duracaoMaxMs, char *erro,
                     size_t tamErro) {
  uint8_t n = 0;
  int item = 0;
  const char *p = txt;
  for (;;) {
    while (*p == ';' || *p == '\n' || *p == '\r' || *p == ' ' || *p == '\t') p++;
    if (!*p) break;
    item++;
    unsigned h, m, dias, dur, vezes;
    int lidos = 0;
    if (sscanf(p, "%u:%u,%u,%u,%u%n", &h, &m, &dias, &dur, &vezes, &lidos) != 5 || lidos == 0) {
      snprintf(erro, tamErro, "item %d: formato invalido", item);
      return -1;
    }
    p += lidos;
    if (*p && *p != ';' && *p != '\n' && *p != '\r' && *p != ' ' && *p != '\t') {
      snprintf(erro, tamErro, "item %d: formato invalido", item);
      return -1;
    }
    if (h > 23 || m > 59) {
      snprintf(erro, tamErro, "item %d: hora invalida", item);
      return -1;
    }
    if (dias < 1 || dias > 127) {
      snprintf(erro, tamErro, "%02u:%02u: escolha pelo menos um dia", h, m);
      return -1;
    }
    if (dur < 100 || dur > duracaoMaxMs) {
      snprintf(erro, tamErro, "%02u:%02u: duracao deve ser de 0,1 a %u s", h, m,
               (unsigned)(duracaoMaxMs / 1000));
      return -1;
    }
    if (vezes < 1 || vezes > 10) {
      snprintf(erro, tamErro, "%02u:%02u: batidas deve ser de 1 a 10", h, m);
      return -1;
    }
    for (uint8_t i = 0; i < n; i++) {
      if (saida[i].hora == h && saida[i].minuto == m && (saida[i].dias & dias)) {
        snprintf(erro, tamErro, "%02u:%02u aparece duas vezes no mesmo dia", h, m);
        return -1;
      }
    }
    if (n >= max) {
      snprintf(erro, tamErro, "no maximo %u horarios", (unsigned)max);
      return -1;
    }
    memset(&saida[n], 0, sizeof(Toque));  // zera também os bytes de preenchimento
    saida[n].hora = (uint8_t)h;
    saida[n].minuto = (uint8_t)m;
    saida[n].dias = (uint8_t)dias;
    saida[n].duracaoMs = (uint16_t)dur;
    saida[n].vezes = (uint8_t)vezes;
    n++;
  }
  for (uint8_t i = 1; i < n; i++) {  // ordena pela hora
    const Toque t = saida[i];
    int j = i - 1;
    while (j >= 0 && saida[j].hora * 60 + saida[j].minuto > t.hora * 60 + t.minuto) {
      saida[j + 1] = saida[j];
      j--;
    }
    saida[j + 1] = t;
  }
  return n;
}

// Escreve a tabela no mesmo formato. Retorna o tamanho, ou 0 se não coube.
inline size_t escreverTabela(const Toque *t, uint8_t n, char *buf, size_t tam) {
  size_t usado = 0;
  if (tam) buf[0] = 0;
  for (uint8_t i = 0; i < n; i++) {
    const int k = snprintf(buf + usado, tam - usado, "%02u:%02u,%u,%u,%u;", t[i].hora, t[i].minuto,
                           t[i].dias, t[i].duracaoMs, t[i].vezes);
    if (k < 0 || (size_t)k >= tam - usado) return 0;
    usado += k;
  }
  return usado;
}
