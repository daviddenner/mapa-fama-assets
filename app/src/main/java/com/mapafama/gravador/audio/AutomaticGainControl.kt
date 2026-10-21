package com.mapafama.gravador.audio

import kotlin.math.sqrt

/**
 * Controle Automático de Ganho (AGC), em software.
 *
 * Objetivo: manter a voz num nível parelho sem estourar.
 *  - Voz baixa / microfone longe -> o ganho sobe devagar (release).
 *  - Som alto / sala barulhenta  -> o ganho cai RÁPIDO assim que um quadro
 *                                   chega perto do máximo (attack), evitando a
 *                                   saturação e o "estalo".
 *
 * O ganho fica limitado entre MIN e MAX, para nunca amplificar silêncio puro.
 * Processa o quadro no próprio lugar (in-place).
 */
class AutomaticGainControl {

    private var ganho = 1.0f

    fun ganhoAtual(): Float = ganho

    fun processar(quadro: ShortArray, len: Int) {
        val rms = rms(quadro, len).toFloat()
        if (rms > 1f) {
            val desejado = (ALVO_RMS / rms).coerceIn(GANHO_MIN, GANHO_MAX)
            // Desce rápido (anti-estouro), sobe devagar (sem "bombear" nas pausas).
            val taxa = if (desejado < ganho) ATTACK else RELEASE
            ganho += taxa * (desejado - ganho)
        }
        for (i in 0 until len) {
            var v = (quadro[i] * ganho).toInt()
            if (v > Short.MAX_VALUE) v = Short.MAX_VALUE.toInt()
            else if (v < Short.MIN_VALUE) v = Short.MIN_VALUE.toInt()
            quadro[i] = v.toShort()
        }
    }

    private fun rms(quadro: ShortArray, len: Int): Double {
        var soma = 0.0
        for (i in 0 until len) { val s = quadro[i].toDouble(); soma += s * s }
        return sqrt(soma / len)
    }

    companion object {
        private const val ALVO_RMS = 6_000f
        private const val GANHO_MIN = 0.3f
        private const val GANHO_MAX = 8.0f
        private const val ATTACK = 0.25f
        private const val RELEASE = 0.02f
    }
}
