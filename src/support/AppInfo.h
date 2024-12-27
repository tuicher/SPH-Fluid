// AppInfo.h
#pragma once
#include <vector>

/**
 * @brief Estructura que encapsula información
 *        sobre la aplicación (FOV, FPS, etc.)
 */
struct AppInfo
{
    float fov       = 45.0f;    // Campo de visión (FOV)
    float maxFPS    = 144.0f;   // Límite para graficar en la UI
    int   sampleIdx = 0;        // Índice para recorrer el array de FPS

    std::vector<float> fpsSamples; // Almacén de muestras de FPS
};
