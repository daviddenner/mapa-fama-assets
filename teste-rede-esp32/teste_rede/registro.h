// Tudo o que o teste mostra passa por aqui: vai para o monitor serial e fica
// guardado num anel de 24 KB, de onde a página (pelo Wi-Fi do próprio ESP) e
// os relatórios salvos na memória leem. Assim o teste funciona sem computador.
#pragma once
#include <Arduino.h>

class Registro : public Print {
 public:
  static const size_t TAM = 24576;

  void iniciar() { trava = xSemaphoreCreateMutex(); }

  size_t write(uint8_t c) override { return write(&c, 1); }

  size_t write(const uint8_t *d, size_t n) override {
    Serial.write(d, n);
    travar();
    for (size_t i = 0; i < n; i++) anel[fim++ % TAM] = (char)d[i];
    destravar();
    return n;
  }

  size_t total() { return fim; }                                         // bytes escritos desde que ligou
  size_t inicioDisponivel() { return fim > TAM ? fim - TAM : 0; }        // mais antigo ainda guardado

  // Copia até 'tam' bytes a partir da posição 'desde'; devolve quantos copiou.
  size_t copiar(size_t desde, char *dst, size_t tam) {
    travar();
    if (desde < inicioDisponivel()) desde = inicioDisponivel();
    size_t n = 0;
    while (desde + n < fim && n < tam) {
      dst[n] = anel[(desde + n) % TAM];
      n++;
    }
    destravar();
    return n;
  }

 private:
  void travar() {
    if (trava) xSemaphoreTake(trava, portMAX_DELAY);
  }
  void destravar() {
    if (trava) xSemaphoreGive(trava);
  }
  char anel[TAM];
  size_t fim = 0;
  SemaphoreHandle_t trava = NULL;
};
