package com.mapafama.gravador

import android.app.Application
import android.app.NotificationChannel
import android.app.NotificationManager
import android.os.Build

/** Application: cria o canal de notificação usado pela gravação. */
class GravadorApp : Application() {

    override fun onCreate() {
        super.onCreate()
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            // IMPORTANCE_LOW: silencioso (não apita a cada atualização), mas
            // sempre visível na barra de notificações — é a transparência que
            // queremos.
            val nm = getSystemService(NotificationManager::class.java)

            // Canal silencioso da gravação em andamento.
            val canal = NotificationChannel(
                CANAL,
                getString(R.string.canal_gravacao),
                NotificationManager.IMPORTANCE_LOW
            ).apply { description = getString(R.string.canal_gravacao_desc) }
            nm.createNotificationChannel(canal)

            // Canal "alto" do aviso ao chegar num local (aparece na frente, com som),
            // para você perceber e tocar em "Gravar".
            val aviso = NotificationChannel(
                CANAL_AVISO,
                getString(R.string.canal_aviso),
                NotificationManager.IMPORTANCE_HIGH
            ).apply { description = getString(R.string.canal_aviso_desc) }
            nm.createNotificationChannel(aviso)
        }
    }

    companion object {
        const val CANAL = "gravacao"
        const val CANAL_AVISO = "aviso_local"
    }
}
