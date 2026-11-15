# Teste de rede para o sino (ESP32 ou ESP32-C3)

Programa de diagnóstico para levar ao campus. Ele responde à pergunta: **dá
para o sino pegar a hora certa pela rede da instituição, mesmo sem login e sem
o MAC cadastrado?** Ele tenta **todas as formas** que o sino poderia usar e
termina com um resumo e uma conclusão.

O programa **não faz login e não tenta passar pelo captcha**. Ele só verifica
o que a rede já oferece para qualquer aparelho conectado.

## Três jeitos de usar (valem ao mesmo tempo)

| Jeito | Como |
|---|---|
| **Celular** (recomendado no campus) | conecte no Wi-Fi **`TESTE-SINO`** (senha `sino12345`) e abra **http://192.168.4.1**. A página mostra a mesma saída do monitor serial, tem os botões "Testar tudo", "Vigiar", a caixa de comandos e os relatórios guardados |
| **Monitor serial** | 115200 baud, final de linha "Nova linha". Digite `ajuda` |
| **Sem nada** | aperte o botão **BOOT**: o teste roda sozinho e o **LED pisca a conclusão** (tabela abaixo). O relatório fica guardado na memória do ESP para você ler depois, em casa |

O ESP pode ser alimentado por um carregador de celular ou power bank: não
precisa de computador.

## Gravar (Arduino IDE)

1. Instale o pacote **esp32 (Espressif)**, versão 2.x ou 3.x. Não precisa de
   nenhuma biblioteca extra.
2. Abra `teste_rede/teste_rede.ino` (as outras abas abrem junto).
3. Placa:
   - **ESP32-C3**: `ESP32C3 Dev Module`, com **USB CDC On Boot: Enabled**
     (sem isso o monitor serial fica mudo; a página continua funcionando).
     Se a IDE não achar a porta, segure BOOT, aperte RST, solte BOOT e grave.
   - **ESP32 comum**: `ESP32 Dev Module`.
   - Partition Scheme: o padrão ("Default 4MB with spiffs") serve.
4. Grave, abra o Monitor Serial em **115200** e espere a mensagem inicial.

> **ESP32-C3 "Super Mini":** muitas dessas plaquinhas têm a antena mal
> projetada e só conectam com **menos potência** de Wi-Fi. O programa percebe a
> falha e tenta sozinho com 8,5 dBm; se funcionar, salva. Também dá para fixar
> com o comando `potencia 8.5` ou pela página.

## Roteiro no campus

1. Ligue o ESP. No celular, entre em `TESTE-SINO` e abra http://192.168.4.1.
2. Clique em **Redes por perto**, digite o nome exato da rede da instituição
   (e a senha, se tiver), **Salvar** e **Testar tudo**. Leva de 2 a 4 minutos.
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

1. **MAC do ESP**, para a TI cadastrar.
2. **Redes por perto**: nome, sinal e tipo de segurança.
3. **Conexão.** Se o roteador recusar, mostra o código e o motivo provável
   (filtro de MAC, senha errada, rede fora de alcance, DHCP sem IP...). Se a
   falha parecer de rádio, repete com menos potência (placas C3).
4. **Dados da rede**: IP, roteador, DNS e o servidor de hora informado pelo
   DHCP, se houver.
5. **DNS**: se funciona, ou se o portal desvia os nomes para a página de login.
6. **Páginas**: o roteador, `1.1.1.1` e o teste de internet do Google. Mostra
   se a internet está liberada ou se tudo cai na página de login, e lê a hora
   do cabeçalho `Date` de cada resposta.
7. **Página de login**: lê só o cabeçalho e o título (HTTPS), sem preencher
   nada. Também testa se HTTPS para fora passa pelo portal.
8. **Servidores de hora (NTP)**: o do DHCP, o roteador, os servidores DNS, a
   página de login e servidores da internet (`a.ntp.br`, `b.ntp.br`, Google).
9. **Outros jeitos, só nos aparelhos da própria rede:** ICMP *timestamp* (muitos
   roteadores respondem a hora do dia em milissegundos), os protocolos antigos
   TIME (porta 37, TCP e UDP) e DAYTIME (porta 13), e NTP por difusão (o ESP só
   escuta se alguém anuncia a hora para a rede toda).
10. **Refinamento**: com uns 30 pedidos seguidos à mesma página, acha o momento
    exato em que o segundo vira e melhora a precisão do `Date` de ±0,5 s para
    alguns milissegundos.
11. **Resumo e conclusão**, com a melhor fonte de hora e a margem de erro de
    cada uma. O resumo é guardado como relatório.

## Como ler a conclusão (e o LED)

Parado, o LED pisca o número da conclusão a cada 3 segundos. Piscando rápido =
teste em andamento. Apagado = ainda sem teste.

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
| `testar` | conecta e faz todos os testes (2 a 4 minutos) |
| `vigiar [MIN]` | fica conectado MIN minutos (30) conferindo a hora a cada minuto |
| `parar` | interrompe o teste ou a vigia |
| `resumo` | mostra de novo o resumo do último teste |
| `relatorios` | lista os relatórios guardados na memória (ficam os 6 últimos) |
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

| | ESP32-C3 (Super Mini / DevKitM) | ESP32 Dev Module |
|---|---|---|
| LED | GPIO 8 (acende em 0) | GPIO 2 |
| Botão | BOOT = GPIO 9 | BOOT = GPIO 0 |

Mude em `PINO_LED`, `LED_ACESO` e `PINO_BOTAO` no começo do `.ino` se a sua
placa for diferente.

## Para quem for mexer no código

| Arquivo | O que é |
|---|---|
| `teste_rede.ino` | conexão, DNS, páginas, NTP, teste completo, vigia e comandos |
| `sondas.ino` | ICMP timestamp, TIME/DAYTIME e escuta de NTP por difusão |
| `web_teste.ino` | página, fila de comandos, relatórios (LittleFS), LED e botão |
| `analise.h`, `tempo.h` | funções puras (leitura de respostas, datas), com testes no PC |
| `registro.h` | a saída: monitor serial + anel de 24 KB para a página e os relatórios |
| `pagina_teste.h` | o HTML da página |

Testes no PC:

```
cd testes && g++ -std=c++17 -Wall -I../teste_rede teste_analise.cpp -o teste && ./teste
```

A compilação automática no GitHub compila para ESP32 e ESP32-C3, com o pacote
esp32 2.0.17 e com o mais novo.
