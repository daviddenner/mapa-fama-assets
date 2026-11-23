# Teste do sino na protoboard

Monte e teste tudo na protoboard antes de fazer a placa. Neste teste o
NodeMCU fica ligado só pelo **USB** (computador ou carregador de celular), sem
a fonte de 5 V.

> **Não ligue 127 V na protoboard.** Ela não é feita para isso. No teste,
> ouça o clique do relé (e veja o LED) ou ligue nele algo de baixa tensão.
> O sino de 127 V só entra na placa definitiva, com bornes e fusível.

## Material

- NodeMCU v3 (LoLin) e cabo micro-USB
- Módulo DS3231 ZS-042 com bateria CR2032
- Relé 5 V SRD-05VDC-SL-C, BC548, 1N4007
- Resistores: 1 kΩ (2), 10 kΩ (1)
- LED (opcional, mostra o relé ligado) e botão táctil (opcional)
- Protoboard e fios jumper (macho-macho e fêmea-macho)

O NodeMCU v3 é largo e cobre a protoboard inteira. Ou junte **duas
protoboards lado a lado** (uma fileira de pinos em cada), ou ligue os pinos
do NodeMCU com fios **fêmea-macho**.

## Ligações, fio a fio

Os nomes dos pinos são os que estão impressos no NodeMCU (G = GND, 3V = 3,3 V,
VU = 5 V do USB).

### Relógio (lado de 4 pinos do módulo)

| Módulo DS3231 | NodeMCU |
|---|---|
| SCL | **D1** |
| SDA | **D2** |
| VCC | **3V** |
| GND | **G** |

### Relé com o BC548

O BC548 com a face reta virada para você e as pernas para baixo é, da
esquerda para a direita: **C** (coletor), **B** (base), **E** (emissor).

| De | Para |
|---|---|
| **D6** | resistor de 1 kΩ → **B** do BC548 |
| **B** do BC548 | resistor de 10 kΩ → **G** (segura o relé desligado no boot) |
| **E** do BC548 | **G** |
| **C** do BC548 | um pino da bobina do relé |
| outro pino da bobina | **VU** (5 V do USB) |
| 1N4007: **faixa (catodo) no VU**, outra ponta no **C** do BC548 | em paralelo com a bobina (roda livre) |
| LED (opcional): perna longa no VU com resistor de 1 kΩ, perna curta no **C** | acende com o relé ligado |

As pernas do relé SRD não encaixam na protoboard. Solde fios curtos nos dois
pinos da **bobina**: são os dois pinos que ficam perto do pino isolado
(o comum). Para um teste mais rápido, um **módulo relé pronto** também serve:
ligue o IN dele no D6 e ajuste `RELE_ATIVO_EM_BAIXO` no `config.h` conforme o
módulo.

### Botão de teste (opcional)

| De | Para |
|---|---|
| **D7** | botão → **G** |

### LEDs de estado

| De | Para |
|---|---|
| **D0** | resistor de 330 Ω → perna longa do **LED verde**; perna curta no **G** |
| **D8** | resistor de 330 Ω → perna longa do **LED vermelho**; perna curta no **G** |

Verde aceso = sistema OK (hora conferida pela rede). Vermelho aceso = perda de
rede; piscando = sem hora (o sino não toca).

## Firmware

1. Na Arduino IDE (pacote ESP8266), abra `sino-gps/sino_gps/sino_gps.ino`.
2. No `config.h`, confira:
   - `RELE_ATIVO_EM_BAIXO false` (relé pelo BC548);
   - `AP_NOME`, `AP_SENHA_PADRAO` e `ADMIN_SENHA_PADRAO`;
   - para testar o aviso sonoro sem esperar meia hora, use por enquanto
     `ALERTA_SEM_ACERTO_MIN 2` e `REDE_INST_ESPERA_MIN 1`. **Volte para 30 e 10
     antes de instalar.**
3. Placa: **NodeMCU 1.0 (ESP-12E Module)**. Grave e abra o monitor serial em
   115200 baud.

## Roteiro do teste

Para testar em casa, use o **roteador de casa ou o roteador do celular** como
se fosse a rede da instituição.

1. **Relógio:** no monitor serial não pode aparecer "DS3231 nao responde". Se
   aparecer, confira SDA no D2 e SCL no D1.
2. **Página:** no celular, adicione a rede oculta `SINO-IFMS-01` com a senha
   do Wi-Fi do sino e abra `http://192.168.4.1` (usuário `admin`).
3. **Rede:** em "Rede e senhas", digite o nome e a senha da rede e clique em
   "Salvar e reiniciar". No monitor serial deve aparecer:
   - `Rede da instituicao: conectado...`
   - `Rede: hora conferida (a.ntp.br, +-X ms)`
   - `RTC acertado pela rede (NTP)` (só na primeira vez ou se ele estiver fora).
   
   A página passa a mostrar o selo verde **hora conferida**.
4. **Relé:** aperte o botão (ou "Tocar agora" na página): o relé estala e o
   LED acende por 2 s.
5. **Horário:** na página, coloque um horário daqui a 2 minutos e salve. O
   relé deve bater exatamente na virada do minuto.
6. **Rede caindo:** desligue o roteador (ou o roteador do celular). Em 30 s o
   LED verde apaga e o **vermelho acende**. Depois de
   `ALERTA_SEM_ACERTO_MIN`, o LED azul passa a dar 2 piscadas por segundo e os
   toques ganham a batida curta do aviso (toque, pausa de 0,5 s, batida de
   0,5 s). Ligue o roteador de novo: em poucos minutos a hora é conferida e o
   aviso some.
7. **Falta de energia:** tire o USB por um minuto e ligue de novo. A hora
   continua certa logo ao ligar (o DS3231 tem bateria), mesmo antes de a rede
   conectar.

## Se algo não funcionar

| Sintoma | O que conferir |
|---|---|
| `DS3231 nao responde` | SDA → D2, SCL → D1, VCC → 3V, GND → G |
| Relé não estala | VU tem 5 V só com o USB ligado; BC548 na ordem C-B-E; resistor de 1 kΩ na base |
| Relé fica ligado o tempo todo | `RELE_ATIVO_EM_BAIXO` errado no `config.h`, ou BC548 invertido |
| Não aparece a rede do sino | É oculta: adicione manualmente pelo nome |
| `Rede da instituicao: nao conectou` | Nome/senha da rede; na rede da instituição, o MAC (comando `wifi`) precisa estar cadastrado |
| `nao consegui conferir a hora` | A rede bloqueia a porta de hora (NTP): peça à TI o servidor de hora interno e digite em "Servidor de hora" na página |
