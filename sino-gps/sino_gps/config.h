// ============================================================================
//  CONFIGURAÇÃO DO SINO  (use exatamente o mesmo arquivo em todos os sinos)
// ============================================================================
#pragma once

// ---------------------------------------------------------------- Fuso horário
// Mato Grosso do Sul: UTC-4 (uma hora a menos que Brasília), sem horário de verão.
// Se um dia voltar o horário de verão, basta mudar para -180 e regravar.
#define FUSO_MINUTOS (-240)

// ------------------------------------------------------------- Pinos (ESP-12)
// Evite GPIO0, GPIO2 e GPIO15 para o relé: eles mudam de estado no boot.
// Nomes do NodeMCU entre parênteses (é a placa do esquema em hardware/).
#define PINO_RELE 12         // GPIO12 (D6) -> resistor de base do BC548 (ou IN do módulo relé)
#define PINO_GPS_RX 14       // GPIO14 (D5) <- TX do módulo GPS (opcional)
#define PINO_SDA 4           // GPIO4  (D2) <-> SDA do DS3231
#define PINO_SCL 5           // GPIO5  (D1) ->  SCL do DS3231
#define PINO_BOTAO_TESTE 13  // GPIO13 (D7) -> botão até o GND (toque manual). Opcional.
#define PINO_LED 2           // GPIO2  (D4): LED azul do próprio ESP-12 (acende em LOW)

// Como o relé é acionado. ERRAR ISTO DEIXA O SINO TOCANDO SEM PARAR.
//   false = o relé liga quando o pino vai a 3,3 V: placa do esquema (BC548),
//           módulo relé com jumper "H", módulo com transistor ou SSR
//   true  = o relé liga quando o pino vai a 0 V (módulo relé com optoacoplador)
#define RELE_ATIVO_EM_BAIXO false

// --------------------------------------------------------------------- Toques
#define INTERVALO_BATIDAS_MS 1000  // pausa entre batidas quando "vezes" > 1
#define DURACAO_MAX_MS 30000       // segurança: o relé nunca fica ligado mais que isto
#define DURACAO_TESTE_MS 2000      // toque do botão de teste / comando "toque"

// O toque ainda acontece se o sino só "perceber" o horário até este número de
// segundos depois (ex.: relógio corrigido bem em cima do horário, ou o ESP
// ocupado alguns segundos atendendo um aparelho lento na página). Melhor tocar
// um pouco atrasado do que não tocar.
#define JANELA_TOQUE_S 30

// ------------------------------------------------------------------------ GPS
#define GPS_BAUD 9600

// Atraso (ms) entre a virada do segundo no GPS e o início da sentença RMC.
// Só afeta a hora "absoluta" em alguns centésimos; como todos os sinos usam o
// mesmo valor e o mesmo modelo de GPS, eles continuam tocando juntos.
#define GPS_ATRASO_NMEA_MS 60

// Logo que é ligado, o GPS pode errar a hora em 1 a 3 s até receber dos
// satélites a tabela de "segundos intercalares" (enviada a cada 12,5 min).
// Por isso o RTC só é acertado depois de este tempo total com sinal.
// (Exceção: se o RTC estiver sem hora nenhuma, ele é acertado na hora.)
#define GPS_ESTAVEL_MIN 15

#define INTERVALO_SYNC_MIN 60  // de quanto em quanto tempo o GPS corrige o RTC

// ------------------------------------------------------ Hora pela rede (NTP)
// Com a rede da instituição conectada, o sino confere a hora num servidor NTP
// a cada INTERVALO_NTP_MIN e corrige o DS3231 se a diferença passar de
// NTP_CORRIGE_ACIMA_MS. Quando a rede cai, o DS3231 segura a hora (erro de
// poucos segundos por mês). Ordem de tentativa: o servidor digitado na página,
// o informado pelo DHCP da rede e estes:
#define NTP_SERVIDOR_1 "a.ntp.br"
#define NTP_SERVIDOR_2 "b.ntp.br"
#define NTP_SERVIDOR_3 "pool.ntp.br"
#define INTERVALO_NTP_MIN 10
#define NTP_CORRIGE_ACIMA_MS 20

// O relógio precisa ser conferido (pela rede ou pelo GPS) pelo menos a cada
// ALERTA_SEM_ACERTO_MIN. Passou disso (ex.: rede fora), o LED pisca 2 vezes por
// segundo e liga o aviso sonoro.
#define ALERTA_SEM_ACERTO_MIN 30

// ----------------------------------------------------------- Aviso sonoro
// Se o relógio ficar mais de ALERTA_SEM_ACERTO_MIN sem ser conferido (rede
// fora), cada toque dos horários ganha uma batida curta no fim:
//   toque normal ... pausa de 0,5 s ... batida de 0,5 s
// Quem ouvir sabe que é preciso olhar o sino. O sino continua na hora certa
// pelo DS3231; o aviso some sozinho quando a rede volta e a hora é conferida.
#define AVISO_SONORO 1  // 0 = sem aviso sonoro
#define AVISO_PAUSA_MS 500
#define AVISO_BATIDA_MS 500

// --------------------------------------------------------- Wi-Fi e página
// O sino cria uma rede Wi-Fi própria, sempre ligada e com o nome OCULTO.
// Para abrir a página: no celular/notebook, "adicionar rede" manualmente com
// este nome e senha, e depois abrir http://192.168.4.1
#define AP_NOME "SINO-IFMS-01"         // um nome diferente para cada sino
#define AP_SENHA_PADRAO "troque-esta-senha"  // mínimo 8 caracteres (dá para trocar na página)
#define AP_OCULTO 1                    // 1 = o nome não aparece na lista de redes
#define AP_CANAL 6

// Login da página (o usuário é fixo; a senha dá para trocar na própria página).
#define ADMIN_USUARIO "admin"
#define ADMIN_SENHA_PADRAO "troque-tambem"

// Rede da instituição (opcional). Também dá para configurar pela página.
// Quando o MAC do sino estiver cadastrado, a página abre também por ela, em
// http://NOME_NA_REDE.local ou pelo IP que aparece na página.
#define REDE_INST_NOME ""
#define REDE_INST_SENHA ""
#define NOME_NA_REDE "sino-ifms-01"

// Se a rede da instituição não conectar em 30 s, desiste e tenta de novo
// depois deste tempo (enquanto tenta, a rede própria do sino oscila um pouco).
#define REDE_INST_ESPERA_MIN 10
