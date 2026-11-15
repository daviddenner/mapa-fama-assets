// ============================================================================
//  Hora pela rede (NTP)  (aba do sketch: a IDE junta com sino_gps.ino)
//
//  Com a rede da instituição conectada, a cada INTERVALO_NTP_MIN o sino faz 4
//  perguntas a um servidor de hora e fica com a de menor tempo de ida e volta
//  (a mais precisa). Se o DS3231 estiver fora por mais de NTP_CORRIGE_ACIMA_MS,
//  grava a hora certa nele exatamente na virada do segundo. Quando a rede cai,
//  o DS3231 segura a hora.
//
//  Servidores, em ordem: o digitado na página, o informado pelo DHCP, o
//  roteador, os servidores DNS da rede e os da internet (config.h). Os da
//  própria rede costumam responder mesmo sem login na página da instituição.
//
//  Nada aqui trava o sino: as respostas são esperadas sem parar o resto, e
//  nenhuma conferência começa a menos de 15 s de um toque.
// ============================================================================

WiFiUDP udpNtp;
bool udpNtpAberto = false;
const uint8_t NTP_PARADO = 0, NTP_ESPERANDO = 1;
const uint8_t NTP_AMOSTRAS = 4;
uint8_t estadoNtp = NTP_PARADO;
uint32_t ntpProximaRodadaMs = 0;
uint8_t ntpIndice = 0;  // qual servidor da lista está sendo usado
uint8_t ntpNumCandidatos = 0;
char ntpNome[48] = "";
IPAddress ntpIp;
uint8_t ntpAmostra = 0;
uint8_t marcaNtp[8];
int64_t ntpT1 = 0;
uint32_t ntpEnviadoMs = 0;
bool ntpTemMelhor = false;
int64_t ntpMelhorDesloc = 0, ntpMelhorAtraso = 0;
int ntpProblema = 0;

// Resultado, para o status e a página
bool ntpAlgumOk = false;
uint32_t ntpUltimoOkMs = 0;
int32_t ntpUltimaDifMs = 0;  // DS3231 - hora certa, na última conferência
uint32_t ntpUltimaMargemMs = 0;
char ntpUltimoServidor[48] = "";
char ntpUltimaFalha[80] = "";
uint8_t ntpFalhasSeguidas = 0;
bool ntpMostrar = false;  // comando "ntp": mostra o resultado no monitor serial

void ntpConferirAgora() { ntpProximaRodadaMs = millis(); }

void ntpRepetirLogo() { ntpProximaRodadaMs = millis() + 5000; }

void ntpConferirEMostrar() {
  if (!instConectada()) {
    Serial.println(F("Rede da instituicao nao conectada (veja o comando wifi)"));
    return;
  }
  ntpMostrar = true;
  ntpConferirAgora();
}

// Servidores, em ordem: o da página, o informado pelo DHCP, o roteador, os
// servidores DNS da rede e os do config.h. Os da própria rede vêm primeiro
// porque costumam responder mesmo antes do login na página da instituição
// (o portal de login só intercepta a navegação); os da internet só respondem
// com a internet liberada.
const uint8_t NTP_MAX_CANDIDATOS = 8;

void juntarCandidato(char lista[][48], uint8_t &n, uint8_t max, const char *nome) {
  if (!nome[0] || n >= max) return;
  for (uint8_t i = 0; i < n; i++)
    if (!strcmp(lista[i], nome)) return;  // já está na lista
  copiarTexto(lista[n++], 48, nome);
}

void juntarCandidatoIp(char lista[][48], uint8_t &n, uint8_t max, const IPAddress &ip) {
  if (ip.isSet()) juntarCandidato(lista, n, max, ip.toString().c_str());
}

uint8_t candidatosNtp(char lista[][48], uint8_t max) {
  uint8_t n = 0;
  juntarCandidato(lista, n, max, cfg.ntpServidor);
  juntarCandidatoIp(lista, n, max, *sntp_getserver(0));
  juntarCandidatoIp(lista, n, max, WiFi.gatewayIP());
  juntarCandidatoIp(lista, n, max, WiFi.dnsIP(0));
  juntarCandidatoIp(lista, n, max, WiFi.dnsIP(1));
  const char *fixos[] = {NTP_SERVIDOR_1, NTP_SERVIDOR_2, NTP_SERVIDOR_3};
  for (uint8_t i = 0; i < 3; i++) juntarCandidato(lista, n, max, fixos[i]);
  return n;
}

void falhaNtp(const char *motivo) {
  estadoNtp = NTP_PARADO;
  snprintf(ntpUltimaFalha, sizeof ntpUltimaFalha, "%s (%s)", motivo, ntpNome[0] ? ntpNome : "-");
  const uint8_t n = ntpNumCandidatos;
  if (ntpMostrar) {
    Serial.printf("Rede: nao consegui conferir a hora: %s\n", ntpUltimaFalha);
  } else if (n && ntpFalhasSeguidas + 1 < n) {  // primeira passada pela lista: avisa e segue
    Serial.printf("Rede: %s; tentando o proximo servidor de hora\n", ntpUltimaFalha);
  } else if (ntpFalhasSeguidas + 1 == (n ? n : 1)) {  // a lista inteira falhou
    Serial.printf("Rede: nenhum servidor de hora respondeu (%s); tento de novo em 15 s\n", ntpUltimaFalha);
  }
  ntpMostrar = false;
  if (ntpFalhasSeguidas < 255) ntpFalhasSeguidas++;
  ntpIndice++;  // da próxima vez tenta o próximo servidor
  // Na primeira passada pela lista, passa logo ao próximo; na segunda, de 15 em
  // 15 s; depois, de 5 em 5 min (o DS3231 segura a hora enquanto isso).
  uint32_t espera = 300000UL;
  if (ntpFalhasSeguidas <= n) espera = 1000UL;
  else if (ntpFalhasSeguidas < n + 5) espera = 15000UL;
  ntpProximaRodadaMs = millis() + espera;
}

void enviarAmostraNtp() {
  while (udpNtp.parsePacket() > 0) {
  }  // descarta respostas atrasadas de perguntas anteriores
  uint8_t pedido[48];
  memset(pedido, 0, sizeof pedido);
  pedido[0] = 0x23;  // versão 4, modo cliente
  const uint64_t marca = micros64();
  memcpy(marcaNtp, &marca, 8);
  memcpy(pedido + 40, marcaNtp, 8);  // o servidor devolve isto: reconhecemos a resposta
  udpNtp.beginPacket(ntpIp, 123);
  udpNtp.write(pedido, sizeof pedido);
  ntpT1 = (int64_t)micros64();
  udpNtp.endPacket();
  ntpEnviadoMs = millis();
  ntpAmostra++;
  estadoNtp = NTP_ESPERANDO;
}

void iniciarRodadaNtp() {
  char lista[NTP_MAX_CANDIDATOS][48];
  const uint8_t n = candidatosNtp(lista, NTP_MAX_CANDIDATOS);
  ntpNumCandidatos = n;
  if (n == 0) {
    ntpNome[0] = 0;
    falhaNtp("nenhum servidor configurado");
    return;
  }
  ntpIndice %= n;
  copiarTexto(ntpNome, sizeof ntpNome, lista[ntpIndice]);
  if (!ntpIp.fromString(ntpNome)) {
    if (WiFi.hostByName(ntpNome, ntpIp, 2000) != 1 || !ntpIp.isSet()) {
      falhaNtp("nao achei o endereco (DNS)");
      return;
    }
  }
  if (!udpNtpAberto) udpNtpAberto = udpNtp.begin(4123);
  ntpAmostra = 0;
  ntpTemMelhor = false;
  ntpProblema = 0;
  enviarAmostraNtp();
}

void terminarRodadaNtp() {
  estadoNtp = NTP_PARADO;
  if (!ntpTemMelhor) {
    falhaNtp(ntpProblema ? textoProblemaNtp(ntpProblema) : "sem resposta");
    return;
  }
  if (ntpMelhorAtraso > 500000LL) {
    falhaNtp("rede lenta demais");
    return;
  }
  const int64_t utcAgora = (int64_t)micros64() + ntpMelhorDesloc;
  if (utcAgora / 1000000LL < (int64_t)EPOCH_MINIMO) {
    falhaNtp("hora do servidor absurda");
    return;
  }
  // Compara com o DS3231: segundo lido + tempo desde a virada do segundo
  const bool comparar = rtcOk && horaValida && !horaManual;
  int64_t difUs = 0;
  if (comparar) {
    const int64_t rtcUs = (int64_t)epochUtc * 1000000LL + (int64_t)(uint32_t)(millis() - inicioSegundoMs) * 1000LL;
    difUs = rtcUs - utcAgora;
  }
  const bool corrigir = !comparar || llabs(difUs) > (int64_t)NTP_CORRIGE_ACIMA_MS * 1000LL;
  if (corrigir && !syncPendente && rtcOk) {
    uint32_t alvo;
    int64_t falta;
    proximoSegundoCheio((int64_t)micros64(), ntpMelhorDesloc, alvo, falta);
    syncAlvoEpoch = alvo;
    syncAlvoMs = millis() + (uint32_t)(falta / 1000);
    syncFonte = FONTE_NTP;
    syncPendente = true;
    if (comparar) {
      Serial.printf("Rede: o DS3231 estava %ld ms %s; corrigindo\n", (long)llabs(difUs / 1000),
                    difUs > 0 ? "adiantado" : "atrasado");
    }
  }
  const bool primeira = !algumaConfirmacao;
  const bool voltou = ntpFalhasSeguidas || (algumaConfirmacao && !horaConfirmadaRecente());
  if (rtcOk) confirmarHora(FONTE_NTP);
  if (primeira) {
    Serial.printf("Rede: hora conferida (%s, +-%lu ms)\n", ntpNome, (unsigned long)(ntpMelhorAtraso / 2000));
  } else if (voltou) {
    Serial.printf("Rede: hora conferida de novo (%s); aviso sonoro desligado\n", ntpNome);
  }
  ntpAlgumOk = true;
  ntpUltimoOkMs = millis();
  ntpUltimaDifMs = (int32_t)(difUs / 1000);
  ntpUltimaMargemMs = (uint32_t)(ntpMelhorAtraso / 2000);
  copiarTexto(ntpUltimoServidor, sizeof ntpUltimoServidor, ntpNome);
  ntpFalhasSeguidas = 0;
  ntpUltimaFalha[0] = 0;
  if (ntpMostrar) {
    Serial.printf("Rede: hora conferida em %s (+-%lu ms); DS3231 estava %ld ms %s\n", ntpNome,
                  (unsigned long)ntpUltimaMargemMs, (long)llabs((long)ntpUltimaDifMs),
                  ntpUltimaDifMs > 0 ? "adiantado" : "atrasado");
    ntpMostrar = false;
  }
  ntpProximaRodadaMs = millis() + INTERVALO_NTP_MIN * 60000UL;
}

void proximaAmostraNtp() {
  // 2 perguntas sem resposta nenhuma: servidor mudo, não vale gastar as outras
  const bool mudo = ntpAmostra >= 2 && !ntpTemMelhor && ntpProblema == 0;
  if (ntpAmostra < NTP_AMOSTRAS && !mudo && instConectada()) {
    enviarAmostraNtp();
  } else {
    terminarRodadaNtp();
  }
}

void atualizarNtp() {
  if (estadoNtp == NTP_ESPERANDO) {
    if (udpNtp.parsePacket() > 0) {
      const int64_t t4 = (int64_t)micros64();
      uint8_t resp[64];
      const int n = udpNtp.read(resp, sizeof resp);
      RespostaNtp r;
      const int p = analisarNtp(resp, n, marcaNtp, r);
      if (p == NTP_NAO_E_NOSSA || p == NTP_NAO_E_RESPOSTA) return;  // continua esperando a nossa
      if (p == NTP_OK) {
        int64_t desloc, atraso;
        calcularNtp(ntpT1, t4, r.recebidoUs, r.enviadoUs, desloc, atraso);
        if (!ntpTemMelhor || atraso < ntpMelhorAtraso) {
          ntpTemMelhor = true;
          ntpMelhorDesloc = desloc;
          ntpMelhorAtraso = atraso;
        }
      } else {
        ntpProblema = p;
      }
      proximaAmostraNtp();
    } else if (millis() - ntpEnviadoMs > 1000) {
      proximaAmostraNtp();  // sem resposta em 1 s
    }
    return;
  }
  if (!instConectada()) return;
  if ((int32_t)(millis() - ntpProximaRodadaMs) < 0) return;
  if (toque.ativo || syncPendente || toqueEmBreve(15)) return;  // não atrapalha um toque
  iniciarRodadaNtp();
}

// Uma linha sobre a hora pela rede (status e página)
void textoNtp(char *b, size_t tam) {
  if (!cfg.instNome[0]) {
    snprintf(b, tam, "rede da instituicao nao configurada");
  } else if (!instConectada()) {
    if (ntpAlgumOk) {
      snprintf(b, tam, "rede fora; ultima conferencia ha %lu min",
               (unsigned long)((millis() - ntpUltimoOkMs) / 60000UL));
    } else {
      snprintf(b, tam, "rede da instituicao desconectada");
    }
  } else if (ntpFalhasSeguidas && ntpFalhasSeguidas < ntpNumCandidatos && !ntpAlgumOk) {
    snprintf(b, tam, "procurando um servidor de hora (%u de %u)", (unsigned)(ntpFalhasSeguidas + 1), (unsigned)ntpNumCandidatos);
  } else if (ntpFalhasSeguidas && ntpUltimaFalha[0]) {
    snprintf(b, tam, "falhou: %s", ntpUltimaFalha);
  } else if (ntpAlgumOk) {
    snprintf(b, tam, "ok, %s (+-%lu ms), conferida ha %lu min", ntpUltimoServidor, (unsigned long)ntpUltimaMargemMs,
             (unsigned long)((millis() - ntpUltimoOkMs) / 60000UL));
  } else {
    snprintf(b, tam, "conectada, conferindo a hora...");
  }
}
