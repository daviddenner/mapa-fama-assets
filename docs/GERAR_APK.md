# Como abrir no Android Studio e gerar o APK

Guia rápido para compilar o Gravador de Áudio e instalar no celular (ex.: S22).

## 1. Instalar o Android Studio

Baixe em <https://developer.android.com/studio> e instale (Windows). Na
primeira abertura, deixe ele baixar o **Android SDK** que ele sugerir.

## 2. Pegar o projeto

O código está no GitHub, na branch `ccr-b6dfa0e5-7pb28p`. Duas opções:

- **Baixar ZIP:** no GitHub, troque para a branch `ccr-b6dfa0e5-7pb28p` →
  botão **Code** → **Download ZIP** → descompacte numa pasta.
- **Ou clonar (se usa Git):**
  ```
  git clone -b ccr-b6dfa0e5-7pb28p https://github.com/daviddenner/mapa-fama-assets.git
  ```

## 3. Abrir no Android Studio

1. **File → Open** (ou "Open" na tela inicial).
2. Selecione a **pasta raiz** do projeto — a que contém `settings.gradle.kts`
   (não a pasta `app`).
3. Confie no projeto se ele perguntar.

## 4. Deixar o Gradle sincronizar

- O Android Studio faz o **Gradle Sync** sozinho e **baixa as dependências**
  (precisa de internet). Pode levar alguns minutos na primeira vez.
- Se ele avisar que falta o **SDK 34** (Android 14), aceite instalar, ou vá em
  **Tools → SDK Manager** e marque o **Android 14 (API 34)**.
- Se aparecer algum erro de versão de plugin/Gradle, ele costuma oferecer um
  botão para corrigir ("Upgrade"/"Install"); pode aceitar.

> Se travar em algum erro que você não entenda, **copie a mensagem e me manda**
> aqui — eu ajusto o código. (Lembra que eu não consegui compilar no ambiente da
> nuvem, então pode pintar um ajuste pequeno.)

## 5. Gerar o APK

1. Menu **Build → Build Bundle(s) / APK(s) → Build APK(s)**.
2. Ao terminar, aparece um aviso "APK(s) generated" com um link **locate**.
3. O arquivo fica em:
   ```
   app/build/outputs/apk/debug/app-debug.apk
   ```

## 6. Instalar no celular

**Jeito A — direto pelo cabo (mais fácil para testar):**
1. No celular: **Ajustes → Sobre o telefone → toque 7x em "Número da versão"**
   para liberar o **Modo desenvolvedor**.
2. Em **Opções do desenvolvedor**, ligue a **Depuração USB**.
3. Conecte o celular no PC por USB e autorize no celular.
4. No Android Studio, escolha o aparelho no topo e clique no **▶ (Run)**.

**Jeito B — mandar o APK para o celular:**
1. Copie `app-debug.apk` para o celular (cabo, Drive, etc.).
2. Abra o arquivo no celular e permita **instalar de fontes desconhecidas**.

## 7. Primeira vez no app

1. Abra o app e conceda **Microfone** e **Notificações**.
2. Toque em **"Permitir rodar sem corte de bateria"** e aceite — é o que
   impede o Android de fechar a gravação.
3. Escolha o **codec**, ligue **cortar silêncios** se quiser.
4. Para nuvem: ligue **"Salvar na nuvem"**, toque em **Escolher pasta** e
   selecione uma pasta do seu Drive.
5. Para o aviso por local: ligue a cerca, conceda localização **"o tempo
   todo"**, e use **"Definir usando minha localização atual"**.

Pronto — toque em **Iniciar gravação** (ou use o atalho nos Ajustes Rápidos).
