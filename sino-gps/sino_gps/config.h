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
#define PINO_RELE 12         // GPIO12 -> IN do módulo relé
#define PINO_GPS_RX 14       // GPIO14 <- TX do módulo GPS
#define PINO_SDA 4           // GPIO4  <-> SDA do DS3231
#define PINO_SCL 5           // GPIO5  ->  SCL do DS3231
#define PINO_BOTAO_TESTE 13  // GPIO13 -> botão até o GND (toque manual). Opcional.
#define PINO_LED 2           // GPIO2: LED azul do próprio ESP-12 (acende em LOW)

// Use o mesmo valor da versão Wi-Fi que já funciona aí:
//   true  = o relé liga quando o pino vai a 0 V (maioria dos módulos com optoacoplador)
//   false = o relé liga quando o pino vai a 3,3 V (jumper "H" / módulo com transistor / SSR)
#define RELE_ATIVO_EM_BAIXO true

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
#define ALERTA_SEM_GPS_H 6     // LED (e o aviso sonoro) avisam se ficou este tempo sem acerto

// ----------------------------------------------------------- Aviso sonoro
// Se o relógio ficar mais de ALERTA_SEM_GPS_H horas sem ser acertado (GPS sem
// sinal), cada toque dos horários ganha uma batida curta no fim:
//   toque normal ... pausa de 0,5 s ... batida de 0,5 s
// Quem ouvir sabe que é preciso olhar o sino. O sino continua na hora certa
// pelo DS3231; o aviso some sozinho quando o GPS volta a acertar o relógio.
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
