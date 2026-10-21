package com.mapafama.gravador.audio

import android.media.MediaCodecInfo

/**
 * Opções de codec/qualidade que aparecem na tela (a opção "mudar o codec").
 *
 * Todas são da família AAC, gravadas em arquivo .m4a — um só caminho de
 * codificação, compatível com praticamente todo aparelho. Para VOZ:
 *
 *  - ALTA/MEDIA/ECONOMICA: AAC-LC em bitrates decrescentes.
 *  - MAXIMA_COMPRESSAO:    HE-AAC, que soa bem mesmo em bitrate bem baixo.
 *
 * A título de referência, 1 hora de voz dá, aproximadamente:
 *   64 kbps ≈ 28 MB · 32 kbps ≈ 14 MB · 16 kbps ≈ 7 MB · 12 kbps ≈ 5 MB.
 */
enum class Codec(
    val rotulo: String,
    val perfilAac: Int,
    val bitrate: Int
) {
    ALTA("AAC 64 kbps (alta qualidade)", MediaCodecInfo.CodecProfileLevel.AACObjectLC, 64_000),
    MEDIA("AAC 32 kbps (equilibrada)", MediaCodecInfo.CodecProfileLevel.AACObjectLC, 32_000),
    ECONOMICA("AAC 16 kbps (econômica)", MediaCodecInfo.CodecProfileLevel.AACObjectLC, 16_000),
    MAXIMA_COMPRESSAO("HE-AAC 12 kbps (máxima compressão)", MediaCodecInfo.CodecProfileLevel.AACObjectHE, 12_000);

    override fun toString() = rotulo
}
