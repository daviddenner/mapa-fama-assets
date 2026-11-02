// Página de configuração do sino (fica na memória flash do ESP).
// Conversa com o sino pelos endereços /api/... (ver web.ino).
#pragma once
#include <pgmspace.h>

static const char PAGINA[] PROGMEM = R"HTML(<!doctype html>
<html lang="pt-BR"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Sino</title>
<style>
:root{color-scheme:light dark;--fundo:#f3f4f6;--cartao:#fff;--texto:#1c2230;--suave:#5b6474;--linha:#d8dce3;--azul:#1f5fbf;--verde:#1d7a39;--amarelo:#9a6400;--vermelho:#b42318}
@media (prefers-color-scheme:dark){:root{--fundo:#121418;--cartao:#1c2027;--texto:#e6e9ef;--suave:#9aa3b2;--linha:#323843;--azul:#6ea2ff;--verde:#5ccf7c;--amarelo:#f0b44c;--vermelho:#ff7b72}}
*{box-sizing:border-box}
body{margin:0;font:16px/1.45 system-ui,-apple-system,"Segoe UI",Roboto,sans-serif;background:var(--fundo);color:var(--texto)}
main{max-width:760px;margin:0 auto;padding:12px 16px 40px}
h1{font-size:20px;margin:8px 0 12px;display:flex;gap:10px;align-items:center;flex-wrap:wrap}
h2{font-size:17px;margin:0 0 10px}
section{background:var(--cartao);border:1px solid var(--linha);border-radius:10px;padding:14px;margin-bottom:14px}
.relogio{font-size:42px;font-weight:600;font-variant-numeric:tabular-nums}
.suave{color:var(--suave);font-size:14px}
.selo{padding:2px 10px;border-radius:99px;font-size:13px;font-weight:600;border:1px solid currentColor}
.ok{color:var(--verde)}.rtc{color:var(--amarelo)}.semhora,.erro{color:var(--vermelho)}
button{font:inherit;padding:8px 14px;border-radius:8px;border:1px solid var(--linha);background:var(--cartao);color:var(--texto);cursor:pointer}
button.prim{background:var(--azul);border-color:var(--azul);color:#fff}
input,textarea{font:inherit;color:var(--texto);background:var(--fundo);border:1px solid var(--linha);border-radius:6px;padding:5px 6px}
table{border-collapse:collapse;width:100%}
td,th{padding:6px 4px;border-bottom:1px solid var(--linha);text-align:left;vertical-align:middle}
th{font-size:13px;color:var(--suave);font-weight:600}
.dias{white-space:nowrap}
.dias label{display:inline-flex;flex-direction:column;align-items:center;font-size:11px;margin-right:3px}
.num{width:70px}
.msg{font-size:14px}
.botoes{display:flex;gap:8px;flex-wrap:wrap;align-items:center;margin-top:10px}
.campo{display:block;margin:10px 0}
.campo input{width:100%;max-width:340px;display:block;margin-top:3px}
textarea{width:100%;min-height:70px;font:13px monospace}
.rmX{display:none}
@media (max-width:600px){table,tbody{display:block}thead{display:none}
tr{display:grid;grid-template-columns:auto auto auto 1fr;gap:6px 8px;align-items:end;padding:10px 0;border-bottom:1px solid var(--linha)}
td{display:block;border:0;padding:0}td::before{content:attr(data-r);display:block;font-size:11px;color:var(--suave);white-space:nowrap}
td.dias{grid-column:1/-1;order:5}td:nth-child(5){justify-self:end}.num{width:52px}
.rmTxt{display:none}.rmX{display:inline}button.rm{padding:6px 11px}}
</style></head><body><main>
<h1>Sino <span id="nome"></span> <span id="selo" class="selo">...</span></h1>

<section>
<div class="relogio" id="hora">--:--:--</div>
<div id="data" class="suave"></div>
<div id="gps" class="suave"></div>
<div>Próximo toque: <b id="proximo">-</b></div>
<div class="botoes"><button id="btTocar">Tocar agora (teste)</button><span id="msgTocar" class="msg"></span></div>
</section>

<section>
<h2>Horários</h2>
<table><thead><tr><th>Hora</th><th>Dias</th><th>Duração (s)</th><th>Batidas</th><th></th></tr></thead>
<tbody id="linhas"></tbody></table>
<p id="vazio" class="suave" hidden>Nenhum horário: o sino não vai tocar.</p>
<div class="botoes"><button id="btAdd">+ Adicionar horário</button><button id="btSalvar" class="prim">Salvar horários</button><span id="msgSalvar" class="msg"></span></div>
<details style="margin-top:12px"><summary class="suave">Copiar a tabela para outro sino</summary>
<p class="suave">Copie este texto, abra a página do outro sino, cole no mesmo lugar, clique em "Usar este texto" e depois em "Salvar horários".</p>
<textarea id="texto" spellcheck="false"></textarea>
<div class="botoes"><button id="btTexto">Usar este texto</button><span id="msgTexto" class="msg"></span></div>
</details>
</section>

<section>
<h2>Relógio</h2>
<p class="suave">O GPS acerta o relógio sozinho. Use o botão só se o GPS ainda não pegou sinal; quando ele pegar, corrige.</p>
<div class="botoes"><button id="btHora">Acertar pela hora deste aparelho</button><span id="msgHora" class="msg"></span></div>
</section>

<section>
<h2>Rede e senhas</h2>
<div id="redeInfo" class="suave"></div>
<label class="campo">Rede Wi-Fi da instituição (vazio = não usar)<input id="instNome" maxlength="32" autocomplete="off"></label>
<label class="campo">Senha da rede da instituição (vazio = manter a atual)<input id="instSenha" type="password" maxlength="64" autocomplete="new-password"></label>
<label class="campo">Nova senha do Wi-Fi do sino (mínimo 8; vazio = manter)<input id="apSenha" type="password" maxlength="63" autocomplete="new-password"></label>
<label class="campo">Nova senha desta página (mínimo 6; vazio = manter)<input id="adminSenha" type="password" maxlength="32" autocomplete="new-password"></label>
<div class="botoes"><button id="btRede" class="prim">Salvar e reiniciar o sino</button><span id="msgRede" class="msg"></span></div>
</section>
</main>
<script>
var DIAS=[["Seg",1],["Ter",2],["Qua",3],["Qui",4],["Sex",5],["Sáb",6],["Dom",0]];
var NOMES=["dom","seg","ter","qua","qui","sex","sáb"];
var token="",lista=[],difRelogio=null,alterado=false;
function $(i){return document.getElementById(i)}
function d2(n){return(n<10?"0":"")+n}
function msg(i,t,erro){var e=$(i);e.textContent=t;e.className="msg "+(erro?"erro":"ok")}
function post(url,dados){dados.token=token;return fetch(url,{method:"POST",headers:{"Content-Type":"application/x-www-form-urlencoded"},body:new URLSearchParams(dados)}).then(function(r){return r.text().then(function(t){if(!r.ok)throw new Error(t||("erro "+r.status));return t})})}
function lerTexto(txt){var out=[];txt.split(/[;\n\r]+/).forEach(function(it){it=it.trim();if(!it)return;var m=/^(\d{1,2}):(\d{2}),(\d+),(\d+),(\d+)$/.exec(it);if(!m)throw new Error("item invalido: "+it);out.push({h:+m[1],m:+m[2],dias:+m[3],ms:+m[4],vezes:+m[5]})});return out}
function escreverTexto(l){return l.map(function(t){return d2(t.h)+":"+d2(t.m)+","+t.dias+","+t.ms+","+t.vezes}).join(";")}
function ordenar(){lista.sort(function(a,b){return(a.h*60+a.m)-(b.h*60+b.m)})}
function celula(tr,rotulo,el){var td=document.createElement("td");td.setAttribute("data-r",rotulo);if(el)td.appendChild(el);tr.appendChild(td);return td}
function desenhar(){
 var tb=$("linhas");tb.innerHTML="";
 lista.forEach(function(t,i){
  var tr=document.createElement("tr");
  var hh=document.createElement("input");hh.type="time";hh.required=true;hh.value=t.h<0?"":d2(t.h)+":"+d2(t.m);
  hh.onchange=function(){var p=hh.value.split(":");if(p.length>=2&&p[0]!==""){t.h=+p[0];t.m=+p[1]}else{t.h=-1}mudou()};
  celula(tr,"Hora",hh);
  var td=celula(tr,"Dias");td.className="dias";
  DIAS.forEach(function(d){var lb=document.createElement("label"),cb=document.createElement("input");cb.type="checkbox";cb.checked=!!(t.dias&(1<<d[1]));cb.onchange=function(){if(cb.checked)t.dias|=1<<d[1];else t.dias&=~(1<<d[1]);mudou()};lb.appendChild(cb);lb.appendChild(document.createTextNode(d[0]));td.appendChild(lb)});
  var du=document.createElement("input");du.type="number";du.className="num";du.min="0.1";du.max="30";du.step="0.1";du.value=t.ms/1000;
  du.onchange=function(){t.ms=Math.round(parseFloat(String(du.value).replace(",","."))*1000);mudou()};
  celula(tr,"Duração (s)",du);
  var vz=document.createElement("input");vz.type="number";vz.className="num";vz.min="1";vz.max="10";vz.step="1";vz.value=t.vezes;
  vz.onchange=function(){t.vezes=parseInt(vz.value,10);mudou()};
  celula(tr,"Batidas",vz);
  var rm=document.createElement("button");rm.className="rm";rm.title="Remover este horário";rm.setAttribute("aria-label","Remover este horário");
  rm.innerHTML='<span class="rmTxt">Remover</span><span class="rmX" aria-hidden="true">✕</span>';rm.onclick=function(){lista.splice(i,1);mudou();desenhar()};
  celula(tr,"",rm);
  tb.appendChild(tr)});
 $("vazio").hidden=lista.length>0;
 $("texto").value=escreverTexto(lista.filter(function(t){return t.h>=0}))}
function mudou(){alterado=true;msg("msgSalvar","alterações não salvas",true)}
function validar(){var vistos={};for(var i=0;i<lista.length;i++){var t=lista[i],n=i+1;
 if(t.h<0)return"linha "+n+": preencha a hora";var hm=d2(t.h)+":"+d2(t.m);
 if(!t.dias)return hm+": marque pelo menos um dia";
 if(!(t.ms>=100&&t.ms<=30000))return hm+": duração de 0,1 a 30 s";
 if(!(t.vezes>=1&&t.vezes<=10))return hm+": batidas de 1 a 10";
 if((vistos[hm]||0)&t.dias)return hm+" aparece duas vezes no mesmo dia";vistos[hm]=(vistos[hm]||0)|t.dias}
 return""}
function carregarHorarios(){return fetch("/api/horarios").then(function(r){return r.text()}).then(function(t){lista=lerTexto(t);ordenar();desenhar();alterado=false})}
function hora(ms){var d=new Date(ms);return{hms:d2(d.getUTCHours())+":"+d2(d.getUTCMinutes())+":"+d2(d.getUTCSeconds()),data:NOMES[d.getUTCDay()]+", "+d2(d.getUTCDate())+"/"+d2(d.getUTCMonth()+1)+"/"+d.getUTCFullYear()}}
function tique(){if(difRelogio===null)return;var h=hora(Date.now()+difRelogio);$("hora").textContent=h.hms;$("data").textContent=h.data}
function estado(){fetch("/api/estado").then(function(r){if(!r.ok)throw 0;return r.json()}).then(function(e){
 token=e.token;$("nome").textContent=e.nome;
 var s=$("selo");
 if(!e.valida){s.textContent="SEM HORA: não vai tocar";s.className="selo semhora";difRelogio=null;$("hora").textContent="--:--:--";$("data").textContent=""}
 else{difRelogio=e.agora*1000-Date.now();tique();
  if(e.tocando){s.textContent="tocando";s.className="selo ok"}
  else if(e.estado=="ok"){s.textContent="GPS ok";s.className="selo ok"}
  else{s.textContent=e.manual?"hora acertada à mão":"só o relógio interno";s.className="selo rtc"}}
 $("gps").textContent="GPS: "+e.gps+(e.sync>=0?" · último acerto há "+e.sync+" min":" · ainda sem acerto pelo GPS")+(e.aviso?" · AVISO SONORO LIGADO (cada toque ganha uma batida curta no fim)":"");
 $("proximo").textContent=e.proximo||"-";
 var t="Wi-Fi do sino: "+e.ap+" (oculto), "+e.apClientes+" aparelho(s) conectado(s), página em http://192.168.4.1. ";
 t+=e.inst?("Rede da instituição \""+e.inst+"\": "+e.instEstado+(e.instIp?", página também em http://"+e.instIp+" ou http://"+e.nome+".local":"")):"Rede da instituição: não configurada.";
 $("redeInfo").textContent=t;
 if(document.activeElement!==$("instNome")&&!$("instNome").dataset.ok){$("instNome").value=e.inst;$("instNome").dataset.ok=1}
}).catch(function(){var s=$("selo");s.textContent="sem conexão com o sino";s.className="selo semhora"})}
$("btAdd").onclick=function(){lista.push({h:-1,m:0,dias:62,ms:3000,vezes:1});mudou();desenhar()};
$("btSalvar").onclick=function(){var e=validar();if(e){msg("msgSalvar",e,true);return}ordenar();
 post("/api/horarios",{tabela:escreverTexto(lista)}).then(function(){alterado=false;return carregarHorarios()}).then(function(){msg("msgSalvar","Salvo! O sino já está usando a nova tabela.")}).catch(function(x){msg("msgSalvar",x.message,true)})};
$("btTexto").onclick=function(){try{lista=lerTexto($("texto").value);ordenar();desenhar();mudou();msg("msgTexto","Tabela carregada. Confira e clique em Salvar horários.")}catch(x){msg("msgTexto",x.message,true)}};
$("btTocar").onclick=function(){if(!confirm("Tocar o sino agora?"))return;post("/api/tocar",{}).then(function(){msg("msgTocar","Tocando.")}).catch(function(x){msg("msgTocar",x.message,true)})};
$("btHora").onclick=function(){var agora=Date.now();if(!confirm("Usar a hora deste aparelho ("+new Date(agora).toLocaleTimeString()+")?"))return;post("/api/hora",{ms:Date.now()}).then(function(){msg("msgHora","Hora acertada.");estado()}).catch(function(x){msg("msgHora",x.message,true)})};
$("btRede").onclick=function(){var ap=$("apSenha").value,ad=$("adminSenha").value;
 if(ap&&ap.length<8){msg("msgRede","A senha do Wi-Fi precisa de pelo menos 8 caracteres",true);return}
 if(ad&&ad.length<6){msg("msgRede","A senha da página precisa de pelo menos 6 caracteres",true);return}
 if(!confirm("Salvar e reiniciar o sino? Ele volta em uns 10 segundos."))return;
 post("/api/rede",{instNome:$("instNome").value,instSenha:$("instSenha").value,apSenha:ap,adminSenha:ad}).then(function(t){msg("msgRede",t)}).catch(function(x){msg("msgRede",x.message,true)})};
window.onbeforeunload=function(){if(alterado)return"Há horários não salvos."};
carregarHorarios().catch(function(){msg("msgSalvar","não consegui ler os horários",true)});
estado();setInterval(estado,5000);setInterval(tique,250);
</script></body></html>)HTML";
