package com.mapafama.gravador.service

import android.Manifest
import android.app.Notification
import android.app.NotificationManager
import android.app.PendingIntent
import android.app.Service
import android.content.Context
import android.content.Intent
import android.content.pm.PackageManager
import android.content.pm.ServiceInfo
import android.os.Build
import android.os.IBinder
import androidx.core.app.NotificationCompat
import androidx.core.content.ContextCompat
import com.mapafama.gravador.GravadorApp
import com.mapafama.gravador.Prefs
import com.mapafama.gravador.R
import com.mapafama.gravador.audio.RecordingEngine
import com.mapafama.gravador.location.LocationTagger
import com.mapafama.gravador.storage.DestinationWriter
import com.mapafama.gravador.ui.MainActivity
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale
import java.util.concurrent.Executors

/**
 * Serviço em PRIMEIRO PLANO que faz a gravação. A notificação fixa (obrigatória
 * para microfone) deixa tudo visível. Enquanto este serviço está em primeiro
 * plano, o Android NÃO encerra a gravação — é este o conserto do "o Android
 * fecha o app".
 *
 * A gravação só acontece entre um START e um STOP disparados por você (botão ou
 * atalho). Não há início automático por boot, por som ou por localização.
 */
class RecordingService : Service() {

    private var engine: RecordingEngine? = null
    private var arquivoAtual: File? = null
    private val pool = Executors.newSingleThreadExecutor()

    override fun onBind(intent: Intent?): IBinder? = null

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_STOP -> { pararGravacao(); return START_NOT_STICKY }
            else -> iniciarGravacao()
        }
        // START_REDELIVER_INTENT: se o sistema matar o serviço no meio (pressão
        // extrema de memória), ele reentrega o último START e a SUA sessão
        // recomeça num novo arquivo. Não é "always-on": só revive uma gravação
        // que VOCÊ iniciou.
        return START_REDELIVER_INTENT
    }

    private fun iniciarGravacao() {
        if (engine != null) return

        // Vamos para primeiro plano JÁ, para cumprir a exigência do sistema de
        // chamar startForeground logo após startForegroundService.
        irParaPrimeiroPlano(falaAtiva = true)

        // Sem permissão de microfone, não há o que fazer.
        if (ContextCompat.checkSelfPermission(this, Manifest.permission.RECORD_AUDIO)
            != PackageManager.PERMISSION_GRANTED
        ) { stopForegroundCompat(); stopSelf(); return }

        // Posição GPS (só se a opção estiver ligada e a permissão concedida).
        var lat: Float? = null; var lng: Float? = null
        if (Prefs.marcarGps(this) && temPermissaoLocalizacao()) {
            LocationTagger.posicaoAtual(this)?.let { lat = it.lat; lng = it.lng }
        }

        val nome = "grav_" + SimpleDateFormat("yyyyMMdd_HHmmss", Locale.US).format(Date()) + ".m4a"
        val arquivo = File(File(filesDir, "gravacoes").apply { mkdirs() }, nome)
        arquivoAtual = arquivo

        engine = RecordingEngine(
            arquivo = arquivo,
            codec = Prefs.codec(this),
            cortarSilencio = Prefs.cortarSilencio(this),
            latitude = lat,
            longitude = lng
        ) { gravandoFala, _ ->
            atualizarNotificacao(gravandoFala)
        }.also { it.iniciar() }

        gravando = true
    }

    private fun pararGravacao() {
        val eng = engine ?: run { stopSelf(); return }
        engine = null
        gravando = false
        val arquivo = eng.parar()

        // Pós-processamento fora da thread principal: metadados + envio ao destino.
        pool.execute {
            escreverSidecar(arquivo)
            val uri = Prefs.pastaDestino(this)
            if (Prefs.salvarNuvem(this) && uri != null) {
                val ok = DestinationWriter.copiarParaDestino(this, arquivo, uri)
                // Só apaga o local se foi pedido E a cópia deu certo.
                if (ok && Prefs.apagarLocal(this)) arquivo.delete()
            }
        }
        stopForegroundCompat()
        stopSelf()
    }

    /** Guarda um arquivo .txt ao lado, com data/hora e local (se houver). */
    private fun escreverSidecar(arquivo: File) {
        runCatching {
            val info = buildString {
                append("arquivo=").append(arquivo.name).append('\n')
                append("data=").append(Date(arquivo.lastModified())).append('\n')
                append("codec=").append(Prefs.codec(this@RecordingService).rotulo).append('\n')
            }
            File(arquivo.parentFile, arquivo.nameWithoutExtension + ".txt").writeText(info)
        }
    }

    // ---- Notificação -------------------------------------------------------

    private fun irParaPrimeiroPlano(falaAtiva: Boolean) {
        val notif = construirNotificacao(falaAtiva)
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.Q) {
            startForeground(ID, notif, ServiceInfo.FOREGROUND_SERVICE_TYPE_MICROPHONE)
        } else {
            startForeground(ID, notif)
        }
    }

    private fun atualizarNotificacao(falaAtiva: Boolean) {
        if (!gravando) return
        val nm = getSystemService(NotificationManager::class.java)
        nm.notify(ID, construirNotificacao(falaAtiva))
    }

    private fun construirNotificacao(falaAtiva: Boolean): Notification {
        val abrir = PendingIntent.getActivity(
            this, 0, Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )
        val parar = PendingIntent.getService(
            this, 1, Intent(this, RecordingService::class.java).setAction(ACTION_STOP),
            PendingIntent.FLAG_UPDATE_CURRENT or PendingIntent.FLAG_IMMUTABLE
        )
        val texto = if (falaAtiva) R.string.notif_gravando else R.string.notif_ouvindo
        return NotificationCompat.Builder(this, GravadorApp.CANAL)
            .setSmallIcon(R.drawable.ic_mic)
            .setContentTitle(getString(R.string.app_name))
            .setContentText(getString(texto))
            .setOngoing(true)
            .setContentIntent(abrir)
            .addAction(R.drawable.ic_mic, getString(R.string.notif_parar), parar)
            .build()
    }

    private fun stopForegroundCompat() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.N) {
            stopForeground(STOP_FOREGROUND_REMOVE)
        } else {
            @Suppress("DEPRECATION") stopForeground(true)
        }
    }

    private fun temPermissaoLocalizacao() =
        ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) ==
            PackageManager.PERMISSION_GRANTED ||
        ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) ==
            PackageManager.PERMISSION_GRANTED

    override fun onDestroy() {
        engine?.parar()
        engine = null
        gravando = false
        pool.shutdown()
        super.onDestroy()
    }

    companion object {
        private const val ID = 1001
        const val ACTION_START = "com.mapafama.gravador.START"
        const val ACTION_STOP = "com.mapafama.gravador.STOP"

        /** Estado simples para a tela e o atalho saberem se está gravando. */
        @Volatile var gravando: Boolean = false
            private set

        fun iniciar(ctx: Context) {
            val i = Intent(ctx, RecordingService::class.java).setAction(ACTION_START)
            ContextCompat.startForegroundService(ctx, i)
        }

        fun parar(ctx: Context) {
            val i = Intent(ctx, RecordingService::class.java).setAction(ACTION_STOP)
            ContextCompat.startForegroundService(ctx, i)
        }
    }
}
