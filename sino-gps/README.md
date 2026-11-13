# Sino IFMS: sino na hora certa pela rede, com relógio de reserva

> A pasta se chama `sino-gps` porque o projeto começou com GPS. Hoje a fonte
> principal da hora é a **rede**; o GPS ficou como opção.

O sino confere a hora pela **rede da instituição** (servidor de hora NTP) e a
guarda num **DS3231**, um relógio de precisão com bateria. Quando a rede cai,
o DS3231 segura a hora e o sino continua tocando certo. Quando ela volta, a
hora é conferida de novo.

Os horários são mudados por uma **página no próprio sino**, aberta pelo Wi-Fi
do sino (nome oculto, com senha) mesmo sem internet ou, com a rede conectada,
também pela rede da instituição.

```
  servidor de hora (NTP)        rede Wi-Fi da instituição
            │                              │
            ▼                              ▼
 ┌───────────────────────┐ D6/GPIO12 ┌──────┐   127 V  ┌──────┐
 │ NodeMCU v3 (ESP-12E)  │──────────▶│ relé │─────────▶│ sino │
 │ Wi-Fi próprio oculto  │   BC548   └──────┘          └──────┘
 └───────────────────────┘
        ▲     │ I2C (D1/D2)
        │     ▼
  ┌─────────────────────────┐
  │ RTC DS3231 + bateria    │  ← o relógio que comanda o sino
  └─────────────────────────┘
```

- Quem comanda o sino é o **DS3231**. A rede só confere esse relógio, a cada
  10 minutos, e corrige se ele estiver mais de 20 ms fora.
- **Rede fora:** o DS3231 segura a hora (erro de poucos segundos por mês).
  Depois de 30 minutos sem conferir, o LED e o **aviso sonoro** avisam.
- **Queda de energia:** o DS3231 tem bateria, então o sino volta tocando na
  hora certa mesmo antes de a rede conectar.
- **GPS (opcional):** um módulo GPS no D5 também acerta o relógio, sem rede
  nenhuma (veja "GPS" mais abaixo).

## Material (por sino)

| Item | Observação |
|---|---|
| NodeMCU v3 (LoLin), relé 5 V com BC548 e fonte 5 V | esquema completo em [`../hardware`](../hardware) |
| Módulo RTC **DS3231** (ZS-042) + bateria CR2032 | lado de 4 pinos: SCL, SDA, VCC, GND |
| Botão (opcional) | toque de teste na mão |
| Snubber RC (100 Ω + 100 nF X2 275 VAC) | em paralelo com o sino (veja Segurança) |
| Módulo GPS GY-NEO6MV2 (opcional) | só se quiser a hora também sem rede |

Para testar na protoboard antes de fazer a placa, siga o
[`../hardware/PROTOBOARD.md`](../hardware/PROTOBOARD.md).

## Ligações

O esquema completo da placa (NodeMCU, DS3231, relé com BC548, fonte de 5 V,
saída de 127 V) está em [`../hardware`](../hardware). Resumo:

| Módulo | Pino | Liga em |
|---|---|---|
| DS3231 | VCC | **3V** (3,3 V do NodeMCU) |
| DS3231 | GND | G (GND) |
| DS3231 | SDA | **GPIO4 (D2)** |
| DS3231 | SCL | **GPIO5 (D1)** |
| Relé | via R1 1 kΩ + BC548 | **GPIO12 (D6)** |
| Botão de teste | um lado | **GPIO13 (D7)** |
| Botão de teste | outro lado | GND |
| GPS (opcional) | VCC | 3,3 V ou 5 V (o módulo tem regulador) |
| GPS (opcional) | GND | GND |
| GPS (opcional) | TX | **GPIO14 (D5)** |
| GPS (opcional) | RX | não ligar |

Com o NodeMCU, os resistores de boot já estão na placa. Com um ESP-12 avulso:
EN, RST, GPIO0 e GPIO2 em 3,3 V (10 kΩ) e GPIO15 em GND (10 kΩ). O LED azul do
próprio ESP-12 (GPIO2, D4) mostra o estado do sino.

> **DS3231 (ZS-042):** alimente com **3,3 V**. Esse módulo tem um circuito que
> "carrega" a bateria. Com 3,3 V ele não faz nada, mas com 5 V ele carrega a
> CR2032, que não é recarregável e pode estufar. Se tiver que usar 5 V, retire
> o diodo/resistor de carga ou use bateria LIR2032.

## Configurar e gravar

1. Abra `sino_gps/sino_gps.ino` na Arduino IDE (com o pacote ESP8266 que você
   já usa). Não precisa instalar nenhuma biblioteca.
2. Em **`config.h`**:
   - `RELE_ATIVO_EM_BAIXO`: `false` para a placa do esquema (`hardware/`, relé
     pelo BC548). Use `true` só com módulo relé que liga em 0 V. Errar isto
     deixa o sino tocando sem parar;
   - `AP_NOME`: o nome do Wi-Fi deste sino (um nome diferente para cada sino,
     ex.: `SINO-IFMS-01`, `SINO-IFMS-02`);
   - `AP_SENHA_PADRAO` e `ADMIN_SENHA_PADRAO`: senhas iniciais do Wi-Fi e da
     página. Troque-as aqui ou, depois, pela própria página;
   - o fuso já está em UTC-4 (MS).
3. **`horarios.h`** é só a tabela inicial (vem com um exemplo de 1,5 s nas
   trocas de aula e 2,5 s no fim do período). Depois de instalado, os horários
   são mudados pela página e ficam guardados na memória do sino. Cada linha é
   `{hora, minuto, dias, duração_ms, vezes}`:
   ```cpp
   {7, 0, SEG_A_SEX, 1500, 1},   // 07:00, seg a sex, 1,5 s
   {11, 30, SEG_A_SEX, 2500, 1}, // fim do período, 2,5 s
   {8, 0, SAB, 1500, 1},         // só sábado
   ```
4. Placa: **NodeMCU 1.0 (ESP-12E Module)** (ou Generic ESP8266 Module com um
   ESP-12 avulso). Grave. Não precisa de nenhuma biblioteca além do pacote ESP8266.
5. Na página do sino, em "Rede e senhas", digite o nome e a senha da rede da
   instituição. Se a TI indicar um servidor de hora interno, digite em
   "Servidor de hora"; sem ele, o sino usa o informado pelo DHCP e o `a.ntp.br`.

## GPS (opcional)

Só para quem quiser a hora também sem rede nenhuma. Ligue o TX do GPS no D5
(conector J3 da placa). O sino usa o GPS e a rede ao mesmo tempo, se tiver os dois.

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

## Página do sino (mudar os horários)

### Como abrir

O sino cria uma rede Wi-Fi própria, **com o nome oculto**: ela não aparece na
lista de redes do celular, e quem não sabe o nome nem tenta entrar.

1. No celular ou notebook, adicione a rede **manualmente**:
   - **Android:** Wi-Fi → "Adicionar rede" → nome `SINO-IFMS-01`, segurança
     WPA/WPA2-Personal, senha do Wi-Fi do sino.
   - **iPhone:** Ajustes → Wi-Fi → "Outra..." → nome, segurança WPA2, senha.
   - **Windows:** Wi-Fi → "Redes ocultas" → nome e senha.
2. Abra **http://192.168.4.1** no navegador.
3. Usuário **admin** e a senha da página.

O celular pode avisar que "a rede não tem internet": é normal, mantenha conectado.

Quando o MAC do sino estiver cadastrado na rede da instituição (configure a
rede na própria página), a mesma página abre também por ela, em
`http://sino-ifms-01.local` ou pelo IP que aparece na página. O MAC aparece no
monitor serial ao ligar e no comando `wifi`.

### O que dá para fazer

| Parte da página | Para que serve |
|---|---|
| Relógio e próximo toque | ver se o sino está certo; o selo verde "hora conferida" indica que a rede (ou o GPS) conferiu a hora nos últimos 30 min |
| **Tocar agora (teste)** | faz o sino bater uma vez (pede confirmação) |
| **Horários** | adicionar, mudar ou remover horários: hora, dias da semana, duração e número de batidas. **Clique em "Salvar horários"**: o sino passa a usar a tabela na hora e ela fica guardada mesmo sem energia |
| Copiar a tabela para outro sino | copia a tabela como texto para colar na página de outro sino |
| Acertar pela hora deste aparelho | só para quando a rede ainda não conferiu a hora; a rede corrige depois |
| Rede e senhas | rede da instituição, servidor de hora, senha do Wi-Fi do sino e senha da página (o sino reinicia depois de salvar) |

**Troque as duas senhas na primeira vez que abrir a página.**

### Segurança da página

- Rede do sino com nome oculto e senha (WPA2).
- Login da página no padrão "Digest": a senha não passa aberta pela rede,
  nem quando a página é aberta pela rede da instituição.
- Depois de 10 senhas erradas seguidas, o login fica bloqueado por 1 minuto.
- Esqueceu a senha? Ligue o sino no computador pelo cabo e use o comando
  `senhas-padrao` no monitor serial: as senhas voltam às do `config.h`.

## Aviso sonoro

Se o relógio ficar mais de **30 minutos** sem ser conferido (rede fora), cada
toque dos horários ganha uma batida curta no fim:

```
toque normal (1,5 s ou 2,5 s) ... pausa de 0,5 s ... batida de 0,5 s
```

O sino continua tocando na hora certa pelo DS3231. O aviso só serve para quem
ouvir saber que a rede do sino está fora. Ele some sozinho quando a rede volta
e a hora é conferida. A página também mostra "AVISO SONORO LIGADO". Tempos e
liga/desliga em `config.h` (`ALERTA_SEM_ACERTO_MIN`, `AVISO_SONORO`,
`AVISO_PAUSA_MS`, `AVISO_BATIDA_MS`).

## LED azul do ESP-12

O LED pisca no início de cada segundo. Com dois sinos lado a lado, os LEDs
piscam juntos, o que é um teste visual de que estão sincronizados.

| LED | Significado |
|---|---|
| 1 piscada por segundo | tudo certo (hora conferida pela rede nos últimos 30 min) |
| 2 piscadas por segundo | funcionando só pelo DS3231 (rede fora): toca normalmente |
| piscando rápido | **sem hora**: não vai tocar (DS3231 sem hora e a rede ainda não conferiu) |
| aceso direto | tocando |

## Monitor serial (115200 baud, com "Nova linha")

| Comando | O que faz |
|---|---|
| `status` | hora, situação da rede, do RTC e do GPS, próximo toque |
| `ntp` | confere a hora pela rede agora e mostra a diferença do DS3231 |
| `horarios` | lista a tabela gravada |
| `toque` | toque de teste |
| `gps` | liga/desliga a exibição das mensagens cruas do GPS |
| `hora 2026-11-02 06:59:50` | acerta a hora local na mão (para teste sem rede) |
| `wifi` | rede do sino, rede da instituição e o MAC do sino |
| `senhas-padrao` | volta as senhas do Wi-Fi e da página às do `config.h` e reinicia |

Com a rede conectada, a hora acertada na mão é corrigida em poucos segundos.

## Teste de bancada (antes de instalar)

O roteiro completo, com a montagem na protoboard, está em
[`../hardware/PROTOBOARD.md`](../hardware/PROTOBOARD.md). Resumo:

1. Pelo celular, abra a página (veja acima), troque as senhas e configure a
   rede da instituição (em casa, use o roteador de casa ou o do celular).
2. No monitor serial devem aparecer `Rede: hora conferida` e
   `RTC acertado pela rede (NTP)`. O LED passa a dar 1 piscada por segundo.
3. Mude um horário para daqui a 2 minutos, salve e espere o relé bater.
4. Desligue o roteador: depois de 30 min os toques ganham o aviso sonoro
   (para testar mais rápido, use `ALERTA_SEM_ACERTO_MIN 2` no `config.h`).
5. Com dois sinos prontos, ligue os dois lado a lado e veja se os LEDs piscam juntos.

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

- **Hora pela rede:** a cada 10 min (`INTERVALO_NTP_MIN`) o sino faz 4
  perguntas ao servidor de hora e fica com a de menor tempo de ida e volta.
  Se o DS3231 estiver mais de 20 ms fora (`NTP_CORRIGE_ACIMA_MS`), a hora
  certa é gravada nele exatamente na virada do segundo. Nada disso trava o
  sino, e nenhuma conferência começa a menos de 15 s de um toque.
- **Servidores de hora:** primeiro o digitado na página, depois o informado
  pelo DHCP da rede, depois `a.ntp.br`, `b.ntp.br` e `pool.ntp.br`. Se um falha,
  o sino passa para o próximo.
- **Precisão:** o ESP lê o DS3231 a cada 5 ms. Pela rede, a margem de cada
  conferência é metade do tempo de ida e volta até o servidor: poucos ms com
  um servidor dentro da instituição, algumas dezenas de ms com o `a.ntp.br`
  pela internet. O comando `ntp` mostra a margem. Sinos conferidos pelo mesmo
  servidor ficam mais próximos entre si. Para comparar: o som leva 20 ms para
  andar 7 m.
- **Com GPS (opcional):**
  - a hora é gravada no DS3231 na virada do segundo do GPS. Na simulação, os
    toques saíram com erro abaixo de 8 ms. Entre sinos com GPS, a diferença
    depende principalmente de usarem o mesmo modelo de módulo
    (`GPS_ATRASO_NMEA_MS`);
  - logo que é ligado, o GPS pode errar a hora em 1 a 3 s até receber dos
    satélites a tabela de "segundos intercalares". Por isso o DS3231 só é
    corrigido pelo GPS depois de 15 min de sinal. A exceção é quando o DS3231
    está sem hora nenhuma: aí ele é acertado na hora e corrigido de novo 15 min
    depois;
  - alguns NEO-6M antigos ou clones informam a data 1024 semanas (~19,6 anos)
    atrasada. O firmware detecta isso e corrige;
  - enquanto o ESP envia a página, os caracteres do GPS ficam na fila; o
    firmware desconta esse atraso ao marcar a hora de chegada (e pula o acerto
    se a fila estiver cheia demais).
- **Rede da instituição:** se não conectar em 30 s, o sino desiste e tenta de
  novo a cada 10 min (`REDE_INST_ESPERA_MIN`). Enquanto procura a rede, o Wi-Fi
  do sino oscila um pouco; por isso ele não fica tentando o tempo todo.
- **Memória:** horários e senhas ficam na EEPROM do ESP, com conferência (CRC).
  Se a memória estiver vazia ou corrompida, o sino volta à tabela de
  `horarios.h` e às senhas do `config.h`.
- **Testes no PC** (para quem for mexer no código):
  ```
  cd testes
  g++ -std=c++17 -Wall -I../sino_gps teste_logica.cpp -o teste && ./teste
  g++ -std=c++17 -Wall -I../sino_gps teste_web.cpp -o teste_web && ./teste_web
  g++ -std=c++17 -Wall -I../sino_gps teste_ntp.cpp -o teste_ntp && ./teste_ntp
  ```
