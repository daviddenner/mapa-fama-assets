// Acesso direto ao RTC DS3231 pelo I2C (sem bibliotecas externas).
// O RTC guarda a hora em UTC; a conversão para a hora local é feita no firmware.
#pragma once
#include <Wire.h>
#include "tempo.h"

namespace ds3231 {

const uint8_t ENDERECO = 0x68;
const uint8_t REG_CONTROLE = 0x0E;
const uint8_t REG_STATUS = 0x0F;
const uint8_t BIT_OSF = 0x80;  // "oscilador parou": a hora guardada não vale

inline uint8_t bcdParaBin(uint8_t v) { return (v >> 4) * 10 + (v & 0x0F); }
inline uint8_t binParaBcd(uint8_t v) { return ((v / 10) << 4) | (v % 10); }

inline bool lerRegistro(uint8_t reg, uint8_t &valor) {
  Wire.beginTransmission(ENDERECO);
  Wire.write(reg);
  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom(ENDERECO, (uint8_t)1) != 1) return false;
  valor = Wire.read();
  return true;
}

inline bool escreverRegistro(uint8_t reg, uint8_t valor) {
  Wire.beginTransmission(ENDERECO);
  Wire.write(reg);
  Wire.write(valor);
  return Wire.endTransmission() == 0;
}

// Liga o oscilador também na bateria e desliga as saídas SQW e 32 kHz
// (economiza a bateria). Retorna false se o DS3231 não responder.
inline bool iniciar() {
  uint8_t c, s;
  if (!lerRegistro(REG_CONTROLE, c) || !lerRegistro(REG_STATUS, s)) return false;
  return escreverRegistro(REG_CONTROLE, (c & ~0x80) | 0x04) &&  // EOSC=0, INTCN=1
         escreverRegistro(REG_STATUS, s & ~0x08);                // EN32kHz=0 (mantém OSF)
}

// true se o oscilador parou em algum momento (bateria acabou/foi retirada).
inline bool perdeuHora(bool &perdeu) {
  uint8_t s;
  if (!lerRegistro(REG_STATUS, s)) return false;
  perdeu = (s & BIT_OSF) != 0;
  return true;
}

// Lê a hora (UTC) como epoch. Retorna false em erro de comunicação ou data inválida.
inline bool ler(uint32_t &epochUtc) {
  Wire.beginTransmission(ENDERECO);
  Wire.write((uint8_t)0x00);
  if (Wire.endTransmission() != 0) return false;
  if (Wire.requestFrom(ENDERECO, (uint8_t)7) != 7) return false;
  uint8_t r[7];
  for (uint8_t i = 0; i < 7; i++) r[i] = Wire.read();

  DataHora t;
  t.seg = bcdParaBin(r[0] & 0x7F);
  t.min = bcdParaBin(r[1] & 0x7F);
  if (r[2] & 0x40) {  // modo 12 h (não usamos, mas alguém pode ter gravado assim)
    t.hora = bcdParaBin(r[2] & 0x1F) % 12 + ((r[2] & 0x20) ? 12 : 0);
  } else {
    t.hora = bcdParaBin(r[2] & 0x3F);
  }
  t.dia = bcdParaBin(r[4] & 0x3F);
  t.mes = bcdParaBin(r[5] & 0x1F);
  t.ano = 2000 + bcdParaBin(r[6]);
  t.diaSemana = 0;
  if (!dataHoraValida(t)) return false;
  epochUtc = paraEpoch(t);
  return true;
}

// Grava a hora (UTC). A escrita dos segundos zera o divisor interno do DS3231,
// então o próximo segundo vira exatamente 1 s depois desta chamada.
inline bool gravar(uint32_t epochUtc) {
  const DataHora t = deEpoch(epochUtc);
  Wire.beginTransmission(ENDERECO);
  Wire.write((uint8_t)0x00);
  Wire.write(binParaBcd(t.seg));
  Wire.write(binParaBcd(t.min));
  Wire.write(binParaBcd(t.hora));  // modo 24 h
  Wire.write((uint8_t)(t.diaSemana + 1));
  Wire.write(binParaBcd(t.dia));
  Wire.write(binParaBcd(t.mes));
  Wire.write(binParaBcd((uint8_t)(t.ano - 2000)));
  if (Wire.endTransmission() != 0) return false;

  uint8_t s;  // hora nova gravada: limpa o aviso de "oscilador parou"
  if (!lerRegistro(REG_STATUS, s)) return false;
  return escreverRegistro(REG_STATUS, s & ~BIT_OSF);
}

}  // namespace ds3231
