// Decide se um horário da tabela deve tocar agora (lógica pura, testável no PC).
#pragma once
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
