package com.mapafama.gravador.tile

import android.service.quicksettings.Tile
import android.service.quicksettings.TileService
import com.mapafama.gravador.service.RecordingService

/**
 * Atalho no painel de Ajustes Rápidos (puxar de cima). Um toque inicia/para a
 * gravação — útil para os atendimentos, sem precisar abrir o app.
 *
 * O toque é uma ação SUA, então o sistema permite iniciar o serviço de
 * microfone a partir daqui. Se a permissão de microfone ainda não foi dada, o
 * serviço simplesmente não grava — abra o app uma vez para conceder.
 */
class RecordTileService : TileService() {

    override fun onStartListening() {
        super.onStartListening()
        atualizar()
    }

    override fun onClick() {
        super.onClick()
        if (RecordingService.gravando) RecordingService.parar(this)
        else RecordingService.iniciar(this)
        atualizar()
    }

    private fun atualizar() {
        qsTile?.apply {
            state = if (RecordingService.gravando) Tile.STATE_ACTIVE else Tile.STATE_INACTIVE
            updateTile()
        }
    }
}
