plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.mapafama.gravador"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.mapafama.gravador"
        minSdk = 26          // Android 8: canais de notificação + AudioRecord moderno
        targetSdk = 34       // Android 14: exige declarar o tipo do serviço (microfone)
        versionCode = 1
        versionName = "1.0"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"), "proguard-rules.pro")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions { jvmTarget = "17" }

    buildFeatures { viewBinding = true }
}

dependencies {
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")
    implementation("com.google.android.material:material:1.12.0")

    // SAF: gravar o arquivo direto numa pasta escolhida por você (inclusive uma
    // pasta do Google Drive), sem precisar de OAuth nem Firebase.
    implementation("androidx.documentfile:documentfile:1.0.1")

    // Base do serviço em primeiro plano.
    implementation("androidx.lifecycle:lifecycle-service:2.8.4")

    // Geofence (cerca virtual): disparar um aviso ao chegar num local.
    implementation("com.google.android.gms:play-services-location:21.3.0")
}
