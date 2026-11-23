// ============================================================================
//  Página pelo Wi-Fi do próprio ESP, fila de comandos, relatórios guardados
//  na memória, LED e botões. Com isto o teste roda sem computador: só com o
//  celular (ou nem isso: botão + LED).
//
//  Tudo roda no mesmo laço: durante um teste, as esperas chamam pausa(), que
//  continua atendendo a página, o LED, os botões e o monitor serial.
// ============================================================================
bool fsOk = false;

// ------------------------------------------------------------ fila de comandos
const uint8_t FILA_CMD = 6;
char filaCmd[FILA_CMD][160];
uint8_t cmdIni = 0, cmdQtd = 0;

void enfileirar(const char *c) {
  if (!strcasecmp(c, "parar")) {  // vale na hora, sem esperar a fila
    if (ocupado) {
      pararPedido = true;
      saida.println(F("> parar  (o que estiver rodando termina o passo atual e para)"));
    } else {
      saida.println(F("> parar  (nada em andamento)"));
    }
    return;
  }
  if (cmdQtd >= FILA_CMD) {
    saida.println(F("(muitos comandos na fila; espere terminar)"));
    return;
  }
  snprintf(filaCmd[(cmdIni + cmdQtd) % FILA_CMD], sizeof filaCmd[0], "%s", c);
  cmdQtd++;
  if (ocupado) SAIDA("(comando '%s' na fila: roda quando o atual terminar)\n", c);
}

bool tirarDaFila(char *dst, size_t tam) {
  if (!cmdQtd) return false;
  snprintf(dst, tam, "%s", filaCmd[cmdIni]);
  cmdIni = (cmdIni + 1) % FILA_CMD;
  cmdQtd--;
  return true;
}

// Página, LED, botões e monitor serial. Chamado no laço e durante as esperas.
void servicoFundo() {
  static bool dentro = false;
  if (dentro) return;  // sem reentrada
  dentro = true;
  lerSerial();
  servidor.handleClient();
  atualizarLed();
  atualizarBotoes();
  dentro = false;
}

// Espera 'ms' sem deixar a página, o LED e os botões parados.
void pausa(uint32_t ms) {
  const uint32_t inicio = millis();
  do {
    servicoFundo();
    delay(1);
  } while (millis() - inicio < ms);
}

// ------------------------------------------------------------ relatórios
// Durante um teste, tudo o que aparece na saída também vai para um arquivo.
File arqRelatorio;
uint32_t relAtual = 0;

void iniciarArquivos() {
  fsOk = montarArquivos();
  if (!fsOk) saida.println(F("(sem area de arquivos: rede e relatorios nao serao guardados)"));
}

void nomeRelatorio(uint32_t n, char *dst, size_t tam) { snprintf(dst, tam, "/r%lu.txt", (unsigned long)n); }

void abrirRelatorio(const char *titulo) {
  if (!fsOk || arqRelatorio) return;
  relAtual = prefs.getUInt("nrel", 0) + 1;
  prefs.putUInt("nrel", relAtual);
  char nome[20];
  if (relAtual > MAX_RELATORIOS) {  // apaga o mais antigo
    nomeRelatorio(relAtual - MAX_RELATORIOS, nome, sizeof nome);
    LittleFS.remove(nome);
  }
  nomeRelatorio(relAtual, nome, sizeof nome);
  arqRelatorio = LittleFS.open(nome, "w");
  if (!arqRelatorio) {
    saida.println(F("(nao consegui criar o relatorio na memoria)"));
    return;
  }
  char hora[48];
  textoHoraAgora(hora, sizeof hora);
  arqRelatorio.printf("Relatorio %lu - %s - %s - %s MAC %s\n", (unsigned long)relAtual, titulo, hora, NOME_CHIP,
                      WiFi.macAddress().c_str());
  saida.copiarPara(&arqRelatorio);
}

void sincronizarRelatorio() {
  if (arqRelatorio) arqRelatorio.flush();
}

void fecharRelatorio() {
  if (!arqRelatorio) return;
  saida.copiarPara(nullptr);
  char hora[48];
  textoHoraAgora(hora, sizeof hora);
  arqRelatorio.printf("(fim do relatorio %lu: %s)\n", (unsigned long)relAtual, hora);
  arqRelatorio.close();
  SAIDA("(relatorio %lu guardado na memoria do ESP: comando 'relatorio %lu' ou pela pagina)\n",
               (unsigned long)relAtual, (unsigned long)relAtual);
}

void listarRelatorios(Print &p, bool paraPagina) {
  if (!fsOk) return;
  const uint32_t ultimo = prefs.getUInt("nrel", 0);
  const uint32_t primeiro = ultimo > MAX_RELATORIOS ? ultimo - MAX_RELATORIOS + 1 : 1;
  int achou = 0;
  for (uint32_t n = primeiro; n <= ultimo; n++) {
    char nome[20];
    nomeRelatorio(n, nome, sizeof nome);
    File f = LittleFS.open(nome, "r");
    if (!f) continue;
    char linha[120];
    const size_t k = f.readBytesUntil('\n', linha, sizeof linha - 1);
    linha[k] = 0;
    if (paraPagina) {
      p.printf("%lu|%lu|%s\n", (unsigned long)n, (unsigned long)f.size(), linha);
    } else {
      p.printf("  %s (%lu bytes)\n", linha, (unsigned long)f.size());
    }
    f.close();
    achou++;
  }
  if (!achou && !paraPagina) p.println("  nenhum relatorio guardado. Rode: testar");
  if (achou && !paraPagina) p.println("  Para ver um: relatorio NUMERO");
}

void mostrarRelatorio(uint32_t n) {
  char nome[20];
  nomeRelatorio(n, nome, sizeof nome);
  File f;
  if (fsOk) f = LittleFS.open(nome, "r");
  if (!f) {
    SAIDA("Relatorio %lu nao existe. Veja: relatorios\n", (unsigned long)n);
    return;
  }
  SAIDA("\n======== inicio do relatorio %lu ========\n", (unsigned long)n);
  char bloco[256];
  while (f.available()) {
    const size_t k = f.read((uint8_t *)bloco, sizeof bloco);
    if (!k) break;
    saida.write((const uint8_t *)bloco, k);
    servicoFundo();
  }
  f.close();
  SAIDA("======== fim do relatorio %lu ========\n", (unsigned long)n);
}

void apagarRelatorios() {
  if (!fsOk) return;
  if (arqRelatorio) {
    saida.println(F("Espere o teste terminar para apagar os relatorios."));
    return;
  }
  const uint32_t ultimo = prefs.getUInt("nrel", 0);
  for (uint32_t n = ultimo > MAX_RELATORIOS ? ultimo - MAX_RELATORIOS + 1 : 1; n <= ultimo; n++) {
    char nome[20];
    nomeRelatorio(n, nome, sizeof nome);
    LittleFS.remove(nome);
  }
  saida.println(F("Relatorios apagados."));
}

// ------------------------------------------------------------ página
void escaparJson(const char *s, char *dst, size_t tam) {
  size_t n = 0;
  for (; *s && n + 2 < tam; s++) {
    if (*s == '"' || *s == '\\') dst[n++] = '\\';
    dst[n++] = (*s < 32) ? ' ' : *s;
  }
  dst[n] = 0;
}

void enviarPagina() {
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send_P(200, "text/html; charset=utf-8", PAGINA_TESTE);
}

void enviarLog() {
  size_t desde = servidor.hasArg("desde") ? strtoul(servidor.arg("desde").c_str(), NULL, 10) : 0;
  const size_t fim = saida.total();
  if (desde < saida.inicioDisponivel() || desde > fim) desde = saida.inicioDisponivel();
  servidor.sendHeader("X-Total", String((unsigned long)fim));
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.setContentLength(fim - desde);
  servidor.send(200, "text/plain; charset=utf-8", "");
  char bloco[512];
  while (desde < fim) {
    const size_t k = saida.copiar(desde, bloco, sizeof bloco);
    if (!k) break;
    servidor.sendContent(bloco, k);
    desde += k;
  }
}

void enviarEstado() {
  char nomeRede[80], rtcJson[100];
  escaparJson(rede, nomeRede, sizeof nomeRede);
  escaparJson(situacaoRtc, rtcJson, sizeof rtcJson);
  char b[640];
  snprintf(b, sizeof b,
           "{\"mac\":\"%s\",\"chip\":\"%s\",\"rede\":\"%s\",\"conectado\":%s,\"ip\":\"%s\",\"ocupado\":%s,"
           "\"conclusao\":%u,\"texto\":\"%s\",\"potencia\":\"%s\",\"pot\":\"%s\",\"rtc\":\"%s\",\"rele\":%lu,"
           "\"total\":%lu}",
           WiFi.macAddress().c_str(), NOME_CHIP, nomeRede, WiFi.status() == WL_CONNECTED ? "true" : "false",
           txtIp(WiFi.localIP()), ocupado ? "true" : "false", conclusao, textoConclusao(conclusao), textoPotencia(),
           potencia == 17 ? "8.5" : potencia == 22 ? "11" : potencia == 30 ? "15" : "max", rtcJson,
           (unsigned long)vezesRele, (unsigned long)saida.total());
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send(200, "application/json", b);
}

void receberComando() {
  if (!servidor.hasArg("c")) {
    servidor.send(400, "text/plain", "falta o comando");
    return;
  }
  enfileirar(servidor.arg("c").c_str());
  servidor.send(200, "text/plain", "ok");
}

void receberRede() {
  if (ocupado) {
    saida.println(F("(pagina: espere o teste terminar para mudar a rede)"));
    servidor.send(409, "text/plain", "ocupado");
    return;
  }
  if (servidor.hasArg("rede")) {
    snprintf(rede, sizeof rede, "%s", servidor.arg("rede").c_str());
    snprintf(senha, sizeof senha, "%s", servidor.arg("senha").c_str());
    salvarRede();
    SAIDA("Pela pagina: rede \"%s\" (%s) salva.\n", rede, senha[0] ? "com senha" : "aberta");
  }
  if (servidor.hasArg("potencia")) definirPotencia(servidor.arg("potencia").c_str());
  servidor.send(200, "text/plain", "ok");
}

void enviarRelatorios() {
  String s;
  struct Coletor : public Print {
    String &s;
    explicit Coletor(String &x) : s(x) {}
    size_t write(uint8_t c) override {
      s += (char)c;
      return 1;
    }
  } coletor(s);
  listarRelatorios(coletor, true);
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.send(200, "text/plain; charset=utf-8", s);
}

void enviarRelatorio() {
  const uint32_t n = servidor.hasArg("n") ? strtoul(servidor.arg("n").c_str(), NULL, 10) : 0;
  char nome[20];
  nomeRelatorio(n, nome, sizeof nome);
  File f;
  if (fsOk) f = LittleFS.open(nome, "r");
  if (!f) {
    servidor.send(404, "text/plain", "relatorio nao existe");
    return;
  }
  servidor.sendHeader("Cache-Control", "no-store");
  servidor.streamFile(f, "text/plain; charset=utf-8");
  f.close();
}

void iniciarPagina() {
  servidor.on("/", HTTP_GET, enviarPagina);
  servidor.on("/api/log", HTTP_GET, enviarLog);
  servidor.on("/api/estado", HTTP_GET, enviarEstado);
  servidor.on("/api/cmd", HTTP_POST, receberComando);
  servidor.on("/api/rede", HTTP_POST, receberRede);
  servidor.on("/api/relatorios", HTTP_GET, enviarRelatorios);
  servidor.on("/relatorio", HTTP_GET, enviarRelatorio);
  servidor.on("/api/apagar", HTTP_POST, []() {
    enfileirar("apagar-relatorios");
    servidor.send(200, "text/plain", "ok");
  });
  servidor.onNotFound([]() { servidor.send(404, "text/plain", "nao existe; abra http://192.168.4.1/"); });
  servidor.begin();
}

// ------------------------------------------------------------ LED e botões
// LED: piscando rapido = trabalhando; parado = pisca N vezes a cada 3 s, N = conclusao
// (1 internet liberada, 2 NTP interno sem login, 3 so NTP da internet, 4 so Date,
//  5 nenhuma fonte, 6 nao conectou). Apagado = ainda sem teste.
void atualizarLed() {
  const uint32_t agora = millis();
  bool ligar = false;
  if (ocupado) {
    ligar = (agora / 100) % 2;
  } else if (conclusao) {
    const uint32_t ciclo = agora % 3000;
    ligar = ciclo / 300 < conclusao && ciclo % 300 < 150;
  }
  digitalWrite(PINO_LED, ligar ? LED_ACESO : !LED_ACESO);
  // LEDs de estado: verde = a rede dá a hora (conclusão 1 a 3); vermelho = não dá (4 a 6)
  if (!ledsEmTeste) {
    const bool temConclusao = !ocupado && conclusao;
    digitalWrite(PINO_LED_VERDE, temConclusao && conclusao <= 3 ? HIGH : LOW);
    digitalWrite(PINO_LED_VERMELHO, temConclusao && conclusao >= 4 ? HIGH : LOW);
  }
}

// Um botão até o GND: devolve true uma vez a cada aperto (com filtro de 80 ms).
struct Botao {
  uint8_t pino;
  uint32_t desde;
  bool tratado;
  bool apertou() {
    if (digitalRead(pino) == LOW) {
      if (!desde) {
        desde = millis() | 1;
      } else if (!tratado && millis() - desde > 80) {
        tratado = true;
        return true;
      }
    } else {
      desde = 0;
      tratado = false;
    }
    return false;
  }
};
Botao botaoTeste = {PINO_BOTAO_TESTE, 0, false};
Botao botaoRele = {PINO_BOTAO_RELE, 0, false};

// FLASH/BOOT: aperta 1 vez = testar tudo; durante um teste = parar.
// S1: testa o relé (fora de um teste).
void atualizarBotoes() {
  if (botaoTeste.apertou()) {
    if (ocupado) {
      enfileirar("parar");
    } else {
      saida.println(F("(botao FLASH/BOOT apertado: testar)"));
      enfileirar("testar");
    }
  }
  if (botaoRele.apertou()) {
    if (ocupado) {
      saida.println(F("(botao S1: espere o teste terminar para testar o rele)"));
    } else {
      saida.println(F("(botao S1 apertado: rele)"));
      enfileirar("rele");
    }
  }
}
