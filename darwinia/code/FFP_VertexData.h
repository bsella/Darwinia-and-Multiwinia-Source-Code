#pragma once

#include <span>

namespace ffp_emulation
{
    struct VertexData
    {
        float x, y, z;
        float r, g, b, a;
        float n_x, n_y, n_z;
        float u0, v0, u1, v1;
    };

    class VertexBuffer
    {
    public:
        void draw(unsigned int primitive_mode, int first, int num_vertices) const;
        void draw(unsigned int primitive_mode, int first, int num_vertices, unsigned int program) const;
    protected:
        VertexBuffer();
        ~VertexBuffer();
        unsigned int m_vao;
        unsigned int m_vbo;
    };

    class StaticVertexBuffer : public VertexBuffer
    {
    public:
        StaticVertexBuffer(std::span<const VertexData>);

        void draw_buffer(unsigned int primitive_mode) const;
    private:
        const int m_num_vertices;
    };

    class DynamicVertexBuffer : public VertexBuffer
    {
    public:
        DynamicVertexBuffer() = default;

        void update(std::span<const VertexData>);

        void draw_buffer(unsigned int primitive_mode) const;
    private:
        int m_num_vertices = 0;
    };
}