package com.mapafama.gravador.audio

import android.annotation.SuppressLint
import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import android.media.audiofx.NoiseSuppressor
import android.util.Log
import com.mapafama.gravador.Prefs
import java.io.File
import java.util.concurrent.atomic.AtomicBoolean

/**
 * Motor de gravação. Roda em uma thread própria:
 *
 *   ler quadro -> AGC -> (opcional) cortar silêncio via VAD -> codificar AAC
 *
 * É SEMPRE iniciado por você (botão ou atalho). Não escuta sozinho em segundo
 * plano: só captura entre [iniciar] e [parar].
 */
class RecordingEngine(
    private val arquivo: File,
    private val codec: Codec,
    private val cortarSilencio: Boolean,
    private val latitude: Float?,
    private val longitude: Float?,
    private val onEstado: (gravandoFala: Boolean, rms: Double) -> Unit
) {
    private val rodando = AtomicBoolean(false)
    private var thread: Thread? = null
    private var encoder: AacEncoder? = null
    private var supressorRuido: NoiseSuppressor? = null

    // Buffer curto (0,5 s) para não cortar o início da palavra ao voltar do silêncio.
    private val preroll = ShortRingBuffer(Prefs.SAMPLE_RATE / 2)
    private val vad = VoiceActivityDetector()
    private val agc = AutomaticGainControl()

    private var emFala = false
    private var hangoverMs = 0

    @SuppressLint("MissingPermission") // a permissão RECORD_AUDIO é checada antes, no serviço/UI
    fun iniciar() {
        if (rodando.getAndSet(true)) return
        arquivo.parentFile?.mkdirs()
        encoder = AacEncoder(arquivo, codec, latitude, longitude)

        val minBuf = AudioRecord.getMinBufferSize(
            Prefs.SAMPLE_RATE, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT
        )
        val record = AudioRecord(
            MediaRecorder.AudioSource.VOICE_RECOGNITION, // sinal mais "cru" para o nosso AGC cuidar
            Prefs.SAMPLE_RATE, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT,
            maxOf(minBuf, Prefs.FRAME_SAMPLES * 2 * 8)
        )
        // Supressor de ruído da plataforma, se o aparelho tiver.
        if (NoiseSuppressor.isAvailable()) {
            supressorRuido = NoiseSuppressor.create(record.audioSessionId)?.apply { enabled = true }
        }

        thread = Thread({ laco(record) }, "gravador").apply {
            priority = Thread.MAX_PRIORITY
            start()
        }
    }

    /** Para a captura, finaliza o arquivo e devolve o File gravado. */
    fun parar(): File {
        rodando.set(false)
        thread?.join(2_000)
        thread = null
        encoder?.finalizar()
        encoder = null
        supressorRuido?.release(); supressorRuido = null
        return arquivo
    }

    private fun laco(record: AudioRecord) {
        val quadro = ShortArray(Prefs.FRAME_SAMPLES)
        try {
            record.startRecording()
            while (rodando.get()) {
                val n = record.read(quadro, 0, quadro.size)
                if (n <= 0) continue
                agc.processar(quadro, n)

                if (!cortarSilencio) {
                    // Grava tudo, sem pular silêncios.
                    encoder?.codificar(quadro, n)
                    onEstado(true, vad.ultimoRms)
                    continue
                }

                // Com corte de silêncio: máquina de estados fala/silêncio.
                preroll.escrever(quadro, n)
                val fala = vad.ehFala(quadro, n)
                if (!emFala) {
                    if (fala) {
                        val anterior = preroll.drenar()          // não corta o começo da fala
                        encoder?.codificar(anterior, anterior.size)
                        encoder?.codificar(quadro, n)
                        emFala = true
                        hangoverMs = Prefs.HANGOVER_SILENCIO_MS
                        onEstado(true, vad.ultimoRms)
                    } else {
                        onEstado(false, vad.ultimoRms)           // silêncio: não grava
                    }
                } else {
                    encoder?.codificar(quadro, n)
                    if (fala) {
                        hangoverMs = Prefs.HANGOVER_SILENCIO_MS
                    } else {
                        hangoverMs -= n * 1000 / Prefs.SAMPLE_RATE
                        if (hangoverMs <= 0) {
                            preroll.limpar()
                            emFala = false
                            onEstado(false, vad.ultimoRms)
                        }
                    }
                }
            }
        } catch (t: Throwable) {
            Log.e("RecordingEngine", "falha no laço de captura", t)
        } finally {
            runCatching { record.stop() }
            runCatching { record.release() }
        }
    }
}
