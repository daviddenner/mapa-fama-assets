<?xml version="1.0" encoding="utf-8"?>
<!DOCTYPE eagle SYSTEM "eagle.dtd">
<eagle version="7.7.0">
<drawing>
<settings>
<setting alwaysvectorfont="no"/>
<setting verticaltext="up"/>
</settings>
<grid distance="0.1" unitdist="inch" unit="inch" style="lines" multiple="1" display="no" altdistance="0.01" altunitdist="inch" altunit="inch"/>
<layers>
<layer number="1" name="Top" color="4" fill="1" visible="yes" active="yes"/>
<layer number="2" name="Route2" color="1" fill="3" visible="no" active="yes"/>
<layer number="3" name="Route3" color="4" fill="3" visible="no" active="yes"/>
<layer number="4" name="Route4" color="1" fill="4" visible="no" active="yes"/>
<layer number="5" name="Route5" color="4" fill="4" visible="no" active="yes"/>
<layer number="6" name="Route6" color="1" fill="8" visible="no" active="yes"/>
<layer number="7" name="Route7" color="4" fill="8" visible="no" active="yes"/>
<layer number="8" name="Route8" color="1" fill="2" visible="no" active="yes"/>
<layer number="9" name="Route9" color="4" fill="2" visible="no" active="yes"/>
<layer number="10" name="Route10" color="1" fill="7" visible="no" active="yes"/>
<layer number="11" name="Route11" color="4" fill="7" visible="no" active="yes"/>
<layer number="12" name="Route12" color="1" fill="5" visible="no" active="yes"/>
<layer number="13" name="Route13" color="4" fill="5" visible="no" active="yes"/>
<layer number="14" name="Route14" color="1" fill="6" visible="no" active="yes"/>
<layer number="15" name="Route15" color="4" fill="6" visible="no" active="yes"/>
<layer number="16" name="Bottom" color="1" fill="1" visible="yes" active="yes"/>
<layer number="17" name="Pads" color="2" fill="1" visible="yes" active="yes"/>
<layer number="18" name="Vias" color="2" fill="1" visible="yes" active="yes"/>
<layer number="19" name="Unrouted" color="6" fill="1" visible="yes" active="yes"/>
<layer number="20" name="Dimension" color="15" fill="1" visible="yes" active="yes"/>
<layer number="21" name="tPlace" color="7" fill="1" visible="yes" active="yes"/>
<layer number="22" name="bPlace" color="7" fill="1" visible="no" active="yes"/>
<layer number="23" name="tOrigins" color="15" fill="1" visible="yes" active="yes"/>
<layer number="24" name="bOrigins" color="15" fill="1" visible="no" active="yes"/>
<layer number="25" name="tNames" color="7" fill="1" visible="yes" active="yes"/>
<layer number="26" name="bNames" color="7" fill="1" visible="no" active="yes"/>
<layer number="27" name="tValues" color="7" fill="1" visible="yes" active="yes"/>
<layer number="28" name="bValues" color="7" fill="1" visible="no" active="yes"/>
<layer number="29" name="tStop" color="7" fill="3" visible="yes" active="yes"/>
<layer number="30" name="bStop" color="7" fill="6" visible="no" active="yes"/>
<layer number="31" name="tCream" color="7" fill="4" visible="no" active="yes"/>
<layer number="32" name="bCream" color="7" fill="5" visible="no" active="yes"/>
<layer number="33" name="tFinish" color="6" fill="3" visible="no" active="yes"/>
<layer number="34" name="bFinish" color="6" fill="6" visible="no" active="yes"/>
<layer number="35" name="tGlue" color="7" fill="4" visible="no" active="yes"/>
<layer number="36" name="bGlue" color="7" fill="5" visible="no" active="yes"/>
<layer number="37" name="tTest" color="7" fill="1" visible="no" active="yes"/>
<layer number="38" name="bTest" color="7" fill="1" visible="no" active="yes"/>
<layer number="39" name="tKeepout" color="4" fill="11" visible="yes" active="yes"/>
<layer number="40" name="bKeepout" color="1" fill="11" visible="no" active="yes"/>
<layer number="41" name="tRestrict" color="4" fill="10" visible="yes" active="yes"/>
<layer number="42" name="bRestrict" color="1" fill="10" visible="no" active="yes"/>
<layer number="43" name="vRestrict" color="2" fill="10" visible="no" active="yes"/>
<layer number="44" name="Drills" color="7" fill="1" visible="yes" active="yes"/>
<layer number="45" name="Holes" color="7" fill="1" visible="yes" active="yes"/>
<layer number="46" name="Milling" color="3" fill="1" visible="no" active="yes"/>
<layer number="47" name="Measures" color="7" fill="1" visible="no" active="yes"/>
<layer number="48" name="Document" color="7" fill="1" visible="no" active="yes"/>
<layer number="49" name="Reference" color="7" fill="1" visible="no" active="yes"/>
<layer number="50" name="dxf" color="7" fill="1" visible="no" active="yes"/>
<layer number="51" name="tDocu" color="7" fill="1" visible="yes" active="yes"/>
<layer number="52" name="bDocu" color="7" fill="1" visible="no" active="yes"/>
<layer number="91" name="Nets" color="2" fill="1" visible="yes" active="yes"/>
<layer number="92" name="Busses" color="1" fill="1" visible="yes" active="yes"/>
<layer number="93" name="Pins" color="2" fill="1" visible="yes" active="yes"/>
<layer number="94" name="Symbols" color="4" fill="1" visible="yes" active="yes"/>
<layer number="95" name="Names" color="7" fill="1" visible="yes" active="yes"/>
<layer number="96" name="Values" color="7" fill="1" visible="yes" active="yes"/>
<layer number="97" name="Info" color="7" fill="1" visible="yes" active="yes"/>
<layer number="98" name="Guide" color="6" fill="1" visible="yes" active="yes"/>
</layers>
<schematic xreflabel="%F%N/%S.%C%R" xrefpart="/%S.%C%R">
<libraries>
<library name="sino_ifms">
<description>Biblioteca do projeto Sino IFMS (gerada automaticamente).</description>
<packages>
<package name="NODEMCU-V3">
<description>NodeMCU v3 (LoLin): 2 fileiras de 15 pinos a 1,1" (27,94 mm); placa de 31 x 58 mm (contorno aproximado). Pino 1 = A0, antena para cima.</description>
<pad name="1" x="0" y="0" drill="1" diameter="1.8" shape="square"/>
<pad name="16" x="27.94" y="0" drill="1" diameter="1.8"/>
<pad name="2" x="0" y="-2.54" drill="1" diameter="1.8"/>
<pad name="17" x="27.94" y="-2.54" drill="1" diameter="1.8"/>
<pad name="3" x="0" y="-5.08" drill="1" diameter="1.8"/>
<pad name="18" x="27.94" y="-5.08" drill="1" diameter="1.8"/>
<pad name="4" x="0" y="-7.62" drill="1" diameter="1.8"/>
<pad name="19" x="27.94" y="-7.62" drill="1" diameter="1.8"/>
<pad name="5" x="0" y="-10.16" drill="1" diameter="1.8"/>
<pad name="20" x="27.94" y="-10.16" drill="1" diameter="1.8"/>
<pad name="6" x="0" y="-12.7" drill="1" diameter="1.8"/>
<pad name="21" x="27.94" y="-12.7" drill="1" diameter="1.8"/>
<pad name="7" x="0" y="-15.24" drill="1" diameter="1.8"/>
<pad name="22" x="27.94" y="-15.24" drill="1" diameter="1.8"/>
<pad name="8" x="0" y="-17.78" drill="1" diameter="1.8"/>
<pad name="23" x="27.94" y="-17.78" drill="1" diameter="1.8"/>
<pad name="9" x="0" y="-20.32" drill="1" diameter="1.8"/>
<pad name="24" x="27.94" y="-20.32" drill="1" diameter="1.8"/>
<pad name="10" x="0" y="-22.86" drill="1" diameter="1.8"/>
<pad name="25" x="27.94" y="-22.86" drill="1" diameter="1.8"/>
<pad name="11" x="0" y="-25.4" drill="1" diameter="1.8"/>
<pad name="26" x="27.94" y="-25.4" drill="1" diameter="1.8"/>
<pad name="12" x="0" y="-27.94" drill="1" diameter="1.8"/>
<pad name="27" x="27.94" y="-27.94" drill="1" diameter="1.8"/>
<pad name="13" x="0" y="-30.48" drill="1" diameter="1.8"/>
<pad name="28" x="27.94" y="-30.48" drill="1" diameter="1.8"/>
<pad name="14" x="0" y="-33.02" drill="1" diameter="1.8"/>
<pad name="29" x="27.94" y="-33.02" drill="1" diameter="1.8"/>
<pad name="15" x="0" y="-35.56" drill="1" diameter="1.8"/>
<pad name="30" x="27.94" y="-35.56" drill="1" diameter="1.8"/>
<wire x1="-1.53" y1="12.7" x2="29.47" y2="12.7" width="0.2032" layer="21"/>
<wire x1="29.47" y1="12.7" x2="29.47" y2="-45.3" width="0.2032" layer="21"/>
<wire x1="29.47" y1="-45.3" x2="-1.53" y2="-45.3" width="0.2032" layer="21"/>
<wire x1="-1.53" y1="-45.3" x2="-1.53" y2="12.7" width="0.2032" layer="21"/>
<wire x1="10.47" y1="-45.3" x2="17.47" y2="-45.3" width="0.6" layer="21"/>
<text x="0" y="14" size="1.27" layer="25">&gt;NAME</text>
<text x="0" y="12.095" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="MOD-4P">
<description>Barra de 4 pinos 2,54 mm do modulo DS3231 (ZS-042). Confira a ordem no modulo.</description>
<pad name="1" x="0" y="0" drill="1" diameter="1.8" shape="square"/>
<pad name="2" x="0" y="-2.54" drill="1" diameter="1.8"/>
<pad name="3" x="0" y="-5.08" drill="1" diameter="1.8"/>
<pad name="4" x="0" y="-7.62" drill="1" diameter="1.8"/>
<wire x1="-1.27" y1="1.27" x2="1.27" y2="1.27" width="0.2032" layer="21"/>
<wire x1="1.27" y1="1.27" x2="1.27" y2="-8.89" width="0.2032" layer="21"/>
<wire x1="1.27" y1="-8.89" x2="-1.27" y2="-8.89" width="0.2032" layer="21"/>
<wire x1="-1.27" y1="-8.89" x2="-1.27" y2="1.27" width="0.2032" layer="21"/>
<text x="2" y="1" size="1.27" layer="25">&gt;NAME</text>
<text x="2" y="-0.905" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="RELE-SRD">
<description>Rele Songle/Sanyou SRD-05VDC-SL-C (medidas do footprint SANYOU_SRD do KiCad).</description>
<pad name="1" x="0" y="0" drill="1.3" diameter="3"/>
<pad name="2" x="1.95" y="-6.05" drill="1" diameter="2.5"/>
<pad name="3" x="14.15" y="-6.05" drill="1.3" diameter="3"/>
<pad name="4" x="14.2" y="6" drill="1.3" diameter="3"/>
<pad name="5" x="1.95" y="5.95" drill="1" diameter="2.5"/>
<wire x1="-1.4" y1="7.8" x2="18.4" y2="7.8" width="0.2032" layer="21"/>
<wire x1="18.4" y1="7.8" x2="18.4" y2="-7.8" width="0.2032" layer="21"/>
<wire x1="18.4" y1="-7.8" x2="-1.4" y2="-7.8" width="0.2032" layer="21"/>
<wire x1="-1.4" y1="-7.8" x2="-1.4" y2="7.8" width="0.2032" layer="21"/>
<text x="-1.4" y="9.5" size="1.27" layer="25">&gt;NAME</text>
<text x="-1.4" y="7.595" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="TO92-CBE">
<description>TO-92, terminais C-B-E em linha a 2,54 mm (BC548, BC337).</description>
<pad name="C" x="-2.54" y="0" drill="0.8" diameter="1.5"/>
<pad name="B" x="0" y="0" drill="0.8" diameter="1.5"/>
<pad name="E" x="2.54" y="0" drill="0.8" diameter="1.5"/>
<wire x1="-2.3" y1="-1.5" x2="2.3" y2="-1.5" width="0.2032" layer="21"/>
<wire x1="2.4" y1="-1.5" x2="2.3182" y2="-0.4906" width="0.2032" layer="21"/>
<wire x1="2.3182" y1="-0.4906" x2="2.0785" y2="0.45" width="0.2032" layer="21"/>
<wire x1="2.0785" y1="0.45" x2="1.6971" y2="1.2577" width="0.2032" layer="21"/>
<wire x1="1.6971" y1="1.2577" x2="1.2" y2="1.8775" width="0.2032" layer="21"/>
<wire x1="1.2" y1="1.8775" x2="0.6212" y2="2.2671" width="0.2032" layer="21"/>
<wire x1="0.6212" y1="2.2671" x2="0" y2="2.4" width="0.2032" layer="21"/>
<wire x1="0" y1="2.4" x2="-0.6212" y2="2.2671" width="0.2032" layer="21"/>
<wire x1="-0.6212" y1="2.2671" x2="-1.2" y2="1.8775" width="0.2032" layer="21"/>
<wire x1="-1.2" y1="1.8775" x2="-1.6971" y2="1.2577" width="0.2032" layer="21"/>
<wire x1="-1.6971" y1="1.2577" x2="-2.0785" y2="0.45" width="0.2032" layer="21"/>
<wire x1="-2.0785" y1="0.45" x2="-2.3182" y2="-0.4906" width="0.2032" layer="21"/>
<wire x1="-2.3182" y1="-0.4906" x2="-2.4" y2="-1.5" width="0.2032" layer="21"/>
<text x="-2.54" y="3.6" size="1.27" layer="25">&gt;NAME</text>
<text x="-2.54" y="1.695" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="R-0207/10">
<description>Resistor axial, furos a 10.16 mm</description>
<pad name="1" x="-5.08" y="0" drill="0.9" diameter="1.8"/>
<pad name="2" x="5.08" y="0" drill="0.9" diameter="1.8"/>
<wire x1="-3.3" y1="1.2" x2="3.3" y2="1.2" width="0.2032" layer="21"/>
<wire x1="3.3" y1="1.2" x2="3.3" y2="-1.2" width="0.2032" layer="21"/>
<wire x1="3.3" y1="-1.2" x2="-3.3" y2="-1.2" width="0.2032" layer="21"/>
<wire x1="-3.3" y1="-1.2" x2="-3.3" y2="1.2" width="0.2032" layer="21"/>
<text x="-3.3" y="2.2" size="1.27" layer="25">&gt;NAME</text>
<text x="-3.3" y="0.295" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="R-0309/12">
<description>Resistor axial, furos a 12.70 mm</description>
<pad name="1" x="-6.35" y="0" drill="0.9" diameter="1.8"/>
<pad name="2" x="6.35" y="0" drill="0.9" diameter="1.8"/>
<wire x1="-4.6" y1="1.2" x2="4.6" y2="1.2" width="0.2032" layer="21"/>
<wire x1="4.6" y1="1.2" x2="4.6" y2="-1.2" width="0.2032" layer="21"/>
<wire x1="4.6" y1="-1.2" x2="-4.6" y2="-1.2" width="0.2032" layer="21"/>
<wire x1="-4.6" y1="-1.2" x2="-4.6" y2="1.2" width="0.2032" layer="21"/>
<text x="-4.6" y="2.2" size="1.27" layer="25">&gt;NAME</text>
<text x="-4.6" y="0.295" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="DO41-10">
<description>Diodo DO-41 (1N4007), furos a 10,16 mm. Faixa = catodo.</description>
<pad name="A" x="-5.08" y="0" drill="1" diameter="1.9"/>
<pad name="C" x="5.08" y="0" drill="1" diameter="1.9"/>
<wire x1="-2.5" y1="1.4" x2="2.5" y2="1.4" width="0.2032" layer="21"/>
<wire x1="2.5" y1="1.4" x2="2.5" y2="-1.4" width="0.2032" layer="21"/>
<wire x1="2.5" y1="-1.4" x2="-2.5" y2="-1.4" width="0.2032" layer="21"/>
<wire x1="-2.5" y1="-1.4" x2="-2.5" y2="1.4" width="0.2032" layer="21"/>
<wire x1="1.6" y1="1.4" x2="1.6" y2="-1.4" width="0.5" layer="21"/>
<text x="-2.5" y="2.4" size="1.27" layer="25">&gt;NAME</text>
<text x="-2.5" y="0.495" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="LED3MM">
<description>LED 3 mm, furos a 2,54 mm (terminal curto/lado chato = catodo).</description>
<pad name="A" x="-1.27" y="0" drill="0.8" diameter="1.5"/>
<pad name="C" x="1.27" y="0" drill="0.8" diameter="1.5" shape="square"/>
<circle x="0" y="0" radius="1.9" width="0.2032" layer="21"/>
<text x="-2" y="3" size="1.27" layer="25">&gt;NAME</text>
<text x="-2" y="1.095" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="C-2.54">
<description>Capacitor ceramico, furos a 2,54 mm</description>
<pad name="1" x="-1.27" y="0" drill="0.8" diameter="1.5"/>
<pad name="2" x="1.27" y="0" drill="0.8" diameter="1.5"/>
<wire x1="-2.4" y1="1.2" x2="2.4" y2="1.2" width="0.2032" layer="21"/>
<wire x1="2.4" y1="1.2" x2="2.4" y2="-1.2" width="0.2032" layer="21"/>
<wire x1="2.4" y1="-1.2" x2="-2.4" y2="-1.2" width="0.2032" layer="21"/>
<wire x1="-2.4" y1="-1.2" x2="-2.4" y2="1.2" width="0.2032" layer="21"/>
<text x="-2.4" y="2.2" size="1.27" layer="25">&gt;NAME</text>
<text x="-2.4" y="0.295" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="C-X2-15">
<description>Capacitor de poliester X2 (275 VAC), furos a 15 mm</description>
<pad name="1" x="-7.5" y="0" drill="1" diameter="2"/>
<pad name="2" x="7.5" y="0" drill="1" diameter="2"/>
<wire x1="-9" y1="3" x2="9" y2="3" width="0.2032" layer="21"/>
<wire x1="9" y1="3" x2="9" y2="-3" width="0.2032" layer="21"/>
<wire x1="9" y1="-3" x2="-9" y2="-3" width="0.2032" layer="21"/>
<wire x1="-9" y1="-3" x2="-9" y2="3" width="0.2032" layer="21"/>
<text x="-9" y="4" size="1.27" layer="25">&gt;NAME</text>
<text x="-9" y="2.095" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="CPOL-8MM">
<description>Capacitor eletrolitico radial 8 mm, furos a 3,5 mm</description>
<pad name="+" x="-1.75" y="0" drill="0.9" diameter="1.7" shape="square"/>
<pad name="-" x="1.75" y="0" drill="0.9" diameter="1.7"/>
<circle x="0" y="0" radius="4.1" width="0.2032" layer="21"/>
<text x="-4" y="5.2" size="1.27" layer="25">&gt;NAME</text>
<text x="-4" y="3.295" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="BORNE-2-5.08">
<description>Borne de parafuso 2 vias, passo 5,08 mm (KF301)</description>
<pad name="1" x="0" y="0" drill="1.3" diameter="2.6" shape="square"/>
<pad name="2" x="5.08" y="0" drill="1.3" diameter="2.6"/>
<wire x1="-2.54" y1="3.8" x2="7.62" y2="3.8" width="0.2032" layer="21"/>
<wire x1="7.62" y1="3.8" x2="7.62" y2="-3.8" width="0.2032" layer="21"/>
<wire x1="7.62" y1="-3.8" x2="-2.54" y2="-3.8" width="0.2032" layer="21"/>
<wire x1="-2.54" y1="-3.8" x2="-2.54" y2="3.8" width="0.2032" layer="21"/>
<text x="-2.54" y="4.8" size="1.27" layer="25">&gt;NAME</text>
<text x="-2.54" y="2.895" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="FUSIVEL-5X20">
<description>Porta-fusivel 5x20 mm para placa (2 presilhas a 22,6 mm) - confira o seu.</description>
<pad name="1" x="-11.3" y="0" drill="1.3" diameter="2.6"/>
<pad name="2" x="11.3" y="0" drill="1.3" diameter="2.6"/>
<wire x1="-13.5" y1="3.5" x2="13.5" y2="3.5" width="0.2032" layer="21"/>
<wire x1="13.5" y1="3.5" x2="13.5" y2="-3.5" width="0.2032" layer="21"/>
<wire x1="13.5" y1="-3.5" x2="-13.5" y2="-3.5" width="0.2032" layer="21"/>
<wire x1="-13.5" y1="-3.5" x2="-13.5" y2="3.5" width="0.2032" layer="21"/>
<text x="-13.5" y="4.5" size="1.27" layer="25">&gt;NAME</text>
<text x="-13.5" y="2.595" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="BOTAO-6X6">
<description>Botao tactil 6x6 mm. Ligado em diagonal (1 e 4): funciona em qualquer marca.</description>
<pad name="1" x="0" y="0" drill="1" diameter="1.8"/>
<pad name="2" x="6.5" y="0" drill="1" diameter="1.8"/>
<pad name="3" x="0" y="-4.5" drill="1" diameter="1.8"/>
<pad name="4" x="6.5" y="-4.5" drill="1" diameter="1.8"/>
<wire x1="0.25" y1="0.75" x2="6.25" y2="0.75" width="0.2032" layer="21"/>
<wire x1="6.25" y1="0.75" x2="6.25" y2="-5.25" width="0.2032" layer="21"/>
<wire x1="6.25" y1="-5.25" x2="0.25" y2="-5.25" width="0.2032" layer="21"/>
<wire x1="0.25" y1="-5.25" x2="0.25" y2="0.75" width="0.2032" layer="21"/>
<circle x="3.25" y="-2.25" radius="1.75" width="0.2032" layer="21"/>
<text x="0" y="1.8" size="1.27" layer="25">&gt;NAME</text>
<text x="0" y="-0.105" size="1.27" layer="27">&gt;VALUE</text>
</package>
<package name="HDR-1X4">
<description>Barra de 4 pinos 2,54 mm (mesma ordem do GPS GY-NEO6MV2: VCC, RX, TX, GND)</description>
<pad name="1" x="0" y="0" drill="1" diameter="1.8" shape="square"/>
<pad name="2" x="0" y="-2.54" drill="1" diameter="1.8"/>
<pad name="3" x="0" y="-5.08" drill="1" diameter="1.8"/>
<pad name="4" x="0" y="-7.62" drill="1" diameter="1.8"/>
<wire x1="-1.27" y1="1.27" x2="1.27" y2="1.27" width="0.2032" layer="21"/>
<wire x1="1.27" y1="1.27" x2="1.27" y2="-8.89" width="0.2032" layer="21"/>
<wire x1="1.27" y1="-8.89" x2="-1.27" y2="-8.89" width="0.2032" layer="21"/>
<wire x1="-1.27" y1="-8.89" x2="-1.27" y2="1.27" width="0.2032" layer="21"/>
<text x="2" y="1" size="1.27" layer="25">&gt;NAME</text>
<text x="2" y="-0.905" size="1.27" layer="27">&gt;VALUE</text>
</package>
</packages>
<symbols>
<symbol name="NODEMCU">
<wire x1="-15.24" y1="20.32" x2="15.24" y2="20.32" width="0.4064" layer="94"/>
<wire x1="15.24" y1="20.32" x2="15.24" y2="-20.32" width="0.4064" layer="94"/>
<wire x1="15.24" y1="-20.32" x2="-15.24" y2="-20.32" width="0.4064" layer="94"/>
<wire x1="-15.24" y1="-20.32" x2="-15.24" y2="20.32" width="0.4064" layer="94"/>
<text x="0" y="21.59" size="1.778" layer="95" align="bottom-center">&gt;NAME</text>
<text x="0" y="-21.59" size="1.778" layer="96" align="top-center">&gt;VALUE</text>
<pin name="A0" x="-20.32" y="17.78" visible="pin" length="middle" direction="pas"/>
<pin name="G@1" x="-20.32" y="15.24" visible="pin" length="middle" direction="pas"/>
<pin name="VU" x="-20.32" y="12.7" visible="pin" length="middle" direction="pas"/>
<pin name="S3" x="-20.32" y="10.16" visible="pin" length="middle" direction="pas"/>
<pin name="S2" x="-20.32" y="7.62" visible="pin" length="middle" direction="pas"/>
<pin name="S1" x="-20.32" y="5.08" visible="pin" length="middle" direction="pas"/>
<pin name="SC" x="-20.32" y="2.54" visible="pin" length="middle" direction="pas"/>
<pin name="S0" x="-20.32" y="0" visible="pin" length="middle" direction="pas"/>
<pin name="SK" x="-20.32" y="-2.54" visible="pin" length="middle" direction="pas"/>
<pin name="G@2" x="-20.32" y="-5.08" visible="pin" length="middle" direction="pas"/>
<pin name="3V@1" x="-20.32" y="-7.62" visible="pin" length="middle" direction="pas"/>
<pin name="EN" x="-20.32" y="-10.16" visible="pin" length="middle" direction="pas"/>
<pin name="RST" x="-20.32" y="-12.7" visible="pin" length="middle" direction="pas"/>
<pin name="G@3" x="-20.32" y="-15.24" visible="pin" length="middle" direction="pas"/>
<pin name="VIN" x="-20.32" y="-17.78" visible="pin" length="middle" direction="pas"/>
<pin name="D0/GPIO16" x="20.32" y="17.78" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="D1/GPIO5" x="20.32" y="15.24" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="D2/GPIO4" x="20.32" y="12.7" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="D3/GPIO0" x="20.32" y="10.16" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="D4/GPIO2" x="20.32" y="7.62" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="3V@2" x="20.32" y="5.08" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="G@4" x="20.32" y="2.54" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="D5/GPIO14" x="20.32" y="0" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="D6/GPIO12" x="20.32" y="-2.54" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="D7/GPIO13" x="20.32" y="-5.08" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="D8/GPIO15" x="20.32" y="-7.62" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="RX" x="20.32" y="-10.16" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="TX" x="20.32" y="-12.7" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="G@5" x="20.32" y="-15.24" visible="pin" length="middle" direction="pas" rot="R180"/>
<pin name="3V@3" x="20.32" y="-17.78" visible="pin" length="middle" direction="pas" rot="R180"/>
</symbol>
<symbol name="RTC_DS3231">
<wire x1="-7.62" y1="6.35" x2="10.16" y2="6.35" width="0.4064" layer="94"/>
<wire x1="10.16" y1="6.35" x2="10.16" y2="-6.35" width="0.4064" layer="94"/>
<wire x1="10.16" y1="-6.35" x2="-7.62" y2="-6.35" width="0.4064" layer="94"/>
<wire x1="-7.62" y1="-6.35" x2="-7.62" y2="6.35" width="0.4064" layer="94"/>
<text x="-7.62" y="7.62" size="1.778" layer="95">&gt;NAME</text>
<text x="-7.62" y="-8.89" size="1.778" layer="96" align="top-left">&gt;VALUE</text>
<text x="5.08" y="2.54" size="1.778" layer="94" align="center">DS3231</text>
<text x="5.08" y="-1.27" size="1.27" layer="94" align="center">+ CR2032</text>
<pin name="SCL" x="-12.7" y="3.81" visible="pin" length="middle" direction="pas"/>
<pin name="SDA" x="-12.7" y="1.27" visible="pin" length="middle" direction="pas"/>
<pin name="VCC" x="-12.7" y="-1.27" visible="pin" length="middle" direction="pas"/>
<pin name="GND" x="-12.7" y="-3.81" visible="pin" length="middle" direction="pas"/>
</symbol>
<symbol name="RELE_BOBINA">
<wire x1="-3.81" y1="2.54" x2="3.81" y2="2.54" width="0.254" layer="94"/>
<wire x1="3.81" y1="2.54" x2="3.81" y2="-2.54" width="0.254" layer="94"/>
<wire x1="3.81" y1="-2.54" x2="-3.81" y2="-2.54" width="0.254" layer="94"/>
<wire x1="-3.81" y1="-2.54" x2="-3.81" y2="2.54" width="0.254" layer="94"/>
<wire x1="-3.81" y1="-2.54" x2="3.81" y2="2.54" width="0.254" layer="94"/>
<text x="5.08" y="1.27" size="1.778" layer="95">&gt;NAME</text>
<text x="5.08" y="-1.27" size="1.778" layer="96" align="top-left">&gt;VALUE</text>
<pin name="A1" x="0" y="5.08" visible="off" length="short" direction="pas" rot="R270"/>
<pin name="A2" x="0" y="-5.08" visible="off" length="short" direction="pas" rot="R90"/>
</symbol>
<symbol name="RELE_CONTATO">
<wire x1="-2.54" y1="0" x2="2.286" y2="-2.159" width="0.254" layer="94"/>
<circle x="-2.54" y="0" radius="0.3" width="0.254" layer="94"/>
<wire x1="2.54" y1="2.54" x2="2.54" y2="1.27" width="0.254" layer="94"/>
<wire x1="2.54" y1="-2.54" x2="2.54" y2="-1.778" width="0.254" layer="94"/>
<text x="-2.54" y="1.27" size="1.27" layer="94">COM</text>
<text x="3.302" y="2.794" size="1.27" layer="94">NA</text>
<text x="3.302" y="-4.064" size="1.27" layer="94">NF</text>
<text x="-2.54" y="-3.81" size="1.778" layer="95" align="top-left">&gt;NAME</text>
<pin name="COM" x="-5.08" y="0" visible="off" length="short" direction="pas"/>
<pin name="NO" x="5.08" y="2.54" visible="off" length="short" direction="pas" rot="R180"/>
<pin name="NC" x="5.08" y="-2.54" visible="off" length="short" direction="pas" rot="R180"/>
</symbol>
<symbol name="NPN">
<wire x1="-0.508" y1="2.032" x2="-0.508" y2="-2.032" width="0.508" layer="94"/>
<wire x1="-2.54" y1="0" x2="-0.508" y2="0" width="0.1524" layer="94"/>
<wire x1="-0.508" y1="0.762" x2="2.54" y2="2.54" width="0.1524" layer="94"/>
<wire x1="-0.508" y1="-0.762" x2="2.54" y2="-2.54" width="0.1524" layer="94"/>
<polygon width="0.1524" layer="94">
<vertex x="2.54" y="-2.54"/>
<vertex x="1.016" y="-2.286"/>
<vertex x="1.778" y="-1.27"/>
</polygon>
<circle x="0.635" y="0" radius="3.302" width="0.254" layer="94"/>
<text x="5.08" y="1.27" size="1.778" layer="95">&gt;NAME</text>
<text x="5.08" y="-1.27" size="1.778" layer="96" align="top-left">&gt;VALUE</text>
<pin name="B" x="-5.08" y="0" visible="off" length="short" direction="pas"/>
<pin name="C" x="2.54" y="5.08" visible="off" length="short" direction="pas" rot="R270"/>
<pin name="E" x="2.54" y="-5.08" visible="off" length="short" direction="pas" rot="R90"/>
</symbol>
<symbol name="RESISTOR">
<wire x1="-2.54" y1="1.016" x2="2.54" y2="1.016" width="0.254" layer="94"/>
<wire x1="2.54" y1="1.016" x2="2.54" y2="-1.016" width="0.254" layer="94"/>
<wire x1="2.54" y1="-1.016" x2="-2.54" y2="-1.016" width="0.254" layer="94"/>
<wire x1="-2.54" y1="-1.016" x2="-2.54" y2="1.016" width="0.254" layer="94"/>
<text x="0" y="1.524" size="1.778" layer="95" align="bottom-center">&gt;NAME</text>
<text x="0" y="-1.524" size="1.778" layer="96" align="top-center">&gt;VALUE</text>
<pin name="1" x="-5.08" y="0" visible="off" length="short" direction="pas"/>
<pin name="2" x="5.08" y="0" visible="off" length="short" direction="pas" rot="R180"/>
</symbol>
<symbol name="DIODO">
<wire x1="-2.54" y1="0" x2="-1.27" y2="0" width="0.1524" layer="94"/>
<polygon width="0.1524" layer="94">
<vertex x="-1.27" y="1.27"/>
<vertex x="-1.27" y="-1.27"/>
<vertex x="1.27" y="0"/>
</polygon>
<wire x1="1.27" y1="1.27" x2="1.27" y2="-1.27" width="0.254" layer="94"/>
<wire x1="1.27" y1="0" x2="2.54" y2="0" width="0.1524" layer="94"/>
<text x="-2.54" y="1.778" size="1.778" layer="95">&gt;NAME</text>
<text x="-2.54" y="-1.778" size="1.778" layer="96" align="top-left">&gt;VALUE</text>
<pin name="A" x="-5.08" y="0" visible="off" length="short" direction="pas"/>
<pin name="C" x="5.08" y="0" visible="off" length="short" direction="pas" rot="R180"/>
</symbol>
<symbol name="LED">
<polygon width="0.1524" layer="94">
<vertex x="-1.27" y="1.27"/>
<vertex x="-1.27" y="-1.27"/>
<vertex x="1.27" y="0"/>
</polygon>
<wire x1="1.27" y1="1.27" x2="1.27" y2="-1.27" width="0.254" layer="94"/>
<wire x1="-2.54" y1="0" x2="-1.27" y2="0" width="0.1524" layer="94"/>
<wire x1="1.27" y1="0" x2="2.54" y2="0" width="0.1524" layer="94"/>
<wire x1="0" y1="1.778" x2="1.016" y2="2.794" width="0.1524" layer="94"/>
<polygon width="0.1524" layer="94">
<vertex x="1.016" y="2.794"/>
<vertex x="0.508" y="2.54"/>
<vertex x="0.762" y="2.286"/>
</polygon>
<wire x1="1.27" y1="1.778" x2="2.286" y2="2.794" width="0.1524" layer="94"/>
<polygon width="0.1524" layer="94">
<vertex x="2.286" y="2.794"/>
<vertex x="1.778" y="2.54"/>
<vertex x="2.032" y="2.286"/>
</polygon>
<text x="-2.54" y="-1.778" size="1.778" layer="95" align="top-left">&gt;NAME</text>
<text x="-2.54" y="-3.81" size="1.778" layer="96" align="top-left">&gt;VALUE</text>
<pin name="A" x="-5.08" y="0" visible="off" length="short" direction="pas"/>
<pin name="C" x="5.08" y="0" visible="off" length="short" direction="pas" rot="R180"/>
</symbol>
<symbol name="CAPACITOR">
<wire x1="-0.635" y1="1.905" x2="-0.635" y2="-1.905" width="0.4064" layer="94"/>
<wire x1="0.635" y1="1.905" x2="0.635" y2="-1.905" width="0.4064" layer="94"/>
<wire x1="-2.54" y1="0" x2="-0.635" y2="0" width="0.1524" layer="94"/>
<wire x1="0.635" y1="0" x2="2.54" y2="0" width="0.1524" layer="94"/>
<text x="-2.54" y="2.286" size="1.778" layer="95">&gt;NAME</text>
<text x="-2.54" y="-2.286" size="1.778" layer="96" align="top-left">&gt;VALUE</text>
<pin name="1" x="-5.08" y="0" visible="off" length="short" direction="pas"/>
<pin name="2" x="5.08" y="0" visible="off" length="short" direction="pas" rot="R180"/>
</symbol>
<symbol name="CAP_POL">
<wire x1="-1.905" y1="0.635" x2="1.905" y2="0.635" width="0.4064" layer="94"/>
<wire x1="-1.905" y1="-0.635" x2="1.905" y2="-0.635" width="0.4064" layer="94"/>
<wire x1="-1.905" y1="-0.635" x2="-1.905" y2="-1.143" width="0.4064" layer="94"/>
<wire x1="1.905" y1="-0.635" x2="1.905" y2="-1.143" width="0.4064" layer="94"/>
<wire x1="0" y1="2.54" x2="0" y2="0.635" width="0.1524" layer="94"/>
<wire x1="0" y1="-0.635" x2="0" y2="-2.54" width="0.1524" layer="94"/>
<text x="-1.524" y="1.524" size="1.27" layer="94" align="bottom-right">+</text>
<text x="2.54" y="1.27" size="1.778" layer="95">&gt;NAME</text>
<text x="2.54" y="-1.27" size="1.778" layer="96" align="top-left">&gt;VALUE</text>
<pin name="+" x="0" y="5.08" visible="off" length="short" direction="pas" rot="R270"/>
<pin name="-" x="0" y="-5.08" visible="off" length="short" direction="pas" rot="R90"/>
</symbol>
<symbol name="BORNE_2">
<wire x1="-2.54" y1="3.81" x2="2.54" y2="3.81" width="0.254" layer="94"/>
<wire x1="2.54" y1="3.81" x2="2.54" y2="-1.27" width="0.254" layer="94"/>
<wire x1="2.54" y1="-1.27" x2="-2.54" y2="-1.27" width="0.254" layer="94"/>
<wire x1="-2.54" y1="-1.27" x2="-2.54" y2="3.81" width="0.254" layer="94"/>
<circle x="0" y="2.54" radius="0.762" width="0.254" layer="94"/>
<text x="-1.016" y="2.54" size="1.27" layer="94" align="center-right">1</text>
<circle x="0" y="0" radius="0.762" width="0.254" layer="94"/>
<text x="-1.016" y="0" size="1.27" layer="94" align="center-right">2</text>
<text x="-2.54" y="4.572" size="1.778" layer="95">&gt;NAME</text>
<text x="-2.54" y="-2.032" size="1.778" layer="96" align="top-left">&gt;VALUE</text>
<pin name="1" x="5.08" y="2.54" visible="off" length="short" direction="pas" rot="R180"/>
<pin name="2" x="5.08" y="0" visible="off" length="short" direction="pas" rot="R180"/>
</symbol>
<symbol name="FUSIVEL">
<wire x1="-2.54" y1="0.889" x2="2.54" y2="0.889" width="0.254" layer="94"/>
<wire x1="2.54" y1="0.889" x2="2.54" y2="-0.889" width="0.254" layer="94"/>
<wire x1="2.54" y1="-0.889" x2="-2.54" y2="-0.889" width="0.254" layer="94"/>
<wire x1="-2.54" y1="-0.889" x2="-2.54" y2="0.889" width="0.254" layer="94"/>
<wire x1="-2.54" y1="0" x2="2.54" y2="0" width="0.1524" layer="94"/>
<text x="-2.54" y="1.524" size="1.778" layer="95">&gt;NAME</text>
<text x="-2.54" y="-1.524" size="1.778" layer="96" align="top-left">&gt;VALUE</text>
<pin name="1" x="-5.08" y="0" visible="off" length="short" direction="pas"/>
<pin name="2" x="5.08" y="0" visible="off" length="short" direction="pas" rot="R180"/>
</symbol>
<symbol name="BOTAO">
<wire x1="-2.54" y1="0" x2="-1.778" y2="0" width="0.1524" layer="94"/>
<wire x1="1.778" y1="0" x2="2.54" y2="0" width="0.1524" layer="94"/>
<circle x="-1.524" y="0" radius="0.254" width="0.254" layer="94"/>
<circle x="1.524" y="0" radius="0.254" width="0.254" layer="94"/>
<wire x1="-1.905" y1="1.27" x2="1.905" y2="1.27" width="0.254" layer="94"/>
<wire x1="0" y1="1.27" x2="0" y2="2.54" width="0.254" layer="94"/>
<text x="-2.54" y="3.048" size="1.778" layer="95">&gt;NAME</text>
<text x="-2.54" y="-1.016" size="1.778" layer="96" align="top-left">&gt;VALUE</text>
<pin name="1" x="-5.08" y="0" visible="off" length="short" direction="pas"/>
<pin name="2" x="5.08" y="0" visible="off" length="short" direction="pas" rot="R180"/>
</symbol>
<symbol name="CONECTOR_GPS">
<wire x1="-2.54" y1="6.35" x2="7.62" y2="6.35" width="0.254" layer="94"/>
<wire x1="7.62" y1="6.35" x2="7.62" y2="-6.35" width="0.254" layer="94"/>
<wire x1="7.62" y1="-6.35" x2="-2.54" y2="-6.35" width="0.254" layer="94"/>
<wire x1="-2.54" y1="-6.35" x2="-2.54" y2="6.35" width="0.254" layer="94"/>
<text x="-1.524" y="3.81" size="1.27" layer="94" align="center-left">VCC</text>
<text x="-1.524" y="1.27" size="1.27" layer="94" align="center-left">RX</text>
<text x="-1.524" y="-1.27" size="1.27" layer="94" align="center-left">TX</text>
<text x="-1.524" y="-3.81" size="1.27" layer="94" align="center-left">GND</text>
<text x="-2.54" y="7.112" size="1.778" layer="95">&gt;NAME</text>
<text x="-2.54" y="-7.112" size="1.778" layer="96" align="top-left">&gt;VALUE</text>
<pin name="1" x="-7.62" y="3.81" visible="off" length="short" direction="pas"/>
<pin name="2" x="-7.62" y="1.27" visible="off" length="short" direction="pas"/>
<pin name="3" x="-7.62" y="-1.27" visible="off" length="short" direction="pas"/>
<pin name="4" x="-7.62" y="-3.81" visible="off" length="short" direction="pas"/>
</symbol>
<symbol name="GND">
<wire x1="0" y1="0" x2="0" y2="-1.27" width="0.254" layer="94"/>
<wire x1="-1.905" y1="-1.27" x2="1.905" y2="-1.27" width="0.254" layer="94"/>
<wire x1="-1.27" y1="-1.905" x2="1.27" y2="-1.905" width="0.254" layer="94"/>
<wire x1="-0.635" y1="-2.54" x2="0.635" y2="-2.54" width="0.254" layer="94"/>
<pin name="GND" x="0" y="0" visible="off" length="point" direction="sup"/>
</symbol>
<symbol name="+5V">
<wire x1="0" y1="0" x2="0" y2="1.27" width="0.254" layer="94"/>
<wire x1="-1.27" y1="1.27" x2="1.27" y2="1.27" width="0.254" layer="94"/>
<text x="0" y="1.778" size="1.778" layer="96" align="bottom-center">&gt;VALUE</text>
<pin name="+5V" x="0" y="0" visible="off" length="point" direction="sup"/>
</symbol>
<symbol name="+3V3">
<wire x1="0" y1="0" x2="0" y2="1.27" width="0.254" layer="94"/>
<wire x1="-1.27" y1="1.27" x2="1.27" y2="1.27" width="0.254" layer="94"/>
<text x="0" y="1.778" size="1.778" layer="96" align="bottom-center">&gt;VALUE</text>
<pin name="+3V3" x="0" y="0" visible="off" length="point" direction="sup"/>
</symbol>
</symbols>
<devicesets>
<deviceset name="NODEMCU" prefix="U" uservalue="yes">
<description>NodeMCU v3 LoLin (ESP-12E / ESP8266)</description>
<gates>
<gate name="G$1" symbol="NODEMCU" x="0" y="0"/>
</gates>
<devices>
<device name="" package="NODEMCU-V3">
<connects>
<connect gate="G$1" pin="A0" pad="1"/>
<connect gate="G$1" pin="G@1" pad="2"/>
<connect gate="G$1" pin="VU" pad="3"/>
<connect gate="G$1" pin="S3" pad="4"/>
<connect gate="G$1" pin="S2" pad="5"/>
<connect gate="G$1" pin="S1" pad="6"/>
<connect gate="G$1" pin="SC" pad="7"/>
<connect gate="G$1" pin="S0" pad="8"/>
<connect gate="G$1" pin="SK" pad="9"/>
<connect gate="G$1" pin="G@2" pad="10"/>
<connect gate="G$1" pin="3V@1" pad="11"/>
<connect gate="G$1" pin="EN" pad="12"/>
<connect gate="G$1" pin="RST" pad="13"/>
<connect gate="G$1" pin="G@3" pad="14"/>
<connect gate="G$1" pin="VIN" pad="15"/>
<connect gate="G$1" pin="D0/GPIO16" pad="16"/>
<connect gate="G$1" pin="D1/GPIO5" pad="17"/>
<connect gate="G$1" pin="D2/GPIO4" pad="18"/>
<connect gate="G$1" pin="D3/GPIO0" pad="19"/>
<connect gate="G$1" pin="D4/GPIO2" pad="20"/>
<connect gate="G$1" pin="3V@2" pad="21"/>
<connect gate="G$1" pin="G@4" pad="22"/>
<connect gate="G$1" pin="D5/GPIO14" pad="23"/>
<connect gate="G$1" pin="D6/GPIO12" pad="24"/>
<connect gate="G$1" pin="D7/GPIO13" pad="25"/>
<connect gate="G$1" pin="D8/GPIO15" pad="26"/>
<connect gate="G$1" pin="RX" pad="27"/>
<connect gate="G$1" pin="TX" pad="28"/>
<connect gate="G$1" pin="G@5" pad="29"/>
<connect gate="G$1" pin="3V@3" pad="30"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="RTC_DS3231" prefix="U" uservalue="yes">
<description>Modulo RTC DS3231 (ZS-042), lado de 4 pinos: SCL, SDA, VCC, GND</description>
<gates>
<gate name="G$1" symbol="RTC_DS3231" x="0" y="0"/>
</gates>
<devices>
<device name="" package="MOD-4P">
<connects>
<connect gate="G$1" pin="SCL" pad="1"/>
<connect gate="G$1" pin="SDA" pad="2"/>
<connect gate="G$1" pin="VCC" pad="3"/>
<connect gate="G$1" pin="GND" pad="4"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="RELE_5V" prefix="K" uservalue="yes">
<description>Rele 5 V SPDT (SRD-05VDC-SL-C): bobina 2-5, COM 1, NA 3, NF 4</description>
<gates>
<gate name="BOBINA" symbol="RELE_BOBINA" x="0" y="0"/>
<gate name="CONTATO" symbol="RELE_CONTATO" x="0" y="15.24"/>
</gates>
<devices>
<device name="" package="RELE-SRD">
<connects>
<connect gate="BOBINA" pin="A1" pad="5"/>
<connect gate="BOBINA" pin="A2" pad="2"/>
<connect gate="CONTATO" pin="COM" pad="1"/>
<connect gate="CONTATO" pin="NO" pad="3"/>
<connect gate="CONTATO" pin="NC" pad="4"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="BC548" prefix="Q" uservalue="yes">
<description>Transistor NPN BC548 (ou BC337, mesma pinagem C-B-E)</description>
<gates>
<gate name="G$1" symbol="NPN" x="0" y="0"/>
</gates>
<devices>
<device name="" package="TO92-CBE">
<connects>
<connect gate="G$1" pin="B" pad="B"/>
<connect gate="G$1" pin="C" pad="C"/>
<connect gate="G$1" pin="E" pad="E"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="RESISTOR" prefix="R" uservalue="yes">
<description>Resistor axial</description>
<gates>
<gate name="G$1" symbol="RESISTOR" x="0" y="0"/>
</gates>
<devices>
<device name="-1/4W" package="R-0207/10">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
<device name="-1/2W" package="R-0309/12">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="DIODO" prefix="D" uservalue="yes">
<description>Diodo retificador</description>
<gates>
<gate name="G$1" symbol="DIODO" x="0" y="0"/>
</gates>
<devices>
<device name="" package="DO41-10">
<connects>
<connect gate="G$1" pin="A" pad="A"/>
<connect gate="G$1" pin="C" pad="C"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="LED" prefix="LED" uservalue="yes">
<description>LED 3 mm</description>
<gates>
<gate name="G$1" symbol="LED" x="0" y="0"/>
</gates>
<devices>
<device name="" package="LED3MM">
<connects>
<connect gate="G$1" pin="A" pad="A"/>
<connect gate="G$1" pin="C" pad="C"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="CAPACITOR" prefix="C" uservalue="yes">
<description>Capacitor sem polaridade</description>
<gates>
<gate name="G$1" symbol="CAPACITOR" x="0" y="0"/>
</gates>
<devices>
<device name="-CERAMICO" package="C-2.54">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
<device name="-X2" package="C-X2-15">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="CAP_ELETROLITICO" prefix="C" uservalue="yes">
<description>Capacitor eletrolitico</description>
<gates>
<gate name="G$1" symbol="CAP_POL" x="0" y="0"/>
</gates>
<devices>
<device name="" package="CPOL-8MM">
<connects>
<connect gate="G$1" pin="+" pad="+"/>
<connect gate="G$1" pin="-" pad="-"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="BORNE_2" prefix="J" uservalue="yes">
<description>Borne de parafuso 2 vias</description>
<gates>
<gate name="G$1" symbol="BORNE_2" x="0" y="0"/>
</gates>
<devices>
<device name="" package="BORNE-2-5.08">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="FUSIVEL" prefix="F" uservalue="yes">
<description>Fusivel 5x20 mm</description>
<gates>
<gate name="G$1" symbol="FUSIVEL" x="0" y="0"/>
</gates>
<devices>
<device name="" package="FUSIVEL-5X20">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="BOTAO" prefix="S" uservalue="yes">
<description>Botao tactil</description>
<gates>
<gate name="G$1" symbol="BOTAO" x="0" y="0"/>
</gates>
<devices>
<device name="" package="BOTAO-6X6">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="4"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="CONECTOR_GPS" prefix="J" uservalue="yes">
<description>Conector do GPS (opcional)</description>
<gates>
<gate name="G$1" symbol="CONECTOR_GPS" x="0" y="0"/>
</gates>
<devices>
<device name="" package="HDR-1X4">
<connects>
<connect gate="G$1" pin="1" pad="1"/>
<connect gate="G$1" pin="2" pad="2"/>
<connect gate="G$1" pin="3" pad="3"/>
<connect gate="G$1" pin="4" pad="4"/>
</connects>
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="GND" prefix="GND">
<description>Terra (0 V)</description>
<gates>
<gate name="1" symbol="GND" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="+5V" prefix="P">
<description>Alimentacao +5V</description>
<gates>
<gate name="1" symbol="+5V" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
<deviceset name="+3V3" prefix="P">
<description>Alimentacao +3V3</description>
<gates>
<gate name="1" symbol="+3V3" x="0" y="0"/>
</gates>
<devices>
<device name="">
<technologies>
<technology name=""/>
</technologies>
</device>
</devices>
</deviceset>
</devicesets>
</library>
</libraries>
<attributes>
</attributes>
<variantdefs>
</variantdefs>
<classes>
<class number="0" name="default" width="0" drill="0">
</class>
</classes>
<parts>
<part name="U1" library="sino_ifms" deviceset="NODEMCU" device="" value="NodeMCU v3 LoLin"/>
<part name="U2" library="sino_ifms" deviceset="RTC_DS3231" device="" value="DS3231 ZS-042"/>
<part name="K1" library="sino_ifms" deviceset="RELE_5V" device="" value="SRD-05VDC-SL-C"/>
<part name="Q1" library="sino_ifms" deviceset="BC548" device="" value="BC548"/>
<part name="R1" library="sino_ifms" deviceset="RESISTOR" device="-1/4W" value="1k"/>
<part name="R2" library="sino_ifms" deviceset="RESISTOR" device="-1/4W" value="10k"/>
<part name="R3" library="sino_ifms" deviceset="RESISTOR" device="-1/4W" value="1k"/>
<part name="R4" library="sino_ifms" deviceset="RESISTOR" device="-1/2W" value="100R 1/2W"/>
<part name="D1" library="sino_ifms" deviceset="DIODO" device="" value="1N4007"/>
<part name="LED1" library="sino_ifms" deviceset="LED" device="" value="RELE LIGADO"/>
<part name="C1" library="sino_ifms" deviceset="CAP_ELETROLITICO" device="" value="470uF 16V"/>
<part name="C2" library="sino_ifms" deviceset="CAPACITOR" device="-CERAMICO" value="100nF"/>
<part name="C3" library="sino_ifms" deviceset="CAPACITOR" device="-X2" value="100nF X2 275VAC"/>
<part name="J1" library="sino_ifms" deviceset="BORNE_2" device="" value="ENTRADA 5V"/>
<part name="J2" library="sino_ifms" deviceset="BORNE_2" device="" value="127V FASE/SINO"/>
<part name="F1" library="sino_ifms" deviceset="FUSIVEL" device="" value="2A"/>
<part name="S1" library="sino_ifms" deviceset="BOTAO" device="" value="TESTE"/>
<part name="J3" library="sino_ifms" deviceset="CONECTOR_GPS" device="" value="GPS (opcional)"/>
<part name="P1" library="sino_ifms" deviceset="+5V" device="" value="+5V"/>
<part name="GND2" library="sino_ifms" deviceset="GND" device=""/>
<part name="P3" library="sino_ifms" deviceset="+5V" device="" value="+5V"/>
<part name="GND4" library="sino_ifms" deviceset="GND" device=""/>
<part name="GND5" library="sino_ifms" deviceset="GND" device=""/>
<part name="GND6" library="sino_ifms" deviceset="GND" device=""/>
<part name="R5" library="sino_ifms" deviceset="RESISTOR" device="-1/4W" value="330"/>
<part name="R6" library="sino_ifms" deviceset="RESISTOR" device="-1/4W" value="330"/>
<part name="LED2" library="sino_ifms" deviceset="LED" device="" value="VERDE"/>
<part name="LED3" library="sino_ifms" deviceset="LED" device="" value="VERMELHO"/>
<part name="GND7" library="sino_ifms" deviceset="GND" device=""/>
<part name="GND8" library="sino_ifms" deviceset="GND" device=""/>
</parts>
<sheets>
<sheet>
<plain>
<wire x1="0" y1="0" x2="279.4" y2="0" width="0.4064" layer="94"/>
<wire x1="279.4" y1="0" x2="279.4" y2="190.5" width="0.4064" layer="94"/>
<wire x1="279.4" y1="190.5" x2="0" y2="190.5" width="0.4064" layer="94"/>
<wire x1="0" y1="190.5" x2="0" y2="0" width="0.4064" layer="94"/>
<text x="12.7" y="177.8" size="2.1844" layer="97">ALIMENTACAO 5V</text>
<text x="73.66" y="139.7" size="2.1844" layer="97">NODEMCU V3 (LOLIN)</text>
<text x="182.88" y="175.26" size="2.1844" layer="97">RELOGIO DS3231 - LADO DE 4 PINOS</text>
<text x="236.22" y="175.26" size="2.1844" layer="97">LEDS DE ESTADO</text>
<text x="182.88" y="147.32" size="2.1844" layer="97">GPS (OPCIONAL)</text>
<text x="213.36" y="142.24" size="2.1844" layer="97">ACIONAMENTO DO RELE</text>
<text x="160.02" y="106.68" size="2.1844" layer="97">BOTAO DE TESTE</text>
<text x="167.64" y="71.12" size="2.1844" layer="97">SAIDA 127V PARA O SINO</text>
<text x="241.3" y="162.56" size="1.778" layer="97">sistema OK</text>
<text x="241.3" y="147.32" size="1.778" layer="97">perda de rede</text>
<text x="7.62" y="72.39" size="2.1844" layer="97">NOTAS</text>
<text x="7.62" y="68.58" size="1.778" layer="97">1. Fonte 5V (ex.: HLK-PM01) em J1: alimenta o rele e o NodeMCU pelo pino VIN.</text>
<text x="7.62" y="64.77" size="1.778" layer="97">2. O 3V do NodeMCU alimenta o RTC (bateria CR2032). VU = 5V do USB: nao ligar.</text>
<text x="7.62" y="60.96" size="1.778" layer="97">3. Q1 liga o rele com D6/GPIO12 em nivel ALTO: no firmware, RELE_ATIVO_EM_BAIXO = false.</text>
<text x="7.62" y="57.15" size="1.778" layer="97">4. R2 mantem o rele desligado enquanto o ESP liga ou e gravado.</text>
<text x="7.62" y="53.34" size="1.778" layer="97">5. D1 = diodo de roda livre (catodo no +5V). Rele com mais de 90 mA: troque Q1 por BC337.</text>
<text x="7.62" y="49.53" size="1.778" layer="97">6. RTC: lado de 4 pinos do ZS-042, na ordem impressa no modulo: SCL, SDA, VCC, GND.</text>
<text x="7.62" y="45.72" size="1.778" layer="97">7. Hora certa pela rede (NTP); o DS3231 segura a hora quando a rede cai. J3 (GPS) e opcional.</text>
<text x="7.62" y="41.91" size="1.778" layer="97">8. 127V: fase -&gt; J2.1 -&gt; F1 -&gt; COM; NA -&gt; J2.2 -&gt; sino; neutro direto no sino.</text>
<text x="7.62" y="38.1" size="1.778" layer="97">9. Snubber R4+C3 opcional (protege o contato e evita reset do ESP).</text>
<text x="7.62" y="34.29" size="1.778" layer="97">10. Na placa: deixar 6 mm ou mais entre a parte de 127V e o resto do circuito.</text>
<text x="7.62" y="30.48" size="1.778" layer="97">11. LED2 verde (D0) = sistema OK; LED3 vermelho (D8) = perda de rede (piscando = sem hora).</text>
<wire x1="182.88" y1="22.86" x2="279.4" y2="22.86" width="0.254" layer="94"/>
<wire x1="182.88" y1="0" x2="182.88" y2="22.86" width="0.254" layer="94"/>
<wire x1="182.88" y1="12.7" x2="279.4" y2="12.7" width="0.254" layer="94"/>
<text x="185.42" y="16.51" size="2.54" layer="94">SINO IFMS - CONTROLADOR DO SINO</text>
<text x="185.42" y="6.35" size="1.778" layer="94">NodeMCU v3 + DS3231 + rele 5V   Rev. 4   2026-11-23</text>
<text x="185.42" y="2.54" size="1.778" layer="94">Firmware: sino-gps/sino_gps</text>
</plain>
<instances>
<instance part="J1" gate="G$1" x="25.4" y="160.02"/>
<instance part="C1" gate="G$1" x="50.8" y="157.48"/>
<instance part="C2" gate="G$1" x="68.58" y="157.48" rot="R90"/>
<instance part="U1" gate="G$1" x="104.14" y="114.3"/>
<instance part="U2" gate="G$1" x="198.12" y="162.56"/>
<instance part="J3" gate="G$1" x="198.12" y="134.62"/>
<instance part="K1" gate="BOBINA" x="236.22" y="116.84"/>
<instance part="D1" gate="G$1" x="226.06" y="116.84" rot="R90"/>
<instance part="R3" gate="G$1" x="215.9" y="124.46" rot="R90"/>
<instance part="LED1" gate="G$1" x="215.9" y="111.76" rot="R270"/>
<instance part="Q1" gate="G$1" x="233.68" y="96.52"/>
<instance part="R1" gate="G$1" x="215.9" y="96.52"/>
<instance part="R2" gate="G$1" x="226.06" y="86.36" rot="R90"/>
<instance part="S1" gate="G$1" x="172.72" y="96.52"/>
<instance part="K1" gate="CONTATO" x="233.68" y="50.8"/>
<instance part="F1" gate="G$1" x="208.28" y="50.8"/>
<instance part="R4" gate="G$1" x="228.6" y="63.5"/>
<instance part="C3" gate="G$1" x="243.84" y="63.5"/>
<instance part="J2" gate="G$1" x="172.72" y="45.72"/>
<instance part="P1" gate="1" x="81.28" y="170.18"/>
<instance part="GND2" gate="1" x="81.28" y="144.78"/>
<instance part="P3" gate="1" x="215.9" y="134.62"/>
<instance part="GND4" gate="1" x="226.06" y="78.74"/>
<instance part="GND5" gate="1" x="236.22" y="86.36"/>
<instance part="GND6" gate="1" x="182.88" y="91.44"/>
<instance part="R5" gate="G$1" x="246.38" y="170.18"/>
<instance part="LED2" gate="G$1" x="261.62" y="170.18"/>
<instance part="R6" gate="G$1" x="246.38" y="154.94"/>
<instance part="LED3" gate="G$1" x="261.62" y="154.94"/>
<instance part="GND7" gate="1" x="271.78" y="165.1"/>
<instance part="GND8" gate="1" x="271.78" y="144.78"/>
</instances>
<busses>
</busses>
<nets>
<net name="+5V" class="0">
<segment>
<pinref part="J1" gate="G$1" pin="1"/>
<wire x1="30.48" y1="162.56" x2="38.1" y2="162.56" width="0.1524" layer="91"/>
<wire x1="38.1" y1="162.56" x2="38.1" y2="170.18" width="0.1524" layer="91"/>
</segment>
<segment>
<wire x1="38.1" y1="170.18" x2="50.8" y2="170.18" width="0.1524" layer="91"/>
<wire x1="50.8" y1="170.18" x2="68.58" y2="170.18" width="0.1524" layer="91"/>
<wire x1="68.58" y1="170.18" x2="81.28" y2="170.18" width="0.1524" layer="91"/>
<junction x="50.8" y="170.18"/>
<junction x="68.58" y="170.18"/>
<pinref part="P1" gate="1" pin="+5V"/>
</segment>
<segment>
<pinref part="C1" gate="G$1" pin="+"/>
<wire x1="50.8" y1="162.56" x2="50.8" y2="170.18" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="C2" gate="G$1" pin="2"/>
<wire x1="68.58" y1="162.56" x2="68.58" y2="170.18" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U1" gate="G$1" pin="VIN"/>
<wire x1="83.82" y1="96.52" x2="76.2" y2="96.52" width="0.1524" layer="91"/>
<label x="76.2" y="96.52" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J3" gate="G$1" pin="1"/>
<wire x1="190.5" y1="138.43" x2="182.88" y2="138.43" width="0.1524" layer="91"/>
<label x="182.88" y="138.43" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="R3" gate="G$1" pin="2"/>
<wire x1="215.9" y1="129.54" x2="215.9" y2="132.08" width="0.1524" layer="91"/>
</segment>
<segment>
<wire x1="215.9" y1="132.08" x2="226.06" y2="132.08" width="0.1524" layer="91"/>
<wire x1="226.06" y1="132.08" x2="236.22" y2="132.08" width="0.1524" layer="91"/>
<junction x="226.06" y="132.08"/>
</segment>
<segment>
<wire x1="215.9" y1="132.08" x2="215.9" y2="134.62" width="0.1524" layer="91"/>
<pinref part="P3" gate="1" pin="+5V"/>
</segment>
<segment>
<pinref part="D1" gate="G$1" pin="C"/>
<wire x1="226.06" y1="121.92" x2="226.06" y2="132.08" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="K1" gate="BOBINA" pin="A1"/>
<wire x1="236.22" y1="121.92" x2="236.22" y2="132.08" width="0.1524" layer="91"/>
</segment>
</net>
<net name="GND" class="0">
<segment>
<pinref part="J1" gate="G$1" pin="2"/>
<wire x1="30.48" y1="160.02" x2="35.56" y2="160.02" width="0.1524" layer="91"/>
<wire x1="35.56" y1="160.02" x2="35.56" y2="144.78" width="0.1524" layer="91"/>
</segment>
<segment>
<wire x1="35.56" y1="144.78" x2="50.8" y2="144.78" width="0.1524" layer="91"/>
<wire x1="50.8" y1="144.78" x2="68.58" y2="144.78" width="0.1524" layer="91"/>
<wire x1="68.58" y1="144.78" x2="81.28" y2="144.78" width="0.1524" layer="91"/>
<junction x="50.8" y="144.78"/>
<junction x="68.58" y="144.78"/>
<pinref part="GND2" gate="1" pin="GND"/>
</segment>
<segment>
<pinref part="C1" gate="G$1" pin="-"/>
<wire x1="50.8" y1="152.4" x2="50.8" y2="144.78" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="C2" gate="G$1" pin="1"/>
<wire x1="68.58" y1="152.4" x2="68.58" y2="144.78" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="U1" gate="G$1" pin="G@3"/>
<wire x1="83.82" y1="99.06" x2="76.2" y2="99.06" width="0.1524" layer="91"/>
<label x="76.2" y="99.06" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="U1" gate="G$1" pin="G@5"/>
<wire x1="124.46" y1="99.06" x2="132.08" y2="99.06" width="0.1524" layer="91"/>
<label x="132.08" y="99.06" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U2" gate="G$1" pin="GND"/>
<wire x1="185.42" y1="158.75" x2="177.8" y2="158.75" width="0.1524" layer="91"/>
<label x="177.8" y="158.75" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="J3" gate="G$1" pin="4"/>
<wire x1="190.5" y1="130.81" x2="182.88" y2="130.81" width="0.1524" layer="91"/>
<label x="182.88" y="130.81" size="1.778" layer="95" rot="R180"/>
</segment>
<segment>
<pinref part="R2" gate="G$1" pin="1"/>
<wire x1="226.06" y1="81.28" x2="226.06" y2="78.74" width="0.1524" layer="91"/>
<pinref part="GND4" gate="1" pin="GND"/>
</segment>
<segment>
<pinref part="Q1" gate="G$1" pin="E"/>
<wire x1="236.22" y1="91.44" x2="236.22" y2="86.36" width="0.1524" layer="91"/>
<pinref part="GND5" gate="1" pin="GND"/>
</segment>
<segment>
<pinref part="S1" gate="G$1" pin="2"/>
<wire x1="177.8" y1="96.52" x2="182.88" y2="96.52" width="0.1524" layer="91"/>
<wire x1="182.88" y1="96.52" x2="182.88" y2="91.44" width="0.1524" layer="91"/>
<pinref part="GND6" gate="1" pin="GND"/>
</segment>
<segment>
<pinref part="LED2" gate="G$1" pin="C"/>
<wire x1="266.7" y1="170.18" x2="271.78" y2="170.18" width="0.1524" layer="91"/>
<wire x1="271.78" y1="170.18" x2="271.78" y2="165.1" width="0.1524" layer="91"/>
<pinref part="GND7" gate="1" pin="GND"/>
</segment>
<segment>
<pinref part="LED3" gate="G$1" pin="C"/>
<wire x1="266.7" y1="154.94" x2="271.78" y2="154.94" width="0.1524" layer="91"/>
<wire x1="271.78" y1="154.94" x2="271.78" y2="144.78" width="0.1524" layer="91"/>
<pinref part="GND8" gate="1" pin="GND"/>
</segment>
</net>
<net name="+3V3" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="3V@3"/>
<wire x1="124.46" y1="96.52" x2="132.08" y2="96.52" width="0.1524" layer="91"/>
<label x="132.08" y="96.52" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U2" gate="G$1" pin="VCC"/>
<wire x1="185.42" y1="161.29" x2="177.8" y2="161.29" width="0.1524" layer="91"/>
<label x="177.8" y="161.29" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="SCL" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="D1/GPIO5"/>
<wire x1="124.46" y1="129.54" x2="132.08" y2="129.54" width="0.1524" layer="91"/>
<label x="132.08" y="129.54" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U2" gate="G$1" pin="SCL"/>
<wire x1="185.42" y1="166.37" x2="177.8" y2="166.37" width="0.1524" layer="91"/>
<label x="177.8" y="166.37" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="SDA" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="D2/GPIO4"/>
<wire x1="124.46" y1="127" x2="132.08" y2="127" width="0.1524" layer="91"/>
<label x="132.08" y="127" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="U2" gate="G$1" pin="SDA"/>
<wire x1="185.42" y1="163.83" x2="177.8" y2="163.83" width="0.1524" layer="91"/>
<label x="177.8" y="163.83" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="GPS_TX" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="D5/GPIO14"/>
<wire x1="124.46" y1="114.3" x2="132.08" y2="114.3" width="0.1524" layer="91"/>
<label x="132.08" y="114.3" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="J3" gate="G$1" pin="3"/>
<wire x1="190.5" y1="133.35" x2="182.88" y2="133.35" width="0.1524" layer="91"/>
<label x="182.88" y="133.35" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="RELE" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="D6/GPIO12"/>
<wire x1="124.46" y1="111.76" x2="132.08" y2="111.76" width="0.1524" layer="91"/>
<label x="132.08" y="111.76" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="R1" gate="G$1" pin="1"/>
<wire x1="210.82" y1="96.52" x2="203.2" y2="96.52" width="0.1524" layer="91"/>
<label x="203.2" y="96.52" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="BOTAO" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="D7/GPIO13"/>
<wire x1="124.46" y1="109.22" x2="132.08" y2="109.22" width="0.1524" layer="91"/>
<label x="132.08" y="109.22" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="S1" gate="G$1" pin="1"/>
<wire x1="167.64" y1="96.52" x2="160.02" y2="96.52" width="0.1524" layer="91"/>
<label x="160.02" y="96.52" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="LED_VERDE" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="D0/GPIO16"/>
<wire x1="124.46" y1="132.08" x2="132.08" y2="132.08" width="0.1524" layer="91"/>
<label x="132.08" y="132.08" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="R5" gate="G$1" pin="1"/>
<wire x1="241.3" y1="170.18" x2="233.68" y2="170.18" width="0.1524" layer="91"/>
<label x="233.68" y="170.18" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="LED_VERMELHO" class="0">
<segment>
<pinref part="U1" gate="G$1" pin="D8/GPIO15"/>
<wire x1="124.46" y1="106.68" x2="132.08" y2="106.68" width="0.1524" layer="91"/>
<label x="132.08" y="106.68" size="1.778" layer="95"/>
</segment>
<segment>
<pinref part="R6" gate="G$1" pin="1"/>
<wire x1="241.3" y1="154.94" x2="233.68" y2="154.94" width="0.1524" layer="91"/>
<label x="233.68" y="154.94" size="1.778" layer="95" rot="R180"/>
</segment>
</net>
<net name="COLETOR_Q1" class="0">
<segment>
<pinref part="LED1" gate="G$1" pin="C"/>
<wire x1="215.9" y1="106.68" x2="215.9" y2="104.14" width="0.1524" layer="91"/>
<wire x1="215.9" y1="104.14" x2="226.06" y2="104.14" width="0.1524" layer="91"/>
<wire x1="226.06" y1="104.14" x2="236.22" y2="104.14" width="0.1524" layer="91"/>
<junction x="226.06" y="104.14"/>
<junction x="236.22" y="104.14"/>
</segment>
<segment>
<pinref part="D1" gate="G$1" pin="A"/>
<wire x1="226.06" y1="111.76" x2="226.06" y2="104.14" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="K1" gate="BOBINA" pin="A2"/>
<wire x1="236.22" y1="111.76" x2="236.22" y2="104.14" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="Q1" gate="G$1" pin="C"/>
<wire x1="236.22" y1="101.6" x2="236.22" y2="104.14" width="0.1524" layer="91"/>
</segment>
</net>
<net name="LED_ANODO" class="0">
<segment>
<pinref part="R3" gate="G$1" pin="1"/>
<pinref part="LED1" gate="G$1" pin="A"/>
<wire x1="215.9" y1="119.38" x2="215.9" y2="116.84" width="0.1524" layer="91"/>
</segment>
</net>
<net name="BASE_Q1" class="0">
<segment>
<pinref part="R1" gate="G$1" pin="2"/>
<pinref part="Q1" gate="G$1" pin="B"/>
<wire x1="220.98" y1="96.52" x2="226.06" y2="96.52" width="0.1524" layer="91"/>
<wire x1="226.06" y1="96.52" x2="228.6" y2="96.52" width="0.1524" layer="91"/>
<junction x="226.06" y="96.52"/>
</segment>
<segment>
<pinref part="R2" gate="G$1" pin="2"/>
<wire x1="226.06" y1="91.44" x2="226.06" y2="96.52" width="0.1524" layer="91"/>
</segment>
</net>
<net name="FASE_127V" class="0">
<segment>
<pinref part="J2" gate="G$1" pin="1"/>
<pinref part="F1" gate="G$1" pin="1"/>
<wire x1="177.8" y1="48.26" x2="190.5" y2="48.26" width="0.1524" layer="91"/>
<wire x1="190.5" y1="48.26" x2="190.5" y2="50.8" width="0.1524" layer="91"/>
<wire x1="190.5" y1="50.8" x2="203.2" y2="50.8" width="0.1524" layer="91"/>
</segment>
</net>
<net name="COM" class="0">
<segment>
<pinref part="F1" gate="G$1" pin="2"/>
<wire x1="213.36" y1="50.8" x2="218.44" y2="50.8" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="K1" gate="CONTATO" pin="COM"/>
<wire x1="218.44" y1="50.8" x2="228.6" y2="50.8" width="0.1524" layer="91"/>
<junction x="218.44" y="50.8"/>
</segment>
<segment>
<pinref part="R4" gate="G$1" pin="1"/>
<wire x1="218.44" y1="50.8" x2="218.44" y2="63.5" width="0.1524" layer="91"/>
<wire x1="218.44" y1="63.5" x2="223.52" y2="63.5" width="0.1524" layer="91"/>
</segment>
</net>
<net name="SNUBBER" class="0">
<segment>
<pinref part="R4" gate="G$1" pin="2"/>
<pinref part="C3" gate="G$1" pin="1"/>
<wire x1="233.68" y1="63.5" x2="238.76" y2="63.5" width="0.1524" layer="91"/>
</segment>
</net>
<net name="SINO" class="0">
<segment>
<pinref part="K1" gate="CONTATO" pin="NO"/>
<wire x1="238.76" y1="53.34" x2="251.46" y2="53.34" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="C3" gate="G$1" pin="2"/>
<wire x1="248.92" y1="63.5" x2="251.46" y2="63.5" width="0.1524" layer="91"/>
<wire x1="251.46" y1="63.5" x2="251.46" y2="53.34" width="0.1524" layer="91"/>
</segment>
<segment>
<pinref part="J2" gate="G$1" pin="2"/>
<wire x1="251.46" y1="53.34" x2="251.46" y2="38.1" width="0.1524" layer="91"/>
<wire x1="251.46" y1="38.1" x2="182.88" y2="38.1" width="0.1524" layer="91"/>
<wire x1="182.88" y1="38.1" x2="182.88" y2="45.72" width="0.1524" layer="91"/>
<wire x1="182.88" y1="45.72" x2="177.8" y2="45.72" width="0.1524" layer="91"/>
<junction x="251.46" y="53.34"/>
</segment>
</net>
<net name="LED2_ANODO" class="0">
<segment>
<pinref part="R5" gate="G$1" pin="2"/>
<pinref part="LED2" gate="G$1" pin="A"/>
<wire x1="251.46" y1="170.18" x2="256.54" y2="170.18" width="0.1524" layer="91"/>
</segment>
</net>
<net name="LED3_ANODO" class="0">
<segment>
<pinref part="R6" gate="G$1" pin="2"/>
<pinref part="LED3" gate="G$1" pin="A"/>
<wire x1="251.46" y1="154.94" x2="256.54" y2="154.94" width="0.1524" layer="91"/>
</segment>
</net>
</nets>
</sheet>
</sheets>
</schematic>
</drawing>
</eagle>
