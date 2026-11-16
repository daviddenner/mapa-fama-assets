// Tudo o que o teste mostra passa por aqui: vai para o monitor serial, fica
// num anel na memória (de onde a página lê) e, durante um teste, é copiado
// para o relatório guardado na memória do ESP. Assim o teste funciona sem
// computador.
#pragma once
#include <Arduino.h>
#include <stdarg.h>

class Registro : public Print {
 public:
#if defined(ESP8266)
  static const size_t TAM = 6144;  // o ESP8266 tem pouca RAM
#else
  static const size_t TAM = 16384;
#endif

  size_t write(uint8_t c) override { return write(&c, 1); }

  size_t write(const uint8_t *d, size_t n) override {
    Serial.write(d, n);
    for (size_t i = 0; i < n; i++) anel[fim++ % TAM] = (char)d[i];
    if (copia) copia->write(d, n);
    return n;
  }

  // printf com o formato guardado na flash (PSTR): no ESP8266 isso deixa os
  // textos fora da RAM. Use pela macro SAIDA("formato", ...).
#if defined(ESP8266)
  size_t printfP(const char *formato, ...) {
#else
  size_t printfP(const char *formato, ...) __attribute__((format(printf, 2, 3))) {  // confere os formatos
#endif
    char b[256];
    va_list a;
    va_start(a, formato);
#if defined(ESP8266)
    int n = vsnprintf_P(b, sizeof b, formato, a);
#else
    int n = vsnprintf(b, sizeof b, formato, a);
#endif
    va_end(a);
    if (n < 0) return 0;
    if (n >= (int)sizeof b) n = sizeof b - 1;  // linha longa demais: corta
    return write((const uint8_t *)b, (size_t)n);
  }

  // Durante um teste, copia tudo também para 'p' (o arquivo do relatório).
  void copiarPara(Print *p) { copia = p; }

  size_t total() { return fim; }                                   // bytes escritos desde que ligou
  size_t inicioDisponivel() { return fim > TAM ? fim - TAM : 0; }  // mais antigo ainda guardado

  // Copia até 'tam' bytes a partir da posição 'desde'; devolve quantos copiou.
  size_t copiar(size_t desde, char *dst, size_t tam) {
    if (desde < inicioDisponivel()) desde = inicioDisponivel();
    size_t n = 0;
    while (desde + n < fim && n < tam) {
      dst[n] = anel[(desde + n) % TAM];
      n++;
    }
    return n;
  }

 private:
  char anel[TAM];
  size_t fim = 0;
  Print *copia = nullptr;
};
