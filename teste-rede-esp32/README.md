# Teste de rede para o sino (ESP32)

Programa de diagnóstico para levar ao campus. Ele responde à pergunta: **dá
para o sino pegar a hora certa pela rede da instituição, mesmo sem login e sem
o MAC cadastrado?**

O programa **não faz login e não tenta passar pelo captcha**. Ele só verifica
o que a rede já oferece para qualquer aparelho conectado. Rode o teste junto
com o pessoal da TI: é a melhor hora para pedir o cadastro do MAC, e eles podem
indicar o servidor de hora da rede.

## O que ele testa

1. **MAC do ESP32**, para a TI cadastrar.
2. **Redes por perto**: nome, sinal e tipo de segurança.
3. **Conexão**. Se o roteador recusar, mostra o código e o motivo provável
   (filtro de MAC, senha errada, rede fora de alcance, DHCP sem IP...).
4. **Dados da rede**: IP, roteador, DNS e o servidor de hora informado pelo
   DHCP, se houver.
5. **DNS**: se funciona, ou se o portal desvia os nomes para a página de login.
6. **Páginas**: o roteador, `1.1.1.1` e o teste de internet do Google. Mostra
   se a internet está liberada ou se tudo cai na página de login, e lê a hora
   do cabeçalho `Date` de cada resposta.
7. **Página de login**: lê só o cabeçalho e o título (HTTPS), sem preencher nada.
8. **Servidores de hora (NTP)**: o do DHCP, o roteador, os servidores DNS, a
   página de login e servidores da internet (`a.ntp.br`, Google).
9. **Refinamento**: com uns 30 pedidos seguidos à mesma página, acha o momento
   exato em que o segundo vira e melhora a precisão do `Date` de ±0,5 s para
   alguns milissegundos.
10. **Resumo e conclusão**, com a melhor fonte de hora e a margem de erro de cada uma.

## Como usar

1. Na Arduino IDE, instale o pacote **esp32 (Espressif)**, versão 2.x ou 3.x,
   e escolha a placa **ESP32 Dev Module**. Não precisa de nenhuma biblioteca extra.
2. Abra `teste_rede/teste_rede.ino` e grave.
3. Abra o Monitor Serial em **115200 baud**, com final de linha **Nova linha**.
4. Digite os comandos:

```
redes                       ← lista as redes por perto
rede IFMS-Alunos            ← escolhe a rede (o nome exato, com maiúsculas)
senha minhaSenha            ← só se a rede tiver senha
testar                      ← faz tudo e mostra o resumo
```

A rede e a senha ficam salvas. Da próxima vez que ligar, ele testa sozinho.

## Todos os comandos

| Comando | O que faz |
|---|---|
| `mac` | mostra o MAC deste ESP32 |
| `redes` | procura as redes Wi-Fi por perto |
| `rede NOME` | escolhe a rede (fica salvo) |
| `senha SENHA` | senha da rede; `senha` sozinho = rede aberta |
| `testar` | conecta e faz todos os testes (de 10 s a uns 2 min) |
| `resumo` | mostra de novo o resumo do último teste |
| `hora` | hora atual segundo a melhor fonte encontrada |
| `info` | IP, roteador, DNS e sinal da conexão atual |
| `ntp IP-ou-NOME` | testa um servidor de hora (ex.: o que a TI indicar) |
| `http ENDERECO` | lê o cabeçalho e o título de uma página (`http://` ou `https://`) |
| `dns NOME` | traduz um nome para IP |
| `desconectar` | sai da rede |
| `esquecer` | apaga a rede e a senha salvas |
| `ajuda` | lista os comandos |

## Sugestão de roteiro no campus

1. `testar` **antes** de pedir qualquer coisa à TI: mostra o que funciona sem
   cadastro.
2. Peça à TI o cadastro do MAC que aparece no topo. Pergunte também se a rede
   tem um servidor de hora (NTP) e teste com `ntp IP-QUE-ELES-DEREM`.
3. `testar` de novo **depois** do cadastro, para comparar.
4. Copie tudo o que apareceu no monitor serial e mande para análise. Com isso
   dá para decidir se o sino acerta a hora pela rede, pelo GPS ou pelos dois.

## Como ler a conclusão

| Conclusão | O que significa |
|---|---|
| **DA PARA USAR** (NTP interno) | um servidor de hora da própria rede responde sem login: é a melhor opção pela rede |
| NTP da internet responde | a porta de hora está aberta para fora, mas a TI pode fechar a qualquer momento |
| só as páginas informam a hora | dá para usar o `Date` refinado (alguns ms), mas é o método mais frágil |
| internet LIBERADA | o MAC já está cadastrado (ou a rede é aberta): funciona enquanto o cadastro existir |
| sem IP / recusado | sem cadastro não há como usar a rede: fique com o GPS |

## Para quem for mexer no código

As funções de análise têm testes no PC:

```
cd testes && g++ -std=c++17 -Wall -I../teste_rede teste_analise.cpp -o teste && ./teste
```
