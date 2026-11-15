// ============================================================================
//  Teste de rede para o sino (ESP32 ou ESP32-C3)
//
//  Descobre se dá para pegar a hora certa pela rede da instituição SEM fazer o
//  login da página (captcha) e sem o MAC estar cadastrado. Tenta TODAS as
//  formas que o sino poderia usar:
//    - servidores de hora (NTP) da própria rede: o informado pelo DHCP, o
//      roteador, os servidores DNS, a página de login; e os da internet;
//    - a hora do cabeçalho "Date" das páginas (inclusive a de login), refinada
//      para alguns milissegundos;
//    - ICMP timestamp, TIME (porta 37), DAYTIME (porta 13) e NTP por difusão.
//  Mostra o MAC (para pedir o cadastro à TI), explica por que a conexão foi
//  recusada e termina com um resumo e uma conclusão.
//  Ele NÃO faz login e não tenta passar pelo captcha.
//
//  Três jeitos de usar, todos ao mesmo tempo:
//    1. Monitor serial: 115200 baud, final de linha "Nova linha". Digite "ajuda".
//    2. Celular: rede Wi-Fi "TESTE-SINO" (senha sino12345), http://192.168.4.1
//    3. Sem nada: botão BOOT inicia o teste; o LED pisca a conclusão (veja o
//       README). O relatório fica guardado na memória do ESP.
//
//  Placa na Arduino IDE (pacote esp32 da Espressif, 2.x ou 3.x):
//    ESP32:    "ESP32 Dev Module"
//    ESP32-C3: "ESP32C3 Dev Module" com "USB CDC On Boot: Enabled"
// ============================================================================
#include <Preferences.h>
#include <WebServer.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#include "analise.h"
#include "esp_timer.h"
#include "pagina_teste.h"
#include "registro.h"
#include "tempo.h"

// ------------------------------------------------------------ Configuração
#define FUSO_MINUTOS (-240)        // Mato Grosso do Sul: UTC-4
#define NOME_NA_REDE "sino-teste"  // nome que aparece para a TI na lista de aparelhos
#define AP_TESTE_NOME "TESTE-SINO"  // Wi-Fi do próprio ESP, para a página
#define AP_TESTE_SENHA "sino12345"
#define MAX_RELATORIOS 6  // quantos relatórios ficam guardados na memória

#if defined(CONFIG_IDF_TARGET_ESP32C3)
#define PINO_LED 8  // LED azul das placas C3 "Super Mini" (acende em 0)
#define LED_ACESO LOW
#define PINO_BOTAO 9  // botão BOOT
#else
#define PINO_LED 2  // LED azul do ESP32 Dev Module
#define LED_ACESO HIGH
#define PINO_BOTAO 0  // botão BOOT
#endif

// Pergunta ao DHCP qual é o servidor de hora da rede. Se der erro de
// compilação nesta parte, mude para 0: o resto do teste continua igual.
#define TESTAR_NTP_DO_DHCP 1

#if TESTAR_NTP_DO_DHCP
#include "esp_sntp.h"
#endif

// ------------------------------------------------------------------ Estado
Registro saida;  // tudo o que o teste mostra: monitor serial + página + relatórios
Preferences prefs;
WebServer servidor(80);
QueueHandle_t filaCmd;
char rede[33] = "";
char senha[65] = "";
volatile bool ocupado = false;      // uma tarefa (teste, vigia...) está rodando
volatile bool pararPedido = false;  // alguém pediu para parar
uint8_t conclusao = 0;              // código do resumo (o LED pisca este número)
uint8_t potencia = 0;               // potência do Wi-Fi: 0 = máxima; senão dBm x 2
bool potenciaTentada = false;       // já tentou reduzir a potência neste teste?
WiFiUDP escutaNtp;
bool escutando = false;

// Eventos do Wi-Fi: chegam por outra tarefa do ESP32 e são lidos depois.
volatile bool evAssociou = false;
volatile bool evTemIP = false;
volatile uint8_t filaMotivo[16];
volatile uint8_t filaIni = 0, filaFim = 0;

enum TipoFonte { NTP_INTERNO, NTP_EXTERNO, DATE_PAGINA, TEMPO_ANTIGO };
struct Fonte {  // uma fonte de hora que funcionou
  char nome[72];
  TipoFonte tipo;
  int64_t deslocUs;     // hora da fonte - relógio do ESP32
  int64_t incertezaUs;  // margem de erro (+-)
  uint32_t ip;          // de onde veio (0 se não se aplica)
};
Fonte fontes[24];
uint8_t nFontes = 0;

struct Alvo {  // servidor NTP a testar
  IPAddress ip;
  char rotulo[48];
};

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

void titulo(const char *t) { saida.printf("\n---- %s ----\n", t); }

void imprimirMs(int64_t us) {
  if (us < 10000) {
    saida.printf("%.1f ms", us / 1000.0);
  } else {
    saida.printf("%lld ms", (long long)(us / 1000));
  }
}

void imprimirHora(int64_t deslocUs) {
  const int64_t ms = (agoraUs() + deslocUs) / 1000;  // ms desde 1970 (UTC)
  const DataHora t = deEpoch((uint32_t)(ms / 1000) + FUSO_MINUTOS * 60);
  saida.printf("%02u/%02u/%04u %02u:%02u:%02u,%03u", t.dia, t.mes, t.ano, t.hora, t.min, t.seg, (unsigned)(ms % 1000));
}

// Hora entre 2024 e 2100? (aparelho com relógio desacertado informa 2000, 1970...)
bool horaPlausivel(int64_t deslocUs) {
  const int64_t s = (agoraUs() + deslocUs) / 1000000LL;
  return s >= 1704067200LL && s < 4102444800LL;
}

void adicionarFonte(const char *nome, int tipo, int64_t desloc, int64_t incerteza, uint32_t ip) {
  if (!horaPlausivel(desloc)) {
    saida.println("      (hora absurda: o relogio desse aparelho esta desacertado; ignorada)");
    return;
  }
  if (nFontes >= sizeof(fontes) / sizeof(fontes[0])) return;
  Fonte &f = fontes[nFontes++];
  snprintf(f.nome, sizeof f.nome, "%s", nome);
  f.tipo = (TipoFonte)tipo;
  f.deslocUs = desloc;
  f.incertezaUs = incerteza;
  f.ip = ip;
}

int melhorFonte() {
  int m = -1;
  for (int i = 0; i < nFontes; i++)
    if (m < 0 || fontes[i].incertezaUs < fontes[m].incertezaUs) m = i;
  return m;
}

// "dd/mm/aaaa hh:mm:ss" pela melhor fonte, ou "hora desconhecida"
void textoHoraAgora(char *dst, size_t tam) {
  const int m = melhorFonte();
  if (m < 0) {
    snprintf(dst, tam, "hora desconhecida");
    return;
  }
  const int64_t s = (agoraUs() + fontes[m].deslocUs) / 1000000LL;
  const DataHora t = deEpoch((uint32_t)s + FUSO_MINUTOS * 60);
  snprintf(dst, tam, "%02u/%02u/%04u %02u:%02u:%02u", t.dia, t.mes, t.ano, t.hora, t.min, t.seg);
}

const char *textoConclusao(uint8_t c) {
  switch (c) {
    case 1: return "internet liberada (MAC cadastrado ou rede aberta)";
    case 2: return "DA PARA USAR: servidor de hora da rede responde sem login";
    case 3: return "so o NTP da internet responde (porta 123 aberta)";
    case 4: return "so as paginas informam a hora (cabecalho Date)";
    case 5: return "conectou, mas NENHUMA fonte de hora sem login";
    case 6: return "nao conectou na rede";
    default: return "ainda sem teste";
  }
}

void mostrarMac() {
  saida.printf("MAC deste %s: %s   <- e este que a TI cadastra\n", ESP.getChipModel(), WiFi.macAddress().c_str());
  saida.printf("Nome na rede:    %s\n", NOME_NA_REDE);
}

void mostrarComoAcessar() {
  saida.printf("Pagina deste teste: Wi-Fi \"%s\" (senha %s) -> http://%s/\n", AP_TESTE_NOME, AP_TESTE_SENHA,
               txtIp(WiFi.softAPIP()));
}

// ============================================================ Potência do Wi-Fi
// Placas ESP32-C3 "Super Mini" têm antena ruim e muitas só conectam com menos
// potência (8,5 dBm). O teste reduz sozinho se a conexão falhar.
const char *textoPotencia() {
  switch (potencia) {
    case 17: return "8,5 dBm";
    case 22: return "11 dBm";
    case 30: return "15 dBm";
    default: return "maxima";
  }
}

void aplicarPotencia() {
  wifi_power_t p = WIFI_POWER_19_5dBm;
  if (potencia == 17) p = WIFI_POWER_8_5dBm;
  if (potencia == 22) p = WIFI_POWER_11dBm;
  if (potencia == 30) p = WIFI_POWER_15dBm;
  WiFi.setTxPower(p);
}

void definirPotencia(const char *texto) {
  uint8_t nova;
  if (!strcmp(texto, "max") || !strcmp(texto, "maxima")) {
    nova = 0;
  } else if (!strcmp(texto, "8.5") || !strcmp(texto, "8,5")) {
    nova = 17;
  } else if (!strcmp(texto, "11")) {
    nova = 22;
  } else if (!strcmp(texto, "15")) {
    nova = 30;
  } else {
    saida.println("Uso: potencia max | 8.5 | 11 | 15");
    return;
  }
  potencia = nova;
  prefs.putUChar("pot", potencia);
  aplicarPotencia();
  saida.printf("Potencia do Wi-Fi: %s (salva).\n", textoPotencia());
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
    saida.printf("  recusado/desconectado: codigo %u = %s\n", m, motivoQueda(m));
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

bool tentarConectar() {
  desconectar();
#if TESTAR_NTP_DO_DHCP
  pedirNtpAoDhcp();
#endif
  saida.printf("Conectando em \"%s\" (%s, potencia %s)...\n", rede, senha[0] ? "com senha" : "rede aberta", textoPotencia());
  if (senha[0]) {
    WiFi.begin((const char *)rede, (const char *)senha);
  } else {
    WiFi.begin((const char *)rede);
  }
  aplicarPotencia();
  const uint32_t inicio = millis();
  uint32_t limite = inicio + 20000;  // 20 s para associar
  bool avisouAssociou = false;
  while ((int32_t)(millis() - limite) < 0) {
    imprimirQuedas();
    if (evAssociou && !avisouAssociou) {
      avisouAssociou = true;
      res.associou = true;
      saida.printf("  associou ao roteador em %.1f s; esperando o IP (DHCP)...\n", (millis() - inicio) / 1000.0);
      limite = millis() + 15000;  // mais 15 s para receber o IP
    }
    if (evTemIP) {
      res.associou = true;
      res.temIP = true;
      res.ip = (uint32_t)WiFi.localIP();
      saida.printf("  CONECTADO em %.1f s, IP %s\n", (millis() - inicio) / 1000.0, txtIp(WiFi.localIP()));
      return true;
    }
    delay(50);
  }
  imprimirQuedas();
  if (res.associou) {
    saida.println("  FALHOU: associou ao roteador, mas o DHCP nao entregou IP.");
    saida.println("  (provavel: a rede so entrega IP para MAC cadastrado)");
  } else if (res.quedas) {
    saida.printf("  FALHOU: o roteador recusou. Ultimo codigo %d = %s\n", res.ultimoMotivo, motivoQueda(res.ultimoMotivo));
  } else {
    saida.println("  FALHOU: sem resposta do roteador (rede fora de alcance?)");
  }
  return false;
}

// Falha que parece ser de radio (e nao de cadastro)? Ai vale tentar com menos potencia.
bool pareceProblemaDeRadio() {
  if (res.temIP) return false;
  if (!res.associou && res.quedas == 0) return true;  // ninguem respondeu
  const int m = res.ultimoMotivo;
  return m == 2 || m == 15 || m == 201 || m == 204 || m == 205;
}

bool conectar() {
  if (tentarConectar()) return true;
  if (potencia == 0 && !potenciaTentada && pareceProblemaDeRadio()) {
    potenciaTentada = true;
    saida.println("  Tentando de novo com menos potencia de Wi-Fi (placas ESP32-C3 Super Mini costumam precisar)...");
    potencia = 17;
    const bool ok = tentarConectar();
    if (ok) {
      prefs.putUChar("pot", potencia);
      saida.println("  Funcionou com 8,5 dBm: potencia salva ('potencia max' desfaz).");
    } else {
      potencia = 0;
    }
    return ok;
  }
  return false;
}

void mostrarInfo() {
  if (WiFi.status() != WL_CONNECTED) {
    saida.println("Nao conectado.");
    return;
  }
  saida.printf("  IP do ESP ........ %s\n", txtIp(WiFi.localIP()));
  saida.printf("  Mascara .......... %s\n", txtIp(WiFi.subnetMask()));
  saida.printf("  Roteador ......... %s\n", txtIp(WiFi.gatewayIP()));
  saida.printf("  DNS .............. %s  %s\n", txtIp(WiFi.dnsIP(0)), txtIp(WiFi.dnsIP(1)));
#if TESTAR_NTP_DO_DHCP
  const IPAddress n = ntpInformadoPeloDhcp();
  saida.printf("  NTP do DHCP ...... %s\n", ipVazio(n) ? "nenhum informado" : txtIp(n));
#endif
  saida.printf("  Sinal ............ %d dBm, canal %ld, roteador %s\n", (int)WiFi.RSSI(), (long)WiFi.channel(),
               WiFi.BSSIDstr().c_str());
}

void procurarRedes() {
  titulo("Redes Wi-Fi encontradas");
  const int n = WiFi.scanNetworks(false, true);
  if (n <= 0) {
    saida.println(n == 0 ? "  nenhuma rede encontrada" : "  erro na busca de redes");
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
  saida.println("  sinal  canal  seguranca                   nome");
  for (int i = 0; i < total; i++) {
    const int k = ordem[i];
    const String nome = WiFi.SSID(k);
    saida.printf("  %4ld   %4ld   %-26s  %s%s\n", (long)WiFi.RSSI(k), (long)WiFi.channel(k),
                 nomeSeguranca((int)WiFi.encryptionType(k)), nome.length() ? nome.c_str() : "(oculta)",
                 (rede[0] && strcmp(nome.c_str(), rede) == 0) ? "   <- escolhida" : "");
  }
  WiFi.scanDelete();
  saida.println("  Para escolher: rede NOME");
}

// ============================================================ DNS
bool resolver(const char *nome, IPAddress &ip, bool imprimir) {
  const int64_t t0 = agoraUs();
  const bool ok = WiFi.hostByName(nome, ip) == 1 && !ipVazio(ip);
  if (imprimir) {
    if (ok) {
      saida.printf("  %-30s -> %s", nome, txtIp(ip));
      if (ehPrivado(ip)) saida.print("  (IP interno!)");
      saida.printf("  em %lld ms\n", (long long)((agoraUs() - t0) / 1000));
    } else {
      saida.printf("  %-30s -> nao resolveu\n", nome);
    }
  }
  return ok;
}

// ============================================================ NTP
// Uma pergunta a um servidor NTP, sem imprimir nada. Devolve true se veio uma
// hora válida; senão 'problema' diz o que houve (0 = sem resposta).
bool ntpBruto(const IPAddress &ip, int64_t &desloc, int64_t &atraso, RespostaNtp &r, int &problema) {
  static uint16_t porta = 40000;
  porta = porta >= 40900 ? 40000 : porta + 1;
  problema = 0;
  WiFiUDP udp;
  if (!udp.begin(porta)) {
    problema = -1;
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
    udp.stop();
    problema = -2;
    return false;
  }
  bool ok = false;
  while (agoraUs() - t1 < 1500000) {
    if (udp.parsePacket() > 0) {
      const int64_t t4 = agoraUs();
      uint8_t resp[64];
      const int n = udp.read(resp, sizeof resp);
      const int p = analisarNtp(resp, n, pedido + 40, r);
      if (p == NTP_NAO_E_NOSSA || p == NTP_NAO_E_RESPOSTA) continue;
      problema = p;
      if (p != NTP_OK) break;
      calcularNtp(t1, t4, r.recebidoUs, r.enviadoUs, desloc, atraso);
      ok = true;
      break;
    }
    delay(1);
  }
  udp.stop();
  return ok;
}

// Pergunta a hora a um servidor NTP e mostra o resultado.
bool consultarNtp(const IPAddress &ip, const char *rotulo, bool guardar) {
  saida.printf("  %-36s ", rotulo);
  int64_t desloc, atraso;
  RespostaNtp r;
  int problema;
  if (!ntpBruto(ip, desloc, atraso, r, problema)) {
    if (problema == -1) {
      saida.println("erro ao abrir UDP");
    } else if (problema == -2) {
      saida.println("nao consegui enviar");
    } else if (problema > 0) {
      saida.println(textoProblemaNtp(problema));
    } else {
      saida.println("sem resposta");
    }
    return false;
  }
  char ref[20];
  textoRefid(r, ref, sizeof ref);
  saida.print("RESPONDEU  ");
  imprimirHora(desloc);
  saida.print("  (+-");
  imprimirMs(atraso / 2);
  saida.printf(", estrato %u, ref %s)\n", r.estrato, ref);
  if (guardar) {
    char nome[72];
    snprintf(nome, sizeof nome, "NTP %s", rotulo);
    adicionarFonte(nome, ehPrivado(ip) ? NTP_INTERNO : NTP_EXTERNO, desloc, atraso / 2, (uint32_t)ip);
  }
  return true;
}

// ============================================================ HTTP / HTTPS
// Faz um GET, lê o cabeçalho (status, Date, Location, Server) e, se pedido,
// um pedaço da página para achar o título. Não envia formulário nenhum.
bool pedirHttp(const char *url, RespostaHttp &r, AmostraData &amostra, bool lerTitulo, const char *&erro) {
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
  saida.printf("  %s\n", url);
  RespostaHttp r;
  AmostraData a;
  const char *erro;
  if (!pedirHttp(url, r, a, true, erro)) {
    saida.printf("      -> %s\n", erro);
    return false;
  }
  if (r.status < 0) {
    saida.println("      -> respondeu, mas nao e HTTP");
    return true;
  }
  saida.printf("      -> status %d em %lld ms", r.status, (long long)((a.recebUs - a.envioUs) / 1000));
  if (r.servidor[0]) saida.printf(", servidor \"%s\"", r.servidor);
  saida.println();
  if (r.local[0]) saida.printf("      -> redireciona para: %s\n", r.local);
  if (r.titulo[0]) saida.printf("      -> titulo da pagina: \"%s\"\n", r.titulo);
  if (r.data[0]) {
    saida.printf("      -> Date: %s", r.data);
    if (r.temData) {
      int64_t desloc, incerteza;
      deslocDeUmaAmostra(a, desloc, incerteza);
      saida.print("  = ");
      imprimirHora(desloc);
      saida.println(" (hora de MS, +-0,5 s)");
      if (guardar) {
        char nome[72];
        snprintf(nome, sizeof nome, "Date de %s", url);
        adicionarFonte(nome, DATE_PAGINA, desloc, incerteza, 0);
        if (!res.urlData[0] && strncasecmp(url, "http://", 7) == 0) snprintf(res.urlData, sizeof res.urlData, "%s", url);
      }
    } else {
      saida.println("  (formato nao reconhecido)");
    }
  } else {
    saida.println("      -> sem cabecalho Date");
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
    saida.println("  a pagina parou de responder");
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
    saida.printf("  nao deu (%d pedidos sem pegar a virada do segundo)\n", pedidos);
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
  saida.printf("  %d pedidos; virada do segundo achada: ", pedidos);
  imprimirHora(desloc);
  saida.print(" (+-");
  imprimirMs(incerteza);
  saida.println(")");
  char nome[72];
  snprintf(nome, sizeof nome, "Date refinado de %s", url);
  adicionarFonte(nome, DATE_PAGINA, desloc, incerteza, 0);
}

// ============================================================ Teste completo
void adicionarAlvo(Alvo *alvos, int &n, int max, const IPAddress &ip, const char *rotulo) {
  if (ipVazio(ip) || n >= max) return;
  for (int i = 0; i < n; i++)
    if (alvos[i].ip == ip) return;  // repetido
  alvos[n].ip = ip;
  snprintf(alvos[n].rotulo, sizeof alvos[n].rotulo, "%s (%s)", txtIp(ip), rotulo);
  n++;
}

uint8_t calcularConclusao() {
  if (!res.rodou) return 0;
  if (!res.temIP) return 6;
  if (res.internet) return 1;
  bool ntpInterno = false, ntpExterno = false, soData = false;
  for (int i = 0; i < nFontes; i++) {
    if (fontes[i].tipo == NTP_INTERNO || fontes[i].tipo == TEMPO_ANTIGO) ntpInterno = true;
    if (fontes[i].tipo == NTP_EXTERNO) ntpExterno = true;
    if (fontes[i].tipo == DATE_PAGINA) soData = true;
  }
  if (ntpInterno) return 2;
  if (ntpExterno) return 3;
  if (soData) return 4;
  return 5;
}

void imprimirResumo() {
  saida.println();
  saida.println("==================== RESUMO ====================");
  if (!res.rodou) {
    saida.println("Nenhum teste feito ainda. Digite: testar");
    return;
  }
  mostrarMac();
  saida.printf("Rede: \"%s\" (%s), potencia %s\n", rede, senha[0] ? "com senha" : "aberta", textoPotencia());
  if (res.temIP) {
    saida.printf("Wi-Fi ............ CONECTOU (IP %s)\n", txtIp(IPAddress(res.ip)));
  } else if (res.associou) {
    saida.println("Wi-Fi ............ associou, mas NAO recebeu IP (DHCP)");
  } else {
    saida.printf("Wi-Fi ............ RECUSADO (codigo %d = %s)\n", res.ultimoMotivo, motivoQueda(res.ultimoMotivo));
  }
  if (res.temIP) {
    saida.printf("Internet ......... %s\n", res.internet ? "LIBERADA" : "bloqueada (falta login/cadastro)");
    if (res.portal[0]) saida.printf("Pagina de login .. %s\n", res.portal);
    saida.printf("DNS .............. %s\n",
                 res.dnsOk ? (res.dnsDesviado ? "responde, mas desviado para IP interno" : "funciona") : "nao responde");
  }
  saida.println("Fontes de hora que funcionaram:");
  const int m = melhorFonte();
  if (m < 0) saida.println("  nenhuma");
  for (int i = 0; i < nFontes; i++) {
    const Fonte &f = fontes[i];
    saida.printf("  %-58s +-", f.nome);
    imprimirMs(f.incertezaUs);
    if (i != m) {
      const int64_t dif = f.deslocUs - fontes[m].deslocUs;
      saida.print(", difere da melhor em ");
      imprimirMs(llabs(dif));
      if (llabs(dif) > 2000000 && llabs(dif) > 2 * f.incertezaUs)
        saida.print(dif > 0 ? " (relogio dessa fonte ADIANTADO)" : " (relogio dessa fonte ATRASADO)");
    } else {
      saida.print("   <- melhor");
    }
    saida.println();
  }
  if (m >= 0) {
    saida.print("Hora agora (melhor fonte): ");
    imprimirHora(fontes[m].deslocUs);
    saida.println(" (MS)");
  }

  conclusao = calcularConclusao();
  saida.println();
  saida.printf("CONCLUSAO (%u): ", conclusao);
  switch (conclusao) {
    case 6:
      saida.println("sem IP nao ha como pegar a hora pela rede.");
      saida.println("  Peca a TI o cadastro do MAC acima, ou use o sino com GPS.");
      break;
    case 1:
      saida.println("a internet esta LIBERADA para este aparelho (MAC cadastrado ou rede aberta).");
      saida.println("  Qualquer servidor de hora funciona enquanto o cadastro existir.");
      saida.println("  Para saber o que funciona SEM cadastro, repita o teste com o MAC fora da lista.");
      break;
    case 2:
      saida.println("DA PARA USAR: um servidor de hora da propria rede responde mesmo SEM login.");
      saida.println("  O sino ja tenta esses servidores primeiro (com o DS3231 de reserva).");
      break;
    case 3:
      saida.println("o NTP da internet responde mesmo SEM login (porta 123 liberada).");
      saida.println("  Funciona, mas a TI pode fechar isso a qualquer momento.");
      break;
    case 4:
      saida.println("so as paginas informam a hora (cabecalho Date), mesmo sem login.");
      saida.println("  Da para usar, com a precisao mostrada acima; e o metodo mais fragil.");
      break;
    default:
      saida.println("NENHUMA fonte de hora sem login. Precisa do cadastro do MAC (ou do GPS).");
      break;
  }
  saida.println("O LED pisca o numero da conclusao a cada 3 s.");
  saida.println("================================================");
}

void testar() {
  if (!rede[0]) {
    saida.println("Escolha a rede primeiro. Ex.: rede IFMS-Visitantes  (use 'redes' para listar, ou a pagina)");
    return;
  }
  memset(&res, 0, sizeof res);
  res.rodou = true;
  nFontes = 0;
  conclusao = 0;
  potenciaTentada = false;
  const size_t inicioRelatorio = saida.total();
  saida.println();
  saida.println("################ TESTE DA REDE ################");
  mostrarMac();
  procurarRedes();

  titulo("1. Conexao");
  if (!conectar()) {
    desconectar();  // para de tentar (evita ficar repetindo)
    imprimirResumo();
    salvarRelatorio(inicioRelatorio, rede);
    return;
  }
  comecarEscutaNtp();  // fica escutando anuncios de hora enquanto o resto roda

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
  if (res.dnsDesviado) saida.println("  Nomes da internet viraram IP interno: o portal desvia o DNS.");

  titulo("4. Paginas (HTTP), sem fazer login");
  char url[160];
  if (!ipVazio(gw)) {
    snprintf(url, sizeof url, "http://%s/", txtIp(gw));
    testarPagina(url, true);
  }
  testarPagina("http://1.1.1.1/", true);
  testarPagina("http://connectivitycheck.gstatic.com/generate_204", true);
  if (pararPedido) goto fim;
  if (res.portal[0]) {
    titulo("5. Pagina de login (so le o cabecalho e o titulo)");
    testarPagina(res.portal, true);
  }
  titulo("5b. HTTPS para fora (passa pelo portal?)");
  testarPagina("https://1.1.1.1/", true);
  if (pararPedido) goto fim;

  {
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
      if (lerUrl(res.portal, u) && resolver(u.host, ipPortal, false)) adicionarAlvo(alvos, nAlvos, 10, ipPortal, "pagina de login");
    }
    if (resolveu[0] && !ehPrivado(ipNome[0])) adicionarAlvo(alvos, nAlvos, 10, ipNome[0], "a.ntp.br");
    adicionarAlvo(alvos, nAlvos, 10, IPAddress(200, 160, 0, 8), "a.ntp.br, IP fixo");
    adicionarAlvo(alvos, nAlvos, 10, IPAddress(200, 189, 40, 8), "b.ntp.br, IP fixo");
    adicionarAlvo(alvos, nAlvos, 10, IPAddress(216, 239, 35, 0), "time.google.com, IP fixo");
    for (int i = 0; i < nAlvos && !pararPedido; i++) consultarNtp(alvos[i].ip, alvos[i].rotulo, true);
    if (pararPedido) goto fim;

    titulo("7. Outros jeitos de pegar a hora na propria rede");
    for (int i = 0; i < nAlvos && !pararPedido; i++) {
      if (!ehPrivado(alvos[i].ip)) continue;  // so os aparelhos da rede
      sondarIcmpTimestamp(alvos[i].ip, alvos[i].rotulo);
      sondarPortaTcp(alvos[i].ip, 37, alvos[i].rotulo);
      sondarTimeUdp(alvos[i].ip, alvos[i].rotulo);
      sondarPortaTcp(alvos[i].ip, 13, alvos[i].rotulo);
    }
    terminarEscutaNtp();
  }
  if (pararPedido) goto fim;

  if (res.urlData[0]) {
    titulo("8. Refinando a hora do cabecalho Date");
    saida.printf("  %s\n", res.urlData);
    refinarData(res.urlData);
  }
fim:
  if (escutando) terminarEscutaNtp();
  if (pararPedido) saida.println("\n(teste interrompido a pedido)");
  imprimirResumo();
  salvarRelatorio(inicioRelatorio, rede);
}

// ============================================================ Vigia
// Fica conectado por um tempo, sem login, conferindo a hora a cada minuto:
// mostra se o portal derruba quem nao fez login e se a fonte continua respondendo.
void vigiar(int minutos) {
  if (!rede[0]) {
    saida.println("Escolha a rede primeiro (rede NOME).");
    return;
  }
  const size_t inicioRelatorio = saida.total();
  saida.printf("\n######## VIGIA: %d min conectado sem login, conferindo a hora a cada minuto ########\n", minutos);
  saida.println("(para interromper: parar, ou aperte o botao)");
  if (WiFi.status() != WL_CONNECTED) {
    memset(&res, 0, sizeof res);
    potenciaTentada = false;
    if (!conectar()) {
      desconectar();
      salvarRelatorio(inicioRelatorio, "vigia (nao conectou)");
      return;
    }
  }
  int alvo = -1;  // de preferencia um NTP da propria rede
  for (int i = 0; i < nFontes; i++)
    if (fontes[i].ip && fontes[i].tipo == NTP_INTERNO && alvo < 0) alvo = i;
  for (int i = 0; i < nFontes; i++)
    if (fontes[i].ip && fontes[i].tipo == NTP_EXTERNO && alvo < 0) alvo = i;
  if (alvo >= 0) {
    saida.printf("Conferindo por %s\n", fontes[alvo].nome);
  } else if (res.urlData[0]) {
    saida.printf("Sem NTP: conferindo pelo Date de %s\n", res.urlData);
  } else {
    saida.println("Sem fonte de hora conhecida (rode 'testar' antes): so vou vigiar a conexao.");
  }
  uint32_t quedas = 0, ok = 0, falhas = 0;
  int64_t deslocRef = alvo >= 0 ? fontes[alvo].deslocUs : 0;
  const uint32_t inicio = millis();
  uint32_t proxima = inicio;
  while ((int32_t)(millis() - (inicio + (uint32_t)minutos * 60000UL)) < 0 && !pararPedido) {
    if ((int32_t)(millis() - proxima) >= 0) {
      proxima += 60000;
      imprimirQuedas();
      saida.printf("[%3lu min] ", (unsigned long)((millis() - inicio) / 60000UL));
      if (WiFi.status() != WL_CONNECTED) {
        quedas++;
        saida.println("DESCONECTADO (o portal ou a rede derrubou); reconectando...");
        res.quedas = 0;
        conectar();
        continue;
      }
      saida.printf("conectado, sinal %d dBm; ", (int)WiFi.RSSI());
      if (alvo >= 0) {
        int64_t d, atraso;
        RespostaNtp r;
        int problema;
        if (ntpBruto(IPAddress(fontes[alvo].ip), d, atraso, r, problema)) {
          ok++;
          saida.print("hora ");
          imprimirHora(d);
          saida.printf(" (+-%.1f ms; %+lld ms desde o inicio, inclui a deriva do cristal do ESP)\n", atraso / 2000.0,
                       (long long)((d - deslocRef) / 1000));
        } else {
          falhas++;
          saida.printf("NTP falhou (%s)\n", problema > 0 ? textoProblemaNtp(problema) : "sem resposta");
        }
      } else if (res.urlData[0]) {
        AmostraData a;
        if (amostrarData(res.urlData, a)) {
          ok++;
          int64_t d, inc;
          deslocDeUmaAmostra(a, d, inc);
          saida.print("Date ");
          imprimirHora(d);
          saida.println(" (+-0,5 s)");
        } else {
          falhas++;
          saida.println("a pagina nao respondeu");
        }
      } else {
        saida.println("ok");
      }
    }
    delay(100);
  }
  saida.printf("\nVIGIA: %lu min; quedas da conexao: %lu; conferencias ok: %lu; falhas: %lu\n",
               (unsigned long)((millis() - inicio) / 60000UL), (unsigned long)quedas, (unsigned long)ok,
               (unsigned long)falhas);
  if (quedas == 0 && falhas == 0) saida.println("  A rede manteve o aparelho conectado e respondendo sem login.");
  salvarRelatorio(inicioRelatorio, "vigia");
}

// ============================================================ Comandos
void salvarRede() {
  prefs.putString("rede", rede);
  prefs.putString("senha", senha);
}

void ajuda() {
  saida.println();
  saida.println("Comandos (digite e tecle Enter; os mesmos valem na pagina):");
  saida.println("  mac                 mostra o MAC deste ESP (para a TI cadastrar)");
  saida.println("  redes               procura as redes Wi-Fi por perto");
  saida.println("  rede NOME           escolhe a rede (fica salvo)");
  saida.println("  senha SENHA         senha da rede; 'senha' sozinho = rede aberta (fica salva)");
  saida.println("  testar              conecta e faz todos os testes (2 a 4 minutos no maximo)");
  saida.println("  vigiar [MIN]        fica conectado MIN minutos (30) conferindo a hora a cada minuto");
  saida.println("  parar               interrompe o teste ou a vigia");
  saida.println("  resumo              mostra de novo o resumo do ultimo teste");
  saida.println("  relatorios          lista os relatorios guardados na memoria do ESP");
  saida.println("  relatorio N         mostra o relatorio N;  apagar-relatorios apaga todos");
  saida.println("  hora                hora atual segundo a melhor fonte encontrada");
  saida.println("  info                IP, roteador, DNS e sinal da conexao atual");
  saida.println("  ntp IP-ou-NOME      testa um servidor de hora (ex.: o que a TI indicar)");
  saida.println("  icmp IP             pede a hora por ICMP timestamp a um aparelho da rede");
  saida.println("  http ENDERECO       le cabecalho e titulo de uma pagina (http:// ou https://)");
  saida.println("  dns NOME            traduz um nome para IP");
  saida.println("  potencia X          potencia do Wi-Fi: max, 8.5, 11 ou 15 dBm (C3 Super Mini: 8.5)");
  saida.println("  desconectar         sai da rede;  esquecer apaga a rede e a senha salvas");
  saida.println("  reiniciar           reinicia o ESP");
  saida.println("  ajuda               esta lista");
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
    saida.printf("> senha %s\n", arg[0] ? "********" : "(vazia)");
  } else {
    saida.printf("> %s %s\n", linha, arg);
  }

  if (!strcmp(linha, "ajuda") || !strcmp(linha, "help") || !strcmp(linha, "?")) {
    ajuda();
  } else if (!strcmp(linha, "mac")) {
    mostrarMac();
    mostrarComoAcessar();
  } else if (!strcmp(linha, "redes")) {
    procurarRedes();
  } else if (!strcmp(linha, "rede")) {
    if (!arg[0]) {
      saida.printf("Rede atual: \"%s\". Para mudar: rede NOME\n", rede);
    } else {
      snprintf(rede, sizeof rede, "%s", arg);
      salvarRede();
      saida.printf("Rede escolhida: \"%s\". Se tiver senha: senha SENHA. Depois: testar\n", rede);
    }
  } else if (!strcmp(linha, "senha")) {
    snprintf(senha, sizeof senha, "%s", arg);
    salvarRede();
    saida.println(senha[0] ? "Senha salva." : "Sem senha (rede aberta).");
  } else if (!strcmp(linha, "testar")) {
    testar();
  } else if (!strcmp(linha, "vigiar")) {
    int min = arg[0] ? atoi(arg) : 30;
    if (min < 1) min = 1;
    if (min > 600) min = 600;
    vigiar(min);
  } else if (!strcmp(linha, "parar")) {
    saida.println("Nada em andamento.");
  } else if (!strcmp(linha, "resumo")) {
    imprimirResumo();
  } else if (!strcmp(linha, "relatorios")) {
    listarRelatorios(saida, false);
  } else if (!strcmp(linha, "relatorio")) {
    if (!arg[0]) {
      saida.println("Uso: relatorio NUMERO  (veja 'relatorios')");
    } else {
      mostrarRelatorio(strtoul(arg, NULL, 10));
    }
  } else if (!strcmp(linha, "apagar-relatorios")) {
    apagarRelatorios();
  } else if (!strcmp(linha, "hora")) {
    const int m = melhorFonte();
    if (m < 0) {
      saida.println("Nenhuma fonte de hora ainda. Digite: testar");
    } else {
      saida.print("Agora: ");
      imprimirHora(fontes[m].deslocUs);
      saida.printf(" (MS), segundo %s\n", fontes[m].nome);
    }
  } else if (!strcmp(linha, "info")) {
    mostrarInfo();
  } else if (!strcmp(linha, "ntp") || !strcmp(linha, "icmp")) {
    IPAddress ip;
    if (!arg[0]) {
      saida.printf("Uso: %s 10.0.0.1   ou   %s a.ntp.br\n", linha, linha);
    } else if (WiFi.status() != WL_CONNECTED) {
      saida.println("Conecte primeiro (testar).");
    } else if (ip.fromString(arg) || resolver(arg, ip, true)) {
      char rotulo[48];
      snprintf(rotulo, sizeof rotulo, "%s (%s)", txtIp(ip), arg);
      if (linha[0] == 'n') {
        consultarNtp(ip, rotulo, true);
      } else {
        sondarIcmpTimestamp(ip, rotulo);
      }
    }
  } else if (!strcmp(linha, "http")) {
    if (!arg[0]) {
      saida.println("Uso: http http://10.0.0.1/   ou   http https://pagina.de.login/");
    } else if (WiFi.status() != WL_CONNECTED) {
      saida.println("Conecte primeiro (testar).");
    } else {
      testarPagina(arg, true);
    }
  } else if (!strcmp(linha, "dns")) {
    IPAddress ip;
    if (!arg[0]) {
      saida.println("Uso: dns a.ntp.br");
    } else if (WiFi.status() != WL_CONNECTED) {
      saida.println("Conecte primeiro (testar).");
    } else {
      resolver(arg, ip, true);
    }
  } else if (!strcmp(linha, "potencia")) {
    if (!arg[0]) {
      saida.printf("Potencia atual: %s. Para mudar: potencia max | 8.5 | 11 | 15\n", textoPotencia());
    } else {
      definirPotencia(arg);
    }
  } else if (!strcmp(linha, "desconectar")) {
    desconectar();
    saida.println("Desconectado.");
  } else if (!strcmp(linha, "esquecer")) {
    rede[0] = 0;
    senha[0] = 0;
    prefs.remove("rede");
    prefs.remove("senha");
    saida.println("Rede e senha apagadas.");
  } else if (!strcmp(linha, "reiniciar")) {
    saida.println("Reiniciando...");
    delay(200);
    ESP.restart();
  } else {
    saida.println("Comando desconhecido. Digite: ajuda");
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
      if (n) enfileirar(linha);
      n = 0;
    } else if (n < sizeof(linha) - 1) {
      linha[n++] = c;
    }
  }
}

// ============================================================ main
void setup() {
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT && defined(CONFIG_IDF_TARGET_ESP32C3)
  Serial.setTxTimeoutMs(0);  // USB sem computador do outro lado: nao pode travar o teste
#endif
  delay(1500);  // tempo para o monitor serial abrir
  saida.iniciar();
  pinMode(PINO_LED, OUTPUT);
  digitalWrite(PINO_LED, !LED_ACESO);
  pinMode(PINO_BOTAO, INPUT_PULLUP);

  prefs.begin("testerede", false);
  snprintf(rede, sizeof rede, "%s", prefs.getString("rede", "").c_str());
  snprintf(senha, sizeof senha, "%s", prefs.getString("senha", "").c_str());
  potencia = prefs.getUChar("pot", 0);

  WiFi.onEvent(aoEventoWiFi);
  WiFi.persistent(false);
  WiFi.setHostname(NOME_NA_REDE);
  WiFi.mode(WIFI_AP_STA);
  WiFi.setSleep(false);  // sem economia de energia: mede o tempo da rede sem atrasos extras
  WiFi.softAP(AP_TESTE_NOME, AP_TESTE_SENHA);
  aplicarPotencia();
  iniciarArquivos();
  iniciarPagina();

  filaCmd = xQueueCreate(8, 160);
  xTaskCreate(tarefaComandos, "comandos", 16384, NULL, 1, NULL);

  saida.println();
  saida.println("=================================================");
  saida.printf(" Teste de rede para o sino (%s)\n", ESP.getChipModel());
  saida.println(" (nao faz login nem tenta passar pelo captcha)");
  mostrarMac();
  mostrarComoAcessar();
  saida.printf(" Botao BOOT = testar; LED pisca a conclusao. Potencia Wi-Fi: %s\n", textoPotencia());
  saida.println("=================================================");
  if (rede[0]) {
    saida.printf("Rede salva: \"%s\". Testando...\n", rede);
    enfileirar("testar");
  } else {
    saida.println("Comece com: redes   (depois: rede NOME, senha SENHA, testar) ou pela pagina");
  }
  ajuda();
}

void loop() {
  lerSerial();
  servidor.handleClient();
  if (!ocupado) imprimirQuedas();
  atualizarLed();
  atualizarBotao();
  delay(5);
}
