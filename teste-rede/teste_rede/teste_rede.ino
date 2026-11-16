// ============================================================================
//  Teste de rede e da placa do sino (ESP-12/ESP8266, ESP32 ou ESP32-C3)
//
//  Descobre se dá para pegar a hora certa pela rede da instituição SEM fazer o
//  login da página (captcha) e sem o MAC estar cadastrado. Tenta TODAS as
//  formas que o sino poderia usar:
//    - servidores de hora (NTP) da própria rede: o informado pelo DHCP, o
//      roteador, os servidores DNS, a página de login; e os da internet;
//    - a hora do cabeçalho "Date" das páginas (inclusive a de login), refinada
//      para alguns milissegundos;
//    - ICMP timestamp, TIME (porta 37), DAYTIME (porta 13) e NTP por difusão.
//  Testa também a placa do sino: relógio DS3231 (e quanto ele está fora da
//  hora da rede) e relé. Mostra o MAC (para pedir o cadastro à TI), explica
//  por que a conexão foi recusada e termina com um resumo e uma conclusão.
//  Ele NÃO faz login e não tenta passar pelo captcha.
//
//  Três jeitos de usar, todos ao mesmo tempo:
//    1. Monitor serial: 115200 baud, final de linha "Nova linha". Digite "ajuda".
//    2. Celular: rede Wi-Fi "TESTE-SINO" (senha sino12345), http://192.168.4.1
//    3. Sem nada: botão FLASH (ESP-12) ou BOOT (ESP32) inicia o teste; o LED
//       pisca a conclusão. O relatório fica guardado na memória do ESP.
//
//  Placa na Arduino IDE:
//    ESP-12 avulso: pacote esp8266, "Generic ESP8266 Module", Flash Size 4MB
//                   (FS:2MB OTA:~1019KB)
//    NodeMCU:       pacote esp8266, "NodeMCU 1.0 (ESP-12E Module)"
//    ESP32:         pacote esp32, "ESP32 Dev Module"
//    ESP32-C3:      pacote esp32, "ESP32C3 Dev Module", "USB CDC On Boot: Enabled"
// ============================================================================
#include "analise.h"
#include "compat.h"
#include "pagina_teste.h"
#include "registro.h"
#include "tempo.h"

// ------------------------------------------------------------ Configuração
#define FUSO_MINUTOS (-240)         // Mato Grosso do Sul: UTC-4
#define NOME_NA_REDE "sino-teste"   // nome que aparece para a TI na lista de aparelhos
#define AP_TESTE_NOME "TESTE-SINO"  // Wi-Fi do próprio ESP, para a página
#define AP_TESTE_SENHA "sino12345"
#define RELE_ATIVO_EM_BAIXO 0  // 0 = relé pelo BC548 (como no sino); 1 = módulo relé que liga em 0 V

#if defined(ESP8266)
// ESP-12 / NodeMCU: os mesmos pinos do sino (veja hardware/PROTOBOARD_ESP12.md)
#define MAX_RELATORIOS 4
#define PINO_SDA 4           // D2: SDA do DS3231
#define PINO_SCL 5           // D1: SCL do DS3231
#define PINO_RELE 12         // D6: resistor de 1 kΩ até a base do BC548
#define PINO_BOTAO_RELE 13   // D7: botão S1 até o GND = testar o relé
#define PINO_BOTAO_TESTE 0   // FLASH (GPIO0) = testar tudo
#define PINO_LED 2           // LED azul do próprio ESP-12 (acende em 0)
#define LED_ACESO LOW
#elif defined(CONFIG_IDF_TARGET_ESP32C3)
#define MAX_RELATORIOS 6
#define PINO_SDA 4
#define PINO_SCL 5
#define PINO_RELE 3
#define PINO_BOTAO_RELE 1
#define PINO_BOTAO_TESTE 9  // BOOT
#define PINO_LED 8          // LED azul das placas C3 "Super Mini" (acende em 0)
#define LED_ACESO LOW
#else  // ESP32 Dev Module
#define MAX_RELATORIOS 6
#define PINO_SDA 21
#define PINO_SCL 22
#define PINO_RELE 26
#define PINO_BOTAO_RELE 27
#define PINO_BOTAO_TESTE 0  // BOOT
#define PINO_LED 2
#define LED_ACESO HIGH
#endif

// ------------------------------------------------------------------ Estado
Registro saida;  // tudo o que o teste mostra: monitor serial + página + relatórios
// Mensagens com o formato na flash (no ESP8266 a RAM é pouca): SAIDA("...", ...)
#define SAIDA(formato, ...) saida.printfP(PSTR(formato), ##__VA_ARGS__)
Preferences prefs;
ServidorWeb servidor(80);
char rede[33] = "";
char senha[65] = "";
volatile bool ocupado = false;      // um teste (ou a vigia) está rodando
volatile bool pararPedido = false;  // alguém pediu para parar
uint8_t conclusao = 0;              // código do resumo (o LED pisca este número)
uint8_t potencia = 0;               // potência do Wi-Fi: 0 = máxima; senão dBm x 2
bool potenciaTentada = false;       // já tentou reduzir a potência neste teste?
WiFiUDP escutaNtp;
bool escutando = false;

// Eventos do Wi-Fi: chegam por fora do laço principal e são lidos depois.
volatile bool evAssociou = false;
volatile bool evTemIP = false;
volatile uint8_t filaMotivo[16];
volatile uint8_t filaIni = 0, filaFim = 0;

enum TipoFonte { NTP_INTERNO, NTP_EXTERNO, DATE_PAGINA, TEMPO_ANTIGO };
struct Fonte {  // uma fonte de hora que funcionou
  char nome[72];
  TipoFonte tipo;
  int64_t deslocUs;     // hora da fonte - relógio do ESP
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
const char *txtIp(const IPAddress &ip) {
  static char buf[4][16];
  static uint8_t i = 0;
  char *b = buf[i++ % 4];
  snprintf(b, 16, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
  return b;
}
bool ipVazio(const IPAddress &ip) { return (uint32_t)ip == 0; }
bool ehPrivado(const IPAddress &ip) { return ipPrivado(ip[0], ip[1]); }

void titulo(const char *t) {
  servicoFundo();
  SAIDA("\n---- %s ----\n", t);
}

void imprimirMs(int64_t us) {
  if (us < 10000) {
    SAIDA("%.1f ms", us / 1000.0);
  } else {
    SAIDA("%lld ms", (long long)(us / 1000));
  }
}

void imprimirHora(int64_t deslocUs) {
  const int64_t ms = (agoraUs() + deslocUs) / 1000;  // ms desde 1970 (UTC)
  const DataHora t = deEpoch((uint32_t)(ms / 1000) + FUSO_MINUTOS * 60);
  SAIDA("%02u/%02u/%04u %02u:%02u:%02u,%03u", t.dia, t.mes, t.ano, t.hora, t.min, t.seg, (unsigned)(ms % 1000));
}

// Hora entre 2024 e 2100? (aparelho com relógio desacertado informa 2000, 1970...)
bool horaPlausivel(int64_t deslocUs) {
  const int64_t s = (agoraUs() + deslocUs) / 1000000LL;
  return s >= 1704067200LL && s < 4102444800LL;
}

void adicionarFonte(const char *nome, int tipo, int64_t desloc, int64_t incerteza, uint32_t ip) {
  if (!horaPlausivel(desloc)) {
    saida.println(F("      (hora absurda: o relogio desse aparelho esta desacertado; ignorada)"));
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

// "dd/mm/aaaa hh:mm:ss" pela melhor fonte; sem fonte, pelo DS3231; senão "hora desconhecida"
void textoHoraAgora(char *dst, size_t tam) {
  const int m = melhorFonte();
  uint32_t s;
  const char *origem = "";
  if (m >= 0) {
    s = (uint32_t)((agoraUs() + fontes[m].deslocUs) / 1000000LL);
  } else if (horaDoRtc(s)) {
    origem = " (DS3231)";
  } else {
    snprintf(dst, tam, "hora desconhecida");
    return;
  }
  const DataHora t = deEpoch(s + FUSO_MINUTOS * 60);
  snprintf(dst, tam, "%02u/%02u/%04u %02u:%02u:%02u%s", t.dia, t.mes, t.ano, t.hora, t.min, t.seg, origem);
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
  SAIDA("MAC deste %s: %s   <- e este que a TI cadastra\n", NOME_CHIP, WiFi.macAddress().c_str());
  SAIDA("Nome na rede:    %s\n", NOME_NA_REDE);
}

void mostrarComoAcessar() {
  SAIDA("Pagina deste teste: Wi-Fi \"%s\" (senha %s) -> http://%s/\n", AP_TESTE_NOME, AP_TESTE_SENHA,
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

void aplicarPotencia() { wifiPotencia(potencia); }

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
    saida.println(F("Uso: potencia max | 8.5 | 11 | 15"));
    return;
  }
  potencia = nova;
  prefs.putUChar("pot", potencia);
  aplicarPotencia();
  SAIDA("Potencia do Wi-Fi: %s (salva).\n", textoPotencia());
}

// ============================================================ Wi-Fi
void anotarQueda(uint8_t motivo) {
  evTemIP = false;
  const uint8_t f = filaFim;
  filaMotivo[f % 16] = motivo;
  filaFim = f + 1;
}

#if defined(ESP8266)
WiFiEventHandler aoAssociar, aoReceberIp, aoCair;  // precisam ficar guardados

void iniciarEventosWiFi() {
  aoAssociar = WiFi.onStationModeConnected([](const WiFiEventStationModeConnected &) { evAssociou = true; });
  aoReceberIp = WiFi.onStationModeGotIP([](const WiFiEventStationModeGotIP &) { evTemIP = true; });
  aoCair = WiFi.onStationModeDisconnected(
      [](const WiFiEventStationModeDisconnected &e) { anotarQueda((uint8_t)e.reason); });
}
#else
void aoEventoWiFi(WiFiEvent_t evento, WiFiEventInfo_t info) {
  switch (evento) {
    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      evAssociou = true;
      break;
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      evTemIP = true;
      break;
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      anotarQueda(info.wifi_sta_disconnected.reason);
      break;
    default:
      break;
  }
}

void iniciarEventosWiFi() { WiFi.onEvent(aoEventoWiFi); }
#endif

void imprimirQuedas() {
  while (filaIni != filaFim) {
    const uint8_t m = filaMotivo[filaIni % 16];
    filaIni = filaIni + 1;
    res.quedas++;
    res.ultimoMotivo = m;
    SAIDA("  recusado/desconectado: codigo %u = %s\n", m, motivoQueda(m));
  }
}

void desconectar() {
  WiFi.disconnect();
  pausa(300);
  filaIni = filaFim;  // descarta o aviso da nossa própria desconexão
  evAssociou = false;
  evTemIP = false;
}

bool tentarConectar() {
  desconectar();
  pedirNtpAoDhcp();
  SAIDA("Conectando em \"%s\" (%s, potencia %s)...\n", rede, senha[0] ? "com senha" : "rede aberta", textoPotencia());
  saida.println(F("  (enquanto conecta, a pagina pode ficar alguns segundos sem responder)"));
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
      SAIDA("  associou ao roteador em %.1f s; esperando o IP (DHCP)...\n", (millis() - inicio) / 1000.0);
      limite = millis() + 15000;  // mais 15 s para receber o IP
    }
    if (evTemIP) {
      res.associou = true;
      res.temIP = true;
      res.ip = (uint32_t)WiFi.localIP();
      SAIDA("  CONECTADO em %.1f s, IP %s\n", (millis() - inicio) / 1000.0, txtIp(WiFi.localIP()));
      return true;
    }
    pausa(50);
  }
  imprimirQuedas();
  if (res.associou) {
    saida.println(F("  FALHOU: associou ao roteador, mas o DHCP nao entregou IP."));
    saida.println(F("  (provavel: a rede so entrega IP para MAC cadastrado)"));
  } else if (res.quedas) {
    SAIDA("  FALHOU: o roteador recusou. Ultimo codigo %d = %s\n", res.ultimoMotivo, motivoQueda(res.ultimoMotivo));
  } else {
    saida.println(F("  FALHOU: sem resposta do roteador (rede fora de alcance?)"));
  }
  return false;
}

// Falha que parece ser de rádio (e não de cadastro)? Aí vale tentar com menos potência.
bool pareceProblemaDeRadio() {
  if (res.temIP) return false;
  if (!res.associou && res.quedas == 0) return true;  // ninguém respondeu
  const int m = res.ultimoMotivo;
  return m == 2 || m == 15 || m == 201 || m == 204 || m == 205;
}

bool conectar() {
  if (tentarConectar()) return true;
  if (potencia == 0 && !potenciaTentada && pareceProblemaDeRadio()) {
    potenciaTentada = true;
    saida.println(F("  Tentando de novo com menos potencia de Wi-Fi (placas com antena ruim costumam precisar)..."));
    potencia = 17;
    const bool ok = tentarConectar();
    if (ok) {
      prefs.putUChar("pot", potencia);
      saida.println(F("  Funcionou com 8,5 dBm: potencia salva ('potencia max' desfaz)."));
    } else {
      potencia = 0;
      aplicarPotencia();
    }
    return ok;
  }
  return false;
}

void mostrarInfo() {
  if (WiFi.status() != WL_CONNECTED) {
    saida.println(F("Nao conectado."));
    return;
  }
  SAIDA("  IP do ESP ........ %s\n", txtIp(WiFi.localIP()));
  SAIDA("  Mascara .......... %s\n", txtIp(WiFi.subnetMask()));
  SAIDA("  Roteador ......... %s\n", txtIp(WiFi.gatewayIP()));
  SAIDA("  DNS .............. %s  %s\n", txtIp(WiFi.dnsIP(0)), txtIp(WiFi.dnsIP(1)));
  const IPAddress n = ntpInformadoPeloDhcp();
  SAIDA("  NTP do DHCP ...... %s\n", ipVazio(n) ? "nenhum informado" : txtIp(n));
  SAIDA("  Sinal ............ %d dBm, canal %ld, roteador %s\n", (int)WiFi.RSSI(), (long)WiFi.channel(),
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
  saida.println(F("  sinal  canal  seguranca                   nome"));
  for (int i = 0; i < total; i++) {
    const int k = ordem[i];
    const String nome = WiFi.SSID(k);
    SAIDA("  %4ld   %4ld   %-26s  %s%s\n", (long)WiFi.RSSI(k), (long)WiFi.channel(k), segurancaDaRede(k),
                 nome.length() ? nome.c_str() : "(oculta)",
                 (rede[0] && strcmp(nome.c_str(), rede) == 0) ? "   <- escolhida" : "");
  }
  WiFi.scanDelete();
  saida.println(F("  Para escolher: rede NOME"));
}

// ============================================================ DNS
bool resolver(const char *nome, IPAddress &ip, bool imprimir) {
  const int64_t t0 = agoraUs();
  const bool ok = WiFi.hostByName(nome, ip) == 1 && !ipVazio(ip);
  if (imprimir) {
    if (ok) {
      SAIDA("  %-30s -> %s", nome, txtIp(ip));
      if (ehPrivado(ip)) saida.print(F("  (IP interno!)"));
      SAIDA("  em %lld ms\n", (long long)((agoraUs() - t0) / 1000));
    } else {
      SAIDA("  %-30s -> nao resolveu\n", nome);
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
  while (agoraUs() - t1 < 1500000) {  // sem atender a página aqui: atrapalharia a medida
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
  servicoFundo();
  SAIDA("  %-36s ", rotulo);
  int64_t desloc, atraso;
  RespostaNtp r;
  int problema;
  if (!ntpBruto(ip, desloc, atraso, r, problema)) {
    if (problema == -1) {
      saida.println(F("erro ao abrir UDP"));
    } else if (problema == -2) {
      saida.println(F("nao consegui enviar"));
    } else if (problema > 0) {
      saida.println(textoProblemaNtp(problema));
    } else {
      saida.println(F("sem resposta"));
    }
    return false;
  }
  char ref[20];
  textoRefid(r, ref, sizeof ref);
  saida.print(F("RESPONDEU  "));
  imprimirHora(desloc);
  saida.print(F("  (+-"));
  imprimirMs(atraso / 2);
  SAIDA(", estrato %u, ref %s)\n", r.estrato, ref);
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
  static Url u;
  if (!lerUrl(url, u)) {
    erro = "endereco invalido";
    return false;
  }
  WiFiClient simples;
  std::unique_ptr<WiFiClientSecure> seguro;  // só é criado para https (no ESP8266 ele reserva ~6 KB)
  Client *c;
  if (u.https) {
    bool blocosPequenos = false;
    if (!memoriaParaTls(u.host, u.porta, blocosPequenos, erro)) return false;
    seguro.reset(new WiFiClientSecure());
    if (!conectarTls(*seguro, u.host, u.porta, blocosPequenos, erro)) return false;
    c = seguro.get();
  } else {
    if (!conectarTcp(simples, u.host, u.porta, 4000)) {
      erro = "nao conectou";
      return false;
    }
    c = &simples;
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
  c->print("\r\nUser-Agent: Mozilla/5.0 (teste-sino-esp)\r\nAccept: */*\r\nConnection: close\r\n\r\n");

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
#if defined(ESP8266)
    static char corpo[1536];
#else
    static char corpo[3072];
#endif
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
  servicoFundo();
  SAIDA("  %s\n", url);
  static RespostaHttp r;
  AmostraData a;
  const char *erro;
  if (!pedirHttp(url, r, a, true, erro)) {
    SAIDA("      -> %s\n", erro);
    return false;
  }
  if (r.status < 0) {
    saida.println(F("      -> respondeu, mas nao e HTTP"));
    return true;
  }
  SAIDA("      -> status %d em %lld ms", r.status, (long long)((a.recebUs - a.envioUs) / 1000));
  if (r.servidor[0]) SAIDA(", servidor \"%s\"", r.servidor);
  saida.println();
  if (r.local[0]) SAIDA("      -> redireciona para: %s\n", r.local);
  if (r.titulo[0]) SAIDA("      -> titulo da pagina: \"%s\"\n", r.titulo);
  if (r.data[0]) {
    SAIDA("      -> Date: %s", r.data);
    if (r.temData) {
      int64_t desloc, incerteza;
      deslocDeUmaAmostra(a, desloc, incerteza);
      saida.print(F("  = "));
      imprimirHora(desloc);
      saida.println(F(" (hora de MS, +-0,5 s)"));
      if (guardar) {
        char nome[72];
        snprintf(nome, sizeof nome, "Date de %s", url);
        adicionarFonte(nome, DATE_PAGINA, desloc, incerteza, 0);
        if (!res.urlData[0] && strncasecmp(url, "http://", 7) == 0) snprintf(res.urlData, sizeof res.urlData, "%s", url);
      }
    } else {
      saida.println(F("  (formato nao reconhecido)"));
    }
  } else {
    saida.println(F("      -> sem cabecalho Date"));
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
  static RespostaHttp r;
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
    saida.println(F("  a pagina parou de responder"));
    return;
  }
  bool virou = false;
  const uint32_t inicio = millis();
  while (millis() - inicio < 2500) {
    pausa(100);
    if (!amostrarData(url, at)) break;
    pedidos++;
    if (at.segundo == ant.segundo + 1) {
      virou = true;
      break;
    }
    ant = at;
  }
  if (!virou) {
    SAIDA("  nao deu (%d pedidos sem pegar a virada do segundo)\n", pedidos);
    return;
  }
  AmostraData par[2] = {ant, at};
  int64_t desloc = 0, incerteza = 0;
  acharVirada(par, 2, desloc, incerteza);

  // Próxima virada (para o segundo at.segundo + 1), no relógio do ESP:
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
  SAIDA("  %d pedidos; virada do segundo achada: ", pedidos);
  imprimirHora(desloc);
  saida.print(F(" (+-"));
  imprimirMs(incerteza);
  saida.println(F(")"));
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
  saida.println(F("==================== RESUMO ===================="));
  if (!res.rodou) {
    saida.println(F("Nenhum teste feito ainda. Digite: testar"));
    return;
  }
  mostrarMac();
  SAIDA("Rede: \"%s\" (%s), potencia %s\n", rede, senha[0] ? "com senha" : "aberta", textoPotencia());
  if (res.temIP) {
    SAIDA("Wi-Fi ............ CONECTOU (IP %s)\n", txtIp(IPAddress(res.ip)));
  } else if (res.associou) {
    saida.println(F("Wi-Fi ............ associou, mas NAO recebeu IP (DHCP)"));
  } else {
    SAIDA("Wi-Fi ............ RECUSADO (codigo %d = %s)\n", res.ultimoMotivo, motivoQueda(res.ultimoMotivo));
  }
  if (res.temIP) {
    SAIDA("Internet ......... %s\n", res.internet ? "LIBERADA" : "bloqueada (falta login/cadastro)");
    if (res.portal[0]) SAIDA("Pagina de login .. %s\n", res.portal);
    SAIDA("DNS .............. %s\n",
                 res.dnsOk ? (res.dnsDesviado ? "responde, mas desviado para IP interno" : "funciona") : "nao responde");
  }
  resumoPlaca();
  saida.println(F("Fontes de hora que funcionaram:"));
  const int m = melhorFonte();
  if (m < 0) saida.println(F("  nenhuma"));
  for (int i = 0; i < nFontes; i++) {
    const Fonte &f = fontes[i];
    SAIDA("  %-58s +-", f.nome);
    imprimirMs(f.incertezaUs);
    if (i != m) {
      const int64_t dif = f.deslocUs - fontes[m].deslocUs;
      saida.print(F(", difere da melhor em "));
      imprimirMs(llabs(dif));
      if (llabs(dif) > 2000000 && llabs(dif) > 2 * f.incertezaUs)
        saida.print(dif > 0 ? " (relogio dessa fonte ADIANTADO)" : " (relogio dessa fonte ATRASADO)");
    } else {
      saida.print(F("   <- melhor"));
    }
    saida.println();
  }
  if (m >= 0) {
    saida.print(F("Hora agora (melhor fonte): "));
    imprimirHora(fontes[m].deslocUs);
    saida.println(F(" (MS)"));
  }

  conclusao = calcularConclusao();
  saida.println();
  SAIDA("CONCLUSAO (%u): ", conclusao);
  switch (conclusao) {
    case 6:
      saida.println(F("sem IP nao ha como pegar a hora pela rede."));
      saida.println(F("  Peca a TI o cadastro do MAC acima, ou use o sino com GPS."));
      break;
    case 1:
      saida.println(F("a internet esta LIBERADA para este aparelho (MAC cadastrado ou rede aberta)."));
      saida.println(F("  Qualquer servidor de hora funciona enquanto o cadastro existir."));
      saida.println(F("  Para saber o que funciona SEM cadastro, repita o teste com o MAC fora da lista."));
      break;
    case 2:
      saida.println(F("DA PARA USAR: um servidor de hora da propria rede responde mesmo SEM login."));
      saida.println(F("  O sino ja tenta esses servidores primeiro (com o DS3231 de reserva)."));
      break;
    case 3:
      saida.println(F("o NTP da internet responde mesmo SEM login (porta 123 liberada)."));
      saida.println(F("  Funciona, mas a TI pode fechar isso a qualquer momento."));
      break;
    case 4:
      saida.println(F("so as paginas informam a hora (cabecalho Date), mesmo sem login."));
      saida.println(F("  Da para usar, com a precisao mostrada acima; e o metodo mais fragil."));
      break;
    default:
      saida.println(F("NENHUMA fonte de hora sem login. Precisa do cadastro do MAC (ou do GPS)."));
      break;
  }
  saida.println(F("O LED pisca o numero da conclusao a cada 3 s."));
  saida.println(F("================================================"));
}

void testar() {
  if (!rede[0]) {
    saida.println(F("Escolha a rede primeiro. Ex.: rede IFMS-Visitantes  (use 'redes' para listar, ou a pagina)"));
    return;
  }
  memset(&res, 0, sizeof res);
  res.rodou = true;
  nFontes = 0;
  conclusao = 0;
  potenciaTentada = false;
  abrirRelatorio(rede);
  saida.println();
  saida.println(F("################ TESTE DA REDE E DA PLACA ################"));
  mostrarMac();
  testarPlaca();
  procurarRedes();

  titulo("1. Conexao");
  if (!conectar()) {
    desconectar();  // para de tentar (evita ficar repetindo)
    imprimirResumo();
    fecharRelatorio();
    return;
  }
  escutando = escutarMulticast(escutaNtp, IPAddress(224, 0, 1, 1), 123);  // escuta anúncios de hora

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
  if (res.dnsDesviado) saida.println(F("  Nomes da internet viraram IP interno: o portal desvia o DNS."));

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
    static Alvo alvos[10];
    int nAlvos = 0;
    adicionarAlvo(alvos, nAlvos, 10, ntpInformadoPeloDhcp(), "informado pelo DHCP");
    adicionarAlvo(alvos, nAlvos, 10, gw, "roteador");
    adicionarAlvo(alvos, nAlvos, 10, dns1, "DNS da rede");
    adicionarAlvo(alvos, nAlvos, 10, dns2, "DNS da rede");
    if (res.portal[0]) {
      static Url u;
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
      if (!ehPrivado(alvos[i].ip)) continue;  // só os aparelhos da rede
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
    SAIDA("  %s\n", res.urlData);
    refinarData(res.urlData);
  }
  if (pararPedido) goto fim;
  compararRtcComRede();
fim:
  if (escutando) terminarEscutaNtp();
  if (pararPedido) saida.println(F("\n(teste interrompido a pedido)"));
  imprimirResumo();
  fecharRelatorio();
}

// ============================================================ Vigia
// Fica conectado por um tempo, sem login, conferindo a hora a cada minuto:
// mostra se o portal derruba quem não fez login e se a fonte continua respondendo.
void vigiar(int minutos) {
  if (!rede[0]) {
    saida.println(F("Escolha a rede primeiro (rede NOME)."));
    return;
  }
  abrirRelatorio("vigia");
  SAIDA("\n######## VIGIA: %d min conectado sem login, conferindo a hora a cada minuto ########\n", minutos);
  saida.println(F("(para interromper: parar, ou aperte o botao FLASH/BOOT)"));
  if (WiFi.status() != WL_CONNECTED) {
    memset(&res, 0, sizeof res);
    potenciaTentada = false;
    if (!conectar()) {
      desconectar();
      fecharRelatorio();
      return;
    }
  }
  int alvo = -1;  // de preferência um NTP da própria rede
  for (int i = 0; i < nFontes; i++)
    if (fontes[i].ip && fontes[i].tipo == NTP_INTERNO && alvo < 0) alvo = i;
  for (int i = 0; i < nFontes; i++)
    if (fontes[i].ip && fontes[i].tipo == NTP_EXTERNO && alvo < 0) alvo = i;
  if (alvo >= 0) {
    SAIDA("Conferindo por %s\n", fontes[alvo].nome);
  } else if (res.urlData[0]) {
    SAIDA("Sem NTP: conferindo pelo Date de %s\n", res.urlData);
  } else {
    saida.println(F("Sem fonte de hora conhecida (rode 'testar' antes): so vou vigiar a conexao."));
  }
  uint32_t quedas = 0, ok = 0, falhas = 0;
  int64_t deslocRef = alvo >= 0 ? fontes[alvo].deslocUs : 0;
  const uint32_t inicio = millis();
  uint32_t proxima = inicio;
  while ((int32_t)(millis() - (inicio + (uint32_t)minutos * 60000UL)) < 0 && !pararPedido) {
    if ((int32_t)(millis() - proxima) >= 0) {
      proxima += 60000;
      imprimirQuedas();
      SAIDA("[%3lu min] ", (unsigned long)((millis() - inicio) / 60000UL));
      if (WiFi.status() != WL_CONNECTED) {
        quedas++;
        saida.println(F("DESCONECTADO (o portal ou a rede derrubou); reconectando..."));
        res.quedas = 0;
        conectar();
        continue;
      }
      SAIDA("conectado, sinal %d dBm; ", (int)WiFi.RSSI());
      if (alvo >= 0) {
        int64_t d, atraso;
        RespostaNtp r;
        int problema;
        if (ntpBruto(IPAddress(fontes[alvo].ip), d, atraso, r, problema)) {
          ok++;
          saida.print(F("hora "));
          imprimirHora(d);
          SAIDA(" (+-%.1f ms; %+lld ms desde o inicio, inclui a deriva do cristal do ESP)\n", atraso / 2000.0,
                       (long long)((d - deslocRef) / 1000));
        } else {
          falhas++;
          SAIDA("NTP falhou (%s)\n", problema > 0 ? textoProblemaNtp(problema) : "sem resposta");
        }
      } else if (res.urlData[0]) {
        AmostraData a;
        if (amostrarData(res.urlData, a)) {
          ok++;
          int64_t d, inc;
          deslocDeUmaAmostra(a, d, inc);
          saida.print(F("Date "));
          imprimirHora(d);
          saida.println(F(" (+-0,5 s)"));
        } else {
          falhas++;
          saida.println(F("a pagina nao respondeu"));
        }
      } else {
        saida.println(F("ok"));
      }
      sincronizarRelatorio();
    }
    pausa(100);
  }
  SAIDA("\nVIGIA: %lu min; quedas da conexao: %lu; conferencias ok: %lu; falhas: %lu\n",
               (unsigned long)((millis() - inicio) / 60000UL), (unsigned long)quedas, (unsigned long)ok,
               (unsigned long)falhas);
  if (quedas == 0 && falhas == 0) saida.println(F("  A rede manteve o aparelho conectado e respondendo sem login."));
  fecharRelatorio();
}

// ============================================================ Comandos
void salvarRede() {
  prefs.putString("rede", rede);
  prefs.putString("senha", senha);
}

void ajuda() {
  saida.println();
  saida.println(F("Comandos (digite e tecle Enter; os mesmos valem na pagina):"));
  saida.println(F("  mac                 mostra o MAC deste ESP (para a TI cadastrar)"));
  saida.println(F("  redes               procura as redes Wi-Fi por perto"));
  saida.println(F("  rede NOME           escolhe a rede (fica salvo)"));
  saida.println(F("  senha SENHA         senha da rede; 'senha' sozinho = rede aberta (fica salva)"));
  saida.println(F("  testar              testa a placa, conecta e testa a rede (2 a 4 minutos)"));
  saida.println(F("  vigiar [MIN]        fica conectado MIN minutos (30) conferindo a hora a cada minuto"));
  saida.println(F("  parar               interrompe o teste ou a vigia"));
  saida.println(F("  resumo              mostra de novo o resumo do ultimo teste"));
  saida.println(F("  rtc                 testa o relogio DS3231 (e compara com a hora da rede)"));
  saida.println(F("  rtc acertar         grava a hora da rede no DS3231, na virada do segundo"));
  saida.println(F("  rele [MS]           liga o rele por MS milissegundos (500)"));
  saida.println(F("  relatorios          lista os relatorios guardados na memoria do ESP"));
  saida.println(F("  relatorio N         mostra o relatorio N;  apagar-relatorios apaga todos"));
  saida.println(F("  hora                hora atual segundo a melhor fonte encontrada"));
  saida.println(F("  info                IP, roteador, DNS e sinal da conexao atual"));
  saida.println(F("  ntp IP-ou-NOME      testa um servidor de hora (ex.: o que a TI indicar)"));
  saida.println(F("  icmp IP             pede a hora por ICMP timestamp a um aparelho da rede"));
  saida.println(F("  http ENDERECO       le cabecalho e titulo de uma pagina (http:// ou https://)"));
  saida.println(F("  dns NOME            traduz um nome para IP"));
  saida.println(F("  potencia X          potencia do Wi-Fi: max, 8.5, 11 ou 15 dBm"));
  saida.println(F("  desconectar         sai da rede;  esquecer apaga a rede e a senha salvas"));
  saida.println(F("  reiniciar           reinicia o ESP"));
  saida.println(F("  ajuda               esta lista"));
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
    SAIDA("> senha %s\n", arg[0] ? "********" : "(vazia)");
  } else {
    SAIDA("> %s %s\n", linha, arg);
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
      SAIDA("Rede atual: \"%s\". Para mudar: rede NOME\n", rede);
    } else {
      snprintf(rede, sizeof rede, "%s", arg);
      salvarRede();
      SAIDA("Rede escolhida: \"%s\". Se tiver senha: senha SENHA. Depois: testar\n", rede);
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
    saida.println(F("Nada em andamento."));
  } else if (!strcmp(linha, "resumo")) {
    imprimirResumo();
  } else if (!strcmp(linha, "rtc")) {
    if (!strcasecmp(arg, "acertar")) {
      acertarRtc();
    } else {
      testarRtc(true);
    }
  } else if (!strcmp(linha, "rele")) {
    testarRele(arg[0] ? (uint32_t)atol(arg) : 500);
  } else if (!strcmp(linha, "relatorios")) {
    listarRelatorios(saida, false);
  } else if (!strcmp(linha, "relatorio")) {
    if (!arg[0]) {
      saida.println(F("Uso: relatorio NUMERO  (veja 'relatorios')"));
    } else {
      mostrarRelatorio(strtoul(arg, NULL, 10));
    }
  } else if (!strcmp(linha, "apagar-relatorios")) {
    apagarRelatorios();
  } else if (!strcmp(linha, "hora")) {
    const int m = melhorFonte();
    if (m < 0) {
      saida.println(F("Nenhuma fonte de hora ainda. Digite: testar"));
    } else {
      saida.print(F("Agora: "));
      imprimirHora(fontes[m].deslocUs);
      SAIDA(" (MS), segundo %s\n", fontes[m].nome);
    }
  } else if (!strcmp(linha, "info")) {
    mostrarInfo();
  } else if (!strcmp(linha, "ntp") || !strcmp(linha, "icmp")) {
    IPAddress ip;
    if (!arg[0]) {
      SAIDA("Uso: %s 10.0.0.1   ou   %s a.ntp.br\n", linha, linha);
    } else if (WiFi.status() != WL_CONNECTED) {
      saida.println(F("Conecte primeiro (testar)."));
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
      saida.println(F("Uso: http http://10.0.0.1/   ou   http https://pagina.de.login/"));
    } else if (WiFi.status() != WL_CONNECTED) {
      saida.println(F("Conecte primeiro (testar)."));
    } else {
      testarPagina(arg, true);
    }
  } else if (!strcmp(linha, "dns")) {
    IPAddress ip;
    if (!arg[0]) {
      saida.println(F("Uso: dns a.ntp.br"));
    } else if (WiFi.status() != WL_CONNECTED) {
      saida.println(F("Conecte primeiro (testar)."));
    } else {
      resolver(arg, ip, true);
    }
  } else if (!strcmp(linha, "potencia")) {
    if (!arg[0]) {
      SAIDA("Potencia atual: %s. Para mudar: potencia max | 8.5 | 11 | 15\n", textoPotencia());
    } else {
      definirPotencia(arg);
    }
  } else if (!strcmp(linha, "desconectar")) {
    desconectar();
    saida.println(F("Desconectado."));
  } else if (!strcmp(linha, "esquecer")) {
    rede[0] = 0;
    senha[0] = 0;
    prefs.remove("rede");
    prefs.remove("senha");
    saida.println(F("Rede e senha apagadas."));
  } else if (!strcmp(linha, "reiniciar")) {
    saida.println(F("Reiniciando..."));
    delay(200);
    ESP.restart();
  } else {
    saida.println(F("Comando desconhecido. Digite: ajuda"));
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
  iniciarPlaca();  // primeiro: relé desligado
  Serial.begin(115200);
#if ARDUINO_USB_CDC_ON_BOOT && defined(CONFIG_IDF_TARGET_ESP32C3)
  Serial.setTxTimeoutMs(0);  // USB sem computador do outro lado: não pode travar o teste
#endif
  delay(1500);  // tempo para o monitor serial abrir
  pinMode(PINO_LED, OUTPUT);
  digitalWrite(PINO_LED, !LED_ACESO);
  pinMode(PINO_BOTAO_TESTE, INPUT_PULLUP);

  iniciarArquivos();  // antes das preferências: no ESP8266 elas ficam em arquivos
  prefs.begin("testerede", false);
  snprintf(rede, sizeof rede, "%s", prefs.getString("rede", "").c_str());
  snprintf(senha, sizeof senha, "%s", prefs.getString("senha", "").c_str());
  potencia = prefs.getUChar("pot", 0);

  iniciarEventosWiFi();
  WiFi.persistent(false);
  wifiNomeNaRede(NOME_NA_REDE);
  WiFi.mode(WIFI_AP_STA);
  wifiSemEconomia();  // sem economia de energia: mede o tempo da rede sem atrasos extras
  WiFi.softAP(AP_TESTE_NOME, AP_TESTE_SENHA);
  aplicarPotencia();
  iniciarPagina();

  saida.println();
  saida.println(F("================================================="));
  SAIDA(" Teste de rede e da placa do sino (%s)\n", NOME_CHIP);
  saida.println(F(" (nao faz login nem tenta passar pelo captcha)"));
  mostrarMac();
  mostrarComoAcessar();
  SAIDA(" Botao GPIO%d (FLASH/BOOT) = testar tudo; S1 (GPIO%d) = testar o rele\n", PINO_BOTAO_TESTE,
               PINO_BOTAO_RELE);
  SAIDA(" LED pisca a conclusao. Potencia Wi-Fi: %s\n", textoPotencia());
  saida.println(F("================================================="));
  if (rede[0]) {
    SAIDA("Rede salva: \"%s\". Testando...\n", rede);
    enfileirar("testar");
  } else {
    saida.println(F("Comece com: redes   (depois: rede NOME, senha SENHA, testar) ou pela pagina"));
  }
  ajuda();
}

void loop() {
  servicoFundo();
  if (!ocupado) {
    char linha[160];
    if (tirarDaFila(linha, sizeof linha)) {
      ocupado = true;
      pararPedido = false;
      executar(linha);
      ocupado = false;
    } else {
      imprimirQuedas();
    }
  }
  delay(2);
}
