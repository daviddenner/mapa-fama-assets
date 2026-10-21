package com.mapafama.gravador.geofence

import android.annotation.SuppressLint
import android.app.PendingIntent
import android.content.Context
import android.content.Intent
import com.google.android.gms.location.Geofence
import com.google.android.gms.location.GeofencingRequest
import com.google.android.gms.location.LocationServices

/**
 * Registra/remove a cerca virtual (geofence): uma circunferência (lat, lng,
 * raio) que, ao ser ENTRADA, dispara o [GeofenceBroadcastReceiver].
 *
 * Observação importante sobre o raio: a precisão típica do GPS é de 10 a 30 m.
 * Raios muito pequenos (ex.: 10 m) podem disparar tarde ou não disparar. O
 * Google recomenda raios a partir de ~100 m; valores menores são aceitos, mas
 * ficam menos confiáveis. O app deixa você escolher, com esse aviso.
 */
object GeofenceManager {

    private const val ID = "local_de_gravacao"

    @SuppressLint("MissingPermission") // localização (inclusive em 2º plano) checada antes de chamar
    fun registrar(ctx: Context, lat: Double, lng: Double, raioMetros: Float) {
        val cerca = Geofence.Builder()
            .setRequestId(ID)
            .setCircularRegion(lat, lng, raioMetros)
            .setExpirationDuration(Geofence.NEVER_EXPIRE)
            .setTransitionTypes(Geofence.GEOFENCE_TRANSITION_ENTER)
            .build()

        val pedido = GeofencingRequest.Builder()
            // Se você já estiver dentro ao ativar, também dispara.
            .setInitialTrigger(GeofencingRequest.INITIAL_TRIGGER_ENTER)
            .addGeofence(cerca)
            .build()

        LocationServices.getGeofencingClient(ctx).addGeofences(pedido, pendingIntent(ctx))
    }

    fun remover(ctx: Context) {
        LocationServices.getGeofencingClient(ctx).removeGeofences(pendingIntent(ctx))
    }

    private fun pendingIntent(ctx: Context): PendingIntent {
        val intent = Intent(ctx, GeofenceBroadcastReceiver::class.java)
        // MUTABLE: o sistema precisa anexar os dados do evento ao PendingIntent.
        return PendingIntent.getBroadcast(
            ctx, 7, intent,
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_MUTABLE
        )
    }
}
