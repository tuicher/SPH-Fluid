// Shader.h
#pragma once
#include <string>
#include <glad/glad.h>
#include <Eigen/Core>

class Shader
{
public:
    Shader() = default;
    ~Shader();

    bool CreateShaderProgram(const char* vertexSource, const char* fragmentSource);

    bool CreateShaderProgramFromFiles(const std::string& vertexPath, const std::string& fragmentPath);

    void Use() const;

    void SetMatrix4(const std::string& uniformName, const Eigen::Matrix4f& matrix);
    void SetMatrix3(const std::string& uniformName, const Eigen::Matrix3f& matrix);
    void SetVector3f(const std::string& uniformName, const Eigen::Vector3f& value);
    
    GLuint GetProgramID() const { return m_ProgramID; }

private:
    GLuint m_ProgramID = 0;

    bool CheckCompileErrors(GLuint shader, const std::string& type);

    GLint GetUniformLocation(const std::string& uniformName) const;
};

