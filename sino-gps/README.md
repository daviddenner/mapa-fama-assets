# Sino GPS: sinos sincronizados sem Wi-Fi

Cada sino acerta o próprio relógio pelo **GPS**. Ele não usa Wi-Fi, roteador,
internet nem cadastro de endereço MAC, então nada que a TI mude na rede faz o
sino parar.

Todos os receptores GPS recebem a hora dos mesmos relógios atômicos dos
satélites. Por isso sinos em prédios diferentes, sem nenhuma ligação entre
eles, tocam juntos, com diferença de poucos milissegundos.

```
  satélites GPS
       │  (hora UTC exata)
       ▼
 ┌───────────┐  TX→GPIO14  ┌─────────┐ GPIO12 ┌──────┐   127 V  ┌──────┐
 │ GPS NEO-6M│────────────▶│ ESP-12  │───────▶│ relé │─────────▶│ sino │
 └───────────┘             └─────────┘        └──────┘          └──────┘
                             ▲     │ I2C (GPIO4/GPIO5)
                             │     ▼
                           ┌─────────────────────────┐
                           │ RTC DS3231 + bateria    │  ← o relógio que comanda o sino
                           └─────────────────────────┘
```

- Quem comanda o sino é o **DS3231** (relógio de precisão com bateria). O GPS
  só corrige esse relógio, uma vez por hora.
- **Sem sinal de GPS**, o DS3231 segura a hora (erro típico de poucos segundos
  por mês). Quando o sinal volta, ele é corrigido de novo.
- **Queda de energia:** o DS3231 tem bateria, então o sino volta tocando na
  hora certa mesmo antes de o GPS pegar sinal.
- O rádio Wi-Fi do ESP fica desligado.

## Por que GPS e não GPRS

| | GPS | GPRS (SIM800L) |
|---|---|---|
| Custo mensal | nenhum | um chip com plano em cada sino |
| Depende de terceiros | não | da operadora (troca o problema da TI pelo da operadora) |
| Precisão da hora | microssegundos | segundos (e nem toda operadora envia hora pela rede) |
| Futuro | GPS não tem previsão de desligar | a rede 2G está sendo desativada aos poucos |
| Alimentação | ~45 mA | picos de 2 A, e o módulo reinicia se a fonte não aguentar |
| Ponto fraco | a antena precisa "ver" o céu | sinal de celular dentro do prédio |

## Material (por sino)

| Item | Observação |
|---|---|
| ESP-12E/F, relé e fonte | os mesmos da versão Wi-Fi |
| Módulo GPS **GY-NEO6MV2** (NEO-6M) ou NEO-M8N | **use o mesmo modelo em todos os sinos** |
| Módulo RTC **DS3231** (ZS-042) + bateria CR2032 | |
| Botão (opcional) | toque de teste na mão |
| Antena GPS ativa com cabo SMA de 3 a 5 m (opcional) | só onde o módulo não pega sinal (precisa de módulo GPS com conector) |
| Snubber RC (100 Ω + 100 nF X2 275 VAC) | em paralelo com o sino (veja Segurança) |

## Ligações

| Módulo | Pino | Liga em |
|---|---|---|
| GPS | VCC | 3,3 V ou 5 V (o módulo tem regulador) |
| GPS | GND | GND |
| GPS | TX | **GPIO14** |
| GPS | RX | não ligar |
| DS3231 | VCC | **3,3 V** |
| DS3231 | GND | GND |
| DS3231 | SDA | **GPIO4** |
| DS3231 | SCL | **GPIO5** |
| Relé | IN | **GPIO12** |
| Botão de teste | um lado | **GPIO13** |
| Botão de teste | outro lado | GND |

Os resistores de boot do ESP-12 continuam como na versão atual: EN, RST, GPIO0
e GPIO2 em 3,3 V (10 kΩ) e GPIO15 em GND (10 kΩ). O LED azul do próprio ESP-12
(GPIO2) mostra o estado do sino.

> **DS3231 (ZS-042):** alimente com **3,3 V**. Esse módulo tem um circuito que
> "carrega" a bateria. Com 3,3 V ele não faz nada, mas com 5 V ele carrega a
> CR2032, que não é recarregável e pode estufar. Se tiver que usar 5 V, retire
> o diodo/resistor de carga ou use bateria LIR2032.

## Configurar e gravar

1. Abra `sino_gps/sino_gps.ino` na Arduino IDE (com o pacote ESP8266 que você
   já usa). Não precisa instalar nenhuma biblioteca.
2. Edite **`horarios.h`** com os horários do campus. O arquivo vem com um
   **exemplo**. Cada linha é: `{hora, minuto, dias, duração_ms, vezes}`.
   ```cpp
   {7, 0, SEG_A_SEX, 5000, 1},   // 07:00, seg a sex, 5 s, 1 batida
   {9, 30, SEG_A_SEX, 2000, 2},  // 09:30, 2 batidas de 2 s
   {8, 0, SAB, 3000, 1},         // só sábado
   ```
3. Em **`config.h`**, ajuste `RELE_ATIVO_EM_BAIXO` para o seu módulo relé (o
   mesmo comportamento da versão atual). O fuso já está em UTC-4 (MS).
4. Placa: **Generic ESP8266 Module** (ou NodeMCU / Wemos se for o caso). Grave.
5. **Grave exatamente o mesmo firmware em todos os sinos.** Assim todos têm a
   mesma tabela de horários.

## Instalação do GPS

- A antena (o quadrado de cerâmica) fica **virada para cima** e precisa "ver"
  o céu. Laje de concreto e telha metálica bloqueiam o sinal. Perto de uma
  janela ou acima do forro, embaixo de telha cerâmica ou de fibrocimento,
  costuma funcionar.
- O LED do módulo GPS pisca 1 vez por segundo quando ele tem sinal. Da primeira
  vez pode levar de 1 a 15 minutos.
- O sino **não precisa de GPS o tempo todo**. Com o DS3231 ele funciona dias
  sem sinal; basta o GPS pegar sinal de vez em quando.
- Em sala sem sinal nenhum, use um módulo GPS com conector SMA e uma antena
  ativa externa do lado de fora.

## LED azul do ESP-12

O LED pisca no início de cada segundo. Com dois sinos lado a lado, os LEDs
piscam juntos, o que é um teste visual de que estão sincronizados.

| LED | Significado |
|---|---|
| 1 piscada por segundo | tudo certo (corrigido pelo GPS nas últimas 6 h) |
| 2 piscadas por segundo | funcionando só pelo DS3231; toca normalmente, mas confira a antena |
| piscando rápido | **sem hora**: não vai tocar (DS3231 sem hora e GPS ainda sem sinal) |
| aceso direto | tocando |

## Monitor serial (115200 baud, com "Nova linha")

| Comando | O que faz |
|---|---|
| `status` | hora, situação do GPS e do RTC, próximo toque |
| `horarios` | lista a tabela gravada |
| `toque` | toque de teste |
| `gps` | liga/desliga a exibição das mensagens cruas do GPS |
| `hora 2026-11-02 06:59:50` | acerta a hora local na mão (para teste sem GPS) |

Se o GPS tiver sinal, ele corrige em poucos segundos uma hora acertada na mão.

## Teste de bancada (antes de instalar)

1. Sem GPS: `hora AAAA-MM-DD 06:59:50` com a data da próxima segunda-feira.
   Em 10 s o relé deve bater (toque das 07:00 do exemplo).
2. Com o GPS na janela: o `status` deve mostrar `GPS: com sinal` e aparecer
   `RTC acertado pelo GPS`. Depois disso o LED passa a dar 1 piscada por segundo.
3. Com dois sinos prontos, ligue os dois lado a lado e veja se os LEDs piscam juntos.

## Segurança (127 V)

- O relé interrompe só a **fase** do sino. Mantenha a fiação de 127 V afastada
  do ESP e do GPS, dentro de caixa fechada.
- Ponha um **snubber RC** (100 Ω + 100 nF classe X2 275 VAC) em paralelo com o
  sino. A bobina do sino gera faíscas e picos que gastam o relé e podem
  reiniciar o ESP.
- O firmware nunca deixa o relé ligado mais de 30 s (`DURACAO_MAX_MS`). Se o
  ESP reiniciar no meio de um toque, ele não toca de novo no mesmo minuto.
- Feriados e férias: o sino segue a tabela da semana. Para desligar nesses dias,
  use um interruptor em série com o sino.

## Detalhes técnicos

- **Precisão:** a hora é gravada no DS3231 exatamente na virada do segundo do
  GPS, e o ESP lê o DS3231 a cada 5 ms. Na simulação, os toques saíram com
  erro abaixo de 8 ms em relação à hora real. Entre sinos, a diferença depende
  principalmente de usarem o mesmo modelo de GPS (`GPS_ATRASO_NMEA_MS`). Para
  comparar: o som leva 20 ms para andar 7 m.
- **15 minutos após ligar:** logo que é ligado, o GPS pode errar a hora em 1 a
  3 s até receber dos satélites a tabela de "segundos intercalares". Por isso
  o DS3231 só é corrigido depois de 15 min de sinal. A exceção é quando o
  DS3231 está sem hora nenhuma: aí ele é acertado na hora e corrigido de novo
  15 min depois.
- **Módulos GPS com data errada:** alguns NEO-6M antigos ou clones informam a
  data 1024 semanas (~19,6 anos) atrasada. O firmware detecta isso e corrige.
- **Testes no PC** (para quem for mexer no código):
  `cd testes && g++ -std=c++17 -Wall -I../sino_gps teste_logica.cpp -o teste && ./teste`
