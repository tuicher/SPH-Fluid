#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>

/**
 * @brief Lee el contenido de un archivo de texto y lo devuelve como std::string.
 *
 * @param filePath Ruta al archivo (vertex shader o fragment shader).
 * @return std::string Contenido completo del fichero.
 * @throws std::runtime_error si no puede abrir el archivo.
 */
static std::string LoadFileAsString(const std::string& filePath)
{
    std::ifstream file(filePath);
    if (!file.is_open())
    {
        throw std::runtime_error("No se pudo abrir el archivo: " + filePath);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}
