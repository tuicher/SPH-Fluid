// Shader.cpp
#include "Shader.h"
#include "../support/Loader.h"
#include <iostream>

Shader::~Shader()
{
    if (m_ProgramID != 0)
    {
        glDeleteProgram(m_ProgramID);
    }
}

bool Shader::CreateShaderProgram(const char* vertexSource, const char* fragmentSource)
{
    // Crear y compilar vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSource, nullptr);
    glCompileShader(vertexShader);
    if (!CheckCompileErrors(vertexShader, "VERTEX")) return false;

    // Crear y compilar fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSource, nullptr);
    glCompileShader(fragmentShader);
    if (!CheckCompileErrors(fragmentShader, "FRAGMENT")) return false;

    // Crear programa y linkear
    m_ProgramID = glCreateProgram();
    glAttachShader(m_ProgramID, vertexShader);
    glAttachShader(m_ProgramID, fragmentShader);
    glLinkProgram(m_ProgramID);
    if (!CheckCompileErrors(m_ProgramID, "PROGRAM")) return false;

    // Limpiar
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return true;
}

bool Shader::CreateShaderProgramFromFiles(const std::string& vertexPath, const std::string& fragmentPath)
{
    try
    {
        std::string vertexCode = LoadFileAsString(vertexPath);
        std::string fragmentCode = LoadFileAsString(fragmentPath);

        return CreateShaderProgram(vertexCode.c_str(), fragmentCode.c_str());
    }
    catch (const std::exception& e)
    {
        std::cerr << "[Shader] Error: " << e.what() << std::endl;
        return false;
    }
}

void Shader::Use() const
{
    glUseProgram(m_ProgramID);
}

bool Shader::CheckCompileErrors(GLuint shader, const std::string& type)
{
    GLint success;
    GLchar infoLog[1024];
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "Error de compilación en shader " << type << "\n" << infoLog << std::endl;
            return false;
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            std::cerr << "Error de linkeo en programa:\n" << infoLog << std::endl;
            return false;
        }
    }
    return true;
}

GLint Shader::GetUniformLocation(const std::string& uniformName) const
{
    GLint loc = glGetUniformLocation(m_ProgramID, uniformName.c_str());
    if (loc == -1)
    {
        // Opcionalmente, puedes loguear un warning.
        std::cerr << "Warning: uniform '" << uniformName << "' not found." << std::endl;
    }
    return loc;
}

void Shader::SetMatrix4(const std::string& uniformName, const Eigen::Matrix4f& matrix)
{
    GLint loc = GetUniformLocation(uniformName);
    if (loc == -1) return;  // uniform no encontrado
    glUniformMatrix4fv(loc, 1, GL_FALSE, matrix.data());
}

void Shader::SetMatrix3(const std::string& uniformName, const Eigen::Matrix3f& matrix)
{
    GLint loc = GetUniformLocation(uniformName);
    if (loc == -1) return;
    glUniformMatrix3fv(loc, 1, GL_FALSE, matrix.data());
}

void Shader::SetVector3f(const std::string& uniformName, const Eigen::Vector3f& value)
{
    GLint loc = GetUniformLocation(uniformName);
    if (loc == -1) return;
    glUniform3f(loc, value.x(), value.y(), value.z());
}
