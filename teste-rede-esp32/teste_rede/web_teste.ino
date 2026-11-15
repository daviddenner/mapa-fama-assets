// ============================================================================
//  Página pelo Wi-Fi do próprio ESP, fila de comandos, relatórios guardados
//  na memória, LED e botão. Com isto o teste roda sem computador: só com o
//  celular (ou nem isso: botão + LED).
// ============================================================================
#include <LittleFS.h>

bool fsOk = false;

// ------------------------------------------------------------ fila de comandos
// Os comandos (do monitor serial, da página ou do botão) rodam numa tarefa
// separada, para a página continuar respondendo durante um teste.
void tarefaComandos(void *) {
  static char linha[160];
  for (;;) {
    if (xQueueReceive(filaCmd, linha, portMAX_DELAY) == pdTRUE) {
      ocupado = true;
      pararPedido = false;
      executar(linha);
      ocupado = false;
    }
  }
}

void enfileirar(const char *c) {
  char l[160];
  snprintf(l, sizeof l, "%s", c);
  if (!strcasecmp(l, "parar")) {  // vale na hora, sem esperar a fila
    if (ocupado) {
      pararPedido = true;
      saida.println("> parar  (o que estiver rodando termina o passo atual e para)");
    } else {
      saida.println("> parar  (nada em andamento)");
    }
    return;
  }
  if (xQueueSend(filaCmd, l, 0) != pdTRUE) saida.println("(muitos comandos na fila; espere terminar)");
}

// ------------------------------------------------------------ relatórios
void iniciarArquivos() {
  fsOk = LittleFS.begin(true);
  if (!fsOk) saida.println("(sem area de arquivos: os relatorios nao serao guardados)");
}

void nomeRelatorio(uint32_t n, char *dst, size_t tam) { snprintf(dst, tam, "/r%lu.txt", (unsigned long)n); }

void salvarRelatorio(size_t desde, const char *titulo) {
  if (!fsOk) return;
  const uint32_t n = prefs.getUInt("nrel", 0) + 1;
  prefs.putUInt("nrel", n);
  char nome[20];
  nomeRelatorio(n, nome, sizeof nome);
  File f = LittleFS.open(nome, "w");
  if (!f) {
    saida.println("(nao consegui gravar o relatorio)");
    return;
  }
  char hora[40];
  textoHoraAgora(hora, sizeof hora);
  f.printf("Relatorio %lu - %s - %s - MAC %s\n", (unsigned long)n, titulo, hora, WiFi.macAddress().c_str());
  if (desde < saida.inicioDisponivel()) {
    f.println("(o comeco se perdeu: saida longa demais)");
    desde = saida.inicioDisponivel();
  }
  char bloco[512];
  const size_t fim = saida.total();
  while (desde < fim) {
    const size_t k = saida.copiar(desde, bloco, sizeof bloco);
    if (!k) break;
    f.write((const uint8_t *)bloco, k);
    desde += k;
  }
  f.close();
  if (n > MAX_RELATORIOS) {
    nomeRelatorio(n - MAX_RELATORIOS, nome, sizeof nome);
    LittleFS.remove(nome);
  }
  saida.printf("(relatorio %lu guardado na memoria do ESP: comando 'relatorio %lu' ou pela pagina)\n", (unsigned long)n,
               (unsigned long)n);
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
  File f = fsOk ? LittleFS.open(nome, "r") : File();
  if (!f) {
    saida.printf("Relatorio %lu nao existe. Veja: relatorios\n", (unsigned long)n);
    return;
  }
  saida.printf("\n======== inicio do relatorio %lu ========\n", (unsigned long)n);
  char bloco[256];
  while (f.available()) {
    const size_t k = f.read((uint8_t *)bloco, sizeof bloco);
    if (!k) break;
    saida.write((const uint8_t *)bloco, k);
  }
  f.close();
  saida.printf("======== fim do relatorio %lu ========\n", (unsigned long)n);
}

void apagarRelatorios() {
  if (!fsOk) return;
  const uint32_t ultimo = prefs.getUInt("nrel", 0);
  for (uint32_t n = ultimo > MAX_RELATORIOS ? ultimo - MAX_RELATORIOS + 1 : 1; n <= ultimo; n++) {
    char nome[20];
    nomeRelatorio(n, nome, sizeof nome);
    LittleFS.remove(nome);
  }
  saida.println("Relatorios apagados.");
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
  char nomeRede[80];
  escaparJson(rede, nomeRede, sizeof nomeRede);
  char b[520];
  snprintf(b, sizeof b,
           "{\"mac\":\"%s\",\"chip\":\"%s\",\"rede\":\"%s\",\"conectado\":%s,\"ip\":\"%s\",\"ocupado\":%s,"
           "\"conclusao\":%u,\"texto\":\"%s\",\"potencia\":\"%s\",\"total\":%lu}",
           WiFi.macAddress().c_str(), ESP.getChipModel(), nomeRede, WiFi.status() == WL_CONNECTED ? "true" : "false",
           txtIp(WiFi.localIP()), ocupado ? "true" : "false", conclusao, textoConclusao(conclusao), textoPotencia(),
           (unsigned long)saida.total());
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
  if (servidor.hasArg("rede")) {
    snprintf(rede, sizeof rede, "%s", servidor.arg("rede").c_str());
    snprintf(senha, sizeof senha, "%s", servidor.arg("senha").c_str());
    salvarRede();
    saida.printf("Pela pagina: rede \"%s\" (%s) salva.\n", rede, senha[0] ? "com senha" : "aberta");
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
  File f = fsOk ? LittleFS.open(nome, "r") : File();
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
    apagarRelatorios();
    servidor.send(200, "text/plain", "ok");
  });
  servidor.onNotFound([]() { servidor.send(404, "text/plain", "nao existe; abra http://192.168.4.1/"); });
  servidor.begin();
}

// ------------------------------------------------------------ LED e botão
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
}

// Botão (BOOT): aperta 1 vez = testar tudo; durante um teste = parar.
void atualizarBotao() {
  static uint32_t desde = 0;
  static bool tratado = false;
  if (digitalRead(PINO_BOTAO) == LOW) {
    if (!desde) {
      desde = millis();
    } else if (!tratado && millis() - desde > 80) {
      tratado = true;
      if (ocupado) {
        enfileirar("parar");
      } else {
        saida.println("(botao apertado: testar)");
        enfileirar("testar");
      }
    }
  } else {
    desde = 0;
    tratado = false;
  }
}
