package com.mapafama.gravador.location

import android.annotation.SuppressLint
import android.content.Context
import android.location.LocationManager

/**
 * Pega uma posição aproximada no início da gravação, para a opção "marcar a
 * posição GPS". Usa a última localização conhecida do sistema (sem manter o GPS
 * ligado o tempo todo, para poupar bateria).
 */
object LocationTagger {

    data class Posicao(val lat: Float, val lng: Float)

    @SuppressLint("MissingPermission") // permissão de localização checada antes de chamar
    fun posicaoAtual(ctx: Context): Posicao? {
        val loc = ultimaLocalizacao(ctx) ?: return null
        return Posicao(loc.latitude.toFloat(), loc.longitude.toFloat())
    }

    /** Localização bruta (precisão Double), usada para definir a cerca virtual. */
    @SuppressLint("MissingPermission")
    fun ultimaLocalizacao(ctx: Context): android.location.Location? {
        val lm = ctx.getSystemService(Context.LOCATION_SERVICE) as? LocationManager ?: return null
        val provedores = listOf(LocationManager.GPS_PROVIDER, LocationManager.NETWORK_PROVIDER)
        return provedores
            .mapNotNull { p -> runCatching { lm.getLastKnownLocation(p) }.getOrNull() }
            .maxByOrNull { it.time }
    }
}
