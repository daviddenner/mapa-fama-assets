package com.mapafama.gravador.audio

import kotlin.math.sqrt

/**
 * Detector de atividade de voz (VAD) por energia, com piso de ruído adaptativo.
 *
 * Aqui ele NÃO liga/desliga o app — ele só decide, DENTRO de uma gravação que
 * você iniciou, se o trecho atual é fala ou silêncio, para a gravação poder
 * pular os silêncios (economiza espaço).
 *
 * Em vez de um limiar fixo, ele aprende o nível de fundo da sala durante os
 * silêncios e marca "fala" quando um quadro sobe claramente acima desse fundo.
 */
class VoiceActivityDetector {

    private var pisoRuido = 180.0
    var ultimoRms: Double = 0.0
        private set

    fun ehFala(quadro: ShortArray, len: Int): Boolean {
        val rms = rms(quadro, len)
        ultimoRms = rms

        val limiar = (pisoRuido * 3.0).coerceAtLeast(180.0)
        val fala = rms > limiar

        // Atualiza o piso só fora da fala, para uma frase longa não "subir" o
        // piso e se calar sozinha.
        if (!fala) {
            val alpha = if (rms < pisoRuido) 0.3 else 0.05
            pisoRuido += alpha * (rms - pisoRuido)
        }
        return fala
    }

    private fun rms(quadro: ShortArray, len: Int): Double {
        var soma = 0.0
        for (i in 0 until len) { val s = quadro[i].toDouble(); soma += s * s }
        return sqrt(soma / len)
    }
}
