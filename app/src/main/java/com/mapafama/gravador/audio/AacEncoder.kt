package com.mapafama.gravador.audio

import android.media.MediaCodec
import android.media.MediaFormat
import android.media.MediaMuxer
import com.mapafama.gravador.Prefs
import java.io.File
import java.nio.ByteBuffer
import java.nio.ByteOrder

/**
 * Codifica PCM 16-bit em AAC e grava num arquivo .m4a (via MediaMuxer).
 *
 * O perfil (AAC-LC ou HE-AAC) e o bitrate vêm do [Codec] escolhido na tela.
 * Se [latitude]/[longitude] forem informados (opção "marcar posição GPS"), a
 * localização é embutida no próprio arquivo .m4a (como nas fotos com local).
 */
class AacEncoder(
    arquivo: File,
    codec: Codec,
    latitude: Float? = null,
    longitude: Float? = null
) {
    private val mediaCodec = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_AUDIO_AAC)
    private val muxer = MediaMuxer(arquivo.absolutePath, MediaMuxer.OutputFormat.MUXER_OUTPUT_MPEG_4)

    private var track = -1
    private var muxerIniciado = false
    private val info = MediaCodec.BufferInfo()
    private var ptsUs = 0L

    /** Milissegundos de áudio já gravados neste arquivo. */
    val duracaoMs: Long get() = ptsUs / 1000L

    init {
        // Grava a posição GPS dentro do arquivo, se fornecida.
        if (latitude != null && longitude != null) {
            runCatching { muxer.setLocation(latitude, longitude) }
        }
        val format = MediaFormat.createAudioFormat(
            MediaFormat.MIMETYPE_AUDIO_AAC, Prefs.SAMPLE_RATE, 1
        ).apply {
            setInteger(MediaFormat.KEY_AAC_PROFILE, codec.perfilAac)
            setInteger(MediaFormat.KEY_BIT_RATE, codec.bitrate)
            setInteger(MediaFormat.KEY_MAX_INPUT_SIZE, 16 * 1024)
        }
        mediaCodec.configure(format, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE)
        mediaCodec.start()
    }

    fun codificar(pcm: ShortArray, len: Int) {
        val bytes = ByteBuffer.allocate(len * 2).order(ByteOrder.LITTLE_ENDIAN)
        for (i in 0 until len) bytes.putShort(pcm[i])
        bytes.flip()
        alimentar(bytes, fim = false)
        drenar(fim = false)
    }

    fun finalizar() {
        alimentar(ByteBuffer.allocate(0), fim = true)
        drenar(fim = true)
        runCatching { if (muxerIniciado) muxer.stop() }
        runCatching { muxer.release() }
        runCatching { mediaCodec.stop() }
        runCatching { mediaCodec.release() }
    }

    private fun alimentar(data: ByteBuffer, fim: Boolean) {
        val idx = mediaCodec.dequeueInputBuffer(TIMEOUT)
        if (idx < 0) return
        val input = mediaCodec.getInputBuffer(idx) ?: return
        input.clear()
        val size = data.remaining()
        if (size > 0) input.put(data)
        val flags = if (fim) MediaCodec.BUFFER_FLAG_END_OF_STREAM else 0
        mediaCodec.queueInputBuffer(idx, 0, size, ptsUs, flags)
        ptsUs += (size / 2) * 1_000_000L / Prefs.SAMPLE_RATE
    }

    private fun drenar(fim: Boolean) {
        while (true) {
            val idx = mediaCodec.dequeueOutputBuffer(info, TIMEOUT)
            when {
                idx == MediaCodec.INFO_TRY_AGAIN_LATER -> if (!fim) return
                idx == MediaCodec.INFO_OUTPUT_FORMAT_CHANGED -> {
                    track = muxer.addTrack(mediaCodec.outputFormat)
                    muxer.start()
                    muxerIniciado = true
                }
                idx >= 0 -> {
                    val out = mediaCodec.getOutputBuffer(idx)
                    if (out != null && info.size > 0 && muxerIniciado &&
                        (info.flags and MediaCodec.BUFFER_FLAG_CODEC_CONFIG) == 0
                    ) {
                        out.position(info.offset)
                        out.limit(info.offset + info.size)
                        muxer.writeSampleData(track, out, info)
                    }
                    mediaCodec.releaseOutputBuffer(idx, false)
                    if ((info.flags and MediaCodec.BUFFER_FLAG_END_OF_STREAM) != 0) return
                }
            }
        }
    }

    companion object { private const val TIMEOUT = 10_000L }
}
