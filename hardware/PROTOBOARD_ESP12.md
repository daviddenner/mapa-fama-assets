# ESP-12 avulso na protoboard: relé e relógio

Este roteiro é para o **ESP-12 sem placa** (o módulo metálico com a antena
desenhada na placa). Esquema completo:
[`esquema_esp12.png`](esquema_esp12.png) (Eagle: `Sino_IFMS_ESP12.sch`).

O NodeMCU é um ESP-12 numa placa que já traz quatro coisas. Com o ESP-12
avulso, elas entram na protoboard:

1. os **3,3 V** (o ESP-12 não aceita 5 V);
2. os **resistores de partida** (sem eles o ESP-12 não liga ou não roda o programa);
3. os botões **RESET** e **FLASH**;
4. um **adaptador USB-serial** para gravar e ver o monitor serial.

> Se o seu ESP-12 já estiver numa placa com USB (NodeMCU, Wemos D1 mini),
> pule os passos 1 a 3 e use só as ligações do relé e do relógio, que usam
> os mesmos GPIOs. Nesse caso, siga o [`PROTOBOARD.md`](PROTOBOARD.md).

> **Não ligue 127 V na protoboard.** No teste, ouça o clique do relé.

## Material

- ESP-12E ou ESP-12F
- **Placa adaptadora** para ESP-12 (a branca, vendida junto ou separada): os
  pinos do ESP-12 têm passo de 2 mm e não encaixam na protoboard. Sem ela,
  solde fios finos nos pinos usados.
- Regulador de 3,3 V: **módulo AMS1117-3.3** (ou uma fonte de protoboard com
  saída de 3,3 V)
- Capacitores: 100 µF (10 V ou mais) e 100 nF
- Resistores: 10 kΩ (6), 1 kΩ (2), 330 Ω (2)
- LEDs: 1 verde e 1 vermelho (estado do sino), mais 1 opcional para o relé
- Botões táteis: 2 (RESET e FLASH), mais 1 opcional (S1, teste do relé)
- **Adaptador USB-serial de 3,3 V** (CH340, CP2102 ou FTDI). Se ele tiver
  jumper de 5 V/3,3 V, deixe em **3,3 V**.
- Relé 5 V SRD-05VDC-SL-C, BC548, 1N4007 (ou um módulo relé pronto)
- Módulo DS3231 ZS-042 com bateria CR2032
- Fios jumper

## 1. Alimentação: 5 V para o relé, 3,3 V para o ESP-12

Os 5 V podem vir do pino **5V do próprio adaptador USB-serial** (ligado no
computador ou num carregador/power bank) ou de um cabo USB cortado.

| De | Para |
|---|---|
| 5 V | entrada (VIN/IN) do AMS1117 e um pino da bobina do relé |
| saída 3,3 V do AMS1117 | trilho de **3,3 V** da protoboard |
| GND | trilho de **GND**: fonte, regulador, ESP-12, adaptador, relógio e relé, tudo junto |
| capacitor de 100 µF | entre 3,3 V e GND, perto do ESP-12 (perna "−" no GND) |
| capacitor de 100 nF | entre 3,3 V e GND, colado no VCC do ESP-12 |

> **Nunca ligue 5 V no ESP-12**, nem no VCC nem nos outros pinos.
> Também não alimente o ESP-12 pelo pino 3V3 do adaptador: ele não aguenta os
> picos do Wi-Fi e o ESP fica reiniciando.

## 2. Partida do ESP-12 (resistores e botões)

| Pino do ESP-12 | Liga em |
|---|---|
| VCC | 3,3 V |
| GND | GND |
| EN (às vezes escrito CH_PD) | 10 kΩ até o 3,3 V |
| RST | 10 kΩ até o 3,3 V; botão **RESET** até o GND |
| GPIO0 | 10 kΩ até o 3,3 V; botão **FLASH** até o GND |
| GPIO2 | 10 kΩ até o 3,3 V (é o LED azul do próprio ESP-12) |
| GPIO15 | 10 kΩ até o **GND** (é também o LED vermelho, abaixo) |
| ADC | não ligar |

Muitas placas adaptadoras brancas já trazem resistores pequenos (SMD) no EN e
no GPIO15. Se a sua tiver, não precisa repetir esses dois.

## 3. Gravação (adaptador USB-serial)

| Adaptador | ESP-12 |
|---|---|
| GND | GND |
| TX (TXD) | **RX** (RXD0) |
| RX (RXD) | **TX** (TXD0) |
| 5V (opcional) | entrada do AMS1117 (alimenta tudo pelo USB) |
| 3V3 | não ligar |

TX e RX são **cruzados**: o que um fala, o outro escuta.

Na Arduino IDE (pacote esp8266):

- Placa: **Generic ESP8266 Module**
- Flash Size: **4MB (FS:2MB OTA:~1019KB)**
- Reset Method: **no dtr (aka ck)**
- Upload Speed: 115200

Para gravar: **segure FLASH, aperte e solte RESET, solte FLASH** e clique em
Carregar. Quando terminar, aperte RESET para o programa rodar. O monitor
serial (115200 baud) funciona pelo mesmo adaptador.

## 4. Relógio e relé

Os mesmos pinos do sino com NodeMCU (o número do GPIO é o que vale).

### Relógio (lado de 4 pinos do módulo)

| Módulo DS3231 | ESP-12 |
|---|---|
| SCL | **GPIO5** |
| SDA | **GPIO4** |
| VCC | **3,3 V** |
| GND | GND |

> Alguns ESP-12 e placas adaptadoras vêm com **GPIO4 e GPIO5 trocados** na
> serigrafia. Se o teste disser que o relógio não responde, troque os dois fios.

### Relé com o BC548

O BC548 com a face reta virada para você e as pernas para baixo é, da
esquerda para a direita: **C** (coletor), **B** (base), **E** (emissor).

| De | Para |
|---|---|
| **GPIO12** | resistor de 1 kΩ → **B** do BC548 |
| **B** do BC548 | resistor de 10 kΩ → GND (segura o relé desligado ao ligar) |
| **E** do BC548 | GND |
| **C** do BC548 | um pino da bobina do relé |
| outro pino da bobina | **5 V** |
| 1N4007: **faixa no 5 V**, outra ponta no **C** | em paralelo com a bobina (roda livre) |
| LED (opcional): 5 V → 1 kΩ → perna longa; perna curta no **C** | acende com o relé ligado |
| botão S1 (opcional): **GPIO13** → botão → GND | testa o relé |

### LEDs de estado

| De | Para |
|---|---|
| **GPIO16** | resistor de 330 Ω → perna longa do **LED verde**; perna curta no GND |
| **GPIO15** | resistor de 330 Ω → perna longa do **LED vermelho**; perna curta no GND |

Verde aceso = sistema OK (hora conferida pela rede). Vermelho aceso = perda de
rede; piscando = sem hora (o sino não toca). O LED vermelho ajuda a manter o
GPIO15 em 0 V na partida, junto com o resistor de 10 kΩ.

As pernas do relé SRD não encaixam na protoboard: solde fios curtos nos dois
pinos da bobina (os dois ao lado do pino comum). Com um **módulo relé pronto**,
ligue o IN dele no GPIO12 e ajuste `RELE_ATIVO_EM_BAIXO` no programa.

## 5. Primeiro teste, em casa

1. Grave o programa de teste: [`../teste-rede/teste_rede/teste_rede.ino`](../teste-rede/teste_rede).
2. Abra o monitor serial (115200). Deve aparecer o **MAC** e a página
   `TESTE-SINO`.
3. Digite `rtc`: deve responder `Relogio DS3231 ... RESPONDEU` e
   `O relogio esta andando`.
4. Digite `rele` (ou aperte S1): o relé estala por meio segundo. Digite
   `leds`: o verde e depois o vermelho acendem por 1 s.
5. Escolha o Wi-Fi de casa (`rede NOME` e `senha SENHA`) e digite `testar`.
   No fim, `rtc acertar` grava a hora certa no DS3231.

Funcionando em casa, leve ao IFMS. Lá, alimente pelo carregador ou power bank
e use o celular, como está no [roteiro do teste](../teste-rede/README.md).

## 6. Depois: o mesmo ESP-12 vira o sino

Grave o firmware do sino ([`../sino-gps`](../sino-gps)) com as mesmas
configurações da Arduino IDE. Os pinos são os mesmos e o MAC também: se a TI
cadastrar o MAC durante o teste, o cadastro continua valendo para o sino.

## Se algo não funcionar

| Sintoma | O que conferir |
|---|---|
| Monitor serial mudo | 115200 baud; TX e RX cruzados; GND comum; ESP rodando (aperte RESET com FLASH solto) |
| Só caracteres estranhos ao ligar | normal: o ESP fala a 74880 baud nos primeiros instantes |
| "Failed to connect to ESP8266" ao gravar | segure FLASH e aperte RESET antes de gravar; TX/RX cruzados; adaptador em 3,3 V |
| Reinicia sozinho ("wdt reset", "brownout") | regulador fraco, falta o capacitor de 100 µF, ou ESP ligado no 3V3 do adaptador |
| Grava, mas não roda | GPIO15 com 10 kΩ ao GND; EN, GPIO0 e GPIO2 com 10 kΩ ao 3,3 V; aperte RESET |
| `Relogio DS3231 ... NAO RESPONDE` | SDA no GPIO4, SCL no GPIO5 (ou trocados, veja acima); VCC no 3,3 V |
| Relé não estala | 5 V na bobina; BC548 na ordem C-B-E; 1 kΩ na base |
| Relé ligado o tempo todo | BC548 invertido, ou `RELE_ATIVO_EM_BAIXO` errado |
| LED não acende no comando `leds` | LED ao contrário (perna longa vai para o resistor); resistor de 330 Ω, não 330 kΩ |
