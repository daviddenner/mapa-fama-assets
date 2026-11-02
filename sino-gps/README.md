# Sino GPS: sinos sincronizados sem depender da rede

Cada sino acerta o próprio relógio pelo **GPS**. Para tocar na hora certa ele
não precisa de roteador, internet nem cadastro de endereço MAC, então nada que
a TI mude na rede faz o sino parar.

Os horários são mudados por uma **página no próprio sino**, aberta pelo Wi-Fi
do sino (nome oculto, com senha) ou, quando o MAC estiver cadastrado, também
pela rede da instituição.

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
- **Aviso sonoro:** se o relógio ficar muito tempo sem ser acertado, cada toque
  ganha uma batida curta no fim (veja abaixo). Quem ouvir sabe que é preciso
  olhar o sino.
- O Wi-Fi do sino serve só para a página: o sino toca do mesmo jeito sem ele.

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
2. Em **`config.h`**:
   - `RELE_ATIVO_EM_BAIXO`: o mesmo comportamento do relé da versão atual;
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
4. Placa: **Generic ESP8266 Module** (ou NodeMCU / Wemos se for o caso). Grave.
   Não precisa de nenhuma biblioteca além do pacote ESP8266.

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
| Relógio e próximo toque | ver se o sino está certo; o selo verde "GPS ok" indica acerto recente |
| **Tocar agora (teste)** | faz o sino bater uma vez (pede confirmação) |
| **Horários** | adicionar, mudar ou remover horários: hora, dias da semana, duração e número de batidas. **Clique em "Salvar horários"**: o sino passa a usar a tabela na hora e ela fica guardada mesmo sem energia |
| Copiar a tabela para outro sino | copia a tabela como texto para colar na página de outro sino |
| Acertar pela hora deste aparelho | só para quando o GPS ainda não pegou sinal; o GPS corrige depois |
| Rede e senhas | rede da instituição, senha do Wi-Fi do sino e senha da página (o sino reinicia depois de salvar) |

**Troque as duas senhas na primeira vez que abrir a página.**

### Segurança da página

- Rede do sino com nome oculto e senha (WPA2).
- Login da página no padrão "Digest": a senha não passa aberta pela rede,
  nem quando a página é aberta pela rede da instituição.
- Depois de 10 senhas erradas seguidas, o login fica bloqueado por 1 minuto.
- Esqueceu a senha? Ligue o sino no computador pelo cabo e use o comando
  `senhas-padrao` no monitor serial: as senhas voltam às do `config.h`.

## Aviso sonoro

Se o relógio ficar mais de **6 horas** sem ser acertado (GPS sem sinal), cada
toque dos horários ganha uma batida curta no fim:

```
toque normal (1,5 s ou 2,5 s) ... pausa de 0,5 s ... batida de 0,5 s
```

O sino continua tocando na hora certa pelo DS3231. O aviso só serve para quem
ouvir saber que é preciso olhar o sino (antena do GPS, ligações). Ele some
sozinho quando o GPS volta a acertar o relógio. A página também mostra
"AVISO SONORO LIGADO". Tempos e liga/desliga em `config.h` (`ALERTA_SEM_GPS_H`,
`AVISO_SONORO`, `AVISO_PAUSA_MS`, `AVISO_BATIDA_MS`).

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
| `wifi` | rede do sino, rede da instituição e o MAC do sino |
| `senhas-padrao` | volta as senhas do Wi-Fi e da página às do `config.h` e reinicia |

Se o GPS tiver sinal, ele corrige em poucos segundos uma hora acertada na mão.

## Teste de bancada (antes de instalar)

1. Sem GPS: `hora AAAA-MM-DD 06:59:50` com a data da próxima segunda-feira.
   Em 10 s o relé deve bater (toque das 07:00 do exemplo).
2. Pelo celular, abra a página (veja acima), troque as senhas, mude um horário
   para daqui a 2 minutos, salve e espere o relé bater.
3. Com o GPS na janela: o `status` deve mostrar `GPS: com sinal` e aparecer
   `RTC acertado pelo GPS`. Depois disso o LED passa a dar 1 piscada por segundo.
4. Com dois sinos prontos, ligue os dois lado a lado e veja se os LEDs piscam juntos.

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
- **A página não atrapalha a precisão:** enquanto o ESP envia a página, os
  caracteres do GPS ficam na fila; o firmware desconta esse atraso ao marcar a
  hora de chegada (e pula o acerto se a fila estiver cheia demais).
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
  ```
