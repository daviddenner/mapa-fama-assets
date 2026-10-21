package com.mapafama.gravador.audio

/**
 * Buffer circular curto (poucos segundos) de amostras PCM.
 *
 * Uso: quando o corte de silêncio está ligado e a fala recomeça depois de uma
 * pausa, usamos este buffer para recuperar a fração de segundo ANTES de a fala
 * ser detectada, evitando cortar o começo da palavra. É um buffer pequeno,
 * usado só dentro de uma gravação já iniciada por você.
 */
class ShortRingBuffer(private val capacidade: Int) {

    private val dados = ShortArray(capacidade)
    private var escrita = 0
    private var tamanho = 0

    fun escrever(src: ShortArray, len: Int) {
        for (i in 0 until len) {
            dados[escrita] = src[i]
            escrita = (escrita + 1) % capacidade
            if (tamanho < capacidade) tamanho++
        }
    }

    /** Devolve o conteúdo (mais antigo primeiro) e esvazia o buffer. */
    fun drenar(): ShortArray {
        val out = ShortArray(tamanho)
        val inicio = ((escrita - tamanho) % capacidade + capacidade) % capacidade
        for (i in 0 until tamanho) out[i] = dados[(inicio + i) % capacidade]
        limpar()
        return out
    }

    fun limpar() { escrita = 0; tamanho = 0 }
}
