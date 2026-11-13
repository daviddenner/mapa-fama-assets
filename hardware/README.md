# Esquema eletrônico do sino (Eagle)

![Esquema do controlador do sino](esquema.png)

| Arquivo | O que é |
|---|---|
| `Sino_IFMS_v2.sch` | esquema para o **Eagle** (7.7 ou mais novo). A biblioteca das peças vai junto, dentro do arquivo |
| `esquema.png` / `esquema.svg` | prévia do mesmo esquema |
| `gerar_esquema.py` | script que gera os três arquivos acima (`python3 gerar_esquema.py`) |

Para abrir: copie `Sino_IFMS_v2.sch` para a pasta do projeto no Eagle
(`Documentos\EAGLE\Sino_IFMS`) e abra pelo Eagle. O nome tem `_v2` de propósito,
para não sobrescrever o esquema que já existe lá. Para desenhar a placa, use
**Board** (ou "Switch to board"): o Eagle cria o `.brd` com todas as peças.

## O circuito

| Bloco | Como funciona |
|---|---|
| **Alimentação** | Uma fonte de 5 V (ex.: HLK-PM01) entra por **J1**. C1 (470 µF) e C2 (100 nF) seguram os picos do Wi-Fi e do relé. Os 5 V alimentam o **relé** e o **NodeMCU pelo pino VIN** |
| **NodeMCU (U1)** | ESP-12E com USB e regulador de 3,3 V. O **3V3 do NodeMCU alimenta o relógio** |
| **Relógio (U2)** | Módulo DS3231 ZS-042 ligado pelo **lado de 4 pinos** (SCL, SDA, VCC, GND), com bateria CR2032 |
| **Relé (K1)** | Relé 5 V acionado por **Q1 (BC548)**. R1 (1 kΩ) na base, **R2 (10 kΩ)** segura o relé desligado enquanto o ESP liga ou é gravado, **D1 (1N4007) é o diodo de roda livre**. LED1 + R3 acendem quando o relé liga |
| **Saída 127 V** | Fase entra em **J2.1**, passa pelo **fusível F1** e vai ao **COM** do relé; o **NA** volta por **J2.2** para o sino. O neutro vai direto ao sino. R4 + C3 (snubber, opcional) protegem o contato |
| **Botão S1** | Toque de teste na mão |
| **J3 (opcional)** | Conector para o GPS GY-NEO6MV2, na mesma ordem dos pinos do módulo (VCC, RX, TX, GND) |

### Pinos do NodeMCU

| Pino | GPIO | Ligado em | No firmware (`config.h`) |
|---|---|---|---|
| D1 | 5 | SCL do DS3231 | `PINO_SCL` |
| D2 | 4 | SDA do DS3231 | `PINO_SDA` |
| D4 | 2 | LED azul do próprio NodeMCU | `PINO_LED` |
| D5 | 14 | TX do GPS (opcional) | `PINO_GPS_RX` |
| D6 | 12 | R1 → base do BC548 (relé) | `PINO_RELE` |
| D7 | 13 | botão de teste S1 (até o GND) | `PINO_BOTAO_TESTE` |
| VIN | – | +5 V da fonte | – |
| 3V3 | – | VCC do DS3231 | – |

> **Importante no firmware:** com o BC548 o relé liga quando D6 vai a 3,3 V,
> então `RELE_ATIVO_EM_BAIXO` tem que ser **`false`** (já está assim no
> `config.h`). Se ficar `true`, o sino toca sem parar.

## Gerenciamento local

- **Gravar o firmware:** pelo USB do NodeMCU (os botões RST e FLASH já estão
  na placa do NodeMCU). Para gravar, prefira desligar a fonte de 5 V. Muitos
  NodeMCU têm um diodo entre o USB e o VIN, mas nem todo clone tem.
- **Mudar horários e senhas sem internet:** pela rede Wi-Fi do próprio sino
  (nome oculto), em `http://192.168.4.1`. Veja `../sino-gps/README.md`.
- **Pela rede da instituição:** quando o MAC estiver cadastrado, a mesma página
  abre pelo IP do sino nessa rede.

## Lista de material

| Ref. | Peça | Qtd. |
|---|---|---|
| U1 | NodeMCU ESP-12E (v2 "Amica"; o v3 "LoLin" é mais largo, ver notas) | 1 |
| U2 | Módulo RTC DS3231 ZS-042 + bateria CR2032 | 1 |
| K1 | Relé 5 V SRD-05VDC-SL-C (Songle) | 1 |
| Q1 | Transistor BC548 (ou BC337, mesma pinagem) | 1 |
| D1 | Diodo 1N4007 | 1 |
| LED1 | LED 3 mm vermelho | 1 |
| R1, R3 | Resistor 1 kΩ 1/4 W | 2 |
| R2 | Resistor 10 kΩ 1/4 W | 1 |
| R4 | Resistor 100 Ω 1/2 W (snubber, opcional) | 1 |
| C1 | Capacitor eletrolítico 470 µF 16 V | 1 |
| C2 | Capacitor cerâmico 100 nF | 1 |
| C3 | Capacitor de poliéster 100 nF **classe X2 275 VAC** (snubber, opcional) | 1 |
| F1 | Fusível 5x20 mm 2 A + porta-fusível de placa (ajuste à corrente do seu sino) | 1 |
| J1, J2 | Borne de parafuso 2 vias, passo 5,08 mm | 2 |
| J3 | Barra de pinos 1x4 (GPS, opcional) | 1 |
| S1 | Botão táctil 6x6 mm | 1 |
| – | Fonte 5 V (ex.: HLK-PM01, 127 V → 5 V 600 mA) | 1 |

## Notas para quando for fazer a placa

- **RTC:** a ordem dos 4 pinos do ZS-042 no esquema é SCL, SDA, VCC, GND.
  **Confira a ordem impressa no seu módulo** antes de fazer a placa.
- **NodeMCU:** o desenho da placa usa o v2 (fileiras a 22,86 mm). Se o seu for
  o v3 LoLin (fileiras a 27,94 mm), avise que eu ajusto, ou use barras fêmea
  separadas.
- **Relé:** o desenho usa as medidas do SRD (pinos 1 = COM, 3 = NA, 4 = NF,
  2 e 5 = bobina). A corrente da bobina é de uns 70 mA, dentro do BC548 (máx.
  100 mA). Com relé de bobina mais forte, use **BC337**.
- **Botão:** ligado pelos pinos em diagonal (1 e 4), assim funciona com
  qualquer marca de botão 6x6.
- **Porta-fusível:** o desenho usa presilhas a 22,6 mm; confira o que você comprar.
- **127 V:** deixe **6 mm ou mais** entre as trilhas de 127 V (J2, F1, contatos
  do relé, snubber) e o resto do circuito. Se der, faça um rasgo na placa entre
  as duas partes.

## Mudanças no esquema

O esquema é gerado pelo `gerar_esquema.py`: mude o script e rode de novo. Antes
de gravar, ele confere se cada fio encosta nos pinos certos e se a biblioteca
está completa. Mexer direto no Eagle também funciona, mas aí o script deixa de
valer para a versão editada.
