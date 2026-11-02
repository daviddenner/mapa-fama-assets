// ============================================================================
//  Teste de rede para o sino (ESP32)
//
//  Descobre se dá para pegar a hora certa pela rede da instituição SEM fazer o
//  login da página (captcha) e sem o MAC estar cadastrado:
//    - mostra o MAC deste ESP32 (para pedir o cadastro à TI);
//    - procura as redes e tenta conectar, explicando o motivo se for recusado;
//    - testa servidores de hora (NTP) da própria rede e da internet;
//    - lê a hora que as páginas (inclusive a de login) informam no cabeçalho;
//    - no fim mostra um resumo com a conclusão.
//  Ele NÃO faz login e não tenta passar pelo captcha.
//
//  Tudo pelo Monitor Serial: 115200 baud, final de linha "Nova linha".
//  Digite "ajuda" para ver os comandos.
//  Placa na Arduino IDE: "ESP32 Dev Module" (pacote esp32 da Espressif, 2.x ou 3.x).
// ============================================================================
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#include "analise.h"
#include "esp_timer.h"
#include "tempo.h"

// ------------------------------------------------------------ Configuração
#define FUSO_MINUTOS (-240)        // Mato Grosso do Sul: UTC-4
#define NOME_NA_REDE "sino-teste"  // nome que aparece para a TI na lista de aparelhos

// Pergunta ao DHCP qual é o servidor de hora da rede. Se der erro de
// compilação nesta parte, mude para 0: o resto do teste continua igual.
#define TESTAR_NTP_DO_DHCP 1

#if TESTAR_NTP_DO_DHCP
#include "esp_sntp.h"
#endif

// ------------------------------------------------------------------ Estado
Preferences prefs;
char rede[33] = "";
char senha[65] = "";

// Eventos do Wi-Fi: chegam por outra tarefa do ESP32 e são lidos no loop.
volatile bool evAssociou = false;
volatile bool evTemIP = false;
volatile uint8_t filaMotivo[16];
volatile uint8_t filaIni = 0, filaFim = 0;

enum TipoFonte { NTP_INTERNO, NTP_EXTERNO, DATE_PAGINA };
struct Fonte {  // uma fonte de hora que funcionou
  char nome[72];
  TipoFonte tipo;
  int64_t deslocUs;     // hora da fonte - relógio do ESP32
  int64_t incertezaUs;  // margem de erro (+-)
};
Fonte fontes[20];
uint8_t nFontes = 0;

struct Resultado {  // só tipos simples: pode ser zerado com memset
  bool rodou, associou, temIP, internet, dnsOk, dnsDesviado;
  int ultimoMotivo;
  uint16_t quedas;
  uint32_t ip;
  char portal[160];   // endereço da página de login, se houver
  char urlData[160];  // página (http) que informou hora, para o refinamento
} res;

// ============================================================ Utilidades
int64_t agoraUs() { return esp_timer_get_time(); }

const char *txtIp(const IPAddress &ip) {
  static char buf[4][16];
  static uint8_t i = 0;
  char *b = buf[i++ % 4];
  snprintf(b, 16, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return b;
}
bool ipVazio(const IPAddress &ip) { return (uint32_t)ip == 0; }
bool ehPrivado(const IPAddress &ip) { return ipPrivado(ip[0], ip[1]); }

void titulo(const char *t) { Serial.printf("\n---- %s ----\n", t); }

void imprimirMs(int64_t us) {
  if (us < 10000) {
    Serial.printf("%.1f ms", us / 1000.0);
  } else {
    Serial.printf("%lld ms", (long long)(us / 1000));
  }
}

void imprimirHora(int64_t deslocUs) {
  const int64_t ms = (agoraUs() + deslocUs) / 1000;  // ms desde 1970 (UTC)
  const DataHora t = deEpoch((uint32_t)(ms / 1000) + FUSO_MINUTOS * 60);
  Serial.printf("%02u/%02u/%04u %02u:%02u:%02u,%03u", t.dia, t.mes, t.ano, t.hora, t.min, t.seg,
                (unsigned)(ms % 1000));
}

// Hora entre 2024 e 2100? (aparelho com relógio desacertado informa 2000, 1970...)
bool horaPlausivel(int64_t deslocUs) {
  const int64_t s = (agoraUs() + deslocUs) / 1000000LL;
  return s >= 1704067200LL && s < 4102444800LL;
}

void adicionarFonte(const char *nome, TipoFonte tipo, int64_t desloc, int64_t incerteza) {
  if (!horaPlausivel(desloc)) {
    Serial.println("      (hora absurda: o relogio desse aparelho esta desacertado; ignorada)");
    return;
  }
  if (nFontes >= sizeof(fontes) / sizeof(fontes[0])) return;
  Fonte &f = fontes[nFontes++];
  snprintf(f.nome, sizeof f.nome, "%s", nome);
  f.tipo = tipo;
  f.deslocUs = desloc;
  f.incertezaUs = incerteza;
}

int melhorFonte() {
  int m = -1;
  for (int i = 0; i < nFontes; i++)
    if (m < 0 || fontes[i].incertezaUs < fontes[m].incertezaUs) m = i;
  return m;
}

void mostrarMac() {
  Serial.printf("MAC deste ESP32: %s   <- e este que a TI cadastra\n", WiFi.macAddress().c_str());
  Serial.printf("Nome na rede:    %s\n", NOME_NA_REDE);
}

// ============================================================ Wi-Fi
void aoEventoWiFi(WiFiEvent_t evento, WiFiEventInfo_t info) {
  switch (evento) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      evAssociou = true;
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      evTemIP = true;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED: {
      evTemIP = false;
      const uint8_t f = filaFim;
      filaMotivo[f % 16] = info.wifi_sta_disconnected.reason;
      filaFim = f + 1;
      break;
    }
    default:
      break;
  }
}

void imprimirQuedas() {
  while (filaIni != filaFim) {
    const uint8_t m = filaMotivo[filaIni % 16];
    filaIni = filaIni + 1;
    res.quedas++;
    res.ultimoMotivo = m;
    Serial.printf("  recusado/desconectado: codigo %u = %s\n", m, motivoQueda(m));
  }
}

#if TESTAR_NTP_DO_DHCP
void pedirNtpAoDhcp() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  esp_sntp_servermode_dhcp(true);
#else
  sntp_servermode_dhcp(1);
#endif
}

IPAddress ntpInformadoPeloDhcp() {
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  const ip_addr_t *a = esp_sntp_getserver(0);
#else
  const ip_addr_t *a = sntp_getserver(0);
#endif
  if (a == NULL || !IP_IS_V4(a)) return IPAddress((uint32_t)0);
  return IPAddress((uint32_t)ip4_addr_get_u32(ip_2_ip4(a)));
}
#endif

void desconectar() {
  WiFi.disconnect();
  delay(300);
  filaIni = filaFim;  // descarta o aviso da nossa própria desconexão
  evAssociou = false;
  evTemIP = false;
}

bool conectar() {
  desconectar();
#if TESTAR_NTP_DO_DHCP
  pedirNtpAoDhcp();
#endif
  Serial.printf("Conectando em \"%s\" (%s)...\n", rede, senha[0] ? "com senha" : "rede aberta");
  if (senha[0]) {
    WiFi.begin((const char *)rede, (const char *)senha);
  } else {
    WiFi.begin((const char *)rede);
  }
  const uint32_t inicio = millis();
  uint32_t limite = inicio + 20000;  // 20 s para associar
  bool avisouAssociou = false;
  while ((int32_t)(millis() - limite) < 0) {
    imprimirQuedas();
    if (evAssociou && !avisouAssociou) {
      avisouAssociou = true;
      res.associou = true;
      Serial.printf("  associou ao roteador em %.1f s; esperando o IP (DHCP)...\n",
                    (millis() - inicio) / 1000.0);
      limite = millis() + 15000;  // mais 15 s para receber o IP
    }
    if (evTemIP) {
      res.associou = true;
      res.temIP = true;
      res.ip = (uint32_t)WiFi.localIP();
      Serial.printf("  CONECTADO em %.1f s, IP %s\n", (millis() - inicio) / 1000.0, txtIp(WiFi.localIP()));
      return true;
    }
    delay(50);
  }
  imprimirQuedas();
  if (res.associou) {
    Serial.println("  FALHOU: associou ao roteador, mas o DHCP nao entregou IP.");
    Serial.println("  (provavel: a rede so entrega IP para MAC cadastrado)");
  } else if (res.quedas) {
    Serial.printf("  FALHOU: o roteador recusou. Ultimo codigo %d = %s\n", res.ultimoMotivo,
                  motivoQueda(res.ultimoMotivo));
  } else {
    Serial.println("  FALHOU: sem resposta do roteador (rede fora de alcance?)");
  }
  return false;
}

void mostrarInfo() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Nao conectado.");
    return;
  }
  Serial.printf("  IP do ESP32 ...... %s\n", txtIp(WiFi.localIP()));
  Serial.printf("  Mascara .......... %s\n", txtIp(WiFi.subnetMask()));
  Serial.printf("  Roteador ......... %s\n", txtIp(WiFi.gatewayIP()));
  Serial.printf("  DNS .............. %s  %s\n", txtIp(WiFi.dnsIP(0)), txtIp(WiFi.dnsIP(1)));
#if TESTAR_NTP_DO_DHCP
  const IPAddress n = ntpInformadoPeloDhcp();
  Serial.printf("  NTP do DHCP ...... %s\n", ipVazio(n) ? "nenhum informado" : txtIp(n));
#endif
  Serial.printf("  Sinal ............ %d dBm, canal %ld, roteador %s\n", (int)WiFi.RSSI(),
                (long)WiFi.channel(), WiFi.BSSIDstr().c_str());
}

void procurarRedes() {
  titulo("Redes Wi-Fi encontradas");
  const int n = WiFi.scanNetworks(false, true);
  if (n <= 0) {
    Serial.println(n == 0 ? "  nenhuma rede encontrada" : "  erro na busca de redes");
    WiFi.scanDelete();
    return;
  }
  const int total = n < 40 ? n : 40;
  int ordem[40];
  for (int i = 0; i < total; i++) ordem[i] = i;
  for (int i = 0; i < total; i++)  // mais forte primeiro
    for (int j = i + 1; j < total; j++)
      if (WiFi.RSSI(ordem[j]) > WiFi.RSSI(ordem[i])) {
        const int t = ordem[i];
        ordem[i] = ordem[j];
        ordem[j] = t;
      }
  Serial.println("  sinal  canal  seguranca                   nome");
  for (int i = 0; i < total; i++) {
    const int k = ordem[i];
    const String nome = WiFi.SSID(k);
    Serial.printf("  %4ld   %4ld   %-26s  %s%s\n", (long)WiFi.RSSI(k), (long)WiFi.channel(k),
                  nomeSeguranca((int)WiFi.encryptionType(k)), nome.length() ? nome.c_str() : "(oculta)",
                  (rede[0] && strcmp(nome.c_str(), rede) == 0) ? "   <- escolhida" : "");
  }
  WiFi.scanDelete();
  Serial.println("  Para escolher: rede NOME");
}

// ============================================================ DNS
bool resolver(const char *nome, IPAddress &ip, bool imprimir) {
  const int64_t t0 = agoraUs();
  const bool ok = WiFi.hostByName(nome, ip) == 1 && !ipVazio(ip);
  if (imprimir) {
    if (ok) {
      Serial.printf("  %-30s -> %s", nome, txtIp(ip));
      if (ehPrivado(ip)) Serial.print("  (IP interno!)");
      Serial.printf("  em %lld ms\n", (long long)((agoraUs() - t0) / 1000));
    } else {
      Serial.printf("  %-30s -> nao resolveu\n", nome);
    }
  }
  return ok;
}

// ============================================================ NTP
// Pergunta a hora a um servidor NTP. Retorna true se veio uma hora válida.
bool consultarNtp(const IPAddress &ip, const char *rotulo, bool guardar) {
  static uint16_t porta = 40000;
  porta = porta >= 40900 ? 40000 : porta + 1;
  Serial.printf("  %-36s ", rotulo);
  WiFiUDP udp;
  if (!udp.begin(porta)) {
    Serial.println("erro ao abrir UDP");
    return false;
  }
  uint8_t pedido[48] = {0};
  pedido[0] = 0x23;  // versão 4, modo cliente
  const int64_t marca = agoraUs();
  memcpy(pedido + 40, &marca, 8);  // o servidor devolve isto: reconhecemos a resposta

  udp.beginPacket(ip, 123);
  udp.write(pedido, sizeof pedido);
  const int64_t t1 = agoraUs();
  if (!udp.endPacket()) {
    Serial.println("nao consegui enviar");
    udp.stop();
    return false;
  }
  int problema = -1;
  RespostaNtp r;
  while (agoraUs() - t1 < 1500000) {
    if (udp.parsePacket() > 0) {
      const int64_t t4 = agoraUs();
      uint8_t resp[64];
      const int n = udp.read(resp, sizeof resp);
      problema = analisarNtp(resp, n, pedido + 40, r);
      if (problema == NTP_NAO_E_NOSSA || problema == NTP_NAO_E_RESPOSTA) continue;
      if (problema != NTP_OK) break;
      int64_t desloc, atraso;
      calcularNtp(t1, t4, r.recebidoUs, r.enviadoUs, desloc, atraso);
      char ref[20];
      textoRefid(r, ref, sizeof ref);
      Serial.print("RESPONDEU  ");
      imprimirHora(desloc);
      Serial.print("  (+-");
      imprimirMs(atraso / 2);
      Serial.printf(", estrato %u, ref %s)\n", r.estrato, ref);
      udp.stop();
      if (guardar) {
        char nome[72];
        snprintf(nome, sizeof nome, "NTP %s", rotulo);
        adicionarFonte(nome, ehPrivado(ip) ? NTP_INTERNO : NTP_EXTERNO, desloc, atraso / 2);
      }
      return true;
    }
    delay(1);
  }
  udp.stop();
  if (problema > 0) {
    Serial.println(textoProblemaNtp(problema));
  } else {
    Serial.println("sem resposta");
  }
  return false;
}

// ============================================================ HTTP / HTTPS
// Faz um GET, lê o cabeçalho (status, Date, Location, Server) e, se pedido,
// um pedaço da página para achar o título. Não envia formulário nenhum.
bool pedirHttp(const char *url, RespostaHttp &r, AmostraData &amostra, bool lerTitulo,
               const char *&erro) {
  limparHttp(r);
  erro = "";
  Url u;
  if (!lerUrl(url, u)) {
    erro = "endereco invalido";
    return false;
  }
  WiFiClient simples;
  WiFiClientSecure seguro;
  Client *c;
  int ok;
  if (u.https) {
    seguro.setInsecure();  // só lemos o cabeçalho; não enviamos dado nenhum
    seguro.setHandshakeTimeout(8);
    ok = seguro.connect(u.host, u.porta, 6000);
    c = &seguro;
  } else {
    ok = simples.connect(u.host, u.porta, 4000);
    c = &simples;
  }
  if (!ok) {
    erro = u.https ? "nao conectou (porta 443 bloqueada ou falha no TLS)" : "nao conectou";
    return false;
  }
  amostra.envioUs = agoraUs();
  amostra.recebUs = 0;
  c->print("GET ");
  c->print(u.caminho);
  c->print(" HTTP/1.1\r\nHost: ");
  c->print(u.host);
  if (u.porta != (u.https ? 443 : 80)) {
    c->print(":");
    c->print(u.porta);
  }
  c->print("\r\nUser-Agent: Mozilla/5.0 (teste-sino-esp32)\r\nAccept: */*\r\nConnection: close\r\n\r\n");

  char linha[200];
  size_t n = 0;
  bool primeira = true, fimCabecalho = false;
  uint32_t limite = millis() + (u.https ? 8000 : 5000);
  while ((int32_t)(millis() - limite) < 0) {
    if (!c->available()) {
      if (!c->connected()) break;
      delay(1);
      continue;
    }
    const int ch = c->read();
    if (ch < 0) continue;
    if (!amostra.recebUs) amostra.recebUs = agoraUs();
    if (ch == '\n') {
      linha[n] = 0;
      if (n && linha[n - 1] == '\r') linha[--n] = 0;
      if (n == 0 && !primeira) {
        fimCabecalho = true;
        break;
      }
      lerLinhaHttp(linha, primeira, r);
      primeira = false;
      n = 0;
    } else if (n < sizeof(linha) - 1) {
      linha[n++] = (char)ch;
    }
  }
  if (fimCabecalho && lerTitulo) {
    static char corpo[3072];
    size_t m = 0;
    limite = millis() + 3000;
    while (m < sizeof(corpo) - 1 && (int32_t)(millis() - limite) < 0) {
      if (!c->available()) {
        if (!c->connected()) break;
        delay(1);
        continue;
      }
      const int ch = c->read();
      if (ch <= 0) continue;
      corpo[m++] = (char)ch;
      if (m >= 8 && strncasecmp(corpo + m - 8, "</title>", 8) == 0) break;
    }
    corpo[m] = 0;
    extrairTitulo(corpo, r.titulo, sizeof r.titulo);
  }
  c->stop();
  if (!amostra.recebUs) {
    erro = "conectou, mas nao respondeu";
    return false;
  }
  if (r.temData) amostra.segundo = r.dataEpoch;
  return true;
}

// Mostra o resultado de um pedido HTTP e guarda a hora do Date como fonte.
bool testarPagina(const char *url, bool guardar) {
  Serial.printf("  %s\n", url);
  RespostaHttp r;
  AmostraData a;
  const char *erro;
  if (!pedirHttp(url, r, a, true, erro)) {
    Serial.printf("      -> %s\n", erro);
    return false;
  }
  if (r.status < 0) {
    Serial.println("      -> respondeu, mas nao e HTTP");
    return true;
  }
  Serial.printf("      -> status %d em %lld ms", r.status, (long long)((a.recebUs - a.envioUs) / 1000));
  if (r.servidor[0]) Serial.printf(", servidor \"%s\"", r.servidor);
  Serial.println();
  if (r.local[0]) Serial.printf("      -> redireciona para: %s\n", r.local);
  if (r.titulo[0]) Serial.printf("      -> titulo da pagina: \"%s\"\n", r.titulo);
  if (r.data[0]) {
    Serial.printf("      -> Date: %s", r.data);
    if (r.temData) {
      int64_t desloc, incerteza;
      deslocDeUmaAmostra(a, desloc, incerteza);
      Serial.print("  = ");
      imprimirHora(desloc);
      Serial.println(" (hora de MS, +-0,5 s)");
      if (guardar) {
        char nome[72];
        snprintf(nome, sizeof nome, "Date de %s", url);
        adicionarFonte(nome, DATE_PAGINA, desloc, incerteza);
        if (!res.urlData[0] && strncasecmp(url, "http://", 7) == 0)
          snprintf(res.urlData, sizeof res.urlData, "%s", url);
      }
    } else {
      Serial.println("  (formato nao reconhecido)");
    }
  } else {
    Serial.println("      -> sem cabecalho Date");
  }

  // Classificação: internet liberada ou página de login no caminho?
  if (strstr(url, "generate_204") && r.status == 204) res.internet = true;
  if (strstr(url, "//1.1.1.1/") && contemSemCaixa(r.servidor, "cloudflare")) res.internet = true;
  if (r.local[0] && !res.portal[0] && !mesmoHost(url, r.local) &&
      (comecaCom(r.local, "http://") || comecaCom(r.local, "https://"))) {
    snprintf(res.portal, sizeof res.portal, "%s", r.local);
  }
  return true;
}

// Uma amostra do Date da página (envio, chegada e segundo informado).
bool amostrarData(const char *url, AmostraData &a) {
  RespostaHttp r;
  const char *erro;
  return pedirHttp(url, r, a, false, erro) && r.temData;
}

// Pedidos repetidos à mesma página para pegar a virada do segundo do Date:
// melhora a precisão de +-0,5 s para alguns milissegundos.
//  1) um pedido a cada ~100 ms até o segundo virar (acha a virada com +-~60 ms);
//  2) uma rajada curta só em volta da virada seguinte (+- o tempo de um pedido).
// No total são uns 30 pedidos em ~3 s: não sobrecarrega o servidor.
void refinarData(const char *url) {
  AmostraData ant, at;
  int pedidos = 1;
  if (!amostrarData(url, ant)) {
    Serial.println("  a pagina parou de responder");
    return;
  }
  bool virou = false;
  const uint32_t inicio = millis();
  while (millis() - inicio < 2500) {
    delay(100);
    if (!amostrarData(url, at)) break;
    pedidos++;
    if (at.segundo == ant.segundo + 1) {
      virou = true;
      break;
    }
    ant = at;
  }
  if (!virou) {
    Serial.printf("  nao deu (%d pedidos sem pegar a virada do segundo)\n", pedidos);
    return;
  }
  AmostraData par[2] = {ant, at};
  int64_t desloc = 0, incerteza = 0;
  acharVirada(par, 2, desloc, incerteza);

  // Próxima virada (para o segundo at.segundo + 1), no relógio do ESP32:
  const int64_t prevista = (int64_t)(at.segundo + 1) * 1000000LL - desloc;
  const int64_t duracao = at.recebUs - at.envioUs;
  while (agoraUs() < prevista - incerteza - 2 * duracao - 5000) delay(1);
  AmostraData antes = at, depois;
  bool fino = false;
  const uint32_t inicioFino = millis();
  while (millis() - inicioFino < 1500) {
    if (!amostrarData(url, depois)) break;
    pedidos++;
    if (depois.segundo == at.segundo + 1) {
      fino = antes.segundo == at.segundo;  // a amostra anterior ainda era do segundo antigo
      break;
    }
    if (depois.segundo != at.segundo) break;  // algo estranho: desiste do refinamento fino
    antes = depois;
  }
  if (fino) {
    AmostraData parFino[2] = {antes, depois};
    int64_t d2, i2;
    if (acharVirada(parFino, 2, d2, i2) && i2 < incerteza) {
      desloc = d2;
      incerteza = i2;
    }
  }
  Serial.printf("  %d pedidos; virada do segundo achada: ", pedidos);
  imprimirHora(desloc);
  Serial.print(" (+-");
  imprimirMs(incerteza);
  Serial.println(")");
  char nome[72];
  snprintf(nome, sizeof nome, "Date refinado de %s", url);
  adicionarFonte(nome, DATE_PAGINA, desloc, incerteza);
}

// ============================================================ Teste completo
struct Alvo {
  IPAddress ip;
  char rotulo[48];
};

void adicionarAlvo(Alvo *alvos, int &n, int max, const IPAddress &ip, const char *rotulo) {
  if (ipVazio(ip) || n >= max) return;
  for (int i = 0; i < n; i++)
    if (alvos[i].ip == ip) return;  // repetido
  alvos[n].ip = ip;
  snprintf(alvos[n].rotulo, sizeof alvos[n].rotulo, "%s (%s)", txtIp(ip), rotulo);
  n++;
}

void imprimirResumo() {
  Serial.println();
  Serial.println("==================== RESUMO ====================");
  if (!res.rodou) {
    Serial.println("Nenhum teste feito ainda. Digite: testar");
    return;
  }
  mostrarMac();
  Serial.printf("Rede: \"%s\" (%s)\n", rede, senha[0] ? "com senha" : "aberta");
  if (res.temIP) {
    Serial.printf("Wi-Fi ............ CONECTOU (IP %s)\n", txtIp(IPAddress(res.ip)));
  } else if (res.associou) {
    Serial.println("Wi-Fi ............ associou, mas NAO recebeu IP (DHCP)");
  } else {
    Serial.printf("Wi-Fi ............ RECUSADO (codigo %d = %s)\n", res.ultimoMotivo,
                  motivoQueda(res.ultimoMotivo));
  }
  if (res.temIP) {
    Serial.printf("Internet ......... %s\n", res.internet ? "LIBERADA" : "bloqueada (falta login/cadastro)");
    if (res.portal[0]) Serial.printf("Pagina de login .. %s\n", res.portal);
    Serial.printf("DNS .............. %s\n", res.dnsOk ? (res.dnsDesviado ? "responde, mas desviado para IP interno" : "funciona")
                                                     : "nao responde");
  }
  Serial.println("Fontes de hora que funcionaram:");
  const int m = melhorFonte();
  if (m < 0) Serial.println("  nenhuma");
  for (int i = 0; i < nFontes; i++) {
    const Fonte &f = fontes[i];
    Serial.printf("  %-58s +-", f.nome);
    imprimirMs(f.incertezaUs);
    if (i != m) {
      const int64_t dif = f.deslocUs - fontes[m].deslocUs;
      Serial.print(", difere da melhor em ");
      imprimirMs(llabs(dif));
      if (llabs(dif) > 2000000 && llabs(dif) > 2 * f.incertezaUs)
        Serial.print(dif > 0 ? " (relogio dessa fonte ADIANTADO)" : " (relogio dessa fonte ATRASADO)");
    } else {
      Serial.print("   <- melhor");
    }
    Serial.println();
  }
  if (m >= 0) {
    Serial.print("Hora agora (melhor fonte): ");
    imprimirHora(fontes[m].deslocUs);
    Serial.println(" (MS)");
  }

  Serial.println();
  Serial.print("CONCLUSAO: ");
  bool ntpInterno = false, ntpExterno = false, soData = false;
  for (int i = 0; i < nFontes; i++) {
    if (fontes[i].tipo == NTP_INTERNO) ntpInterno = true;
    if (fontes[i].tipo == NTP_EXTERNO) ntpExterno = true;
    if (fontes[i].tipo == DATE_PAGINA) soData = true;
  }
  if (!res.temIP) {
    Serial.println("sem IP nao ha como pegar a hora pela rede.");
    Serial.println("  Peca a TI o cadastro do MAC acima, ou use o sino com GPS.");
  } else if (res.internet) {
    Serial.println("a internet esta LIBERADA para este aparelho (MAC cadastrado ou rede aberta).");
    Serial.println("  Qualquer servidor de hora funciona enquanto o cadastro existir.");
  } else if (ntpInterno) {
    Serial.println("DA PARA USAR: o servidor de hora interno responde mesmo SEM login.");
    Serial.println("  O sino pode acertar o relogio por ele (com o DS3231 de reserva).");
  } else if (ntpExterno) {
    Serial.println("o NTP da internet responde mesmo SEM login (porta 123 liberada).");
    Serial.println("  Funciona, mas a TI pode fechar isso a qualquer momento.");
  } else if (soData) {
    Serial.println("so as paginas informam a hora (cabecalho Date), mesmo sem login.");
    Serial.println("  Da para usar, com a precisao mostrada acima; e o metodo mais fragil.");
  } else {
    Serial.println("NENHUMA fonte de hora sem login. Precisa do cadastro do MAC (ou do GPS).");
  }
  Serial.println("Copie tudo o que apareceu aqui no monitor e mande para analise.");
  Serial.println("================================================");
}

void testar() {
  if (!rede[0]) {
    Serial.println("Escolha a rede primeiro. Ex.: rede IFMS-Visitantes  (use 'redes' para listar)");
    return;
  }
  memset(&res, 0, sizeof res);
  res.rodou = true;
  nFontes = 0;
  Serial.println();
  Serial.println("################ TESTE DA REDE ################");
  mostrarMac();
  procurarRedes();

  titulo("1. Conexao");
  if (!conectar()) {
    desconectar();  // para de tentar (evita ficar repetindo)
    imprimirResumo();
    return;
  }

  titulo("2. Dados da rede");
  mostrarInfo();
  const IPAddress gw = WiFi.gatewayIP(), dns1 = WiFi.dnsIP(0), dns2 = WiFi.dnsIP(1);

  titulo("3. DNS (traducao de nomes)");
  const char *nomes[] = {"a.ntp.br", "pool.ntp.br", "time.google.com", "connectivitycheck.gstatic.com"};
  IPAddress ipNome[4];
  bool resolveu[4];
  for (int i = 0; i < 4; i++) {
    resolveu[i] = resolver(nomes[i], ipNome[i], true);
    if (resolveu[i]) {
      res.dnsOk = true;
      if (ehPrivado(ipNome[i])) res.dnsDesviado = true;
    }
  }
  if (res.dnsDesviado) Serial.println("  Nomes da internet viraram IP interno: o portal desvia o DNS.");

  titulo("4. Paginas (HTTP), sem fazer login");
  char url[160];
  if (!ipVazio(gw)) {
    snprintf(url, sizeof url, "http://%s/", txtIp(gw));
    testarPagina(url, true);
  }
  testarPagina("http://1.1.1.1/", true);
  testarPagina("http://connectivitycheck.gstatic.com/generate_204", true);
  if (res.portal[0]) {
    titulo("5. Pagina de login (so le o cabecalho e o titulo)");
    testarPagina(res.portal, true);
  }

  titulo("6. Servidores de hora (NTP)");
  Alvo alvos[10];
  int nAlvos = 0;
#if TESTAR_NTP_DO_DHCP
  adicionarAlvo(alvos, nAlvos, 10, ntpInformadoPeloDhcp(), "informado pelo DHCP");
#endif
  adicionarAlvo(alvos, nAlvos, 10, gw, "roteador");
  adicionarAlvo(alvos, nAlvos, 10, dns1, "DNS da rede");
  adicionarAlvo(alvos, nAlvos, 10, dns2, "DNS da rede");
  if (res.portal[0]) {
    Url u;
    IPAddress ipPortal;
    if (lerUrl(res.portal, u) && resolver(u.host, ipPortal, false))
      adicionarAlvo(alvos, nAlvos, 10, ipPortal, "pagina de login");
  }
  if (resolveu[0] && !ehPrivado(ipNome[0])) adicionarAlvo(alvos, nAlvos, 10, ipNome[0], "a.ntp.br");
  adicionarAlvo(alvos, nAlvos, 10, IPAddress(200, 160, 0, 8), "a.ntp.br, IP fixo");
  adicionarAlvo(alvos, nAlvos, 10, IPAddress(216, 239, 35, 0), "time.google.com, IP fixo");
  for (int i = 0; i < nAlvos; i++) consultarNtp(alvos[i].ip, alvos[i].rotulo, true);

  if (res.urlData[0]) {
    titulo("7. Refinando a hora do cabecalho Date");
    Serial.printf("  %s\n", res.urlData);
    refinarData(res.urlData);
  }
  imprimirResumo();
}

// ============================================================ Comandos
void salvarRede() {
  prefs.putString("rede", rede);
  prefs.putString("senha", senha);
}

void ajuda() {
  Serial.println();
  Serial.println("Comandos (digite e tecle Enter):");
  Serial.println("  mac                 mostra o MAC deste ESP32 (para a TI cadastrar)");
  Serial.println("  redes               procura as redes Wi-Fi por perto");
  Serial.println("  rede NOME           escolhe a rede (fica salvo)");
  Serial.println("  senha SENHA         senha da rede; 'senha' sozinho = rede aberta (fica salva)");
  Serial.println("  testar              conecta e faz todos os testes (2 a 3 minutos no maximo)");
  Serial.println("  resumo              mostra de novo o resumo do ultimo teste");
  Serial.println("  hora                hora atual segundo a melhor fonte encontrada");
  Serial.println("  info                IP, roteador, DNS e sinal da conexao atual");
  Serial.println("  ntp IP-ou-NOME      testa um servidor de hora (ex.: o que a TI indicar)");
  Serial.println("  http ENDERECO       le cabecalho e titulo de uma pagina (http:// ou https://)");
  Serial.println("  dns NOME            traduz um nome para IP");
  Serial.println("  desconectar         sai da rede");
  Serial.println("  esquecer            apaga a rede e a senha salvas");
  Serial.println("  ajuda               esta lista");
}

void executar(char *linha) {
  // separa o comando (minúsculo) do resto da linha (mantém maiúsculas)
  char *arg = strchr(linha, ' ');
  if (arg) {
    *arg++ = 0;
    while (*arg == ' ') arg++;
  } else {
    arg = linha + strlen(linha);
  }
  for (char *p = linha; *p; p++) *p = tolower((unsigned char)*p);

  if (!strcmp(linha, "senha")) {
    Serial.printf("> senha %s\n", arg[0] ? "********" : "(vazia)");
  } else {
    Serial.printf("> %s %s\n", linha, arg);
  }

  if (!strcmp(linha, "ajuda") || !strcmp(linha, "help") || !strcmp(linha, "?")) {
    ajuda();
  } else if (!strcmp(linha, "mac")) {
    mostrarMac();
  } else if (!strcmp(linha, "redes")) {
    procurarRedes();
  } else if (!strcmp(linha, "rede")) {
    if (!arg[0]) {
      Serial.printf("Rede atual: \"%s\". Para mudar: rede NOME\n", rede);
    } else {
      snprintf(rede, sizeof rede, "%s", arg);
      salvarRede();
      Serial.printf("Rede escolhida: \"%s\". Se tiver senha: senha SENHA. Depois: testar\n", rede);
    }
  } else if (!strcmp(linha, "senha")) {
    snprintf(senha, sizeof senha, "%s", arg);
    salvarRede();
    Serial.println(senha[0] ? "Senha salva." : "Sem senha (rede aberta).");
  } else if (!strcmp(linha, "testar")) {
    testar();
  } else if (!strcmp(linha, "resumo")) {
    imprimirResumo();
  } else if (!strcmp(linha, "hora")) {
    const int m = melhorFonte();
    if (m < 0) {
      Serial.println("Nenhuma fonte de hora ainda. Digite: testar");
    } else {
      Serial.print("Agora: ");
      imprimirHora(fontes[m].deslocUs);
      Serial.printf(" (MS), segundo %s\n", fontes[m].nome);
    }
  } else if (!strcmp(linha, "info")) {
    mostrarInfo();
  } else if (!strcmp(linha, "ntp")) {
    IPAddress ip;
    if (!arg[0]) {
      Serial.println("Uso: ntp 10.0.0.1   ou   ntp a.ntp.br");
    } else if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Conecte primeiro (testar).");
    } else if (ip.fromString(arg) || resolver(arg, ip, true)) {
      char rotulo[48];
      snprintf(rotulo, sizeof rotulo, "%s (%s)", txtIp(ip), arg);
      consultarNtp(ip, rotulo, true);
    }
  } else if (!strcmp(linha, "http")) {
    if (!arg[0]) {
      Serial.println("Uso: http http://10.0.0.1/   ou   http https://pagina.de.login/");
    } else if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Conecte primeiro (testar).");
    } else {
      testarPagina(arg, true);
    }
  } else if (!strcmp(linha, "dns")) {
    IPAddress ip;
    if (!arg[0]) {
      Serial.println("Uso: dns a.ntp.br");
    } else if (WiFi.status() != WL_CONNECTED) {
      Serial.println("Conecte primeiro (testar).");
    } else {
      resolver(arg, ip, true);
    }
  } else if (!strcmp(linha, "desconectar")) {
    desconectar();
    Serial.println("Desconectado.");
  } else if (!strcmp(linha, "esquecer")) {
    rede[0] = 0;
    senha[0] = 0;
    prefs.remove("rede");
    prefs.remove("senha");
    Serial.println("Rede e senha apagadas.");
  } else {
    Serial.println("Comando desconhecido. Digite: ajuda");
  }
}

void lerSerial() {
  static char linha[160];
  static size_t n = 0;
  while (Serial.available()) {
    const char c = (char)Serial.read();
    if (c == '\r' || c == '\n') {
      while (n && linha[n - 1] == ' ') n--;
      linha[n] = 0;
      if (n) executar(linha);
      n = 0;
    } else if (n < sizeof(linha) - 1) {
      linha[n++] = c;
    }
  }
}

// ============================================================ main
void setup() {
  Serial.begin(115200);
  delay(1500);  // tempo para o monitor serial abrir

  prefs.begin("testerede", false);
  snprintf(rede, sizeof rede, "%s", prefs.getString("rede", "").c_str());
  snprintf(senha, sizeof senha, "%s", prefs.getString("senha", "").c_str());

  WiFi.onEvent(aoEventoWiFi);
  WiFi.persistent(false);
  WiFi.setHostname(NOME_NA_REDE);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);  // sem economia de energia: mede o tempo da rede sem atrasos extras

  Serial.println();
  Serial.println("=================================================");
  Serial.println(" Teste de rede para o sino (ESP32)");
  Serial.println(" (nao faz login nem tenta passar pelo captcha)");
  mostrarMac();
  Serial.println("=================================================");
  if (rede[0]) {
    Serial.printf("Rede salva: \"%s\". Testando...\n", rede);
    testar();
  } else {
    Serial.println("Comece com: redes   (depois: rede NOME, senha SENHA, testar)");
  }
  ajuda();
}

void loop() {
  lerSerial();
  imprimirQuedas();
  delay(10);
}
