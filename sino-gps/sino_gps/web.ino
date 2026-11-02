// ============================================================================
//  Wi-Fi e página de configuração do sino
//  (aba do sketch: a IDE junta este arquivo com sino_gps.ino)
//
//  - Rede própria do sino: sempre ligada, com nome oculto e senha.
//  - Rede da instituição (opcional): quando o MAC do sino estiver cadastrado,
//    a mesma página abre também por ela.
//  - Página protegida por usuário e senha (login Digest: a senha não passa
//    aberta pela rede) e por um código de sessão em todo pedido de mudança.
// ============================================================================

ESP8266WebServer servidor(80);

char segredoLogin[17];  // sorteado a cada vez que o sino liga
char tokenSessao[17];   // vai junto em todo pedido de mudança
uint8_t falhasLogin = 0;
bool loginBloqueado = false;
uint32_t loginBloqueadoDesdeMs = 0;
uint32_t reiniciarEmMs = 0;

const uint8_t INST_DESLIGADA = 0, INST_TENTANDO = 1, INST_CONECTADA = 2, INST_ESPERANDO = 3;
uint8_t estadoInst = INST_DESLIGADA;
uint32_t marcoInstMs = 0;

void hexAleatorio(char *destino, size_t bytes) {
  for (size_t i = 0; i < bytes; i += 4) snprintf(destino + 2 * i, 9, "%08lx", (unsigned long)ESP.random());
  destino[2 * bytes] = 0;
}

// ================================================================== Wi-Fi
void conectarInst() {
  Serial.printf("Rede da instituicao: conectando em \"%s\"...\n", cfg.instNome);
  WiFi.begin(cfg.instNome, cfg.instSenha);
  estadoInst = INST_TENTANDO;
  marcoInstMs = millis();
}

void iniciarRede() {
  WiFi.persistent(false);  // não grava nada da rede na flash por conta própria
  WiFi.setAutoReconnect(false);
  WiFi.mode(WIFI_AP_STA);
  WiFi.hostname(NOME_NA_REDE);
  if (!WiFi.softAP(AP_NOME, cfg.apSenha, AP_CANAL, AP_OCULTO, 4)) {
    Serial.println(F("ERRO ao criar a rede do sino; usando a senha padrao do config.h"));
    WiFi.softAP(AP_NOME, AP_SENHA_PADRAO, AP_CANAL, AP_OCULTO, 4);
  }
  if (MDNS.begin(NOME_NA_REDE)) MDNS.addService("http", "tcp", 80);
  if (cfg.instNome[0]) conectarInst();
}

void atualizarRede() {
  switch (estadoInst) {
    case INST_TENTANDO:
      if (WiFi.status() == WL_CONNECTED) {
        estadoInst = INST_CONECTADA;
        Serial.printf("Rede da instituicao: conectado, pagina tambem em http://%s ou http://%s.local\n",
                      WiFi.localIP().toString().c_str(), NOME_NA_REDE);
      } else if (millis() - marcoInstMs > 30000UL) {
        WiFi.disconnect();  // para de procurar: a rede do sino fica estável
        estadoInst = INST_ESPERANDO;
        marcoInstMs = millis();
        Serial.printf("Rede da instituicao: nao conectou (MAC cadastrado? senha?). Tento de novo em %d min\n",
                      REDE_INST_ESPERA_MIN);
      }
      break;
    case INST_CONECTADA:
      if (WiFi.status() != WL_CONNECTED) {
        Serial.println(F("Rede da instituicao: a conexao caiu; tento de novo em 30 s"));
        WiFi.disconnect();
        estadoInst = INST_ESPERANDO;
        marcoInstMs = millis() - (REDE_INST_ESPERA_MIN * 60000UL - 30000UL);
      }
      break;
    case INST_ESPERANDO:
      if (millis() - marcoInstMs >= REDE_INST_ESPERA_MIN * 60000UL) conectarInst();
      break;
    default:
      break;
  }
}

const char *textoInst() {
  if (!cfg.instNome[0]) return "nao configurada";
  switch (estadoInst) {
    case INST_TENTANDO: return "tentando conectar";
    case INST_CONECTADA: return "conectado";
    case INST_ESPERANDO: return "nao conectou (tenta de novo de tempos em tempos)";
    default: return "desligada";
  }
}

void imprimirRede() {
  Serial.printf("Wi-Fi do sino: \"%s\" (%s), pagina em http://%s, %u aparelho(s) conectado(s)\n", AP_NOME,
                AP_OCULTO ? "nome oculto" : "nome visivel", WiFi.softAPIP().toString().c_str(),
                WiFi.softAPgetStationNum());
  Serial.printf("Rede da instituicao: %s (%s)", cfg.instNome[0] ? cfg.instNome : "-", textoInst());
  if (estadoInst == INST_CONECTADA) Serial.printf(", IP %s", WiFi.localIP().toString().c_str());
  Serial.println();
  Serial.printf("MAC do sino (para a TI cadastrar): %s\n", WiFi.macAddress().c_str());
}

// ================================================================== Login
const char *metodoTexto() {
  switch (servidor.method()) {
    case HTTP_POST: return "POST";
    case HTTP_PUT: return "PUT";
    case HTTP_DELETE: return "DELETE";
    default: return "GET";
  }
}

bool autorizado() {
  if (loginBloqueado) {
    if (millis() - loginBloqueadoDesdeMs < 60000UL) {
      servidor.send(429, "text/plain; charset=utf-8", "Muitas senhas erradas. Espere 1 minuto.");
      return false;
    }
    loginBloqueado = false;
    falhasLogin = 0;
  }
  int r = DIGEST_NEGADO;
  const String &cab = servidor.header("Authorization");
  if (cab.length()) {
    r = verificarDigest(cab.c_str(), metodoTexto(), servidor.uri().c_str(), ADMIN_USUARIO, cfg.adminSenha,
                        segredoLogin, millis() / 1000);
    if (r == DIGEST_OK) {
      falhasLogin = 0;
      return true;
    }
    if (r == DIGEST_NEGADO && ++falhasLogin >= 10) {
      loginBloqueado = true;
      loginBloqueadoDesdeMs = millis();
      Serial.println(F("Pagina: 10 senhas erradas seguidas, login bloqueado por 1 minuto"));
    }
  }
  char desafio[200];
  desafioDigest(segredoLogin, millis() / 1000, r == DIGEST_VENCIDO, desafio, sizeof desafio);
  servidor.sendHeader("WWW-Authenticate", desafio);
  servidor.send(401, "text/plain; charset=utf-8", "Precisa de usuario e senha.");
  return false;
}

bool tokenOk() {
  if (servidor.arg("token") == tokenSessao) return true;
  servidor.send(403, "text/plain; charset=utf-8", "Pagina desatualizada: recarregue a pagina.");
  return false;
}

// ================================================================ Rotas
void paginaInicial() {
  if (!autorizado()) return;
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send_P(200, PSTR("text/html; charset=utf-8"), PAGINA);
}

void apiEstado() {
  if (!autorizado()) return;
  char ap[70], inst[70], prox[48], ip[20] = "", nome[40];
  jsonTexto(ap, sizeof ap, AP_NOME);
  jsonTexto(inst, sizeof inst, cfg.instNome);
  jsonTexto(nome, sizeof nome, NOME_NA_REDE);
  if (estadoInst == INST_CONECTADA) snprintf(ip, sizeof ip, "%s", WiFi.localIP().toString().c_str());

  const char *estado = (!rtcOk || !horaValida) ? "semhora"
                       : (!algumSyncGps || millis() - ultimoSyncMs > ALERTA_SEM_GPS_H * 3600000UL) ? "rtc"
                                                                                                   : "ok";
  const char *gps = millis() - ultimoDadoGpsMs > 3000 ? "sem dados (confira a ligacao do GPS)"
                    : gpsComSinal                       ? "com sinal"
                                                        : "procurando satelites";
  const long sync = algumSyncGps ? (long)((millis() - ultimoSyncMs) / 60000UL) : -1L;
  const uint32_t local = paraLocal(epochUtc);
  uint32_t fracao = millis() - inicioSegundoMs;
  if (fracao > 999) fracao = 999;
  prox[0] = 0;
  if (horaValida) {
    const uint32_t p = proximoToque(cfg.horarios, cfg.nHorarios, local);
    if (p) {
      const DataHora t = deEpoch(p);
      snprintf(prox, sizeof prox, "%02u:%02u (%s, %02u/%02u)", t.hora, t.min, NOME_DIA[t.diaSemana], t.dia, t.mes);
    }
  }
  char json[700];
  snprintf(json, sizeof json,
           "{\"token\":\"%s\",\"nome\":\"%s\",\"valida\":%s,\"manual\":%s,\"agora\":%lu.%03lu,\"estado\":\"%s\","
           "\"tocando\":%s,\"gps\":\"%s\",\"sync\":%ld,\"proximo\":\"%s\",\"ap\":\"%s\",\"apClientes\":%u,"
           "\"inst\":\"%s\",\"instEstado\":\"%s\",\"instIp\":\"%s\",\"aviso\":%s}",
           tokenSessao, nome, horaValida ? "true" : "false", horaManual ? "true" : "false", (unsigned long)local,
           (unsigned long)fracao, estado, toque.releLigado ? "true" : "false", gps, sync, prox, ap,
           WiFi.softAPgetStationNum(), inst, textoInst(), ip, avisoSonoroAtivo() ? "true" : "false");
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send(200, "application/json", json);
}

void apiHorarios() {
  if (!autorizado()) return;
  if (servidor.method() == HTTP_POST) {
    if (!tokenOk()) return;
    Toque nova[MAX_HORARIOS];
    char erro[80];
    const int n = lerTabela(servidor.arg("tabela").c_str(), nova, MAX_HORARIOS, DURACAO_MAX_MS, erro, sizeof erro);
    if (n < 0) {
      servidor.send(400, "text/plain; charset=utf-8", erro);
      return;
    }
    memcpy(cfg.horarios, nova, sizeof(Toque) * n);
    cfg.nHorarios = (uint8_t)n;
    if (!salvarConfig()) {
      servidor.send(500, "text/plain; charset=utf-8", "Nao consegui gravar na memoria do sino.");
      return;
    }
    Serial.printf("Pagina: nova tabela salva (%d horarios)\n", n);
    servidor.send(200, "text/plain; charset=utf-8", "ok");
    return;
  }
  char txt[MAX_HORARIOS * 24 + 8];
  escreverTabela(cfg.horarios, cfg.nHorarios, txt, sizeof txt);
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send(200, "text/plain; charset=utf-8", txt);
}

void apiTocar() {
  if (!autorizado() || !tokenOk()) return;
  Serial.println(F("Pagina: toque de teste"));
  iniciarToque(DURACAO_TESTE_MS, 1, false);
  servidor.send(200, "text/plain; charset=utf-8", "ok");
}

void apiHora() {
  if (!autorizado() || !tokenOk()) return;
  const uint64_t ms = strtoull(servidor.arg("ms").c_str(), nullptr, 10);  // hora do aparelho (UTC)
  const uint32_t utc = (uint32_t)(ms / 1000);
  if (utc < EPOCH_MINIMO) {
    servidor.send(400, "text/plain; charset=utf-8", "A hora deste aparelho parece errada.");
    return;
  }
  // grava no começo do próximo segundo, para o RTC ficar em fase
  syncAlvoEpoch = utc + 1;
  syncAlvoMs = millis() + (uint32_t)(1000 - ms % 1000);
  syncDoGps = false;
  syncPendente = true;
  Serial.println(F("Pagina: hora acertada pela hora do aparelho (o GPS corrige quando tiver sinal)"));
  servidor.send(200, "text/plain; charset=utf-8", "ok");
}

void apiRede() {
  if (!autorizado() || !tokenOk()) return;
  String instNome = servidor.arg("instNome");
  instNome.trim();
  const String &instSenha = servidor.arg("instSenha");
  const String &apSenha = servidor.arg("apSenha");
  const String &adminSenha = servidor.arg("adminSenha");
  if (instNome.length() > 32 || instSenha.length() > 64 || apSenha.length() > 63 || adminSenha.length() > 32) {
    servidor.send(400, "text/plain; charset=utf-8", "Texto longo demais.");
    return;
  }
  if (apSenha.length() && apSenha.length() < 8) {
    servidor.send(400, "text/plain; charset=utf-8", "A senha do Wi-Fi precisa de pelo menos 8 caracteres.");
    return;
  }
  if (adminSenha.length() && adminSenha.length() < 6) {
    servidor.send(400, "text/plain; charset=utf-8", "A senha da pagina precisa de pelo menos 6 caracteres.");
    return;
  }
  const bool trocouRede = strcmp(instNome.c_str(), cfg.instNome) != 0;
  copiarTexto(cfg.instNome, sizeof cfg.instNome, instNome.c_str());
  if (trocouRede || instSenha.length()) copiarTexto(cfg.instSenha, sizeof cfg.instSenha, instSenha.c_str());
  if (apSenha.length()) copiarTexto(cfg.apSenha, sizeof cfg.apSenha, apSenha.c_str());
  if (adminSenha.length()) copiarTexto(cfg.adminSenha, sizeof cfg.adminSenha, adminSenha.c_str());
  if (!salvarConfig()) {
    servidor.send(500, "text/plain; charset=utf-8", "Nao consegui gravar na memoria do sino.");
    return;
  }
  Serial.println(F("Pagina: rede/senhas alteradas; reiniciando"));
  servidor.send(200, "text/plain; charset=utf-8",
                "Salvo. O sino esta reiniciando: espere uns 10 s e reconecte (com a senha nova, se trocou).");
  reiniciarEmMs = millis() + 1500;
  if (!reiniciarEmMs) reiniciarEmMs = 1;
}

void naoEncontrado() {
  if (!autorizado()) return;
  servidor.send(404, "text/plain; charset=utf-8", "Nao encontrado.");
}

void iniciarWeb() {
  hexAleatorio(segredoLogin, 8);
  hexAleatorio(tokenSessao, 8);
  servidor.on("/", HTTP_GET, paginaInicial);
  servidor.on("/api/estado", HTTP_GET, apiEstado);
  servidor.on("/api/horarios", apiHorarios);  // GET lê, POST grava
  servidor.on("/api/tocar", HTTP_POST, apiTocar);
  servidor.on("/api/hora", HTTP_POST, apiHora);
  servidor.on("/api/rede", HTTP_POST, apiRede);
  servidor.onNotFound(naoEncontrado);
  servidor.begin();
}

void atualizarWeb() {
  servidor.handleClient();
  MDNS.update();
  atualizarRede();
  if (reiniciarEmMs && (int32_t)(millis() - reiniciarEmMs) >= 0) {
    rele(false);
    Serial.println(F("Reiniciando..."));
    delay(100);
    ESP.restart();
  }
}
