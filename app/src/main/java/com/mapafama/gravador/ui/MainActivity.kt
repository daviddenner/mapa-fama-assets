package com.mapafama.gravador.ui

import android.Manifest
import android.annotation.SuppressLint
import android.content.Intent
import android.content.pm.PackageManager
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.provider.Settings
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import com.mapafama.gravador.Prefs
import com.mapafama.gravador.R
import com.mapafama.gravador.audio.Codec
import com.mapafama.gravador.databinding.ActivityMainBinding
import com.mapafama.gravador.geofence.GeofenceManager
import com.mapafama.gravador.location.LocationTagger
import com.mapafama.gravador.service.RecordingService

/**
 * Tela principal (visível). Iniciar/parar e TODAS as opções:
 * codec, cortar silêncio, marcar GPS, salvar na nuvem (pasta), apagar local e
 * o botão de isenção de bateria.
 */
class MainActivity : AppCompatActivity() {

    private lateinit var b: ActivityMainBinding
    private val handler = Handler(Looper.getMainLooper())

    // Permissão de microfone -> se concedida, inicia a gravação.
    private val pedirMicrofone = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { concedida -> if (concedida) iniciarServico() }

    // Permissão de localização (para a opção GPS).
    private val pedirLocalizacao = registerForActivityResult(
        ActivityResultContracts.RequestMultiplePermissions()
    ) { /* se negar, a gravação só fica sem a marca de local */ }

    // Permissão de notificação (Android 13+), para a notificação fixa aparecer.
    private val pedirNotificacao = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { /* se negar, a gravação funciona, mas sem a notificação visível */ }

    // Localização "o tempo todo" (2º plano), necessária para a cerca virtual
    // disparar com o app fechado. No Android 11+ o sistema abre os Ajustes.
    private val pedirLocalizacaoFundo = registerForActivityResult(
        ActivityResultContracts.RequestPermission()
    ) { concedida -> if (concedida) registrarGeofenceSePossivel() }

    // Seletor de pasta de destino (SAF) — pode ser uma pasta do Drive.
    private val escolherPasta = registerForActivityResult(
        ActivityResultContracts.OpenDocumentTree()
    ) { uri: Uri? ->
        if (uri != null) {
            contentResolver.takePersistableUriPermission(
                uri, Intent.FLAG_GRANT_READ_URI_PERMISSION or Intent.FLAG_GRANT_WRITE_URI_PERMISSION
            )
            Prefs.setPastaDestino(this, uri.toString())
            mostrarPasta()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        b = ActivityMainBinding.inflate(layoutInflater)
        setContentView(b.root)

        configurarCodec()
        restaurarOpcoes()

        // Em Android 13+ pedimos a permissão de notificação (para a notificação
        // fixa de "gravando" aparecer).
        if (Build.VERSION.SDK_INT >= 33) {
            pedirNotificacao.launch(Manifest.permission.POST_NOTIFICATIONS)
        }

        b.toggleButton.setOnClickListener {
            if (RecordingService.gravando) {
                RecordingService.parar(this)
            } else {
                // Pede o microfone; se concedido, o callback inicia a gravação.
                pedirMicrofone.launch(Manifest.permission.RECORD_AUDIO)
            }
        }

        b.silenceCheck.setOnCheckedChangeListener { _, v -> Prefs.setCortarSilencio(this, v) }
        b.gpsCheck.setOnCheckedChangeListener { _, v ->
            Prefs.setMarcarGps(this, v)
            if (v) pedirLocalizacao.launch(
                arrayOf(Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION)
            )
        }
        b.cloudCheck.setOnCheckedChangeListener { _, v -> Prefs.setSalvarNuvem(this, v) }
        b.deleteLocalCheck.setOnCheckedChangeListener { _, v -> Prefs.setApagarLocal(this, v) }

        b.pickFolderButton.setOnClickListener { escolherPasta.launch(null) }
        b.batteryButton.setOnClickListener { pedirIsencaoBateria() }

        // ---- Cerca virtual (gravar ao chegar num local) ----
        b.useLocationButton.setOnClickListener { definirLocalAtual() }
        b.geofenceCheck.setOnCheckedChangeListener { _, ativo ->
            Prefs.setGeofenceAtivo(this, ativo)
            if (ativo) {
                // Precisa de localização "o tempo todo" para funcionar com o app fechado.
                if (Build.VERSION.SDK_INT >= 29 &&
                    ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_BACKGROUND_LOCATION)
                    != PackageManager.PERMISSION_GRANTED
                ) {
                    pedirLocalizacaoFundo.launch(Manifest.permission.ACCESS_BACKGROUND_LOCATION)
                } else {
                    registrarGeofenceSePossivel()
                }
            } else {
                GeofenceManager.remover(this)
            }
        }
    }

    /** Usa a localização atual + o raio digitado para definir o local da cerca. */
    private fun definirLocalAtual() {
        if (!temPermissaoLocalizacao()) {
            pedirLocalizacao.launch(
                arrayOf(Manifest.permission.ACCESS_FINE_LOCATION, Manifest.permission.ACCESS_COARSE_LOCATION)
            )
            return
        }
        val loc = LocationTagger.ultimaLocalizacao(this)
        if (loc == null) {
            Toast.makeText(this, "Sem localização ainda. Abra o mapa/GPS e tente de novo.", Toast.LENGTH_LONG).show()
            return
        }
        val raio = b.radiusInput.text.toString().toFloatOrNull()?.coerceAtLeast(10f) ?: 100f
        Prefs.setGeofenceLocal(this, loc.latitude, loc.longitude, raio)
        mostrarGeofence()
        if (b.geofenceCheck.isChecked) registrarGeofenceSePossivel()
    }

    /** Registra a cerca se ela estiver ligada, com local definido e permissões ok. */
    private fun registrarGeofenceSePossivel() {
        val local = Prefs.geofenceLocal(this)
        if (local == null) {
            Toast.makeText(this, "Defina o local primeiro.", Toast.LENGTH_SHORT).show()
            return
        }
        if (!temPermissaoLocalizacao()) return
        val (lat, lng, raio) = local
        GeofenceManager.registrar(this, lat, lng, raio)
    }

    private fun mostrarGeofence() {
        val local = Prefs.geofenceLocal(this)
        b.geoStatus.text = if (local == null) "(nenhum local definido)"
        else "local: %.5f, %.5f · raio %.0f m".format(local.first, local.second, local.third)
    }

    private fun temPermissaoLocalizacao() =
        ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_FINE_LOCATION) ==
            PackageManager.PERMISSION_GRANTED ||
        ContextCompat.checkSelfPermission(this, Manifest.permission.ACCESS_COARSE_LOCATION) ==
            PackageManager.PERMISSION_GRANTED

    private fun configurarCodec() {
        val adapter = ArrayAdapter(this, android.R.layout.simple_spinner_dropdown_item, Codec.values())
        b.codecSpinner.adapter = adapter
        b.codecSpinner.setSelection(Prefs.codec(this).ordinal)
        b.codecSpinner.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(p: AdapterView<*>?, v: android.view.View?, pos: Int, id: Long) {
                Prefs.setCodec(this@MainActivity, Codec.values()[pos])
            }
            override fun onNothingSelected(p: AdapterView<*>?) {}
        }
    }

    private fun restaurarOpcoes() {
        b.silenceCheck.isChecked = Prefs.cortarSilencio(this)
        b.gpsCheck.isChecked = Prefs.marcarGps(this)
        b.cloudCheck.isChecked = Prefs.salvarNuvem(this)
        b.deleteLocalCheck.isChecked = Prefs.apagarLocal(this)
        b.geofenceCheck.isChecked = Prefs.geofenceAtivo(this)
        Prefs.geofenceLocal(this)?.let { b.radiusInput.setText(it.third.toInt().toString()) }
        mostrarPasta()
        mostrarGeofence()
    }

    private fun mostrarPasta() {
        val uri = Prefs.pastaDestino(this)
        b.folderText.text = if (uri == null) "(nenhuma pasta escolhida)"
        else "destino: " + Uri.parse(uri).lastPathSegment
    }

    private fun iniciarServico() = RecordingService.iniciar(this)

    @SuppressLint("BatteryLife")
    private fun pedirIsencaoBateria() {
        // Abre a tela do sistema para você liberar o app da economia de bateria.
        // É o conserto do "o Android fecha, ainda mais em economia de bateria".
        runCatching {
            startActivity(
                Intent(Settings.ACTION_REQUEST_IGNORE_BATTERY_OPTIMIZATIONS)
                    .setData(Uri.parse("package:$packageName"))
            )
        }
    }

    // Atualiza o texto de status enquanto a tela está aberta.
    private val atualizador = object : Runnable {
        override fun run() {
            val gravando = RecordingService.gravando
            b.statusText.setText(if (gravando) R.string.status_gravando else R.string.status_parado)
            b.toggleButton.setText(if (gravando) R.string.parar else R.string.iniciar)
            handler.postDelayed(this, 500)
        }
    }

    override fun onResume() { super.onResume(); handler.post(atualizador) }
    override fun onPause() { super.onPause(); handler.removeCallbacks(atualizador) }
}
