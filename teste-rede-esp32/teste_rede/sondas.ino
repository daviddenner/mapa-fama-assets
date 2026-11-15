// ============================================================================
//  Outras formas de pegar a hora na rede, além do NTP e do cabeçalho Date:
//    - ICMP "timestamp" (tipo 13): muitos roteadores respondem com a hora do
//      dia em ms, mesmo sem login (o Windows normalmente não responde);
//    - protocolos antigos TIME (porta 37, TCP e UDP) e DAYTIME (porta 13);
//    - NTP por difusão (broadcast/multicast): o ESP só escuta.
//  Nenhuma delas envia dado nenhum além do próprio pedido.
// ============================================================================
#include <lwip/sockets.h>

// ------------------------------------------------------------ ICMP timestamp
bool sondarIcmpTimestamp(const IPAddress &ip, const char *rotulo) {
  saida.printf("  ICMP timestamp %-22s ", rotulo);
  const int s = lwip_socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (s < 0) {
    saida.println("(este pacote nao deixa abrir socket ICMP)");
    return false;
  }
  struct timeval espera = {1, 0};
  lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &espera, sizeof espera);
  static uint16_t seq = 0;
  seq++;
  const uint16_t id = 0x5153;
  uint8_t pedido[20];
  montarIcmpTimestamp(pedido, id, seq, (uint32_t)(agoraUs() / 1000));
  struct sockaddr_in para;
  memset(&para, 0, sizeof para);
  para.sin_family = AF_INET;
  para.sin_addr.s_addr = (uint32_t)ip;
  const int64_t t1 = agoraUs();
  if (lwip_sendto(s, pedido, sizeof pedido, 0, (struct sockaddr *)&para, sizeof para) < 0) {
    saida.println("nao consegui enviar");
    lwip_close(s);
    return false;
  }
  bool ok = false;
  for (;;) {
    uint8_t resp[96];
    struct sockaddr_in de;
    socklen_t tamDe = sizeof de;
    const int n = lwip_recvfrom(s, resp, sizeof resp, 0, (struct sockaddr *)&de, &tamDe);
    const int64_t t4 = agoraUs();
    if (n <= 0) {
      saida.println("sem resposta");
      break;
    }
    uint32_t recebMs, envioMs;
    if (!lerIcmpTimestamp(resp, n, id, seq, recebMs, envioMs)) {
      if (t4 - t1 > 1500000) {
        saida.println("sem resposta");
        break;
      }
      continue;  // outro pacote ICMP qualquer (ex.: ping de alguem)
    }
    const int64_t rtt = t4 - t1;
    // hora do dia (UTC) do aparelho ~ no meio do caminho; convertida para MS
    int64_t localMs = ((int64_t)envioMs + (int64_t)FUSO_MINUTOS * 60000LL) % 86400000LL;
    if (localMs < 0) localMs += 86400000LL;
    saida.printf("RESPONDEU  %02ld:%02ld:%02ld,%03ld (hora de MS, sem data; +-%.1f ms)", (long)(localMs / 3600000),
                 (long)(localMs / 60000 % 60), (long)(localMs / 1000 % 60), (long)(localMs % 1000), rtt / 2000.0);
    const int m = melhorFonte();
    if (m >= 0) {
      const uint32_t melhorMs = (uint32_t)(((agoraUs() + fontes[m].deslocUs) / 1000) % 86400000LL);
      const int32_t dif = diferencaNoDiaMs(envioMs, melhorMs) + (int32_t)(rtt / 2000);
      saida.printf("; difere da melhor fonte em %ld ms", (long)dif);
    }
    saida.println();
    ok = true;
    break;
  }
  lwip_close(s);
  return ok;
}

// ------------------------------------------------- TIME (37) e DAYTIME (13)
bool sondarPortaTcp(const IPAddress &ip, uint16_t porta, const char *rotulo) {
  saida.printf("  TCP %-3u %-30s ", porta, rotulo);
  WiFiClient c;
  const int64_t t1 = agoraUs();
  if (!c.connect(ip, porta, 1000)) {
    saida.println("fechada ou sem resposta");
    return false;
  }
  uint8_t b[64];
  int n = 0;
  const uint32_t limite = millis() + 1500;
  while (n < (int)sizeof b - 1 && (int32_t)(millis() - limite) < 0) {
    if (c.available()) {
      b[n++] = (uint8_t)c.read();
      if (porta == 37 && n >= 4) break;
    } else if (!c.connected()) {
      break;
    } else {
      delay(1);
    }
  }
  const int64_t t4 = agoraUs();
  c.stop();
  if (n == 0) {
    saida.println("conectou, mas nao mandou nada");
    return false;
  }
  if (porta == 37) {
    if (n < 4) {
      saida.println("resposta curta demais");
      return false;
    }
    const int64_t desloc = (int64_t)epochDoTime868(b) * 1000000LL - (t1 + t4) / 2;
    saida.print("RESPONDEU  ");
    imprimirHora(desloc);
    saida.println(" (+-0,5 s)");
    char nome[72];
    snprintf(nome, sizeof nome, "TIME (porta 37) %s", rotulo);
    adicionarFonte(nome, TEMPO_ANTIGO, desloc, 500000 + (t4 - t1) / 2, (uint32_t)ip);
    return true;
  }
  b[n] = 0;
  while (n && (b[n - 1] == '\r' || b[n - 1] == '\n')) b[--n] = 0;
  saida.printf("RESPONDEU  \"%s\"\n", (char *)b);
  return true;
}

bool sondarTimeUdp(const IPAddress &ip, const char *rotulo) {
  saida.printf("  UDP 37  %-30s ", rotulo);
  WiFiUDP u;
  if (!u.begin(41037)) {
    saida.println("erro ao abrir UDP");
    return false;
  }
  u.beginPacket(ip, 37);
  u.write((uint8_t)0);
  const int64_t t1 = agoraUs();
  u.endPacket();
  while (agoraUs() - t1 < 800000) {
    if (u.parsePacket() >= 4) {
      const int64_t t4 = agoraUs();
      uint8_t b[8];
      u.read(b, 4);
      u.stop();
      const int64_t desloc = (int64_t)epochDoTime868(b) * 1000000LL - (t1 + t4) / 2;
      saida.print("RESPONDEU  ");
      imprimirHora(desloc);
      saida.println(" (+-0,5 s)");
      char nome[72];
      snprintf(nome, sizeof nome, "TIME (UDP 37) %s", rotulo);
      adicionarFonte(nome, TEMPO_ANTIGO, desloc, 500000 + (t4 - t1) / 2, (uint32_t)ip);
      return true;
    }
    delay(1);
  }
  u.stop();
  saida.println("sem resposta");
  return false;
}

// ------------------------------------------------- NTP por difusão (só escuta)
void comecarEscutaNtp() {
  escutando = escutaNtp.beginMulticast(IPAddress(224, 0, 1, 1), 123) != 0;
}

void terminarEscutaNtp() {
  saida.print("  NTP por difusao (porta 123, so escutando): ");
  int achou = 0;
  if (escutando) {
    while (escutaNtp.parsePacket() > 0) {
      uint8_t b[64];
      const int n = escutaNtp.read(b, sizeof b);
      if (n < 48) continue;
      const uint8_t modo = b[0] & 7;
      saida.printf("\n      pacote de %s, modo %u%s, estrato %u", txtIp(escutaNtp.remoteIP()), modo,
                   modo == 5 ? " (difusao)" : "", b[1]);
      achou++;
    }
    escutaNtp.stop();
    escutando = false;
  }
  if (achou) {
    saida.println("\n      (ha um servidor anunciando a hora para a rede toda)");
  } else {
    saida.println("nada chegou durante o teste (normal: poucas redes usam)");
  }
}
