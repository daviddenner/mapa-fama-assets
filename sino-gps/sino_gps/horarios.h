// ============================================================================
//  HORÁRIOS INICIAIS DOS TOQUES  (hora local de MS)
//  Esta é só a tabela de fábrica: o sino a usa até alguém salvar outra pela
//  página do sino (ela fica guardada na memória e sobrevive a falta de energia).
//  >>> EXEMPLO: troque pelos horários reais do campus. <<<
// ============================================================================
#pragma once
#include <stdint.h>

// Dias da semana (podem ser combinados com |, ex.: SEG | QUA | SEX)
#define DOM (1 << 0)
#define SEG (1 << 1)
#define TER (1 << 2)
#define QUA (1 << 3)
#define QUI (1 << 4)
#define SEX (1 << 5)
#define SAB (1 << 6)
#define SEG_A_SEX (SEG | TER | QUA | QUI | SEX)
#define TODOS (DOM | SEG_A_SEX | SAB)

#define MAX_HORARIOS 40  // quantos horários cabem na memória

struct Toque {
  uint8_t hora;
  uint8_t minuto;
  uint8_t dias;
  uint16_t duracaoMs;  // tempo de cada batida (máx. DURACAO_MAX_MS)
  uint8_t vezes;       // número de batidas (1 = toque único)
};

const Toque HORARIOS[] = {
    // hora min  dias        duração vezes
    // ---- Matutino
    {7, 0, SEG_A_SEX, 1500, 1},    // entrada
    {7, 50, SEG_A_SEX, 1500, 1},   // troca de aula
    {8, 40, SEG_A_SEX, 1500, 1},   // troca de aula
    {9, 30, SEG_A_SEX, 1500, 1},   // início do intervalo
    {9, 50, SEG_A_SEX, 1500, 1},   // fim do intervalo
    {10, 40, SEG_A_SEX, 1500, 1},  // troca de aula
    {11, 30, SEG_A_SEX, 2500, 1},  // fim do período (toque mais longo)
    // ---- Vespertino
    {13, 0, SEG_A_SEX, 1500, 1},
    {13, 50, SEG_A_SEX, 1500, 1},
    {14, 40, SEG_A_SEX, 1500, 1},
    {15, 30, SEG_A_SEX, 1500, 1},
    {15, 50, SEG_A_SEX, 1500, 1},
    {16, 40, SEG_A_SEX, 1500, 1},
    {17, 30, SEG_A_SEX, 2500, 1},
    // ---- Noturno
    {19, 0, SEG_A_SEX, 1500, 1},
    {19, 50, SEG_A_SEX, 1500, 1},
    {20, 40, SEG_A_SEX, 1500, 1},
    {20, 55, SEG_A_SEX, 1500, 1},
    {21, 45, SEG_A_SEX, 1500, 1},
    {22, 35, SEG_A_SEX, 2500, 1},
};

const uint8_t NUM_HORARIOS = sizeof(HORARIOS) / sizeof(HORARIOS[0]);
static_assert(sizeof(HORARIOS) / sizeof(HORARIOS[0]) <= MAX_HORARIOS, "tabela inicial grande demais");
