// ============================================================================
//  Sino escolar sincronizado por GPS
//  ESP-12 (ESP8266) + GPS NEO-6M/NEO-M8N + RTC DS3231 + relé (sino 127 V)
//
//  O sino não depende de roteador nem de internet para tocar na hora certa:
//  - O GPS dá a hora exata (a mesma para todos os sinos, com erro de milissegundos).
//  - O DS3231 guarda a hora (com bateria) e mantém o sino funcionando quando
//    o GPS fica sem sinal ou depois de uma queda de energia.
//  - Os horários são mudados por uma página no próprio sino (web.ino), aberta
//    pela rede Wi-Fi do sino (nome oculto) ou pela rede da instituição.
//    Ficam guardados na memória; horarios.h é só a tabela inicial.
//  - O resto da configuração fica em config.h.
//
//  Placa na Arduino IDE: "Generic ESP8266 Module" (ou NodeMCU / Wemos D1 mini).
//  Não precisa instalar nenhuma biblioteca além do pacote ESP8266.
// ============================================================================
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#include "agenda.h"
#include "armazenamento.h"
#include "config.h"
#include "ds3231.h"
#include "horarios.h"
#include "nmea.h"
#include "pagina.h"
#include "tempo.h"
#include "web_util.h"

SoftwareSerial gpsSerial(PINO_GPS_RX, -1);  // só recebe do GPS
LeitorNMEA nmea;

// Um relógio que diga uma data anterior à da compilação está errado.
const uint32_t EPOCH_MINIMO = epochDaCompilacao(__DATE__);
static const char *const NOME_DIA[] = {"dom", "seg", "ter", "qua", "qui", "sex", "sab"};

Configuracao cfg;  // horários, senhas e rede (guardados na memória)

// ----------------------------------------------------------------- Relógio
bool rtcOk = false;           // DS3231 respondendo
bool horaValida = false;      // DS3231 tem hora confiável (só toca se true)
uint32_t epochUtc = 0;        // segundo atual lido do DS3231
uint32_t inicioSegundoMs = 0; // millis() da última virada de segundo
uint32_t proximaLeituraRtcMs = 0;
uint8_t falhasRtc = 0;
uint32_t ultimoMinutoTocado = 0;

// --------------------------------------------------------------------- GPS
bool gpsComSinal = false;
uint32_t ultimaRmcComFixMs = 0;
uint32_t ultimoDadoGpsMs = 0;
uint32_t contagemSentencas = 0;
uint32_t segundosComFix = 0;  // total desde que ligou
bool mostrarNmea = false;

bool syncPendente = false;
bool syncEstavel = false;
bool syncDoGps = true;  // false = acerto pela hora do celular (página)
int filaNoDolar = 0;    // caracteres já na fila quando chegou o '$' da sentença
uint32_t syncAlvoEpoch = 0;
uint32_t syncAlvoMs = 0;
bool jaSincronizouEstavel = false;
bool horaManual = false;  // acertada pelo comando "hora": o GPS corrige assim que puder
bool algumSyncGps = false;
uint32_t ultimoSyncMs = 0;

// ------------------------------------------------------------------- Toque
struct {
  bool ativo;
  bool releLigado;
  bool aviso;  // ainda falta a batida curta do aviso sonoro
  uint8_t restantes;
  uint32_t duracao;
  uint32_t pausa;
  uint32_t marco;
  uint32_t ligadoDesde;
} toque;

inline uint32_t paraLocal(uint32_t utc) { return utc + (int32_t)FUSO_MINUTOS * 60; }

// ============================================================ Memória
bool salvarConfig() {
  selarConfig(cfg);
  EEPROM.put(0, cfg);
  return EEPROM.commit();
}

void configPadrao() {
  memset(&cfg, 0, sizeof cfg);
  cfg.nHorarios = NUM_HORARIOS;
  memcpy(cfg.horarios, HORARIOS, sizeof(HORARIOS));
  copiarTexto(cfg.apSenha, sizeof cfg.apSenha, AP_SENHA_PADRAO);
  copiarTexto(cfg.adminSenha, sizeof cfg.adminSenha, ADMIN_SENHA_PADRAO);
  copiarTexto(cfg.instNome, sizeof cfg.instNome, REDE_INST_NOME);
  copiarTexto(cfg.instSenha, sizeof cfg.instSenha, REDE_INST_SENHA);
}

void carregarConfig() {
  EEPROM.begin(sizeof(Configuracao));
  EEPROM.get(0, cfg);
  if (configValida(cfg)) {
    Serial.printf("Configuracao lida da memoria: %u horarios\n", cfg.nHorarios);
    return;
  }
  configPadrao();
  salvarConfig();
  Serial.println(F("Memoria vazia: usando os horarios de horarios.h e as senhas do config.h"));
}

void imprimirDataHora(uint32_t epochLocal) {
  const DataHora t = deEpoch(epochLocal);
  Serial.printf("%04u-%02u-%02u %02u:%02u:%02u %s", t.ano, t.mes, t.dia, t.hora, t.min, t.seg,
                NOME_DIA[t.diaSemana]);
}

// ================================================================ Relé/toque
void rele(bool ligar) {
  digitalWrite(PINO_RELE, (ligar != RELE_ATIVO_EM_BAIXO) ? HIGH : LOW);
  toque.releLigado = ligar;
  if (ligar) toque.ligadoDesde = millis();
}

void iniciarToque(uint32_t duracaoMs, uint8_t vezes, bool comAviso) {
  if (duracaoMs == 0) return;
  if (duracaoMs > DURACAO_MAX_MS) duracaoMs = DURACAO_MAX_MS;
  toque.ativo = true;
  toque.aviso = comAviso;
  toque.restantes = vezes ? vezes : 1;
  toque.duracao = duracaoMs;
  toque.pausa = INTERVALO_BATIDAS_MS;
  toque.marco = millis();
  rele(true);
}

// O relógio está há tempo demais sem ser acertado? (conta desde que ligou, se
// ainda não acertou nenhuma vez)
bool semAcertoHaMuitoTempo() {
  const uint32_t desde = algumSyncGps ? ultimoSyncMs : 0;
  return millis() - desde > ALERTA_SEM_GPS_H * 3600000UL;
}

bool avisoSonoroAtivo() { return AVISO_SONORO && semAcertoHaMuitoTempo(); }

void atualizarToque() {
  const uint32_t agora = millis();
  if (toque.releLigado && agora - toque.ligadoDesde >= DURACAO_MAX_MS) {  // segurança
    rele(false);
    toque.ativo = false;
    Serial.println(F("AVISO: rele desligado pelo limite de seguranca"));
    return;
  }
  if (!toque.ativo) return;
  if (toque.releLigado) {
    if (agora - toque.marco >= toque.duracao) {
      rele(false);
      toque.marco += toque.duracao;
      if (--toque.restantes == 0) {
        if (toque.aviso) {  // aviso sonoro: pausa curta e uma batida curta
          toque.aviso = false;
          toque.restantes = 1;
          toque.duracao = AVISO_BATIDA_MS;
          toque.pausa = AVISO_PAUSA_MS;
        } else {
          toque.ativo = false;
        }
      }
    }
  } else if (agora - toque.marco >= toque.pausa) {
    toque.marco += toque.pausa;
    rele(true);
  }
}

// =================================================================== Status
void imprimirStatus() {
  if (horaValida) {
    imprimirDataHora(paraLocal(epochUtc));
  } else {
    Serial.print(F("SEM HORA VALIDA (nao vai tocar)"));
  }
  Serial.print(F(" | RTC: "));
  Serial.print(rtcOk ? F("ok") : F("NAO RESPONDE"));
  Serial.print(F(" | GPS: "));
  if (millis() - ultimoDadoGpsMs > 3000) {
    Serial.print(F("SEM DADOS (confira TX do GPS -> GPIO14)"));
  } else if (gpsComSinal) {
    Serial.printf("com sinal (%lu s no total)", (unsigned long)segundosComFix);
  } else {
    Serial.print(F("procurando satelites"));
  }
  Serial.print(F(" | ultimo acerto GPS: "));
  if (algumSyncGps) {
    Serial.printf("ha %lu min", (unsigned long)((millis() - ultimoSyncMs) / 60000UL));
  } else {
    Serial.print(F("nenhum desde que ligou"));
  }
  if (avisoSonoroAtivo()) Serial.print(F(" | AVISO SONORO LIGADO"));
  if (horaValida) {
    const uint32_t prox = proximoToque(cfg.horarios, cfg.nHorarios, paraLocal(epochUtc));
    if (prox) {
      Serial.print(F(" | proximo toque: "));
      imprimirDataHora(prox);
    }
  }
  Serial.println();
}

void listarHorarios() {
  Serial.printf("%u horarios (hora local, fuso %d min):\n", cfg.nHorarios, FUSO_MINUTOS);
  for (uint8_t i = 0; i < cfg.nHorarios; i++) {
    const Toque &h = cfg.horarios[i];
    Serial.printf("  %02u:%02u  %5u ms x%u  ", h.hora, h.minuto, h.duracaoMs, h.vezes);
    for (uint8_t d = 0; d < 7; d++)
      if (h.dias & (1 << d)) Serial.printf("%s ", NOME_DIA[d]);
    Serial.println();
  }
}

void ajuda() {
  Serial.println(F("Comandos (digite e tecle Enter, 115200 baud):"));
  Serial.println(F("  status                       mostra hora, GPS e proximo toque"));
  Serial.println(F("  horarios                     lista a tabela de horarios"));
  Serial.println(F("  toque                        toque de teste"));
  Serial.println(F("  gps                          liga/desliga a exibicao do NMEA cru"));
  Serial.println(F("  hora AAAA-MM-DD HH:MM:SS     acerta a hora local na mao (teste)"));
  Serial.println(F("  wifi                         rede do sino, rede da instituicao e MAC"));
  Serial.println(F("  senhas-padrao                volta as senhas do config.h e reinicia"));
}

// =================================================================== Relógio
void verificarRtc() {
  bool perdeu = true;
  uint32_t e = 0;
  rtcOk = ds3231::iniciar() && ds3231::perdeuHora(perdeu);
  const bool leu = rtcOk && ds3231::ler(e);
  horaValida = rtcOk && !perdeu && leu && e >= EPOCH_MINIMO;
  if (leu) {
    epochUtc = e;
    inicioSegundoMs = millis();
  }
  if (!rtcOk) {
    Serial.println(F("ERRO: DS3231 nao responde (confira SDA=GPIO4, SCL=GPIO5)"));
  } else if (!horaValida) {
    Serial.println(F("RTC sem hora valida: aguardando o GPS (ou use o comando 'hora')"));
  }
}

void novoSegundo() {
  if (!horaValida) return;
  if (epochUtc < EPOCH_MINIMO) {
    horaValida = false;
    return;
  }
  const uint32_t local = paraLocal(epochUtc);
  const int i = verificarAgenda(cfg.horarios, cfg.nHorarios, local, ultimoMinutoTocado, JANELA_TOQUE_S);
  if (i >= 0) {
    const bool aviso = avisoSonoroAtivo();
    iniciarToque(cfg.horarios[i].duracaoMs, cfg.horarios[i].vezes, aviso);
    Serial.print(F(">>> TOQUE "));
    imprimirDataHora(local);
    if (aviso) Serial.print(F("  (com aviso sonoro: relogio sem acerto ha muito tempo)"));
    Serial.println();
  }
  if (local % 60 == 0) imprimirStatus();
}

void atualizarRelogio() {
  const uint32_t agora = millis();
  if ((int32_t)(agora - proximaLeituraRtcMs) < 0) return;
  proximaLeituraRtcMs = agora + 5;  // ~200 leituras/s: acha a virada do segundo com erro < 5 ms

  uint32_t e;
  if (!ds3231::ler(e)) {
    if (falhasRtc < 255 && ++falhasRtc == 20) {
      rtcOk = false;
      Serial.println(F("ERRO: DS3231 parou de responder"));
    }
    return;
  }
  falhasRtc = 0;
  if (!rtcOk) verificarRtc();  // voltou a responder
  if (e == epochUtc) return;
  epochUtc = e;
  inicioSegundoMs = agora;
  novoSegundo();
}

// ======================================================================= GPS
void processarRmc(const LeituraRMC &rmc, uint32_t inicioMs) {
  if (!rmc.comFix) return;
  const uint32_t utc = corrigirDataGps(paraEpoch(rmc.utc), EPOCH_MINIMO);
  if (utc == 0) return;

  ultimaRmcComFixMs = millis();
  segundosComFix++;
  if (!gpsComSinal) {
    gpsComSinal = true;
    Serial.println(F("GPS: sinal OK"));
  }

  if (filaNoDolar > 40) return;  // o laço atrasou demais: a hora de chegada não é confiável

  const bool estavel = segundosComFix >= GPS_ESTAVEL_MIN * 60UL;
  const bool precisa =
      !horaValida || horaManual ||
      (estavel && (!jaSincronizouEstavel || millis() - ultimoSyncMs >= INTERVALO_SYNC_MIN * 60000UL));
  if (!precisa || syncPendente) return;

  // O '$' desta sentença chegou 'desde' ms depois do segundo 'utc'.
  // Agenda a gravação do RTC para a próxima virada de segundo.
  const uint32_t desde = rmc.fracMs + GPS_ATRASO_NMEA_MS;
  syncAlvoEpoch = utc + 1 + desde / 1000;
  syncAlvoMs = inicioMs + (1000 - desde % 1000);
  syncEstavel = estavel;
  syncDoGps = true;
  syncPendente = true;
}

void executarSyncPendente() {
  if (!syncPendente) return;
  const int32_t atraso = (int32_t)(millis() - syncAlvoMs);
  if (atraso < 0) return;
  syncPendente = false;
  if (atraso > 20) return;  // perdeu o momento certo; tenta no próximo segundo

  const bool eraConfiavel = horaValida && !horaManual;
  const int32_t diferenca = (int32_t)(syncAlvoEpoch - epochUtc);  // 0 ou 1 = RTC já estava certo
  if (!ds3231::gravar(syncAlvoEpoch)) {
    static uint32_t ultimoErroMs = 0;
    if (millis() - ultimoErroMs > 60000UL || ultimoErroMs == 0) {
      ultimoErroMs = millis();
      Serial.println(F("ERRO: nao consegui gravar o DS3231"));
    }
    return;
  }
  horaValida = true;
  proximaLeituraRtcMs = millis();  // lê o RTC de novo já
  if (!syncDoGps) {  // acerto pela hora do aparelho (página)
    horaManual = true;
    ultimoMinutoTocado = paraLocal(syncAlvoEpoch) / 60;  // não toca no minuto do acerto
    Serial.print(F("RTC acertado pela hora do aparelho: "));
    imprimirDataHora(paraLocal(syncAlvoEpoch));
    Serial.println();
    return;
  }
  horaManual = false;
  algumSyncGps = true;
  ultimoSyncMs = millis();
  if (syncEstavel) jaSincronizouEstavel = true;

  Serial.print(F("RTC acertado pelo GPS: "));
  imprimirDataHora(paraLocal(syncAlvoEpoch));
  if (!eraConfiavel) {
    Serial.print(syncEstavel ? F(" (primeiro acerto)") : F(" (provisorio, confirma em ~15 min)"));
  } else if (diferenca > 1) {
    Serial.printf(" (RTC estava ~%ld s atrasado)", (long)(diferenca - 1));
  } else if (diferenca < 0) {
    Serial.printf(" (RTC estava ~%ld s adiantado)", (long)(-diferenca));
  }
  Serial.println();
}

void lerGps() {
  while (gpsSerial.available()) {
    const char c = gpsSerial.read();
    uint32_t agora = millis();
    if (c == '$') {
      // Se o laço demorou (ex.: enviando a página), o '$' chegou antes de ser lido:
      // desconta o tempo dos caracteres que já estão na fila (10 bits cada).
      filaNoDolar = gpsSerial.available();
      agora -= (uint32_t)filaNoDolar * 10000UL / GPS_BAUD;
    }
    if (mostrarNmea) Serial.write(c);
    LeituraRMC rmc;
    uint32_t inicioMs;
    if (nmea.alimentar(c, agora, rmc, inicioMs)) processarRmc(rmc, inicioMs);
  }
  if (nmea.sentencasOk != contagemSentencas) {
    contagemSentencas = nmea.sentencasOk;
    ultimoDadoGpsMs = millis();
  }
  if (gpsComSinal && millis() - ultimaRmcComFixMs > 3000) {
    gpsComSinal = false;
    Serial.println(F("GPS: sem sinal (o RTC segue mantendo a hora)"));
  }
}

// ============================================================ LED e botão
// LED do ESP-12, piscando junto com a virada do segundo:
//   1 piscada por segundo ...... tudo certo (acertado pelo GPS recentemente)
//   2 piscadas por segundo ..... funcionando só pelo RTC (veja a antena do GPS)
//   piscando rápido ............ SEM HORA: não vai tocar
//   aceso direto ............... tocando
void atualizarLed() {
  const uint32_t agora = millis();
  const uint32_t fase = agora - inicioSegundoMs;
  bool aceso;
  if (toque.releLigado) {
    aceso = true;
  } else if (!rtcOk || !horaValida) {
    aceso = (agora / 100) % 2;
  } else if (!algumSyncGps || agora - ultimoSyncMs > ALERTA_SEM_GPS_H * 3600000UL) {
    aceso = fase < 60 || (fase >= 200 && fase < 260);
  } else {
    aceso = fase < 60;
  }
  digitalWrite(PINO_LED, aceso ? LOW : HIGH);
}

void atualizarBotao() {
  static bool ultimoLido = false, estado = false;
  static uint32_t mudouEm = 0;
  const bool lido = digitalRead(PINO_BOTAO_TESTE) == LOW;
  if (lido != ultimoLido) {
    ultimoLido = lido;
    mudouEm = millis();
  } else if (lido != estado && millis() - mudouEm > 50) {
    estado = lido;
    if (estado) {
      Serial.println(F("Botao: toque de teste"));
      iniciarToque(DURACAO_TESTE_MS, 1, false);
    }
  }
}

// ========================================================= Comandos serial
void acertarNaMao(const char *texto) {
  int a, me, d, h, mi, s;
  if (sscanf(texto, "%d-%d-%d %d:%d:%d", &a, &me, &d, &h, &mi, &s) != 6) {
    Serial.println(F("Formato: hora 2026-11-01 07:00:00"));
    return;
  }
  const DataHora t = {(uint16_t)a, (uint8_t)me, (uint8_t)d, (uint8_t)h, (uint8_t)mi, (uint8_t)s, 0};
  if (!dataHoraValida(t)) {
    Serial.println(F("Data/hora invalida"));
    return;
  }
  const uint32_t utc = paraEpoch(t) - (int32_t)FUSO_MINUTOS * 60;
  if (utc < EPOCH_MINIMO) {
    Serial.println(F("Data anterior a da compilacao do firmware: recusada"));
    return;
  }
  if (!ds3231::gravar(utc)) {
    Serial.println(F("ERRO: nao consegui gravar o DS3231"));
    return;
  }
  horaValida = true;
  horaManual = true;
  ultimoMinutoTocado = paraLocal(utc) / 60;  // não toca no minuto em que foi acertado
  proximaLeituraRtcMs = millis();
  Serial.println(F("Hora acertada (o GPS vai corrigir quando tiver sinal)"));
}

void executarComando(const char *cmd) {
  if (!strcmp(cmd, "status")) {
    imprimirStatus();
  } else if (!strcmp(cmd, "horarios")) {
    listarHorarios();
  } else if (!strcmp(cmd, "toque")) {
    Serial.println(F("Toque de teste"));
    iniciarToque(DURACAO_TESTE_MS, 1, false);
  } else if (!strcmp(cmd, "gps")) {
    mostrarNmea = !mostrarNmea;
  } else if (!strncmp(cmd, "hora ", 5)) {
    acertarNaMao(cmd + 5);
  } else if (!strcmp(cmd, "wifi")) {
    imprimirRede();
  } else if (!strcmp(cmd, "senhas-padrao")) {
    copiarTexto(cfg.apSenha, sizeof cfg.apSenha, AP_SENHA_PADRAO);
    copiarTexto(cfg.adminSenha, sizeof cfg.adminSenha, ADMIN_SENHA_PADRAO);
    salvarConfig();
    Serial.println(F("Senhas do Wi-Fi e da pagina voltaram as do config.h. Reiniciando..."));
    delay(200);
    ESP.restart();
  } else {
    ajuda();
  }
}

void lerSerial() {
  static char linha[48];
  static uint8_t n = 0;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r' || c == '\n') {
      linha[n] = 0;
      if (n) executarComando(linha);
      n = 0;
    } else if (n < sizeof(linha) - 1) {
      linha[n++] = c;
    }
  }
}

// ==================================================================== main
void setup() {
  rele(false);  // relé desligado antes de qualquer outra coisa
  pinMode(PINO_RELE, OUTPUT);
  rele(false);
  pinMode(PINO_LED, OUTPUT);
  digitalWrite(PINO_LED, HIGH);
  pinMode(PINO_BOTAO_TESTE, INPUT_PULLUP);

  Serial.begin(115200);
  gpsSerial.begin(GPS_BAUD);
  Wire.begin(PINO_SDA, PINO_SCL);
  Wire.setClock(400000);
  delay(100);

  Serial.println();
  Serial.println(F("=== Sino GPS ==="));
  carregarConfig();
  verificarRtc();
  if (horaValida) {
    // Se o ESP reiniciou no meio de um toque, não toca de novo no mesmo minuto.
    ultimoMinutoTocado = paraLocal(epochUtc) / 60;
    imprimirStatus();
  }
  iniciarRede();
  iniciarWeb();
  imprimirRede();
  ajuda();
}

void loop() {
  lerGps();
  executarSyncPendente();
  atualizarRelogio();
  atualizarToque();
  atualizarBotao();
  atualizarLed();
  lerSerial();
  atualizarWeb();

  static uint32_t ultimoAvisoMs = 0;  // sem hora: mostra o estado a cada 10 s
  if (!horaValida && millis() - ultimoAvisoMs >= 10000) {
    ultimoAvisoMs = millis();
    imprimirStatus();
  }
}
