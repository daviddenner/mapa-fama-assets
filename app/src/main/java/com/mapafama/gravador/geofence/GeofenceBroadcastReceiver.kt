package com.mapafama.gravador.geofence

import android.annotation.SuppressLint
import android.app.PendingIntent
import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import androidx.core.app.NotificationCompat
import com.google.android.gms.location.Geofence
import com.google.android.gms.location.GeofencingEvent
import com.mapafama.gravador.GravadorApp
import com.mapafama.gravador.R
import com.mapafama.gravador.service.RecordingService
import com.mapafama.gravador.ui.MainActivity

/**
 * Dispara quando você ENTRA no local da cerca.
 *
 * POR QUE NÃO GRAVA SOZINHO AQUI: a partir do Android 12, um app em segundo
 * plano NÃO pode abrir o microfone sozinho (é exatamente a mesma restrição que
 * fechava seu app). Então, em vez de tentar burlar o sistema (o que falharia),
 * a chegada no local dispara um AVISO destacado "Gravar?" — e você começa com
 * UM toque. Assim é confiável, fica no seu controle e você pode avisar quem
 * está por perto antes de iniciar.
 */
class GeofenceBroadcastReceiver : BroadcastReceiver() {

    @SuppressLint("MissingPermission") // POST_NOTIFICATIONS é pedida na tela; sem ela, apenas não aparece
    override fun onReceive(context: Context, intent: Intent) {
        val evento = GeofencingEvent.fromIntent(intent) ?: return
        if (evento.hasError()) return
        if (evento.geofenceTransition != Geofence.GEOFENCE_TRANSITION_ENTER) return

        // Ação da notificação: tocar inicia a gravação (iniciada por VOCÊ,
        // portanto permitida pelo sistema).
        val gravar = PendingIntent.getForegroundService(
            context, 11,
            Intent(context, RecordingService::class.java).setAction(RecordingService.ACTION_START),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )
        val abrir = PendingIntent.getActivity(
            context, 12, Intent(context, MainActivity::class.java),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )

        val notif = NotificationCompat.Builder(context, GravadorApp.CANAL_AVISO)
            .setSmallIcon(R.drawable.ic_mic)
            .setContentTitle(context.getString(R.string.aviso_chegou_titulo))
            .setContentText(context.getString(R.string.aviso_chegou_texto))
            .setPriority(NotificationCompat.PRIORITY_HIGH)
            .setAutoCancel(true)
            .setContentIntent(abrir)
            .addAction(R.drawable.ic_mic, context.getString(R.string.aviso_gravar), gravar)
            .build()

        androidx.core.app.NotificationManagerCompat.from(context).notify(2002, notif)
    }
}
