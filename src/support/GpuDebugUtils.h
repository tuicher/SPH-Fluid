// GpuDebugUtils.h
#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <cassert>

namespace dbg
{

    template<typename T>
    std::vector<T> readBuffer(GLuint ssbo)
    {
        GLint64 sz = 0;
        glGetNamedBufferParameteri64v(ssbo, GL_BUFFER_SIZE, &sz);
        std::vector<T> out(sz / sizeof(T));
        glGetNamedBufferSubData(ssbo, 0, sz, out.data());
        return out;
    }

    inline bool isSorted(const std::vector<uint32_t>& v)
    {
        for (size_t i=1;i<v.size();++i)
            if (v[i] < v[i-1]) return false;
        return true;
    }

    // imprime los 5 primeros/5 últimos elementos
    template<typename T>
    void dumpEdges(const std::string& tag, const std::vector<T>& v)
    {
        auto n = v.size();
        std::cout << "--- " << tag << " (" << n << ") ---\n";
        for (size_t i=0;i<std::min<size_t>(5,n);++i)
            std::cout << "  ["<<i<<"] = " << v[i] << "\n";
        if (n>10){
            std::cout << "  ...\n";
            for (size_t i=n-5;i<n;++i)
                std::cout << "  ["<<i<<"] = " << v[i] << "\n";
        }
    }
} // namespace dbg
