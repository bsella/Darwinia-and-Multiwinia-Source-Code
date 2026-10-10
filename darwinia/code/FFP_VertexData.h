#pragma once

#include <span>

namespace ffp_emulation
{
    class Uniforms
    {
    public:
        explicit Uniforms(unsigned int program);

        const unsigned int program;

        const int texture_enabled_location;
        const int texture_env_mode_location;
        const int texture_env_color_location;
        const int texture_env_combine_rgb_location;
        const int texture_location;
        const int light_enabled_location;
        const int light_position_location;
        const int light_ambient_location;
        const int light_diffuse_location;
        const int light_specular_location;
        const int clip_plane_enabled_location;
        const int clip_plane_location;
        const int model_view_location;
        const int projection_location;
        const int color_material_enabled_location;
        const int lighting_enabled_location;
        const int material_shininess_location;
        const int material_specular_location;
        const int material_diffuse_location;
        const int material_ambient_location;
        const int scene_ambient_location;
        const int alpha_test_location;
    };
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
        void draw(unsigned int primitive_mode, int first, int num_vertices, const Uniforms&) const;
    protected:
        VertexBuffer();
        VertexBuffer(unsigned int vao, unsigned int vbo);
        ~VertexBuffer();
        unsigned int m_vao;
        unsigned int m_vbo;
    };

    class StaticVertexBuffer : public VertexBuffer
    {
    public:
        StaticVertexBuffer(std::span<const VertexData>);

        void draw_buffer(unsigned int primitive_mode) const;
        void draw_buffer(unsigned int primitive_mode, unsigned int program) const;
        void draw_buffer(unsigned int primitive_mode, const Uniforms&) const;
    private:
        const int m_num_vertices;
    };

    class DynamicVertexBuffer : public VertexBuffer
    {
    public:
        DynamicVertexBuffer() = default;
        DynamicVertexBuffer(unsigned int vao, unsigned int vbo);

        void update(std::span<const VertexData>);
        void update(void* data, size_t byte_count, int num_vertices);

        void draw_buffer(unsigned int primitive_mode) const;
        void draw_buffer(unsigned int primitive_mode, unsigned int program) const;
        void draw_buffer(unsigned int primitive_mode, const Uniforms&) const;
    private:
        int m_num_vertices = 0;
    };
}