# Gravador de Áudio

Aplicativo Android (Kotlin) de gravação de voz **iniciada por você**, pensado
para professor(a) registrar aulas e atendimentos. Visível, com notificação fixa
enquanto grava — nada roda escondido.

## O que ele faz

- **Você inicia a gravação** pelo botão do app ou pelo **atalho nos Ajustes
  Rápidos** (puxar a barra de cima — 1 toque). Nada de ligar sozinho por som ou
  por estar com o celular guardado.
- **Serviço em primeiro plano** (tipo microfone) + **isenção de bateria**: é o
  conserto para o problema de "o Android fecha o app, ainda mais em economia de
  bateria". Enquanto grava, a notificação fica visível e o sistema não encerra.
- **Opções (na tela):**
  - **Codec / qualidade:** de AAC 64 kbps até HE-AAC 12 kbps (máxima compressão).
  - **Cortar silêncios:** usa detecção de voz (VAD) para não gravar o silêncio,
    economizando espaço — aplicada *dentro* da gravação que você iniciou.
  - **Marcar posição GPS:** grava a localização dentro do arquivo `.m4a`
    (como em fotos com local). Só quando ligada e com permissão concedida.
  - **Salvar na nuvem:** escolha uma **pasta de destino** (pode ser uma pasta do
    Google Drive). O arquivo é copiado para lá e, se você marcar, apagado do
    celular depois de confirmado — para não lotar o armazenamento.
  - **Avisar para gravar ao chegar num local (cerca virtual / geofence):** você
    define um ponto + raio; ao chegar, o celular mostra um aviso destacado
    "Gravar?" e você começa com **um toque**. Veja a observação abaixo.
- Também faz **AGC** (controle automático de ganho, anti-estouro) e usa o
  **supressor de ruído** da plataforma, quando o aparelho tem.

## Sobre a cerca virtual (chegar e gravar)

A partir do Android 12, um app em segundo plano **não pode abrir o microfone
sozinho** — é a mesma regra do sistema que derrubava gravadores. Por isso a
chegada no local **não grava sozinha e escondida**: ela dispara um aviso e a
gravação começa quando **você toca** em "Gravar". Assim é confiável, fica no seu
controle e dá para avisar quem está por perto antes de iniciar.

> Dica de raio: o GPS costuma errar de 10 a 30 m. Raios muito pequenos (ex.:
> 10 m) podem falhar; ~100 m é mais confiável.

## Estrutura de arquivos

```
app/src/main/
├── AndroidManifest.xml
├── java/com/mapafama/gravador/
│   ├── GravadorApp.kt            # canais de notificação
│   ├── Prefs.kt                  # opções salvas + parâmetros do áudio
│   ├── audio/
│   │   ├── Codec.kt              # opções de codec/qualidade
│   │   ├── VoiceActivityDetector.kt   # detecção de voz (corte de silêncio)
│   │   ├── AutomaticGainControl.kt    # AGC (anti-estouro)
│   │   ├── ShortRingBuffer.kt         # buffer curto (não cortar início da fala)
│   │   ├── AacEncoder.kt              # codificação AAC + geotag no arquivo
│   │   └── RecordingEngine.kt         # laço de captura (junta tudo)
│   ├── service/RecordingService.kt    # serviço em primeiro plano (grava)
│   ├── location/LocationTagger.kt     # posição para o geotag / cerca
│   ├── storage/DestinationWriter.kt   # copiar para a pasta (Drive via SAF)
│   ├── geofence/
│   │   ├── GeofenceManager.kt         # registra/remove a cerca
│   │   └── GeofenceBroadcastReceiver.kt  # chegada no local -> aviso "Gravar?"
│   ├── tile/RecordTileService.kt      # atalho nos Ajustes Rápidos
│   └── ui/MainActivity.kt             # tela: iniciar/parar + todas as opções
└── res/…                               # layout, textos (pt-BR), ícones, tema
```

## Permissões (AndroidManifest.xml)

| Permissão | Para quê |
|---|---|
| `RECORD_AUDIO` | capturar o áudio |
| `FOREGROUND_SERVICE` + `FOREGROUND_SERVICE_MICROPHONE` | gravar sem ser encerrado |
| `POST_NOTIFICATIONS` | mostrar a notificação de "gravando" |
| `ACCESS_FINE/COARSE_LOCATION` | opção de marcar GPS e definir a cerca |
| `ACCESS_BACKGROUND_LOCATION` | cerca disparar com o app fechado |
| `REQUEST_IGNORE_BATTERY_OPTIMIZATIONS` | pedir isenção da economia de bateria |
| `WAKE_LOCK` | manter a CPU durante a gravação |

Não há permissão de `INTERNET` nem de `BOOT`: o app não liga sozinho na
inicialização e não fala com servidores externos — o envio à nuvem é a cópia
para a pasta do Drive (o app do Drive cuida da rede).

## Como configurar (resumo)

1. Instale e abra o app uma vez; conceda **microfone** e **notificações**.
2. Toque em **"Permitir rodar sem corte de bateria"** e aceite — isso resolve o
   fechamento pelo Android.
3. Escolha o **codec**, ligue **cortar silêncios** se quiser.
4. Para nuvem: ligue **"Salvar na nuvem"**, toque em **"Escolher pasta"** e
   selecione uma pasta do seu Drive.
5. Para GPS/cerca: ligue as opções e conceda a localização ("o tempo todo" para
   a cerca).

## Aviso legal (importante e a seu favor)

Gravar aulas e atendimentos é legítimo, mas grava a voz de outras pessoas
(muitas vezes estudantes, às vezes menores de idade). Avise quem está presente,
e verifique a **política da sua instituição** e a **LGPD** (dados de crianças e
adolescentes têm proteção reforçada). Gravar de forma transparente é o que
protege você — e é por isso que este app é visível e iniciado por você.
