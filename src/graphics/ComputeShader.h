// ComputeShader.h
#pragma once

#include <string>
#include <unordered_map>
#include <glad/glad.h>
#include <Eigen/Dense>

#include "../support/Loader.h" 

#include <fstream>
#include <sstream>
#include <iostream>

class ComputeShader
{
public:
    ComputeShader() noexcept : programID_(0) {};
    explicit ComputeShader(const std::string& path)
        :programID_(compile(LoadFileAsString(path)))
    {};
    ~ComputeShader();

    void use() const;
    void dispatch(GLuint x, GLuint y = 1, GLuint z = 1) const;

    // Uniform setters
    void setUniform(const std::string& name, int value);
    void setUniform(const std::string& name, GLuint value);
    void setUniform(const std::string& name, float value);
    void setUniform(const std::string& name, float x, float y, float z);
    void setUniform(const std::string& name, int x, int y, int z);
    void setUniform(const std::string& name, const Eigen::Vector3f& v);

private:
    GLuint programID_;
    mutable std::unordered_map<std::string, GLint> uniformLocations_;

    GLuint compile(const std::string& source);
    GLint getUniformLocation(const std::string& name) const;
};