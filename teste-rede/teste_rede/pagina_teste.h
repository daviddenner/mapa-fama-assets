// Página do teste, servida pelo Wi-Fi do próprio ESP (TESTE-SINO, 192.168.4.1).
// Mostra a mesma saída do monitor serial, manda comandos e baixa os relatórios.
#pragma once
#include <Arduino.h>

static const char PAGINA_TESTE[] PROGMEM = R"PAG(<!doctype html><html lang="pt-BR"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1"><title>Teste do sino</title>
<style>
body{font-family:system-ui,sans-serif;margin:0;background:#f4f4f4;color:#222}
header{background:#1b5e20;color:#fff;padding:10px 14px}h1{font-size:1.1em;margin:0}
main{padding:10px 14px;max-width:900px;margin:auto}
section{background:#fff;border-radius:8px;padding:10px 12px;margin:10px 0;box-shadow:0 1px 2px #0002}
label{display:block;margin:6px 0 2px;font-size:.9em}
input,select{width:100%;box-sizing:border-box;padding:8px;font-size:1em;border:1px solid #bbb;border-radius:6px;background:#fff;color:#222}
button{padding:9px 12px;margin:4px 4px 0 0;font-size:1em;border:0;border-radius:6px;background:#1b5e20;color:#fff}
button.c{background:#666}
pre{background:#111;color:#cfc;padding:10px;border-radius:8px;font-size:.78em;line-height:1.3;max-height:60vh;overflow:auto;white-space:pre-wrap;word-break:break-word}
.s{font-size:.95em;line-height:1.5}.ok{color:#1b5e20;font-weight:bold}.at{color:#b71c1c;font-weight:bold}
.linha{display:flex;gap:6px}.linha input{flex:1}
</style></head><body>
<header><h1>Teste da rede e da placa do sino</h1></header><main>
<section class=s id=est>carregando...</section>
<section>
<label>Rede Wi-Fi da instituição (nome exato)</label><input id=rede autocapitalize=off>
<label>Senha da rede (vazio = rede aberta)</label><input id=senha type=password>
<label>Potência do Wi-Fi (só mude se não conectar; placas ESP32-C3 "Super Mini" costumam precisar de 8,5 dBm)</label>
<select id=pot><option value=max>máxima</option><option value=8.5>8,5 dBm</option><option value=11>11 dBm</option><option value=15>15 dBm</option></select>
<br><button onclick="salvar()">Salvar</button><button onclick="cmd('testar')">Testar tudo</button>
<button class=c onclick="cmd('redes')">Redes por perto</button><button class=c onclick="cmd('resumo')">Resumo</button>
<button class=c onclick="cmd('vigiar 30')">Vigiar 30 min</button><button class=c onclick="cmd('parar')">Parar</button>
</section>
<section><b>Placa do sino</b><br>
<button class=c onclick="cmd('rele')">Testar relé</button><button class=c onclick="cmd('rtc')">Relógio</button>
<button class=c onclick="if(confirm('Gravar no relogio DS3231 a hora da rede?'))cmd('rtc acertar')">Acertar relógio pela rede</button>
</section>
<section><div class=linha><input id=cmd placeholder="comando (ex.: ntp 10.0.0.1 | http http://10.0.0.1/ | ajuda)" autocapitalize=off onkeydown="if(event.key=='Enter')enviar()"><button onclick="enviar()">Enviar</button></div></section>
<section><b>Saída</b> (a mesma do monitor serial) <button class=c onclick="document.getElementById('log').textContent=''">Limpar tela</button><pre id=log></pre></section>
<section><b>Relatórios guardados no ESP</b> <button class=c onclick="relatorios()">Atualizar</button>
<button class=c onclick="if(confirm('Apagar todos os relatorios?'))fetch('/api/apagar',{method:'POST'}).then(relatorios)">Apagar</button><div id=rel></div></section>
</main><script>
let desde=0,preenchido=false;
const F={method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'}};
function cmd(c){fetch('/api/cmd',Object.assign({body:'c='+encodeURIComponent(c)},F))}
function enviar(){const i=document.getElementById('cmd');const v=i.value.trim();if(v){cmd(v);i.value=''}}
function salvar(){const q='rede='+encodeURIComponent(rede.value)+'&senha='+encodeURIComponent(senha.value)+'&potencia='+pot.value;
fetch('/api/rede',Object.assign({body:q},F)).then(()=>{senha.value=''})}
async function lerLog(){try{const r=await fetch('/api/log?desde='+desde);const t=await r.text();const tot=+r.headers.get('X-Total');
if(t){const p=document.getElementById('log');const noFim=p.scrollTop+p.clientHeight>=p.scrollHeight-40;p.textContent+=t;if(noFim)p.scrollTop=p.scrollHeight}
if(tot)desde=tot}catch(e){}}
async function estado(){try{const e=await (await fetch('/api/estado')).json();
if(!preenchido){rede.value=e.rede;pot.value=e.pot;preenchido=true}
document.getElementById('est').innerHTML='<b>MAC:</b> '+e.mac+' ('+e.chip+') &nbsp; <b>Potência:</b> '+e.potencia+
'<br><b>Rede:</b> '+(e.rede||'(nenhuma escolhida)')+' — '+(e.conectado?'conectado, IP '+e.ip:'desconectado')+
'<br><b>Relógio DS3231:</b> '+e.rtc+' &nbsp; <b>Relé:</b> '+(e.rele?'testado '+e.rele+'x':'não testado')+
'<br><b>Situação:</b> '+(e.ocupado?'<span class=at>trabalhando...</span>':'parado')+
'<br><b>Conclusão:</b> '+(e.conclusao?'<span class='+(e.conclusao<=2?'ok':'at')+'>'+e.texto+'</span>':'ainda sem teste')}catch(e){}}
async function relatorios(){try{const t=await (await fetch('/api/relatorios')).text();
document.getElementById('rel').innerHTML=t.trim()?t.trim().split('\n').map(l=>{const c=l.split('|');return '<div><a href="/relatorio?n='+c[0]+'">relatório '+c[0]+'</a> ('+c[1]+' bytes) '+c.slice(2).join('|')+'</div>'}).join(''):'nenhum ainda'}catch(e){}}
setInterval(lerLog,1000);setInterval(estado,3000);lerLog();estado();relatorios();
</script></body></html>)PAG";
