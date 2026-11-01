// ============================================================================
//  HORÁRIOS DOS TOQUES  (hora local de MS)
//  >>> EXEMPLO: troque pelos horários reais do campus. <<<
//  Grave o MESMO arquivo em todos os sinos para eles tocarem juntos.
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
    {7, 0, SEG_A_SEX, 5000, 1},    // entrada
    {7, 50, SEG_A_SEX, 3000, 1},   // troca de aula
    {8, 40, SEG_A_SEX, 3000, 1},   // troca de aula
    {9, 30, SEG_A_SEX, 2000, 2},   // início do intervalo (2 batidas)
    {9, 50, SEG_A_SEX, 5000, 1},   // fim do intervalo
    {10, 40, SEG_A_SEX, 3000, 1},  // troca de aula
    {11, 30, SEG_A_SEX, 2000, 3},  // saída (3 batidas)
    // ---- Vespertino
    {13, 0, SEG_A_SEX, 5000, 1},
    {13, 50, SEG_A_SEX, 3000, 1},
    {14, 40, SEG_A_SEX, 3000, 1},
    {15, 30, SEG_A_SEX, 2000, 2},
    {15, 50, SEG_A_SEX, 5000, 1},
    {16, 40, SEG_A_SEX, 3000, 1},
    {17, 30, SEG_A_SEX, 2000, 3},
    // ---- Noturno
    {19, 0, SEG_A_SEX, 5000, 1},
    {19, 50, SEG_A_SEX, 3000, 1},
    {20, 40, SEG_A_SEX, 2000, 2},
    {20, 55, SEG_A_SEX, 5000, 1},
    {21, 45, SEG_A_SEX, 3000, 1},
    {22, 35, SEG_A_SEX, 2000, 3},
};

const uint8_t NUM_HORARIOS = sizeof(HORARIOS) / sizeof(HORARIOS[0]);
