# Teste da rede e da placa do sino (ESP-12, ESP32 ou ESP32-C3)

Programa de diagnóstico para levar ao campus. Ele responde à pergunta: **dá
para o sino pegar a hora certa pela rede da instituição, mesmo sem login e sem
o MAC cadastrado?** Ele tenta **todas as formas** que o sino poderia usar e
termina com um resumo e uma conclusão. Testa também a **placa do sino**: o
relógio DS3231 (e quantos milissegundos ele está fora da hora da rede) e o relé.

O programa **não faz login e não tenta passar pelo captcha**. Ele só verifica
o que a rede já oferece para qualquer aparelho conectado.

Roda no **ESP-12 / ESP8266** (o mesmo chip do sino), no ESP32 e no ESP32-C3.
Montagem do ESP-12 avulso com relé e relógio:
[`../hardware/PROTOBOARD_ESP12.md`](../hardware/PROTOBOARD_ESP12.md).

## Três jeitos de usar (valem ao mesmo tempo)

| Jeito | Como |
|---|---|
| **Celular** (recomendado no campus) | conecte no Wi-Fi **`TESTE-SINO`** (senha `sino12345`) e abra **http://192.168.4.1**. A página mostra a mesma saída do monitor serial e tem os botões "Testar tudo", "Vigiar", "Testar relé", "Testar LEDs", "Relógio", a caixa de comandos e os relatórios guardados |
| **Monitor serial** | 115200 baud, final de linha "Nova linha". Digite `ajuda` |
| **Sem nada** | aperte o botão **FLASH** (ESP-12) ou **BOOT** (ESP32): o teste roda sozinho e o **LED pisca a conclusão** (tabela abaixo). O relatório fica guardado na memória do ESP para você ler depois, em casa |

O ESP pode ser alimentado por um carregador de celular ou power bank: não
precisa de computador.

## Gravar (Arduino IDE)

Não precisa de nenhuma biblioteca extra. Abra `teste_rede/teste_rede.ino` (as
outras abas abrem junto) e escolha a placa:

| Placa | Pacote | Na Arduino IDE |
|---|---|---|
| **ESP-12 avulso** | esp8266 | **Generic ESP8266 Module**, Flash Size **4MB (FS:2MB OTA:~1019KB)**, Reset Method **no dtr (aka ck)**. Grave segurando FLASH: segure FLASH, aperte e solte RESET, solte FLASH, clique em Carregar |
| NodeMCU | esp8266 | NodeMCU 1.0 (ESP-12E Module) |
| ESP32 | esp32 (2.x ou 3.x) | ESP32 Dev Module |
| ESP32-C3 | esp32 (2.x ou 3.x) | ESP32C3 Dev Module, com **USB CDC On Boot: Enabled**. Se a IDE disser que o programa é grande demais, use Partition Scheme **Huge APP** |

Depois de gravar, abra o Monitor Serial em **115200** e espere a mensagem
inicial com o MAC.

> **ESP32-C3 "Super Mini":** muitas dessas plaquinhas têm a antena mal
> projetada e só conectam com **menos potência** de Wi-Fi. O programa percebe a
> falha e tenta sozinho com 8,5 dBm; se funcionar, salva.

## Roteiro no campus

1. Ligue o ESP. No celular, entre em `TESTE-SINO` e abra http://192.168.4.1.
2. Clique em **Redes por perto**, digite o nome exato da rede da instituição
   (e a senha, se tiver), **Salvar** e **Testar tudo**. Leva de 2 a 4 minutos.
   Enquanto ele conecta, a página pode ficar alguns segundos sem responder.
3. Leia a **conclusão** no fim. Ela também fica no LED e no relatório.
4. Se der, clique em **Vigiar 30 min**: o ESP fica conectado sem login,
   conferindo a hora a cada minuto. Isso mostra se o portal derruba quem não
   fez login depois de um tempo.
5. Peça à TI o cadastro do MAC (aparece no topo) e pergunte se a rede tem um
   servidor de hora interno. Teste com `ntp IP-QUE-ELES-DEREM`.
6. Com o MAC cadastrado, rode **Testar tudo** de novo: dá para comparar os dois
   relatórios.
7. Em casa, abra a página de novo (ou use `relatorios` e `relatorio N` no
   monitor serial) e copie os relatórios para análise.

Se a instituição tiver mais de uma rede (alunos, servidores, visitantes), teste
cada uma: a página deixa trocar a rede e rodar de novo.

## O que ele testa

0. **Placa:** o DS3231 responde? A hora guardada vale (ou o relógio parou)?
   Está andando? Temperatura do chip. Os **LEDs de estado** (verde e
   vermelho) acendem 1 s cada: confira. O relé **não** é acionado no teste
   automático: use o botão S1, o comando `rele` ou o botão da página.
1. **MAC do ESP**, para a TI cadastrar, e **redes por perto** (nome, sinal e
   tipo de segurança).
2. **Conexão.** Se o roteador recusar, mostra o código e o motivo provável
   (filtro de MAC, senha errada, rede fora de alcance, DHCP sem IP...). Se a
   falha parecer de rádio, repete com menos potência.
3. **Dados da rede**: IP, roteador, DNS e o servidor de hora informado pelo
   DHCP, se houver.
4. **DNS**: se funciona, ou se o portal desvia os nomes para a página de login.
5. **Páginas**: o roteador, `1.1.1.1` e o teste de internet do Google. Mostra
   se a internet está liberada ou se tudo cai na página de login, e lê a hora
   do cabeçalho `Date` de cada resposta.
6. **Página de login**: lê só o cabeçalho e o título (HTTPS), sem preencher
   nada. Também testa se HTTPS para fora passa pelo portal. (No ESP8266, se
   faltar memória para o HTTPS, ele avisa e pula.)
7. **Servidores de hora (NTP)**: o do DHCP, o roteador, os servidores DNS, a
   página de login e servidores da internet (`a.ntp.br`, `b.ntp.br`, Google).
8. **Outros jeitos, só nos aparelhos da própria rede:** ICMP *timestamp* (muitos
   roteadores respondem a hora do dia em milissegundos), os protocolos antigos
   TIME (porta 37, TCP e UDP) e DAYTIME (porta 13), e NTP por difusão (o ESP só
   escuta se alguém anuncia a hora para a rede toda).
9. **Refinamento**: com uns 30 pedidos seguidos à mesma página, acha o momento
   exato em que o segundo vira e melhora a precisão do `Date` de ±0,5 s para
   alguns milissegundos.
10. **Relógio x rede**: mede, na virada do segundo, quantos milissegundos o
    DS3231 está adiantado ou atrasado em relação à melhor fonte de hora.
11. **Resumo e conclusão**, com a melhor fonte de hora e a margem de erro de
    cada uma. Tudo fica guardado como relatório.

## Como ler a conclusão (e o LED)

Parado, o LED azul pisca o número da conclusão a cada 3 segundos. Piscando
rápido = teste em andamento. Apagado = ainda sem teste. Se a placa tiver os
LEDs de estado, depois do teste o **verde** acende para as conclusões 1 a 3 (a
rede dá a hora) e o **vermelho** para 4 a 6.

| Piscadas | Conclusão | O que significa |
|---|---|---|
| 1 | internet LIBERADA | o MAC já está cadastrado (ou a rede é aberta): funciona enquanto o cadastro existir. Repita o teste com o MAC fora da lista para saber o que funciona sem cadastro |
| **2** | **DÁ PARA USAR** | um servidor de hora da própria rede responde **sem login**: o sino já tenta esses servidores primeiro |
| 3 | só NTP da internet | a porta de hora está aberta para fora, mas a TI pode fechar a qualquer momento |
| 4 | só as páginas informam a hora | dá para usar o `Date` refinado (alguns ms), mas é o método mais frágil (o sino ainda não tem isso) |
| 5 | nenhuma fonte sem login | conectou, mas nada responde: precisa do cadastro do MAC (ou do GPS) |
| 6 | não conectou | sem IP não há como usar a rede: fique com o cadastro ou o GPS |

## Todos os comandos

Valem no monitor serial e na caixa de comandos da página.

| Comando | O que faz |
|---|---|
| `mac` | mostra o MAC deste ESP e como abrir a página |
| `redes` | procura as redes Wi-Fi por perto |
| `rede NOME` | escolhe a rede (fica salvo) |
| `senha SENHA` | senha da rede; `senha` sozinho = rede aberta |
| `testar` | testa a placa, conecta e testa a rede (2 a 4 minutos) |
| `vigiar [MIN]` | fica conectado MIN minutos (30) conferindo a hora a cada minuto |
| `parar` | interrompe o teste ou a vigia |
| `resumo` | mostra de novo o resumo do último teste |
| `rtc` | testa o relógio DS3231 e compara com a hora da rede, se já tiver uma |
| `rtc acertar` | grava a hora da rede no DS3231, na virada do segundo (como o sino faz) |
| `rele [MS]` | liga o relé por MS milissegundos (500; no máximo 3000) |
| `leds` | acende o LED verde e depois o vermelho, 1 s cada |
| `relatorios` | lista os relatórios guardados na memória (4 no ESP-12, 6 no ESP32) |
| `relatorio N` | mostra o relatório N; `apagar-relatorios` apaga todos |
| `hora` | hora atual segundo a melhor fonte encontrada |
| `info` | IP, roteador, DNS e sinal da conexão atual |
| `ntp IP-ou-NOME` | testa um servidor de hora (ex.: o que a TI indicar) |
| `icmp IP` | pede a hora por ICMP timestamp a um aparelho da rede |
| `http ENDERECO` | lê o cabeçalho e o título de uma página (`http://` ou `https://`) |
| `dns NOME` | traduz um nome para IP |
| `potencia X` | potência do Wi-Fi: `max`, `8.5`, `11` ou `15` dBm |
| `desconectar` | sai da rede; `esquecer` apaga a rede e a senha salvas |
| `reiniciar` | reinicia o ESP |
| `ajuda` | lista os comandos |

## Pinos usados

| | ESP-12 / NodeMCU (iguais aos do sino) | ESP32-C3 | ESP32 Dev Module |
|---|---|---|---|
| Relógio SDA / SCL | GPIO4 (D2) / GPIO5 (D1) | GPIO4 / GPIO5 | GPIO21 / GPIO22 |
| Relé (via BC548) | GPIO12 (D6) | GPIO3 | GPIO26 |
| Botão S1: testar o relé | GPIO13 (D7) | GPIO1 | GPIO27 |
| Botão: testar tudo | FLASH = GPIO0 | BOOT = GPIO9 | BOOT = GPIO0 |
| LED azul | GPIO2, do próprio ESP-12 (acende em 0) | GPIO8 (acende em 0) | GPIO2 |
| LED verde / vermelho (330 Ω até o GND) | GPIO16 (D0) / GPIO15 (D8) | GPIO6 / GPIO7 | GPIO32 / GPIO33 |

Mude no começo do `.ino` se a sua placa for diferente. Com módulo relé que
liga em 0 V, use `RELE_ATIVO_EM_BAIXO 1`.

## Para quem for mexer no código

| Arquivo | O que é |
|---|---|
| `teste_rede.ino` | conexão, DNS, páginas, NTP, teste completo, vigia e comandos |
| `placa.ino` | relógio DS3231 (teste, medida na virada do segundo, acerto) e relé |
| `sondas.ino` | ICMP timestamp, TIME/DAYTIME e escuta de NTP por difusão |
| `web_teste.ino` | página, fila de comandos, relatórios (LittleFS), LED e botões |
| `compat.h` | as diferenças entre ESP8266 e ESP32, num lugar só |
| `analise.h`, `tempo.h` | funções puras (leitura de respostas, datas), com testes no PC |
| `ds3231.h` | acesso ao DS3231 (cópia do firmware do sino) |
| `registro.h` | a saída: monitor serial + anel na memória (página) + relatório |
| `pagina_teste.h` | o HTML da página |

Tudo roda num laço só: durante um teste, as esperas continuam atendendo a
página, o LED e os botões.

Testes no PC:

```
cd testes && g++ -std=c++17 -Wall -I../teste_rede teste_analise.cpp -o teste && ./teste
```

A compilação automática no GitHub compila para ESP-12 (Generic ESP8266 4MB),
NodeMCU, ESP32 e ESP32-C3, com o pacote esp32 2.0.17 e com o mais novo.
