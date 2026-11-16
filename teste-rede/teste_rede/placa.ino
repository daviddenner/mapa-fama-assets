// ============================================================================
//  Placa do sino: relógio DS3231 e relé, nos mesmos pinos do sino.
//    - "rtc": o DS3231 responde? a hora guardada vale? quantos ms ele está
//      fora da hora da rede (medido na virada do segundo)?
//    - "rtc acertar": grava a hora da rede no DS3231 na virada do segundo,
//      do mesmo jeito que o sino faz.
//    - "rele": liga o relé por meio segundo (o teste automático não mexe nele).
// ============================================================================
#include <Wire.h>

#include "ds3231.h"

#if RELE_ATIVO_EM_BAIXO
#define RELE_LIGADO LOW
#define RELE_DESLIGADO HIGH
#else
#define RELE_LIGADO HIGH
#define RELE_DESLIGADO LOW
#endif

const uint8_t ENDERECO_EEPROM_ZS042 = 0x57;  // memória AT24C32 que vem no módulo ZS-042

char situacaoRtc[72] = "ainda nao testado";
bool rtcResponde = false;
bool rtcDiferencaMedida = false;
int64_t rtcDiferencaUs = 0;  // DS3231 - hora certa, na última medida
uint32_t vezesRele = 0;

void iniciarPlaca() {
  digitalWrite(PINO_RELE, RELE_DESLIGADO);  // antes do pinMode: não estala ao ligar
  pinMode(PINO_RELE, OUTPUT);
  pinMode(PINO_BOTAO_RELE, INPUT_PULLUP);
  Wire.begin(PINO_SDA, PINO_SCL);
  Wire.setClock(400000);
}

bool i2cResponde(uint8_t endereco) {
  Wire.beginTransmission(endereco);
  return Wire.endTransmission() == 0;
}

// Hora do DS3231 (UTC), se ele responder e a hora guardada valer.
bool horaDoRtc(uint32_t &epochUtc) {
  bool perdeu = true;
  return i2cResponde(ds3231::ENDERECO) && ds3231::perdeuHora(perdeu) && !perdeu && ds3231::ler(epochUtc) &&
         epochUtc >= 1704067200UL;
}

void imprimirHoraUtc(uint32_t epochUtc) {
  const DataHora t = deEpoch(epochUtc + FUSO_MINUTOS * 60);
  SAIDA("%02u/%02u/%04u %02u:%02u:%02u", t.dia, t.mes, t.ano, t.hora, t.min, t.seg);
}

// Espera o segundo do DS3231 virar. Devolve o instante da virada no relógio do
// ESP (erro de uma leitura, ~0,1 ms) e o segundo novo (UTC). Primeiro acha a
// virada mais ou menos, sem travar o Wi-Fi; depois lê sem parar só nos
// últimos milissegundos antes da virada seguinte. Leva de 1 a 2,5 s.
bool esperarViradaRtc(int64_t &viradaUs, uint32_t &epochNovo) {
  uint8_t s0, s;
  if (!ds3231::lerRegistro(0x00, s0)) return false;
  int64_t limite = agoraUs() + 1500000;
  do {
    delay(2);
    if (!ds3231::lerRegistro(0x00, s)) return false;
    if (agoraUs() > limite) return false;  // o segundo não vira: oscilador parado
  } while (s == s0);
  const int64_t aprox = agoraUs();  // a virada foi há no máximo ~2,5 ms
  while (agoraUs() < aprox + 970000) delay(1);
  s0 = s;
  int64_t antes = agoraUs();
  limite = antes + 120000;
  for (;;) {
    if (!ds3231::lerRegistro(0x00, s)) return false;
    const int64_t depois = agoraUs();
    if (s != s0) {
      viradaUs = (antes + depois) / 2;
      return ds3231::ler(epochNovo);
    }
    if (depois > limite) return false;
    antes = depois;
  }
}

// Mede quanto o DS3231 está fora da melhor fonte de hora. false = sem medida.
bool medirDiferencaRtc(int fonte) {
  int64_t virada;
  uint32_t e;
  if (fonte < 0 || !esperarViradaRtc(virada, e)) return false;
  rtcDiferencaUs = (int64_t)e * 1000000LL - (virada + fontes[fonte].deslocUs);
  rtcDiferencaMedida = true;
  return true;
}

void imprimirDiferencaRtc(int fonte) {
  const int64_t d = rtcDiferencaUs;
  saida.print(F("  DS3231 x "));
  saida.print(fontes[fonte].nome);
  saida.print(F(": "));
  if (llabs(d) > 3600000000LL) {
    saida.print(d > 0 ? "mais de 1 hora ADIANTADO" : "mais de 1 hora ATRASADO");
  } else if (llabs(d) < 1000) {
    saida.print(F("praticamente igual ("));
    imprimirMs(llabs(d));
    saida.print(F(")"));
  } else {
    imprimirMs(llabs(d));
    saida.print(d > 0 ? " ADIANTADO" : " ATRASADO");
  }
  saida.print(F(" (margem da fonte +-"));
  imprimirMs(fontes[fonte].incertezaUs);
  saida.println(F(")"));
  if (llabs(d) > 1000000) saida.println(F("  Mais de 1 s fora: use 'rtc acertar' (o sino acerta sozinho quando tem rede)."));
  if (llabs(d) > 3600000000LL) {
    snprintf(situacaoRtc, sizeof situacaoRtc, "responde; hora errada (mais de 1 h)");
  } else {
    snprintf(situacaoRtc, sizeof situacaoRtc, "responde; %ld ms %s", (long)(llabs(d) / 1000),
             d > 0 ? "adiantado" : "atrasado");
  }
}

// Teste do relógio. 'comparar' = também mede contra a melhor fonte de hora.
void testarRtc(bool comparar) {
  SAIDA("  Relogio DS3231 (SDA GPIO%d, SCL GPIO%d): ", PINO_SDA, PINO_SCL);
  rtcResponde = i2cResponde(ds3231::ENDERECO);
  if (!rtcResponde) {
    saida.println(F("NAO RESPONDE"));
    SAIDA("      confira: SDA no GPIO%d, SCL no GPIO%d, VCC no 3,3 V e GND.\n", PINO_SDA, PINO_SCL);
    int achou = 0;
    for (uint8_t a = 1; a < 127; a++)
      if (i2cResponde(a)) {
        SAIDA("      (respondeu outro aparelho no endereco 0x%02X)\n", a);
        achou++;
      }
    if (!achou) saida.println(F("      Nenhum aparelho I2C respondeu: provavel fio solto ou SDA/SCL trocados."));
    snprintf(situacaoRtc, sizeof situacaoRtc, "NAO RESPONDE (confira as ligacoes)");
    return;
  }
  saida.print(F("RESPONDEU"));
  if (i2cResponde(ENDERECO_EEPROM_ZS042)) saida.print(F(" (com a memoria AT24C32 do modulo ZS-042)"));
  saida.println();

  bool perdeu = false;
  uint32_t e = 0;
  if (!ds3231::iniciar() || !ds3231::perdeuHora(perdeu)) {
    saida.println(F("      mas a leitura falhou (fio com mau contato?)"));
    snprintf(situacaoRtc, sizeof situacaoRtc, "responde, mas a leitura falha");
    return;
  }
  if (!ds3231::ler(e)) {
    saida.println(F("  Hora guardada: INVALIDA (relogio nunca acertado?). Use 'rtc acertar'."));
    snprintf(situacaoRtc, sizeof situacaoRtc, "responde; hora guardada invalida");
    return;
  }
  saida.print(F("  Hora guardada: "));
  imprimirHoraUtc(e);
  saida.print(F(" (MS)"));
  if (perdeu) {
    saida.println(F("  -> NAO VALE: o relogio parou em algum momento"));
    saida.println(F("      (primeira vez ligado, sem bateria ou bateria fraca). Use 'rtc acertar'."));
  } else {
    saida.println();
  }
  uint8_t tMsb, tLsb;
  if (ds3231::lerRegistro(0x11, tMsb) && ds3231::lerRegistro(0x12, tLsb)) {
    const int quartos = (int8_t)tMsb * 4 + (tLsb >> 6);
    SAIDA("  Temperatura do chip: %s%d,%02d C\n", quartos < 0 ? "-" : "", abs(quartos) / 4,
                 (abs(quartos) % 4) * 25);
  }
  snprintf(situacaoRtc, sizeof situacaoRtc, perdeu ? "responde; hora NAO vale (parou)" : "responde; hora guardada");

  int64_t v;
  uint32_t e2;
  if (!esperarViradaRtc(v, e2)) {
    saida.println(F("  O segundo do DS3231 nao virou: o oscilador esta parado."));
    snprintf(situacaoRtc, sizeof situacaoRtc, "responde, mas o relogio esta PARADO");
    return;
  }
  saida.println(F("  O relogio esta andando (o segundo virou)."));
  const int m = melhorFonte();
  if (comparar && m >= 0 && fontes[m].incertezaUs < 600000) {
    rtcDiferencaUs = (int64_t)e2 * 1000000LL - (v + fontes[m].deslocUs);
    rtcDiferencaMedida = true;
    imprimirDiferencaRtc(m);
  } else if (comparar) {
    saida.println(F("  (sem fonte de hora da rede para comparar; rode 'testar')"));
  }
}

// Depois das fontes de hora: quanto o DS3231 está fora da melhor delas.
void compararRtcComRede() {
  titulo("9. Relogio DS3231 x hora da rede");
  if (!rtcResponde) {
    saida.println(F("  (o DS3231 nao respondeu no inicio do teste)"));
    return;
  }
  const int m = melhorFonte();
  if (m < 0 || fontes[m].incertezaUs >= 600000) {
    saida.println(F("  (nenhuma fonte de hora boa o bastante para comparar)"));
    return;
  }
  if (medirDiferencaRtc(m)) {
    imprimirDiferencaRtc(m);
  } else {
    saida.println(F("  nao consegui medir (o segundo do DS3231 nao virou?)"));
  }
}

// Grava a hora da melhor fonte no DS3231, na virada do segundo.
void acertarRtc() {
  const int m = melhorFonte();
  if (m < 0 || fontes[m].incertezaUs > 100000) {
    saida.println(F("Precisa de uma fonte de hora com margem menor que 100 ms. Rode 'testar' antes."));
    return;
  }
  if (!i2cResponde(ds3231::ENDERECO)) {
    saida.println(F("O DS3231 nao responde. Rode 'rtc' para ver o motivo."));
    return;
  }
  ds3231::iniciar();
  SAIDA("Acertando o DS3231 pela fonte %s...\n", fontes[m].nome);
  const int64_t certoAgora = agoraUs() + fontes[m].deslocUs;
  int64_t alvo = certoAgora / 1000000LL + 1;
  if (alvo * 1000000LL - certoAgora < 50000) alvo++;  // tempo para se preparar
  const int64_t quando = alvo * 1000000LL - fontes[m].deslocUs;  // no relógio do ESP
  while (agoraUs() < quando - 3000) delay(1);
  while (agoraUs() < quando) delayMicroseconds(10);
  if (!ds3231::gravar((uint32_t)alvo)) {
    saida.println(F("  Falhou a gravacao (fio com mau contato?)."));
    return;
  }
  saida.print(F("  Gravado: "));
  imprimirHoraUtc((uint32_t)alvo);
  saida.println(F(" (MS)"));
  if (medirDiferencaRtc(m)) imprimirDiferencaRtc(m);
}

void testarRele(uint32_t ms) {
  if (ms < 50) ms = 50;
  if (ms > 3000) ms = 3000;
  vezesRele++;
  SAIDA("Rele (GPIO%d): LIGADO por %lu ms... ", PINO_RELE, (unsigned long)ms);
  digitalWrite(PINO_RELE, RELE_LIGADO);
  const uint32_t inicio = millis();
  while (millis() - inicio < ms) delay(1);
  digitalWrite(PINO_RELE, RELE_DESLIGADO);
  saida.println(F("desligado. Deve ter estalado e o LED do rele acendido."));
}

// Parte da placa no teste completo (o relé fica de fora de propósito).
void testarPlaca() {
  titulo("0. Placa: relogio DS3231 e rele");
  testarRtc(false);
  SAIDA("  Rele (GPIO%d): nao e acionado no teste automatico. Teste com o botao S1 (GPIO%d),\n", PINO_RELE,
               PINO_BOTAO_RELE);
  saida.println(F("      com o comando 'rele' ou pelo botao da pagina."));
}

void resumoPlaca() {
  SAIDA("Relogio DS3231 ... %s\n", situacaoRtc);
  if (vezesRele) {
    SAIDA("Rele ............. acionado %lu vez(es) nesta sessao (confira se estalou)\n", (unsigned long)vezesRele);
  } else {
    saida.println(F("Rele ............. ainda nao testado (botao S1, 'rele' ou pagina)"));
  }
}
