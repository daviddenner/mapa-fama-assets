#!/usr/bin/env python3
"""Gera o esquema eletrônico do sino (Eagle .sch) e uma prévia em SVG.

    python3 gerar_esquema.py            -> Sino_IFMS_v2.sch e esquema.svg nesta pasta

O esquema é montado aqui, peça por peça, com uma biblioteca própria embutida
no arquivo (não depende de bibliotecas instaladas no Eagle). Antes de gravar,
o script confere se cada fio desenhado encosta nos pinos da sua ligação e se
fios de ligações diferentes não se tocam.
"""
import math
import os
import sys
from xml.sax.saxutils import escape

G = 2.54  # grade de 0,1"

# ============================================================== utilidades


def f(v):
    s = ("%.4f" % v).rstrip("0").rstrip(".")
    return "0" if s in ("-0", "") else s


def gira(x, y, rot):
    r = rot % 360
    if r == 0:
        return x, y
    if r == 90:
        return -y, x
    if r == 180:
        return -x, -y
    return y, -x


def q(v):  # arredonda para comparar posições
    return round(v, 3)


# ============================================================== biblioteca
class Simbolo:
    def __init__(self, nome):
        self.nome = nome
        self.itens = []  # (tipo, dados)
        self.pinos = {}  # nome -> (x, y, comprimento, rot, visivel, direcao)

    def fio(self, x1, y1, x2, y2, w=0.254):
        self.itens.append(("wire", (x1, y1, x2, y2, w)))

    def caixa(self, x1, y1, x2, y2, w=0.4064):
        for a, b, c, d in ((x1, y1, x2, y1), (x2, y1, x2, y2), (x2, y2, x1, y2), (x1, y2, x1, y1)):
            self.fio(a, b, c, d, w)

    def circulo(self, x, y, r, w=0.254):
        self.itens.append(("circle", (x, y, r, w)))

    def poligono(self, pontos):
        self.itens.append(("polygon", pontos))

    def texto(self, x, y, txt, tam=1.778, camada=94, alinha=None, rot=0):
        self.itens.append(("text", (x, y, txt, tam, camada, alinha, rot)))

    def pino(self, nome, x, y, comp="short", rot=0, visivel="off", direcao="pas"):
        self.pinos[nome] = (x, y, comp, rot, visivel, direcao)


class Pacote:
    def __init__(self, nome, descricao=""):
        self.nome = nome
        self.descricao = descricao
        self.itens = []

    def furo(self, nome, x, y, broca=0.9, diam=1.8, forma=None):
        self.itens.append(("pad", (nome, x, y, broca, diam, forma)))

    def fio(self, x1, y1, x2, y2, w=0.2032, camada=21):
        self.itens.append(("wire", (x1, y1, x2, y2, w, camada)))

    def caixa(self, x1, y1, x2, y2, w=0.2032):
        for a, b, c, d in ((x1, y1, x2, y1), (x2, y1, x2, y2), (x2, y2, x1, y2), (x1, y2, x1, y1)):
            self.fio(a, b, c, d, w)

    def circulo(self, x, y, r, w=0.2032):
        self.itens.append(("circle", (x, y, r, w)))

    def nomes(self, x, y):
        self.itens.append(("text", (x, y, ">NAME", 1.27, 25)))
        self.itens.append(("text", (x, y - 1.905, ">VALUE", 1.27, 27)))


class Conjunto:  # deviceset
    def __init__(self, nome, prefixo, portas, descricao="", valor_livre=True):
        self.nome = nome
        self.prefixo = prefixo
        self.portas = portas  # [(nome_porta, simbolo, x, y)]
        self.descricao = descricao
        self.valor_livre = valor_livre
        self.variantes = []  # [(nome, pacote|None, {(porta, pino): "pads"})]

    def variante(self, nome, pacote, ligacoes):
        self.variantes.append((nome, pacote, ligacoes))
        return self


SIMBOLOS, PACOTES, CONJUNTOS = {}, {}, {}


def novo_simbolo(nome):
    s = Simbolo(nome)
    SIMBOLOS[nome] = s
    return s


def novo_pacote(nome, descricao=""):
    p = Pacote(nome, descricao)
    PACOTES[nome] = p
    return p


def novo_conjunto(*a, **k):
    c = Conjunto(*a, **k)
    CONJUNTOS[c.nome] = c
    return c


# ---------------------------------------------------------------- NodeMCU
# Nomes como estão impressos no NodeMCU v3 (LoLin): G = GND, 3V = 3,3 V, VU = 5 V do USB
ESQ = ["A0", "G@1", "VU", "S3", "S2", "S1", "SC", "S0", "SK", "G@2", "3V@1", "EN", "RST", "G@3", "VIN"]
DIR = ["D0/GPIO16", "D1/GPIO5", "D2/GPIO4", "D3/GPIO0", "D4/GPIO2", "3V@2", "G@4", "D5/GPIO14", "D6/GPIO12",
       "D7/GPIO13", "D8/GPIO15", "RX", "TX", "G@5", "3V@3"]

s = novo_simbolo("NODEMCU")
s.caixa(-15.24, 20.32, 15.24, -20.32)
s.texto(0, 21.59, ">NAME", camada=95, alinha="bottom-center")
s.texto(0, -21.59, ">VALUE", camada=96, alinha="top-center")
for i, n in enumerate(ESQ):
    s.pino(n, -20.32, 17.78 - i * G, comp="middle", rot=0, visivel="pin")
for i, n in enumerate(DIR):
    s.pino(n, 20.32, 17.78 - i * G, comp="middle", rot=180, visivel="pin")

p = novo_pacote("NODEMCU-V3", "NodeMCU v3 (LoLin): 2 fileiras de 15 pinos a 1,1\" (27,94 mm); placa de "
                              "31 x 58 mm (contorno aproximado). Pino 1 = A0, antena para cima.")
for i in range(15):
    p.furo(str(i + 1), 0, -i * G, 1.0, 1.8, "square" if i == 0 else None)
    p.furo(str(i + 16), 27.94, -i * G, 1.0, 1.8)
p.caixa(-1.53, 12.7, 29.47, -45.3)
p.fio(10.47, -45.3, 17.47, -45.3, 0.6)  # lado do conector USB
p.nomes(0, 14.0)
ligs = {}
for i, n in enumerate(ESQ):
    ligs[("G$1", n)] = str(i + 1)
for i, n in enumerate(DIR):
    ligs[("G$1", n)] = str(i + 16)
novo_conjunto("NODEMCU", "U", [("G$1", "NODEMCU", 0, 0)],
              "NodeMCU v3 LoLin (ESP-12E / ESP8266)").variante("", "NODEMCU-V3", ligs)

# ------------------------------------------------- RTC DS3231 (lado de 4 pinos)
s = novo_simbolo("RTC_DS3231")
s.caixa(-7.62, 6.35, 10.16, -6.35)
s.texto(-7.62, 7.62, ">NAME", camada=95)
s.texto(-7.62, -8.89, ">VALUE", camada=96, alinha="top-left")
s.texto(5.08, 2.54, "DS3231", alinha="center")
s.texto(5.08, -1.27, "+ CR2032", tam=1.27, alinha="center")
for i, n in enumerate(["SCL", "SDA", "VCC", "GND"]):
    s.pino(n, -12.7, 3.81 - i * G, comp="middle", visivel="pin")
p = novo_pacote("MOD-4P", "Barra de 4 pinos 2,54 mm do modulo DS3231 (ZS-042). Confira a ordem no modulo.")
for i in range(4):
    p.furo(str(i + 1), 0, -i * G, 1.0, 1.8, "square" if i == 0 else None)
p.caixa(-1.27, 1.27, 1.27, -8.89)
p.nomes(2.0, 1.0)
novo_conjunto("RTC_DS3231", "U", [("G$1", "RTC_DS3231", 0, 0)],
              "Modulo RTC DS3231 (ZS-042), lado de 4 pinos: SCL, SDA, VCC, GND").variante(
    "", "MOD-4P", {("G$1", "SCL"): "1", ("G$1", "SDA"): "2", ("G$1", "VCC"): "3", ("G$1", "GND"): "4"})

# ----------------------------------------------------- relé (bobina + contato)
s = novo_simbolo("RELE_BOBINA")
s.caixa(-3.81, 2.54, 3.81, -2.54, 0.254)
s.fio(-3.81, -2.54, 3.81, 2.54)
s.texto(5.08, 1.27, ">NAME", camada=95)
s.texto(5.08, -1.27, ">VALUE", camada=96, alinha="top-left")
s.pino("A1", 0, 5.08, rot=270)
s.pino("A2", 0, -5.08, rot=90)

s = novo_simbolo("RELE_CONTATO")
s.fio(-2.54, 0, 2.286, -2.159)  # alavanca em repouso, encostada no NF
s.circulo(-2.54, 0, 0.3)
s.fio(2.54, 2.54, 2.54, 1.27)
s.fio(2.54, -2.54, 2.54, -1.778)
s.texto(-2.54, 1.27, "COM", tam=1.27)
s.texto(3.302, 2.794, "NA", tam=1.27)
s.texto(3.302, -4.064, "NF", tam=1.27)
s.texto(-2.54, -3.81, ">NAME", camada=95, alinha="top-left")
s.pino("COM", -5.08, 0, rot=0)
s.pino("NO", 5.08, 2.54, rot=180)
s.pino("NC", 5.08, -2.54, rot=180)

p = novo_pacote("RELE-SRD", "Rele Songle/Sanyou SRD-05VDC-SL-C (medidas do footprint SANYOU_SRD do KiCad).")
p.furo("1", 0, 0, 1.3, 3.0)          # COM
p.furo("2", 1.95, -6.05, 1.0, 2.5)   # bobina
p.furo("3", 14.15, -6.05, 1.3, 3.0)  # NA
p.furo("4", 14.2, 6.0, 1.3, 3.0)     # NF
p.furo("5", 1.95, 5.95, 1.0, 2.5)    # bobina
p.caixa(-1.4, 7.8, 18.4, -7.8)
p.nomes(-1.4, 9.5)
novo_conjunto("RELE_5V", "K", [("BOBINA", "RELE_BOBINA", 0, 0), ("CONTATO", "RELE_CONTATO", 0, 15.24)],
              "Rele 5 V SPDT (SRD-05VDC-SL-C): bobina 2-5, COM 1, NA 3, NF 4").variante(
    "", "RELE-SRD", {("BOBINA", "A1"): "5", ("BOBINA", "A2"): "2", ("CONTATO", "COM"): "1",
                     ("CONTATO", "NO"): "3", ("CONTATO", "NC"): "4"})

# ------------------------------------------------------------ transistor NPN
s = novo_simbolo("NPN")
s.fio(-0.508, 2.032, -0.508, -2.032, 0.508)  # base
s.fio(-2.54, 0, -0.508, 0, 0.1524)
s.fio(-0.508, 0.762, 2.54, 2.54, 0.1524)     # coletor
s.fio(-0.508, -0.762, 2.54, -2.54, 0.1524)   # emissor
s.poligono([(2.54, -2.54), (1.016, -2.286), (1.778, -1.27)])  # seta (NPN: para fora)
s.circulo(0.635, 0, 3.302)
s.texto(5.08, 1.27, ">NAME", camada=95)
s.texto(5.08, -1.27, ">VALUE", camada=96, alinha="top-left")
s.pino("B", -5.08, 0, rot=0)
s.pino("C", 2.54, 5.08, rot=270)
s.pino("E", 2.54, -5.08, rot=90)
p = novo_pacote("TO92-CBE", "TO-92, terminais C-B-E em linha a 2,54 mm (BC548, BC337).")
p.furo("C", -2.54, 0, 0.8, 1.5)
p.furo("B", 0, 0, 0.8, 1.5)
p.furo("E", 2.54, 0, 0.8, 1.5)
p.fio(-2.3, -1.5, 2.3, -1.5)
for i in range(12):  # meia-lua
    a1, a2 = math.pi * i / 12, math.pi * (i + 1) / 12
    p.fio(2.4 * math.cos(a1), -1.5 + 3.9 * math.sin(a1), 2.4 * math.cos(a2), -1.5 + 3.9 * math.sin(a2))
p.nomes(-2.54, 3.6)
novo_conjunto("BC548", "Q", [("G$1", "NPN", 0, 0)], "Transistor NPN BC548 (ou BC337, mesma pinagem C-B-E)").variante(
    "", "TO92-CBE", {("G$1", "B"): "B", ("G$1", "C"): "C", ("G$1", "E"): "E"})

# ----------------------------------------------------------------- resistor
s = novo_simbolo("RESISTOR")
s.caixa(-2.54, 1.016, 2.54, -1.016, 0.254)
s.texto(0, 1.524, ">NAME", camada=95, alinha="bottom-center")
s.texto(0, -1.524, ">VALUE", camada=96, alinha="top-center")
s.pino("1", -5.08, 0, rot=0)
s.pino("2", 5.08, 0, rot=180)
for nome, passo, corpo in (("R-0207/10", 10.16, 3.3), ("R-0309/12", 12.7, 4.6)):
    p = novo_pacote(nome, "Resistor axial, furos a %.2f mm" % passo)
    p.furo("1", -passo / 2, 0, 0.9, 1.8)
    p.furo("2", passo / 2, 0, 0.9, 1.8)
    p.caixa(-corpo, 1.2, corpo, -1.2)
    p.nomes(-corpo, 2.2)
novo_conjunto("RESISTOR", "R", [("G$1", "RESISTOR", 0, 0)], "Resistor axial").variante(
    "-1/4W", "R-0207/10", {("G$1", "1"): "1", ("G$1", "2"): "2"}).variante(
    "-1/2W", "R-0309/12", {("G$1", "1"): "1", ("G$1", "2"): "2"})

# ------------------------------------------------------------ diodo e LED
s = novo_simbolo("DIODO")
s.fio(-2.54, 0, -1.27, 0, 0.1524)
s.poligono([(-1.27, 1.27), (-1.27, -1.27), (1.27, 0)])
s.fio(1.27, 1.27, 1.27, -1.27)
s.fio(1.27, 0, 2.54, 0, 0.1524)
s.texto(-2.54, 1.778, ">NAME", camada=95)
s.texto(-2.54, -1.778, ">VALUE", camada=96, alinha="top-left")
s.pino("A", -5.08, 0, rot=0)
s.pino("C", 5.08, 0, rot=180)
p = novo_pacote("DO41-10", "Diodo DO-41 (1N4007), furos a 10,16 mm. Faixa = catodo.")
p.furo("A", -5.08, 0, 1.0, 1.9)
p.furo("C", 5.08, 0, 1.0, 1.9)
p.caixa(-2.5, 1.4, 2.5, -1.4)
p.fio(1.6, 1.4, 1.6, -1.4, 0.5)
p.nomes(-2.5, 2.4)
novo_conjunto("DIODO", "D", [("G$1", "DIODO", 0, 0)], "Diodo retificador").variante(
    "", "DO41-10", {("G$1", "A"): "A", ("G$1", "C"): "C"})

s = novo_simbolo("LED")
s.poligono([(-1.27, 1.27), (-1.27, -1.27), (1.27, 0)])
s.fio(1.27, 1.27, 1.27, -1.27)
s.fio(-2.54, 0, -1.27, 0, 0.1524)
s.fio(1.27, 0, 2.54, 0, 0.1524)
for dx in (0, 1.27):  # setinhas de luz
    s.fio(0.0 + dx, 1.778, 1.016 + dx, 2.794, 0.1524)
    s.poligono([(1.016 + dx, 2.794), (0.508 + dx, 2.54), (0.762 + dx, 2.286)])
s.texto(-2.54, -1.778, ">NAME", camada=95, alinha="top-left")
s.texto(-2.54, -3.81, ">VALUE", camada=96, alinha="top-left")
s.pino("A", -5.08, 0, rot=0)
s.pino("C", 5.08, 0, rot=180)
p = novo_pacote("LED3MM", "LED 3 mm, furos a 2,54 mm (terminal curto/lado chato = catodo).")
p.furo("A", -1.27, 0, 0.8, 1.5)
p.furo("C", 1.27, 0, 0.8, 1.5, "square")
p.circulo(0, 0, 1.9)
p.nomes(-2.0, 3.0)
novo_conjunto("LED", "LED", [("G$1", "LED", 0, 0)], "LED 3 mm").variante(
    "", "LED3MM", {("G$1", "A"): "A", ("G$1", "C"): "C"})

# ---------------------------------------------------------------- capacitores
s = novo_simbolo("CAPACITOR")
s.fio(-0.635, 1.905, -0.635, -1.905, 0.4064)
s.fio(0.635, 1.905, 0.635, -1.905, 0.4064)
s.fio(-2.54, 0, -0.635, 0, 0.1524)
s.fio(0.635, 0, 2.54, 0, 0.1524)
s.texto(-2.54, 2.286, ">NAME", camada=95)
s.texto(-2.54, -2.286, ">VALUE", camada=96, alinha="top-left")
s.pino("1", -5.08, 0, rot=0)
s.pino("2", 5.08, 0, rot=180)
p = novo_pacote("C-2.54", "Capacitor ceramico, furos a 2,54 mm")
p.furo("1", -1.27, 0, 0.8, 1.5)
p.furo("2", 1.27, 0, 0.8, 1.5)
p.caixa(-2.4, 1.2, 2.4, -1.2)
p.nomes(-2.4, 2.2)
p = novo_pacote("C-X2-15", "Capacitor de poliester X2 (275 VAC), furos a 15 mm")
p.furo("1", -7.5, 0, 1.0, 2.0)
p.furo("2", 7.5, 0, 1.0, 2.0)
p.caixa(-9.0, 3.0, 9.0, -3.0)
p.nomes(-9.0, 4.0)
novo_conjunto("CAPACITOR", "C", [("G$1", "CAPACITOR", 0, 0)], "Capacitor sem polaridade").variante(
    "-CERAMICO", "C-2.54", {("G$1", "1"): "1", ("G$1", "2"): "2"}).variante(
    "-X2", "C-X2-15", {("G$1", "1"): "1", ("G$1", "2"): "2"})

s = novo_simbolo("CAP_POL")  # vertical: + em cima
s.fio(-1.905, 0.635, 1.905, 0.635, 0.4064)
s.fio(-1.905, -0.635, 1.905, -0.635, 0.4064)
s.fio(-1.905, -0.635, -1.905, -1.143, 0.4064)
s.fio(1.905, -0.635, 1.905, -1.143, 0.4064)
s.fio(0, 2.54, 0, 0.635, 0.1524)
s.fio(0, -0.635, 0, -2.54, 0.1524)
s.texto(-1.524, 1.524, "+", tam=1.27, alinha="bottom-right")
s.texto(2.54, 1.27, ">NAME", camada=95)
s.texto(2.54, -1.27, ">VALUE", camada=96, alinha="top-left")
s.pino("+", 0, 5.08, rot=270)
s.pino("-", 0, -5.08, rot=90)
p = novo_pacote("CPOL-8MM", "Capacitor eletrolitico radial 8 mm, furos a 3,5 mm")
p.furo("+", -1.75, 0, 0.9, 1.7, "square")
p.furo("-", 1.75, 0, 0.9, 1.7)
p.circulo(0, 0, 4.1)
p.nomes(-4.0, 5.2)
novo_conjunto("CAP_ELETROLITICO", "C", [("G$1", "CAP_POL", 0, 0)], "Capacitor eletrolitico").variante(
    "", "CPOL-8MM", {("G$1", "+"): "+", ("G$1", "-"): "-"})

# ------------------------------------------------- bornes, fusível, botão, conector
s = novo_simbolo("BORNE_2")
s.caixa(-2.54, 3.81, 2.54, -1.27, 0.254)
for i in range(2):
    s.circulo(0, 2.54 - i * G, 0.762)
    s.texto(-1.016, 2.54 - i * G, str(i + 1), tam=1.27, alinha="center-right")
s.texto(-2.54, 4.572, ">NAME", camada=95)
s.texto(-2.54, -2.032, ">VALUE", camada=96, alinha="top-left")
s.pino("1", 5.08, 2.54, rot=180)
s.pino("2", 5.08, 0, rot=180)
p = novo_pacote("BORNE-2-5.08", "Borne de parafuso 2 vias, passo 5,08 mm (KF301)")
p.furo("1", 0, 0, 1.3, 2.6, "square")
p.furo("2", 5.08, 0, 1.3, 2.6)
p.caixa(-2.54, 3.8, 7.62, -3.8)
p.nomes(-2.54, 4.8)
novo_conjunto("BORNE_2", "J", [("G$1", "BORNE_2", 0, 0)], "Borne de parafuso 2 vias").variante(
    "", "BORNE-2-5.08", {("G$1", "1"): "1", ("G$1", "2"): "2"})

s = novo_simbolo("FUSIVEL")
s.caixa(-2.54, 0.889, 2.54, -0.889, 0.254)
s.fio(-2.54, 0, 2.54, 0, 0.1524)
s.texto(-2.54, 1.524, ">NAME", camada=95)
s.texto(-2.54, -1.524, ">VALUE", camada=96, alinha="top-left")
s.pino("1", -5.08, 0, rot=0)
s.pino("2", 5.08, 0, rot=180)
p = novo_pacote("FUSIVEL-5X20", "Porta-fusivel 5x20 mm para placa (2 presilhas a 22,6 mm) - confira o seu.")
p.furo("1", -11.3, 0, 1.3, 2.6)
p.furo("2", 11.3, 0, 1.3, 2.6)
p.caixa(-13.5, 3.5, 13.5, -3.5)
p.nomes(-13.5, 4.5)
novo_conjunto("FUSIVEL", "F", [("G$1", "FUSIVEL", 0, 0)], "Fusivel 5x20 mm").variante(
    "", "FUSIVEL-5X20", {("G$1", "1"): "1", ("G$1", "2"): "2"})

s = novo_simbolo("BOTAO")
s.fio(-2.54, 0, -1.778, 0, 0.1524)
s.fio(1.778, 0, 2.54, 0, 0.1524)
s.circulo(-1.524, 0, 0.254)
s.circulo(1.524, 0, 0.254)
s.fio(-1.905, 1.27, 1.905, 1.27)
s.fio(0, 1.27, 0, 2.54)
s.texto(-2.54, 3.048, ">NAME", camada=95)
s.texto(-2.54, -1.016, ">VALUE", camada=96, alinha="top-left")
s.pino("1", -5.08, 0, rot=0)
s.pino("2", 5.08, 0, rot=180)
p = novo_pacote("BOTAO-6X6", "Botao tactil 6x6 mm. Ligado em diagonal (1 e 4): funciona em qualquer marca.")
p.furo("1", 0, 0, 1.0, 1.8)
p.furo("2", 6.5, 0, 1.0, 1.8)
p.furo("3", 0, -4.5, 1.0, 1.8)
p.furo("4", 6.5, -4.5, 1.0, 1.8)
p.caixa(0.25, 0.75, 6.25, -5.25)
p.circulo(3.25, -2.25, 1.75)
p.nomes(0, 1.8)
novo_conjunto("BOTAO", "S", [("G$1", "BOTAO", 0, 0)], "Botao tactil").variante(
    "", "BOTAO-6X6", {("G$1", "1"): "1", ("G$1", "2"): "4"})

s = novo_simbolo("CONECTOR_GPS")
s.caixa(-2.54, 6.35, 7.62, -6.35, 0.254)
for i, n in enumerate(["VCC", "RX", "TX", "GND"]):
    s.texto(-1.524, 3.81 - i * G, n, tam=1.27, alinha="center-left")
s.texto(-2.54, 7.112, ">NAME", camada=95)
s.texto(-2.54, -7.112, ">VALUE", camada=96, alinha="top-left")
for i in range(4):
    s.pino(str(i + 1), -7.62, 3.81 - i * G, rot=0)
p = novo_pacote("HDR-1X4", "Barra de 4 pinos 2,54 mm (mesma ordem do GPS GY-NEO6MV2: VCC, RX, TX, GND)")
for i in range(4):
    p.furo(str(i + 1), 0, -i * G, 1.0, 1.8, "square" if i == 0 else None)
p.caixa(-1.27, 1.27, 1.27, -8.89)
p.nomes(2.0, 1.0)
novo_conjunto("CONECTOR_GPS", "J", [("G$1", "CONECTOR_GPS", 0, 0)], "Conector do GPS (opcional)").variante(
    "", "HDR-1X4", {("G$1", "1"): "1", ("G$1", "2"): "2", ("G$1", "3"): "3", ("G$1", "4"): "4"})

# ------------------------------------------------------- símbolos de alimentação
s = novo_simbolo("GND")
s.fio(0, 0, 0, -1.27, 0.254)
s.fio(-1.905, -1.27, 1.905, -1.27, 0.254)
s.fio(-1.27, -1.905, 1.27, -1.905, 0.254)
s.fio(-0.635, -2.54, 0.635, -2.54, 0.254)
s.pino("GND", 0, 0, comp="point", direcao="sup")
novo_conjunto("GND", "GND", [("1", "GND", 0, 0)], "Terra (0 V)", valor_livre=False).variante("", None, {})
for nome in ("+5V", "+3V3"):
    s = novo_simbolo(nome)
    s.fio(0, 0, 0, 1.27, 0.254)
    s.fio(-1.27, 1.27, 1.27, 1.27, 0.254)
    s.texto(0, 1.778, ">VALUE", camada=96, alinha="bottom-center")
    s.pino(nome, 0, 0, comp="point", direcao="sup")
    novo_conjunto(nome, "P", [("1", nome, 0, 0)], "Alimentacao %s" % nome, valor_livre=False).variante("", None, {})


# ================================================================ esquema
class Esquema:
    def __init__(self):
        self.pecas = {}  # nome -> (conjunto, variante, valor)
        self.instancias = []  # (peca, porta, x, y, rot)
        self.redes = {}  # nome -> [segmentos]; segmento = list de itens
        self.plano = []  # textos/fios soltos
        self.n_alim = 0

    def peca(self, nome, conjunto, variante="", valor=None):
        self.pecas[nome] = (conjunto, variante, valor)

    def coloca(self, nome, porta, x, y, rot=0):
        self.instancias.append((nome, porta, x, y, rot))

    def pino(self, nome, porta, pino):
        conj = CONJUNTOS[self.pecas[nome][0]]
        sim = [g for g in conj.portas if g[0] == porta][0][1]
        px, py = SIMBOLOS[sim].pinos[pino][:2]
        for (pn, gp, x, y, rot) in self.instancias:
            if pn == nome and gp == porta:
                dx, dy = gira(px, py, rot)
                return (q(x + dx), q(y + dy))
        raise KeyError((nome, porta))

    def segmento(self, rede):
        seg = []
        self.redes.setdefault(rede, []).append(seg)
        return seg

    def alimentacao(self, rede, x, y):
        """Coloca um símbolo de alimentação (GND, +5V, +3V3) em (x, y)."""
        self.n_alim += 1
        nome = ("GND%d" if rede == "GND" else "P%d") % self.n_alim
        self.peca(nome, rede, "", rede if rede != "GND" else None)
        self.coloca(nome, "1", x, y)
        return nome


E = Esquema()


def ligar(rede, *caminho, rotulo=None, juncoes=(), alim=None):
    """Um segmento da rede: pinos e pontos ligados por fios retos, em ordem.
    Cada item é ("pino", peça, porta, pino) ou um ponto (x, y)."""
    seg = E.segmento(rede)
    pts = []
    for it in caminho:
        if isinstance(it, tuple) and len(it) == 3 and isinstance(it[0], str):
            pt = E.pino(*it)
            seg.append(("pinref",) + it)
        else:
            pt = (q(it[0]), q(it[1]))
        pts.append(pt)
    for a, b in zip(pts, pts[1:]):
        if a != b:
            seg.append(("wire", a, b))
    for j in juncoes:
        seg.append(("junction", (q(j[0]), q(j[1]))))
    if rotulo:
        (x, y), rot = rotulo
        seg.append(("label", (q(x), q(y)), rot))
    if alim:
        nome = E.alimentacao(rede, *pts[-1])
        seg.append(("pinref", nome, "1", rede))
    return seg


def toco(rede, peca, porta, pino, lado, comp=7.62):
    """Fio curto saindo do pino até um rótulo com o nome da rede."""
    x, y = E.pino(peca, porta, pino)
    x2 = x - comp if lado == "esq" else x + comp
    ligar(rede, (peca, porta, pino), (x2, y), rotulo=((x2, y), 180 if lado == "esq" else 0))


def ramo(rede, de, ate, via=None, juncoes=(), alim=False):
    caminho = [de] + (list(via) if via else []) + [ate]
    return ligar(rede, *caminho, juncoes=juncoes, alim=alim)


# ----------------------------------------------------------------- peças
E.peca("U1", "NODEMCU", "", "NodeMCU v3 LoLin")
E.peca("U2", "RTC_DS3231", "", "DS3231 ZS-042")
E.peca("K1", "RELE_5V", "", "SRD-05VDC-SL-C")
E.peca("Q1", "BC548", "", "BC548")
E.peca("R1", "RESISTOR", "-1/4W", "1k")
E.peca("R2", "RESISTOR", "-1/4W", "10k")
E.peca("R3", "RESISTOR", "-1/4W", "1k")
E.peca("R4", "RESISTOR", "-1/2W", "100R 1/2W")
E.peca("D1", "DIODO", "", "1N4007")
E.peca("LED1", "LED", "", "RELE LIGADO")
E.peca("C1", "CAP_ELETROLITICO", "", "470uF 16V")
E.peca("C2", "CAPACITOR", "-CERAMICO", "100nF")
E.peca("C3", "CAPACITOR", "-X2", "100nF X2 275VAC")
E.peca("J1", "BORNE_2", "", "ENTRADA 5V")
E.peca("J2", "BORNE_2", "", "127V FASE/SINO")
E.peca("F1", "FUSIVEL", "", "2A")
E.peca("S1", "BOTAO", "", "TESTE")
E.peca("J3", "CONECTOR_GPS", "", "GPS (opcional)")

# ------------------------------------------------------------ posições
E.coloca("J1", "G$1", 25.4, 160.02)
E.coloca("C1", "G$1", 50.8, 157.48)
E.coloca("C2", "G$1", 68.58, 157.48, 90)
E.coloca("U1", "G$1", 104.14, 114.3)
E.coloca("U2", "G$1", 198.12, 162.56)
E.coloca("J3", "G$1", 198.12, 134.62)
E.coloca("K1", "BOBINA", 236.22, 116.84)
E.coloca("D1", "G$1", 226.06, 116.84, 90)
E.coloca("R3", "G$1", 215.9, 124.46, 90)
E.coloca("LED1", "G$1", 215.9, 111.76, 270)
E.coloca("Q1", "G$1", 233.68, 96.52)
E.coloca("R1", "G$1", 215.9, 96.52)
E.coloca("R2", "G$1", 226.06, 86.36, 90)
E.coloca("S1", "G$1", 172.72, 96.52)
E.coloca("K1", "CONTATO", 233.68, 50.8)
E.coloca("F1", "G$1", 208.28, 50.8)
E.coloca("R4", "G$1", 228.6, 63.5)
E.coloca("C3", "G$1", 243.84, 63.5)
E.coloca("J2", "G$1", 172.72, 45.72)

# ------------------------------------------------------------- ligações
# Alimentação 5 V: borne, capacitores e símbolos
x5, xg = 38.1, 35.56
ramo("+5V", ("J1", "G$1", "1"), (x5, 170.18), via=[(x5, 162.56)])
ligar("+5V", (x5, 170.18), (50.8, 170.18), (68.58, 170.18), (81.28, 170.18), juncoes=[(50.8, 170.18), (68.58, 170.18)],
      alim=True)
ramo("+5V", ("C1", "G$1", "+"), (50.8, 170.18))
ramo("+5V", ("C2", "G$1", "2"), (68.58, 170.18))
ramo("GND", ("J1", "G$1", "2"), (xg, 144.78), via=[(xg, 160.02)])
ligar("GND", (xg, 144.78), (50.8, 144.78), (68.58, 144.78), (81.28, 144.78), juncoes=[(50.8, 144.78), (68.58, 144.78)],
      alim=True)
ramo("GND", ("C1", "G$1", "-"), (50.8, 144.78))
ramo("GND", ("C2", "G$1", "1"), (68.58, 144.78))

# NodeMCU: tudo por rótulos
toco("+5V", "U1", "G$1", "VIN", "esq")
toco("GND", "U1", "G$1", "G@3", "esq")
toco("GND", "U1", "G$1", "G@5", "dir")
toco("+3V3", "U1", "G$1", "3V@3", "dir")
toco("SCL", "U1", "G$1", "D1/GPIO5", "dir")
toco("SDA", "U1", "G$1", "D2/GPIO4", "dir")
toco("GPS_TX", "U1", "G$1", "D5/GPIO14", "dir")
toco("RELE", "U1", "G$1", "D6/GPIO12", "dir")
toco("BOTAO", "U1", "G$1", "D7/GPIO13", "dir")

# Relógio e GPS
toco("SCL", "U2", "G$1", "SCL", "esq")
toco("SDA", "U2", "G$1", "SDA", "esq")
toco("+3V3", "U2", "G$1", "VCC", "esq")
toco("GND", "U2", "G$1", "GND", "esq")
toco("+5V", "J3", "G$1", "1", "esq")
toco("GPS_TX", "J3", "G$1", "3", "esq")
toco("GND", "J3", "G$1", "4", "esq")

# Acionamento do relé: +5V -> bobina -> coletor; diodo de roda livre; LED
y5 = 132.08
ramo("+5V", ("R3", "G$1", "2"), (215.9, y5))
ligar("+5V", (215.9, y5), (226.06, y5), (236.22, y5), juncoes=[(226.06, y5)])
ligar("+5V", (215.9, y5), (215.9, y5 + 2.54), alim=True)
ramo("+5V", ("D1", "G$1", "C"), (226.06, y5))
ramo("+5V", ("K1", "BOBINA", "A1"), (236.22, y5))
yn = 104.14
ramo("COLETOR_Q1", ("LED1", "G$1", "C"), (236.22, yn), via=[(215.9, yn), (226.06, yn)], juncoes=[(226.06, yn), (236.22, yn)])
ramo("COLETOR_Q1", ("D1", "G$1", "A"), (226.06, yn))
ramo("COLETOR_Q1", ("K1", "BOBINA", "A2"), (236.22, yn))
ramo("COLETOR_Q1", ("Q1", "G$1", "C"), (236.22, yn))
ramo("LED_ANODO", ("R3", "G$1", "1"), ("LED1", "G$1", "A"))
ramo("BASE_Q1", ("R1", "G$1", "2"), ("Q1", "G$1", "B"), via=[(226.06, 96.52)], juncoes=[(226.06, 96.52)])
ramo("BASE_Q1", ("R2", "G$1", "2"), (226.06, 96.52))
ramo("GND", ("R2", "G$1", "1"), (226.06, 78.74), alim=True)
ramo("GND", ("Q1", "G$1", "E"), (236.22, 86.36), alim=True)
toco("RELE", "R1", "G$1", "1", "esq")

# Botão de teste
toco("BOTAO", "S1", "G$1", "1", "esq")
ramo("GND", ("S1", "G$1", "2"), (182.88, 91.44), via=[(182.88, 96.52)], alim=True)

# Saída de 127 V: fase -> fusível -> COM; NA -> sino; snubber em paralelo
ramo("FASE_127V", ("J2", "G$1", "1"), ("F1", "G$1", "1"), via=[(190.5, 48.26), (190.5, 50.8)])
ramo("COM", ("F1", "G$1", "2"), (218.44, 50.8))
ramo("COM", (218.44, 50.8), ("K1", "CONTATO", "COM"), juncoes=[(218.44, 50.8)])
ramo("COM", (218.44, 50.8), ("R4", "G$1", "1"), via=[(218.44, 63.5)])
ramo("SNUBBER", ("R4", "G$1", "2"), ("C3", "G$1", "1"))
ramo("SINO", ("K1", "CONTATO", "NO"), (251.46, 53.34))
ramo("SINO", ("C3", "G$1", "2"), (251.46, 53.34), via=[(251.46, 63.5)])
ramo("SINO", (251.46, 53.34), ("J2", "G$1", "2"), via=[(251.46, 38.1), (182.88, 38.1), (182.88, 45.72)],
     juncoes=[(251.46, 53.34)])

# ---------------------------------------------------------------- textos
TITULOS = [
    (12.7, 177.8, "ALIMENTACAO 5V"),
    (73.66, 139.7, "NODEMCU V3 (LOLIN)"),
    (182.88, 175.26, "RELOGIO DS3231 - LADO DE 4 PINOS"),
    (182.88, 147.32, "GPS (OPCIONAL)"),
    (213.36, 142.24, "ACIONAMENTO DO RELE"),
    (160.02, 106.68, "BOTAO DE TESTE"),
    (167.64, 71.12, "SAIDA 127V PARA O SINO"),
]
NOTAS = [
    "NOTAS",
    "1. Fonte 5V (ex.: HLK-PM01) em J1: alimenta o rele e o NodeMCU pelo pino VIN.",
    "2. O 3V do NodeMCU alimenta o RTC (bateria CR2032). VU = 5V do USB: nao ligar.",
    "3. Q1 liga o rele com D6/GPIO12 em nivel ALTO: no firmware, RELE_ATIVO_EM_BAIXO = false.",
    "4. R2 mantem o rele desligado enquanto o ESP liga ou e gravado.",
    "5. D1 = diodo de roda livre (catodo no +5V). Rele com mais de 90 mA: troque Q1 por BC337.",
    "6. RTC: lado de 4 pinos do ZS-042, na ordem impressa no modulo: SCL, SDA, VCC, GND.",
    "7. Hora certa pela rede (NTP); o DS3231 segura a hora quando a rede cai. J3 (GPS) e opcional.",
    "8. 127V: fase -> J2.1 -> F1 -> COM; NA -> J2.2 -> sino; neutro direto no sino.",
    "9. Snubber R4+C3 opcional (protege o contato e evita reset do ESP).",
    "10. Na placa: deixar 6 mm ou mais entre a parte de 127V e o resto do circuito.",
]


# ================================================================ conferência
def conferir():
    erros = []
    pontos_pino = {}  # ponto -> [(rede, peca, pino)]
    fios_rede = {}
    for rede, segs in E.redes.items():
        for seg in segs:
            fios = [(it[1], it[2]) for it in seg if it[0] == "wire"]
            fios_rede.setdefault(rede, []).extend(fios)
            ends = {}
            for a, b in fios:
                ends.setdefault(a, set()).add(b)
                ends.setdefault(b, set()).add(a)

            def no_meio(p, a, b):
                if a[0] == b[0] == p[0]:
                    return min(a[1], b[1]) < p[1] < max(a[1], b[1])
                if a[1] == b[1] == p[1]:
                    return min(a[0], b[0]) < p[0] < max(a[0], b[0])
                return False

            # pontos que tocam o meio de outro fio do mesmo segmento (junção em "T")
            for p in list(ends):
                for a, b in fios:
                    if no_meio(p, a, b):
                        ends[p].update((a, b))
                        ends[a].add(p)
                        ends[b].add(p)
                        if not any(it[0] == "junction" and it[1] == p for it in seg):
                            erros.append("%s: falta junção em %s" % (rede, p))
            pinos = []
            for it in seg:
                if it[0] == "pinref":
                    pt = E.pino(it[1], it[2], it[3])
                    pinos.append(pt)
                    pontos_pino.setdefault(pt, []).append((rede, it[1], it[3]))
                    if fios and pt not in ends:
                        erros.append("%s: pino %s.%s em %s não encosta em fio" % (rede, it[1], it[3], pt))
                if it[0] == "label" and not any(it[1] in (a, b) for a, b in fios):
                    erros.append("%s: rótulo fora do fio em %s" % (rede, it[1]))
            # tudo ligado?
            if ends:
                inicio = next(iter(ends))
                vistos, pilha = {inicio}, [inicio]
                while pilha:
                    for v in ends[pilha.pop()]:
                        if v not in vistos:
                            vistos.add(v)
                            pilha.append(v)
                if set(ends) - vistos:
                    erros.append("%s: segmento partido %s" % (rede, sorted(set(ends) - vistos)))
    # um ponto de pino não pode ser de duas redes; fios não podem tocar outra rede
    for pt, lst in pontos_pino.items():
        if len({r for r, _, _ in lst}) > 1:
            erros.append("ponto %s em redes diferentes: %s" % (pt, lst))
    todos_pinos = {}
    for (pn, gp, x, y, rot) in E.instancias:
        conj = CONJUNTOS[E.pecas[pn][0]]
        sim = [g for g in conj.portas if g[0] == gp][0][1]
        for nome in SIMBOLOS[sim].pinos:
            todos_pinos[E.pino(pn, gp, nome)] = (pn, nome)
    for rede, fios in fios_rede.items():
        for a, b in fios:
            for pt, (pn, nome) in todos_pinos.items():
                dono = [r for r, p2, n2 in pontos_pino.get(pt, [])]
                passa = (a[0] == b[0] == pt[0] and min(a[1], b[1]) <= pt[1] <= max(a[1], b[1])) or \
                        (a[1] == b[1] == pt[1] and min(a[0], b[0]) <= pt[0] <= max(a[0], b[0]))
                if passa and rede not in dono:
                    erros.append("fio de %s passa pelo pino %s.%s (%s)" % (rede, pn, nome, dono or "solto"))
        for outra, fios2 in fios_rede.items():
            if outra <= rede:
                continue
            for a, b in fios:
                for c, d in fios2:
                    for p in (c, d):
                        if (a[0] == b[0] == p[0] and min(a[1], b[1]) <= p[1] <= max(a[1], b[1])) or \
                                (a[1] == b[1] == p[1] and min(a[0], b[0]) <= p[0] <= max(a[0], b[0])):
                            erros.append("redes %s e %s se tocam em %s" % (rede, outra, p))
    return erros


# ===================================================================== XML
CAMADAS = [
    (1, 'Top', 4, 1), (2, 'Route2', 1, 3), (3, 'Route3', 4, 3), (4, 'Route4', 1, 4), (5, 'Route5', 4, 4),
    (6, 'Route6', 1, 8), (7, 'Route7', 4, 8), (8, 'Route8', 1, 2), (9, 'Route9', 4, 2),
    (10, 'Route10', 1, 7), (11, 'Route11', 4, 7), (12, 'Route12', 1, 5), (13, 'Route13', 4, 5),
    (14, 'Route14', 1, 6), (15, 'Route15', 4, 6), (16, 'Bottom', 1, 1), (17, 'Pads', 2, 1),
    (18, 'Vias', 2, 1), (19, 'Unrouted', 6, 1), (20, 'Dimension', 15, 1), (21, 'tPlace', 7, 1),
    (22, 'bPlace', 7, 1), (23, 'tOrigins', 15, 1), (24, 'bOrigins', 15, 1), (25, 'tNames', 7, 1),
    (26, 'bNames', 7, 1), (27, 'tValues', 7, 1), (28, 'bValues', 7, 1), (29, 'tStop', 7, 3),
    (30, 'bStop', 7, 6), (31, 'tCream', 7, 4), (32, 'bCream', 7, 5), (33, 'tFinish', 6, 3),
    (34, 'bFinish', 6, 6), (35, 'tGlue', 7, 4), (36, 'bGlue', 7, 5), (37, 'tTest', 7, 1),
    (38, 'bTest', 7, 1), (39, 'tKeepout', 4, 11), (40, 'bKeepout', 1, 11), (41, 'tRestrict', 4, 10),
    (42, 'bRestrict', 1, 10), (43, 'vRestrict', 2, 10), (44, 'Drills', 7, 1), (45, 'Holes', 7, 1),
    (46, 'Milling', 3, 1), (47, 'Measures', 7, 1), (48, 'Document', 7, 1), (49, 'Reference', 7, 1),
    (50, 'dxf', 7, 1), (51, 'tDocu', 7, 1), (52, 'bDocu', 7, 1), (91, 'Nets', 2, 1), (92, 'Busses', 1, 1),
    (93, 'Pins', 2, 1), (94, 'Symbols', 4, 1), (95, 'Names', 7, 1), (96, 'Values', 7, 1), (97, 'Info', 7, 1),
    (98, 'Guide', 6, 1),
]
COMPRIMENTO = {"point": 0, "short": 2.54, "middle": 5.08, "long": 7.62}


def a(**k):
    return " ".join('%s="%s"' % (n.replace("_", "-") if n not in ("class_",) else "class", escape(str(v), {'"': "&quot;"}))
                    for n, v in k.items() if v is not None)


def xml_esquema():
    L = ['<?xml version="1.0" encoding="utf-8"?>', '<!DOCTYPE eagle SYSTEM "eagle.dtd">', '<eagle version="7.7.0">',
         "<drawing>", "<settings>", '<setting alwaysvectorfont="no"/>', '<setting verticaltext="up"/>', "</settings>",
         '<grid distance="0.1" unitdist="inch" unit="inch" style="lines" multiple="1" display="no" '
         'altdistance="0.01" altunitdist="inch" altunit="inch"/>', "<layers>"]
    for n, nome, cor, fill in CAMADAS:
        vis = "yes" if n >= 91 or n in (1, 16, 17, 18, 19, 20, 21, 23, 25, 27, 29, 39, 41, 44, 45, 51) else "no"
        L.append('<layer %s/>' % a(number=n, name=nome, color=cor, fill=fill, visible=vis, active="yes"))
    L += ["</layers>", '<schematic xreflabel="%F%N/%S.%C%R" xrefpart="/%S.%C%R">',
          "<libraries>", '<library name="sino_ifms">',
          "<description>Biblioteca do projeto Sino IFMS (gerada automaticamente).</description>", "<packages>"]
    for p in PACOTES.values():
        L.append('<package name="%s">' % escape(p.nome))
        if p.descricao:
            L.append("<description>%s</description>" % escape(p.descricao))
        for tipo, d in p.itens:
            if tipo == "pad":
                nome, x, y, broca, diam, forma = d
                L.append("<pad %s/>" % a(name=nome, x=f(x), y=f(y), drill=f(broca), diameter=f(diam), shape=forma))
            elif tipo == "wire":
                x1, y1, x2, y2, w, cam = d
                L.append("<wire %s/>" % a(x1=f(x1), y1=f(y1), x2=f(x2), y2=f(y2), width=f(w), layer=cam))
            elif tipo == "circle":
                x, y, r, w = d
                L.append("<circle %s/>" % a(x=f(x), y=f(y), radius=f(r), width=f(w), layer=21))
            elif tipo == "text":
                x, y, t, tam, cam = d
                L.append("<text %s>%s</text>" % (a(x=f(x), y=f(y), size=f(tam), layer=cam), escape(t)))
        L.append("</package>")
    L += ["</packages>", "<symbols>"]
    for s in SIMBOLOS.values():
        L.append('<symbol name="%s">' % escape(s.nome))
        for tipo, d in s.itens:
            if tipo == "wire":
                x1, y1, x2, y2, w = d
                L.append("<wire %s/>" % a(x1=f(x1), y1=f(y1), x2=f(x2), y2=f(y2), width=f(w), layer=94))
            elif tipo == "circle":
                x, y, r, w = d
                L.append("<circle %s/>" % a(x=f(x), y=f(y), radius=f(r), width=f(w), layer=94))
            elif tipo == "polygon":
                L.append('<polygon width="0.1524" layer="94">')
                for (x, y) in d:
                    L.append("<vertex %s/>" % a(x=f(x), y=f(y)))
                L.append("</polygon>")
            elif tipo == "text":
                x, y, t, tam, cam, alinha, rot = d
                L.append("<text %s>%s</text>" % (a(x=f(x), y=f(y), size=f(tam), layer=cam,
                                                    rot=("R%d" % rot) if rot else None, align=alinha), escape(t)))
        for nome, (x, y, comp, rot, vis, dirc) in s.pinos.items():
            L.append("<pin %s/>" % a(name=nome, x=f(x), y=f(y), visible=vis, length=comp, direction=dirc,
                                     rot=("R%d" % rot) if rot else None))
        L.append("</symbol>")
    L += ["</symbols>", "<devicesets>"]
    for c in CONJUNTOS.values():
        L.append('<deviceset %s>' % a(name=c.nome, prefix=c.prefixo, uservalue="yes" if c.valor_livre else None))
        L.append("<description>%s</description>" % escape(c.descricao))
        L.append("<gates>")
        for gn, sim, x, y in c.portas:
            L.append("<gate %s/>" % a(name=gn, symbol=sim, x=f(x), y=f(y)))
        L.append("</gates>")
        L.append("<devices>")
        for vn, pac, ligs in c.variantes:
            L.append("<device %s>" % a(name=vn, package=pac))
            if ligs:
                L.append("<connects>")
                for (g, pin), pad in ligs.items():
                    L.append("<connect %s/>" % a(gate=g, pin=pin, pad=pad))
                L.append("</connects>")
            L.append('<technologies>\n<technology name=""/>\n</technologies>')
            L.append("</device>")
        L.append("</devices>")
        L.append("</deviceset>")
    L += ["</devicesets>", "</library>", "</libraries>", "<attributes>", "</attributes>", "<variantdefs>",
          "</variantdefs>", "<classes>", '<class number="0" name="default" width="0" drill="0">', "</class>",
          "</classes>", "<parts>"]
    for nome, (conj, var, valor) in E.pecas.items():
        L.append("<part %s/>" % a(name=nome, library="sino_ifms", deviceset=conj, device=var, value=valor))
    L += ["</parts>", "<sheets>", "<sheet>", "<plain>"]
    for tipo, d in E.plano:
        if tipo == "text":
            x, y, t, tam, cam = d
            L.append("<text %s>%s</text>" % (a(x=f(x), y=f(y), size=f(tam), layer=cam), escape(t)))
        elif tipo == "wire":
            x1, y1, x2, y2, w, cam = d
            L.append("<wire %s/>" % a(x1=f(x1), y1=f(y1), x2=f(x2), y2=f(y2), width=f(w), layer=cam))
        elif tipo == "frame":
            x1, y1, x2, y2 = d
            L.append("<frame %s/>" % a(x1=f(x1), y1=f(y1), x2=f(x2), y2=f(y2), columns=8, rows=5, layer=94))
    L += ["</plain>", "<instances>"]
    for (pn, gp, x, y, rot) in E.instancias:
        L.append("<instance %s/>" % a(part=pn, gate=gp, x=f(x), y=f(y), rot=("R%d" % rot) if rot else None))
    L += ["</instances>", "<busses>", "</busses>", "<nets>"]
    for rede, segs in E.redes.items():
        L.append('<net name="%s" class="0">' % escape(rede))
        for seg in segs:
            L.append("<segment>")
            for it in seg:
                if it[0] == "pinref":
                    L.append("<pinref %s/>" % a(part=it[1], gate=it[2], pin=it[3]))
                elif it[0] == "wire":
                    (x1, y1), (x2, y2) = it[1], it[2]
                    L.append("<wire %s/>" % a(x1=f(x1), y1=f(y1), x2=f(x2), y2=f(y2), width="0.1524", layer=91))
                elif it[0] == "junction":
                    L.append("<junction %s/>" % a(x=f(it[1][0]), y=f(it[1][1])))
                elif it[0] == "label":
                    (x, y), rot = it[1], it[2]
                    L.append("<label %s/>" % a(x=f(x), y=f(y), size="1.778", layer=95, rot=("R%d" % rot) if rot else None))
            L.append("</segment>")
        L.append("</net>")
    L += ["</nets>", "</sheet>", "</sheets>", "</schematic>", "</drawing>", "</eagle>"]
    return "\n".join(L) + "\n"


def montar_plano():
    for (a1, b1, a2, b2) in ((0, 0, 279.4, 0), (279.4, 0, 279.4, 190.5), (279.4, 190.5, 0, 190.5), (0, 190.5, 0, 0)):
        E.plano.append(("wire", (a1, b1, a2, b2, 0.4064, 94)))
    for x, y, t in TITULOS:
        E.plano.append(("text", (x, y, t, 2.1844, 97)))
    for i, linha in enumerate(NOTAS):
        E.plano.append(("text", (7.62, 72.39 - i * 3.81, linha, 1.778 if i else 2.1844, 97)))
    # quadro de título
    x0, x1, y0, y1 = 182.88, 279.4, 0, 22.86  # encosta na borda de baixo e da direita
    for (a1, b1, a2, b2) in ((x0, y1, x1, y1), (x0, y0, x0, y1), (x0, 12.7, x1, 12.7)):
        E.plano.append(("wire", (a1, b1, a2, b2, 0.254, 94)))
    E.plano.append(("text", (185.42, 16.51, "SINO IFMS - CONTROLADOR DO SINO", 2.54, 94)))
    E.plano.append(("text", (185.42, 6.35, "NodeMCU v3 + DS3231 + rele 5V   Rev. 3   2026-11-13", 1.778, 94)))
    E.plano.append(("text", (185.42, 2.54, "Firmware: sino-gps/sino_gps", 1.778, 94)))


# ===================================================================== SVG
def svg_esquema(xml_texto):
    """Desenha o próprio arquivo .sch (lido de volta) em SVG, para conferir."""
    import xml.etree.ElementTree as ET
    raiz = ET.fromstring(xml_texto.split("\n", 2)[2])
    sch = raiz.find("drawing/schematic")
    lib = sch.find("libraries/library")
    simbolos = {s.get("name"): s for s in lib.find("symbols")}
    conjuntos = {d.get("name"): d for d in lib.find("devicesets")}
    pecas = {p.get("name"): p for p in sch.find("parts")}
    folha = sch.find("sheets/sheet")
    W, H, K = 279.4, 190.5, 6.0
    out = ['<svg xmlns="http://www.w3.org/2000/svg" width="%d" height="%d" viewBox="0 0 %d %d" '
           'font-family="Arial, Helvetica, sans-serif">' % (W * K, H * K, W * K, H * K),
           '<rect width="100%" height="100%" fill="#ffffff"/>']

    def X(x):
        return (x) * K

    def Y(y):
        return (H - y) * K

    def linha(x1, y1, x2, y2, cor, w):
        out.append('<line x1="%.1f" y1="%.1f" x2="%.1f" y2="%.1f" stroke="%s" stroke-width="%.2f" '
                   'stroke-linecap="round"/>' % (X(x1), Y(y1), X(x2), Y(y2), cor, max(w * K, 1.0)))

    ALIN = {"bottom-left": ("start", "auto"), "bottom-center": ("middle", "auto"), "bottom-right": ("end", "auto"),
            "center-left": ("start", "middle"), "center": ("middle", "middle"), "center-right": ("end", "middle"),
            "top-left": ("start", "hanging"), "top-center": ("middle", "hanging"), "top-right": ("end", "hanging")}

    def texto(x, y, t, tam, cor, alinha="bottom-left", rot=0, negrito=False):
        rot %= 360
        if rot == 180:  # Eagle mantém o texto legível
            rot = 0
            alinha = {"bottom-left": "top-right", "top-right": "bottom-left", "bottom-right": "top-left",
                      "top-left": "bottom-right", "center-left": "center-right", "center-right": "center-left",
                      "bottom-center": "top-center", "top-center": "bottom-center"}.get(alinha, alinha)
        if rot == 270:
            rot = 90
            alinha = {"bottom-left": "top-right", "top-right": "bottom-left", "bottom-right": "top-left",
                      "top-left": "bottom-right", "center-left": "center-right", "center-right": "center-left",
                      "bottom-center": "top-center", "top-center": "bottom-center"}.get(alinha, alinha)
        anc, base = ALIN.get(alinha, ("start", "auto"))
        tr = ' transform="rotate(-%d %.1f %.1f)"' % (rot, X(x), Y(y)) if rot else ""
        out.append('<text x="%.1f" y="%.1f" font-size="%.1f" fill="%s" text-anchor="%s" dominant-baseline="%s"%s%s>%s</text>'
                   % (X(x), Y(y), tam * K * 0.95, cor, anc, base, tr, ' font-weight="bold"' if negrito else "", escape(t)))

    plano = folha.find("plain")
    for e in plano:
        if e.tag == "frame":
            x1, y1, x2, y2 = (float(e.get(k)) for k in ("x1", "y1", "x2", "y2"))
            for (a1, b1, a2, b2) in ((x1, y1, x2, y1), (x2, y1, x2, y2), (x2, y2, x1, y2), (x1, y2, x1, y1)):
                linha(a1, b1, a2, b2, "#555", 0.3)
            for (a1, b1, a2, b2) in ((x1 + 4, y1 + 4, x2 - 4, y1 + 4), (x2 - 4, y1 + 4, x2 - 4, y2 - 4),
                                     (x2 - 4, y2 - 4, x1 + 4, y2 - 4), (x1 + 4, y2 - 4, x1 + 4, y1 + 4)):
                linha(a1, b1, a2, b2, "#555", 0.2)
        elif e.tag == "wire":
            linha(*(float(e.get(k)) for k in ("x1", "y1", "x2", "y2")), "#8a1c1c", float(e.get("width")))
        elif e.tag == "text":
            cam = e.get("layer")
            texto(float(e.get("x")), float(e.get("y")), e.text, float(e.get("size")),
                  "#1f3f8f" if cam == "97" else "#8a1c1c", negrito=float(e.get("size")) > 2)

    for inst in folha.find("instances"):
        parte = pecas[inst.get("part")]
        ds = conjuntos[parte.get("deviceset")]
        porta = [g for g in ds.find("gates") if g.get("name") == inst.get("gate")][0]
        sim = simbolos[porta.get("symbol")]
        ox, oy = float(inst.get("x")), float(inst.get("y"))
        ri = int((inst.get("rot") or "R0")[1:])

        def T(x, y):
            dx, dy = gira(x, y, ri)
            return ox + dx, oy + dy

        nome = parte.get("name")
        valor = parte.get("value") or parte.get("deviceset")
        for e in sim:
            if e.tag == "wire":
                x1, y1 = T(float(e.get("x1")), float(e.get("y1")))
                x2, y2 = T(float(e.get("x2")), float(e.get("y2")))
                linha(x1, y1, x2, y2, "#8a1c1c", float(e.get("width")))
            elif e.tag == "circle":
                cx, cy = T(float(e.get("x")), float(e.get("y")))
                out.append('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="none" stroke="#8a1c1c" stroke-width="%.2f"/>'
                           % (X(cx), Y(cy), float(e.get("radius")) * K, max(float(e.get("width")) * K, 1)))
            elif e.tag == "polygon":
                pts = [T(float(v.get("x")), float(v.get("y"))) for v in e]
                out.append('<polygon points="%s" fill="#8a1c1c"/>' % " ".join("%.1f,%.1f" % (X(px), Y(py)) for px, py in pts))
            elif e.tag == "text":
                t = e.text
                if t == ">NAME":
                    t, cor = nome, "#444"
                elif t == ">VALUE":
                    t, cor = valor, "#444"
                    if ds.get("prefix") in ("GND",):
                        continue
                else:
                    cor = "#8a1c1c"
                tx, ty = T(float(e.get("x")), float(e.get("y")))
                rt = int((e.get("rot") or "R0")[1:]) + ri
                texto(tx, ty, t, float(e.get("size")), cor, e.get("align") or "bottom-left", rt)
            elif e.tag == "pin":
                comp = COMPRIMENTO[e.get("length") or "long"]
                pr = int((e.get("rot") or "R0")[1:])
                px, py = float(e.get("x")), float(e.get("y"))
                ex, ey = gira(comp, 0, pr)
                a1 = T(px, py)
                a2 = T(px + ex, py + ey)
                if comp:
                    linha(a1[0], a1[1], a2[0], a2[1], "#8a1c1c", 0.1524)
                if e.get("visible") in ("pin", "both"):
                    nm = e.get("name").split("@")[0]
                    dx, dy = gira(comp + 0.762, 0, pr)
                    tx, ty = T(px + dx, py + dy)
                    ang = (pr + ri) % 360
                    if ang == 0:
                        texto(tx, ty, nm, 1.524, "#8a1c1c", "center-left", 0)
                    elif ang == 180:
                        texto(tx, ty, nm, 1.524, "#8a1c1c", "center-right", 0)
                    elif ang == 90:
                        texto(tx, ty, nm, 1.524, "#8a1c1c", "center-left", 90)
                    else:
                        texto(tx, ty, nm, 1.524, "#8a1c1c", "center-right", 90)
    for net in folha.find("nets"):
        nome = net.get("name")
        for seg in net:
            for e in seg:
                if e.tag == "wire":
                    linha(*(float(e.get(k)) for k in ("x1", "y1", "x2", "y2")), "#1a7a2e", 0.1524)
                elif e.tag == "junction":
                    out.append('<circle cx="%.1f" cy="%.1f" r="%.1f" fill="#1a7a2e"/>'
                               % (X(float(e.get("x"))), Y(float(e.get("y"))), 0.6 * K))
                elif e.tag == "label":
                    rt = int((e.get("rot") or "R0")[1:])
                    x, y = float(e.get("x")), float(e.get("y"))
                    texto(x + (0.5 if rt == 0 else -0.5), y + 0.4, nome, 1.778, "#1a7a2e", "bottom-left", rt)
    out.append("</svg>")
    return "\n".join(out)


def validar_xml(xml_texto):
    """Relê o .sch pronto e confere a consistência interna (como o Eagle faria ao abrir)."""
    import xml.etree.ElementTree as ET
    erros = []
    raiz = ET.fromstring(xml_texto.split("\n", 2)[2])
    sch = raiz.find("drawing/schematic")
    lib = sch.find("libraries/library")
    pacotes = {p.get("name"): {pad.get("name") for pad in p.iter("pad")} for p in lib.find("packages")}
    simbolos = {s.get("name"): {pin.get("name") for pin in s.iter("pin")} for s in lib.find("symbols")}
    conjuntos = {}
    for ds in lib.find("devicesets"):
        portas = {g.get("name"): g.get("symbol") for g in ds.find("gates")}
        for g, sim in portas.items():
            if sim not in simbolos:
                erros.append("%s: símbolo %s não existe" % (ds.get("name"), sim))
        variantes = {}
        for dev in ds.find("devices"):
            pac = dev.get("package")
            ligs = {(c.get("gate"), c.get("pin")): c.get("pad") for c in dev.iter("connect")}
            if pac:
                if pac not in pacotes:
                    erros.append("%s: pacote %s não existe" % (ds.get("name"), pac))
                todos = {(g, pin) for g, sim in portas.items() for pin in simbolos.get(sim, ())}
                if set(ligs) != todos:
                    erros.append("%s%s: pinos sem pad: %s" % (ds.get("name"), dev.get("name"), sorted(todos - set(ligs))))
                usados = []
                for pads in ligs.values():
                    for pad in pads.split():
                        if pad not in pacotes.get(pac, ()):
                            erros.append("%s: pad %s não existe em %s" % (ds.get("name"), pad, pac))
                        usados.append(pad)
                if len(usados) != len(set(usados)):
                    erros.append("%s: pad usado duas vezes" % ds.get("name"))
            variantes[dev.get("name")] = pac
        conjuntos[ds.get("name")] = (portas, variantes)
    pecas = {}
    for p in sch.find("parts"):
        ds, dev = p.get("deviceset"), p.get("device")
        if ds not in conjuntos or dev not in conjuntos[ds][1]:
            erros.append("peça %s: %s/%s não existe" % (p.get("name"), ds, dev))
            continue
        pecas[p.get("name")] = conjuntos[ds][0]
    folha = sch.find("sheets/sheet")
    colocadas = set()
    for inst in folha.find("instances"):
        pn, g = inst.get("part"), inst.get("gate")
        if pn not in pecas or g not in pecas[pn]:
            erros.append("instância %s/%s inválida" % (pn, g))
        colocadas.add((pn, g))
    for pn, portas in pecas.items():
        for g in portas:
            if (pn, g) not in colocadas:
                erros.append("porta %s de %s não foi colocada na folha" % (g, pn))
    vistos = {}
    nomes_redes = []
    for net in folha.find("nets"):
        nomes_redes.append(net.get("name"))
        for seg in net:
            for pr in seg.iter("pinref"):
                pn, g, pin = pr.get("part"), pr.get("gate"), pr.get("pin")
                if pn not in pecas or g not in pecas[pn] or pin not in simbolos[pecas[pn][g]]:
                    erros.append("pinref inválido %s/%s/%s" % (pn, g, pin))
                chave = (pn, g, pin)
                if chave in vistos and vistos[chave] != net.get("name"):
                    erros.append("pino %s em duas redes: %s e %s" % (chave, vistos[chave], net.get("name")))
                vistos[chave] = net.get("name")
    if len(nomes_redes) != len(set(nomes_redes)):
        erros.append("nome de rede repetido")
    return erros, len(vistos)


def main():
    aqui = os.path.dirname(os.path.abspath(__file__))
    montar_plano()
    erros = conferir()
    if erros:
        print("ERROS:")
        for e in erros:
            print("  " + e)
        sys.exit(1)
    xml = xml_esquema()
    erros, n_pinos = validar_xml(xml)
    if erros:
        print("ERROS NO ARQUIVO:")
        for e in erros:
            print("  " + e)
        sys.exit(1)
    with open(os.path.join(aqui, "Sino_IFMS_v2.sch"), "w", encoding="utf-8") as arq:
        arq.write(xml)
    with open(os.path.join(aqui, "esquema.svg"), "w", encoding="utf-8") as arq:
        arq.write(svg_esquema(xml))
    n_lig = sum(1 for r in E.redes)
    print("OK: %d pecas, %d redes, %d pinos ligados, %d simbolos -> Sino_IFMS_v2.sch e esquema.svg" %
          (len([p for p in E.pecas if not p.startswith(("GND", "P"))]), n_lig, n_pinos, len(SIMBOLOS)))


if __name__ == "__main__":
    main()
