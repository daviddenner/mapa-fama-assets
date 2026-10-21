package com.mapafama.gravador

import android.content.Context
import com.mapafama.gravador.audio.Codec

/**
 * Guarda as escolhas do usuário (as opções da tela) em SharedPreferences, e
 * também centraliza alguns parâmetros fixos do áudio.
 */
object Prefs {

    // ---- Formato de captura (fixo) ----------------------------------------
    // 16 kHz mono: ideal para voz — inteligível e leve.
    const val SAMPLE_RATE = 16_000
    const val FRAME_SAMPLES = SAMPLE_RATE / 50   // 20 ms por quadro

    // Quanto tempo de silêncio contínuo até pausar a gravação (corte de silêncio).
    const val HANGOVER_SILENCIO_MS = 1_500

    private const val ARQ = "gravador_prefs"
    private const val K_CODEC = "codec"
    private const val K_CORTAR_SILENCIO = "cortar_silencio"
    private const val K_GPS = "gps"
    private const val K_NUVEM = "nuvem"
    private const val K_APAGAR_LOCAL = "apagar_local"
    private const val K_PASTA = "pasta_destino_uri"

    private fun p(ctx: Context) = ctx.getSharedPreferences(ARQ, Context.MODE_PRIVATE)

    var Context.codec: Codec
        get() = runCatching { Codec.valueOf(p(this).getString(K_CODEC, null) ?: "") }
            .getOrDefault(Codec.ECONOMICA)
        set(v) { p(this).edit().putString(K_CODEC, v.name).apply() }

    fun codec(ctx: Context): Codec = ctx.codec
    fun setCodec(ctx: Context, c: Codec) { ctx.codec = c }

    fun cortarSilencio(ctx: Context) = p(ctx).getBoolean(K_CORTAR_SILENCIO, true)
    fun setCortarSilencio(ctx: Context, v: Boolean) = p(ctx).edit().putBoolean(K_CORTAR_SILENCIO, v).apply()

    fun marcarGps(ctx: Context) = p(ctx).getBoolean(K_GPS, false)
    fun setMarcarGps(ctx: Context, v: Boolean) = p(ctx).edit().putBoolean(K_GPS, v).apply()

    fun salvarNuvem(ctx: Context) = p(ctx).getBoolean(K_NUVEM, false)
    fun setSalvarNuvem(ctx: Context, v: Boolean) = p(ctx).edit().putBoolean(K_NUVEM, v).apply()

    fun apagarLocal(ctx: Context) = p(ctx).getBoolean(K_APAGAR_LOCAL, false)
    fun setApagarLocal(ctx: Context, v: Boolean) = p(ctx).edit().putBoolean(K_APAGAR_LOCAL, v).apply()

    /** URI (SAF) da pasta de destino escolhida — pode ser uma pasta do Drive. */
    fun pastaDestino(ctx: Context): String? = p(ctx).getString(K_PASTA, null)
    fun setPastaDestino(ctx: Context, uri: String?) = p(ctx).edit().putString(K_PASTA, uri).apply()

    // ---- Cerca virtual (geofence) -----------------------------------------
    private const val K_GEO_ATIVO = "geo_ativo"
    private const val K_GEO_LAT = "geo_lat"
    private const val K_GEO_LNG = "geo_lng"
    private const val K_GEO_RAIO = "geo_raio"

    fun geofenceAtivo(ctx: Context) = p(ctx).getBoolean(K_GEO_ATIVO, false)
    fun setGeofenceAtivo(ctx: Context, v: Boolean) = p(ctx).edit().putBoolean(K_GEO_ATIVO, v).apply()

    /** Local salvo da cerca, ou null se nunca foi definido. */
    fun geofenceLocal(ctx: Context): Triple<Double, Double, Float>? {
        val pr = p(ctx)
        if (!pr.contains(K_GEO_LAT)) return null
        val lat = java.lang.Double.longBitsToDouble(pr.getLong(K_GEO_LAT, 0))
        val lng = java.lang.Double.longBitsToDouble(pr.getLong(K_GEO_LNG, 0))
        val raio = pr.getFloat(K_GEO_RAIO, 100f)
        return Triple(lat, lng, raio)
    }

    fun setGeofenceLocal(ctx: Context, lat: Double, lng: Double, raio: Float) {
        p(ctx).edit()
            .putLong(K_GEO_LAT, java.lang.Double.doubleToRawLongBits(lat))
            .putLong(K_GEO_LNG, java.lang.Double.doubleToRawLongBits(lng))
            .putFloat(K_GEO_RAIO, raio)
            .apply()
    }
}
