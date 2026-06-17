// =============================================================================
//  app/build.gradle.kts  --  the parts that matter for the NDK/native build.
// -----------------------------------------------------------------------------
//  Trimmed to the essentials. A real module also has the usual Android Gradle
//  Plugin boilerplate (dependencies, etc.) that Android Studio generates.
//  GPLv2. See LICENSE.
// =============================================================================

plugins {
    id("com.android.application")
    kotlin("android")
}

android {
    namespace = "com.example.doom"
    compileSdk = 34

    defaultConfig {
        applicationId = "com.example.doom"
        minSdk = 24
        targetSdk = 34

        // Tell Gradle which ABIs to build the native library for.
        ndk { abiFilters += listOf("arm64-v8a", "x86_64") }
    }

    // Point Gradle at our CMakeLists.txt so it compiles the engine + JNI bridge.
    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions { jvmTarget = "17" }
}
