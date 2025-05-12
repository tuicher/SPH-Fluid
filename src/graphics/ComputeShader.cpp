#include "ComputeShader.h"

ComputeShader::~ComputeShader()
{
    if (programID_) {
        glDeleteProgram(programID_);
    }
}

void ComputeShader::use() const
{
    glUseProgram(programID_);
}

void ComputeShader::dispatch(GLuint x, GLuint y, GLuint z) const
{
    glDispatchCompute(x, y, z);
}

void ComputeShader::setUniform(const std::string& name, int value)
{
    glUniform1i(getUniformLocation(name), value);
}

void ComputeShader::setUniform(const std::string& name, GLuint value)
{
    glUniform1ui(getUniformLocation(name), value);
}

void ComputeShader::setUniform(const std::string& name, float value)
{
    glUniform1f(getUniformLocation(name), value);
}

void ComputeShader::setUniform(const std::string& name, float x, float y, float z)
{
    glUniform3f(getUniformLocation(name), x, y, z);
}

void ComputeShader::setUniform(const std::string& name, int x, int y, int z)
{
    glUniform3i(getUniformLocation(name), x, y, z);
}

void ComputeShader::setUniform(const std::string& name, const Eigen::Vector3f& v)
{
    glUniform3f(getUniformLocation(name), v.x(), v.y(), v.z());
}

GLint ComputeShader::getUniformLocation(const std::string& name) const
{
    auto it = uniformLocations_.find(name);
    if (it != uniformLocations_.end())
        return it->second;

    GLint location = glGetUniformLocation(programID_, name.c_str());
    if (location == -1) {
        std::cerr << "[ComputeShader] Warning: uniform '" << name << "' not found.\n";
    }
    uniformLocations_[name] = location;
    return location;
}

GLuint ComputeShader::compile(const std::string& source)
{
    //std::cout << source;
    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "[ComputeShader] Compilation failed:\n" << log << std::endl;
        glDeleteShader(shader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, shader);
    glLinkProgram(program);
    glDeleteShader(shader);

    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (status != GL_TRUE) {
        char log[1024];
        glGetProgramInfoLog(program, sizeof(log), nullptr, log);
        std::cerr << "[ComputeShader] Linking failed:\n" << log << std::endl;
        glDeleteProgram(program);
        return 0;
    }

    return program;
}