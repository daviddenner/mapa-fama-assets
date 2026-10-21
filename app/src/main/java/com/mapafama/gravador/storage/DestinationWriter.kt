package com.mapafama.gravador.storage

import android.content.Context
import android.net.Uri
import androidx.documentfile.provider.DocumentFile
import java.io.File

/**
 * Copia a gravação finalizada para a pasta de destino que você escolheu pelo
 * seletor do Android (SAF). Essa pasta pode ser uma pasta do Google Drive — e,
 * nesse caso, é o próprio app do Drive que faz o envio pela rede. Não
 * precisamos de OAuth, Firebase, nem permissão de internet.
 *
 * Fluxo: grava localmente -> copia para o destino -> (opcional) apaga o local.
 */
object DestinationWriter {

    /**
     * @return true se copiou com sucesso para o destino.
     */
    fun copiarParaDestino(ctx: Context, origem: File, pastaUri: String): Boolean {
        return try {
            val pasta = DocumentFile.fromTreeUri(ctx, Uri.parse(pastaUri)) ?: return false
            if (!pasta.canWrite()) return false

            val destino = pasta.createFile("audio/mp4", origem.name) ?: return false
            ctx.contentResolver.openOutputStream(destino.uri)?.use { saida ->
                origem.inputStream().use { entrada -> entrada.copyTo(saida) }
            } ?: return false
            true
        } catch (e: Exception) {
            false
        }
    }
}
