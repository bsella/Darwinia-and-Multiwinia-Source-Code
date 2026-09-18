//#include "FFP_emulation.h"
#include <GL/glew.h>

#include "FFP_VertexData.h"

#include <array>
#include <cassert>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>

#include <GL/glext.h>
#include <cstdint>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <optional>
#include <span>
#include <vector>
#include <iostream>
#include <stack>

namespace ffp_emulation
{
    namespace
    {
        constexpr GLenum UNSET_MODE = ~0;

        GLenum g_primitive_mode = UNSET_MODE;
        GLenum g_matrix_mode    = GL_MODELVIEW;

        std::vector<VertexData> g_vertex_buffer_data;

        VertexData g_current_vertex;

        std::optional<DynamicVertexBuffer> g_vertex_buffer;

        GLuint g_program;

        std::stack<glm::mat4> g_model_view;
        std::stack<glm::mat4> g_projection;

        struct TextureEnvironment
        {
            bool      enabled     = false;
            GLuint    mode        = GL_MODULATE;
            GLuint    combine_rgb = 0;
            glm::vec4 color       = {0.0, 0.0, 0.0, 0.0};
        };

        GLuint g_active_texture = 0;
        std::array<TextureEnvironment, 2> g_texture_env;

        struct Light
        {
            int       enabled = false;
            GLint     enabled_location;

            glm::vec4 position = {0.0, 0.0, 1.0, 0.0};
            GLint     position_location;

            glm::vec4 ambient  = {0.0, 0.0, 0.0, 1.0};
            GLint     ambient_location;

            glm::vec4 diffuse  = {0.0, 0.0, 0.0, 1.0};
            GLint     diffuse_location;

            glm::vec4 specular = {0.0, 0.0, 0.0, 1.0};
            GLint     specular_location;
        };

        std::array<Light, 2> g_lights;

        bool g_color_material_enabled = false;
        bool g_lighting_enabled = false;

        float g_material_shininess;
        glm::vec4 g_material_specular = {0.0, 0.0, 0.0, 1.0};
        glm::vec4 g_material_diffuse  = {0.8, 0.8, 0.8, 1.0};
        glm::vec4 g_material_ambient  = {0.2, 0.2, 0.2, 1.0};

        glm::vec4 g_scene_ambient  = {0.2, 0.2, 0.2, 1.0};

        GLint g_model_view_location;
        GLint g_projection_location;
        GLint g_texture0_env_mode_loc;
        GLint g_texture0_env_color_loc;
        GLint g_texture0_env_combine_rgb_loc;
        GLint g_texture0_location;
        GLint g_texture1_env_mode_loc;
        GLint g_texture1_env_color_loc;
        GLint g_texture1_env_combine_rgb_loc;
        GLint g_texture1_location;
        GLint g_color_material_enabled_loc;
        GLint g_lighting_enabled_loc;
        GLint g_material_shininess_loc;
        GLint g_material_specular_loc;
        GLint g_material_diffuse_loc;
        GLint g_material_ambient_loc;
        GLint g_scene_ambient_loc;

        constexpr const char* vertex_shader_source =
R"(
#version 410

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec4 in_colour;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec2 in_uv0_texcoord;
layout (location = 4) in vec2 in_uv1_texcoord;

layout (location = 0) out vec4 out_colour;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec2 out_uv0_texcoord;
layout (location = 3) out vec2 out_uv1_texcoord;
layout (location = 4) out vec4 out_vertex_position;

uniform mat4 u_model_view;
uniform mat4 u_projection;

void main()
{
    out_vertex_position = u_model_view * vec4(in_position, 1.0);
    gl_Position = u_projection * out_vertex_position;

    out_vertex_position /= out_vertex_position.w;

    out_colour      = in_colour;
    out_normal      = normalize((u_model_view * vec4(in_normal, 0.0)).xyz);
    out_uv0_texcoord = in_uv0_texcoord;
    out_uv1_texcoord = in_uv1_texcoord;
}
)";


constexpr const char* fragment_shader_source =
R"(
#version 410

#define GL_ADD 0x0104
#define GL_MODULATE 0x2100
#define GL_DECAL 0x2101
#define GL_BLEND 0x0BE2
#define GL_REPLACE 0x1E01
#define GL_COMBINE 0x8570
#define GL_ADD_SIGNED 0x8574
#define GL_INTERPOLATE 0x8575
#define GL_SUBTRACT 0x84E7

layout (location = 0) in vec4 in_colour;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv0_texcoord;
layout (location = 3) in vec2 in_uv1_texcoord;
layout (location = 4) in vec4 in_vertex_position;

layout (location = 0) out vec4 out_colour;

uniform uint u_texture0_env_mode;
uniform uint u_texture0_env_combine_rgb;
uniform vec4 u_texture0_env_color;

uniform uint u_texture1_env_mode;
uniform uint u_texture1_env_combine_rgb;
uniform vec4 u_texture1_env_color;

uniform sampler2D texture0;
uniform sampler2D texture1;

uniform int   u_color_material_enabled;
uniform int   u_lighting_enabled;

uniform int   u_light0_enabled;
uniform vec4  u_light0_position;
uniform vec4  u_light0_ambient;
uniform vec4  u_light0_diffuse;
uniform vec4  u_light0_specular;

uniform int   u_light1_enabled;
uniform vec4  u_light1_position;
uniform vec4  u_light1_ambient;
uniform vec4  u_light1_diffuse;
uniform vec4  u_light1_specular;

uniform float u_material_shininess;
uniform vec4  u_material_specular;
uniform vec4  u_material_diffuse;
uniform vec4  u_material_ambient;

uniform vec4  u_scene_ambient;

vec4 sample_texture(uint stage)
{
    switch(stage)
    {
        case 0: return texture(texture0, in_uv0_texcoord);
        case 1: return texture(texture1, in_uv1_texcoord);
    }

    return texture(texture0, in_uv0_texcoord);
}

vec4 apply_texture(vec4 color, uint stage)
{
    uint texture_env_mode;
    uint texture_env_combine_rgb;
    vec4 texture_env_color;

    switch(stage)
    {
        case 0:
            texture_env_mode        = u_texture0_env_mode;
            texture_env_color       = u_texture0_env_color;
            texture_env_combine_rgb = u_texture0_env_combine_rgb;
        break;

        case 1:
            texture_env_mode        = u_texture1_env_mode;
            texture_env_color       = u_texture1_env_color;
            texture_env_combine_rgb = u_texture1_env_combine_rgb;
        break;

        default: return color;
    }

    switch(texture_env_mode)
    {
    case GL_ADD:
        color += sample_texture(stage);
        break;
    case GL_MODULATE:
        color *= sample_texture(stage);
        break;
    case GL_REPLACE:
        color = sample_texture(stage);
        break;
    case GL_DECAL:
        vec4 color_sample = sample_texture(stage);
        color.rgb = color.rgb * (1 - color_sample.a) + color_sample.rgb * color_sample.a;
        break;
    case GL_COMBINE:
        switch(texture_env_combine_rgb)
        {
        case GL_REPLACE:
            color.rgb = sample_texture(stage).rgb;
            break;
        case GL_MODULATE:
            color.rgb *= sample_texture(stage).rgb;
            break;
        case GL_ADD:
            color.rgb += sample_texture(stage).rgb;
            break;
        case GL_ADD_SIGNED:
            color.rgb += sample_texture(stage).rgb - vec3(0.5);
            break;
        case GL_INTERPOLATE:
            // TODO
            break;
        case GL_SUBTRACT:
            color.rgb -= sample_texture(stage).rgb;
            break;
        }
    }
    
    return color;
}

const uint GL_SINGLE_COLOR            = 0;
const uint GL_SEPARATE_SPECULAR_COLOR = 1;

const uint COLOR_CONTROL = GL_SINGLE_COLOR;

bool light_enabled(uint index)
{
    switch(index)
    {
        case 0: return u_light0_enabled != 0;
        case 1: return u_light1_enabled != 0;
    }
    return u_light0_enabled != 0;
}

vec4 light_position(uint index)
{
    switch(index)
    {
        case 0: return u_light0_position;
        case 1: return u_light1_position;
    }
    return u_light0_position;
}
vec4 light_ambient(uint index)
{
    switch(index)
    {
        case 0: return u_light0_ambient;
        case 1: return u_light1_ambient;
    }
    return u_light0_ambient;
}
vec4 light_diffuse(uint index)
{
    switch(index)
    {
        case 0: return u_light0_diffuse;
        case 1: return u_light1_diffuse;
    }
    return u_light0_diffuse;
}
vec4 light_specular(uint index)
{
    switch(index)
    {
        case 0: return u_light0_specular;
        case 1: return u_light1_specular;
    }
    return u_light0_specular;
}

const float CONSTANT_ATTENUATION  = 1.0;
const float LINEAR_ATTENUATION    = 0.0;
const float QUADRATIC_ATTENUATION = 0.0;

vec3 vector(vec4 P1, vec4 P2)
{
    if(P2.w == 0.0)
    {
        return normalize(P2.xyz);
    }

    if(P1.w == 0.0)
    {
        return normalize(-P1.xyz);
    }

    return normalize(P2.xyz - P1.xyz);
}

void main()
{
    out_colour = in_colour;

    if(u_lighting_enabled != 0)
    {
        vec4 light = u_material_ambient * u_scene_ambient;

        for(uint i = 0; i < 2; i++)
        {
            if(!light_enabled(i))
                continue;

            vec4 position = light_position(i);

            float atti = 1.0;

            if(position.w != 0)
            {
                float VP_dist = distance(in_vertex_position.xyz, position.xyz);
                atti = 1.0 / (CONSTANT_ATTENUATION + LINEAR_ATTENUATION * VP_dist + QUADRATIC_ATTENUATION * VP_dist * VP_dist );
            }

            vec3 VP = vector(in_vertex_position, position);

            float nVP = max(dot(in_normal, VP), 0.0);

            float fi = nVP != 0 ? 1.0 : 0.0;

            vec3 hi = VP + vec3(0.0, 0.0, 1.0);

            vec4 light_colour =
                  u_material_ambient * light_ambient(i)
                + nVP * u_material_diffuse * light_diffuse(i)
                + fi * pow(max(dot(in_normal, normalize(hi)), 0.0), u_material_shininess) * u_material_specular * light_specular(i);

            light += atti * light_colour;
        }

        out_colour *= vec4(light.xyz, 1.0);
    }

    out_colour = apply_texture(out_colour, 0);
    out_colour = apply_texture(out_colour, 1);

    if (out_colour.w == 0.0)
        discard;
}
)";

        glm::mat4& get_current_matrix()
        {
            assert(g_matrix_mode == GL_MODELVIEW || g_matrix_mode == GL_PROJECTION);

            switch (g_matrix_mode)
            {
                case GL_MODELVIEW:  return g_model_view.top();
                case GL_PROJECTION: return g_projection.top();
            }

            return g_model_view.top();
        }
    }

    VertexBuffer::VertexBuffer()
    {
        glGenVertexArrays(1, &m_vao);
        glBindVertexArray(m_vao);

        {
            glGenBuffers(1, &m_vbo);

            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), ((std::uint8_t*)nullptr));
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), ((std::uint8_t*)nullptr) + 3  * sizeof(float));
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), ((std::uint8_t*)nullptr) + 7  * sizeof(float));
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), ((std::uint8_t*)nullptr) + 10 * sizeof(float));
            glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), ((std::uint8_t*)nullptr) + 12 * sizeof(float));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glEnableVertexAttribArray(3);
            glEnableVertexAttribArray(4);
        }

        glBindVertexArray(0);
    }

    VertexBuffer::~VertexBuffer()
    {
        glDeleteBuffers(1, &m_vbo);
        glDeleteVertexArrays(1, &m_vao);
    }

    StaticVertexBuffer::StaticVertexBuffer(std::span<const VertexData> vertex_data)
        : m_num_vertices(vertex_data.size())
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(VertexData), vertex_data.data(), GL_STATIC_DRAW);
    }

    void StaticVertexBuffer::draw_buffer(unsigned int primitive_mode) const
    {
        VertexBuffer::draw(primitive_mode, 0, m_num_vertices);
    }

    void DynamicVertexBuffer::draw_buffer(unsigned int primitive_mode) const
    {
        VertexBuffer::draw(primitive_mode, 0, m_num_vertices);
    }

    void DynamicVertexBuffer::update(std::span<const VertexData> vertex_data)
    {
        m_num_vertices = vertex_data.size();

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
        glBufferData(GL_ARRAY_BUFFER, vertex_data.size() * sizeof(VertexData), vertex_data.data(), GL_DYNAMIC_DRAW);
    }

    void VertexBuffer::draw(GLenum primitive_mode, GLint first, GLsizei num_vertices) const
    {
        glUseProgram(g_program);
    
        // Upload the matrices
        glUniformMatrix4fv(g_model_view_location, 1, GL_FALSE, glm::value_ptr(g_model_view.top()));
        glUniformMatrix4fv(g_projection_location, 1, GL_FALSE, glm::value_ptr(g_projection.top()));

        // Update the bound texture id
        glUniform1i(g_texture0_location, 0);
        glUniform1i(g_texture1_location, 1);

        auto& tex0_env = g_texture_env[0];
        auto& tex1_env = g_texture_env[1];

        glUniform1ui(g_texture0_env_mode_loc, tex0_env.enabled ? tex0_env.mode : 0);
        glUniform1ui(g_texture0_env_combine_rgb_loc, tex0_env.combine_rgb);
        glUniform4f(g_texture0_env_color_loc, tex0_env.color.r, tex0_env.color.g, tex0_env.color.b, tex0_env.color.a);
        glUniform1ui(g_texture1_env_mode_loc, tex1_env.enabled ? tex1_env.mode : 0);
        glUniform1ui(g_texture1_env_combine_rgb_loc, tex1_env.combine_rgb);
        glUniform4f(g_texture1_env_color_loc, tex1_env.color.r, tex1_env.color.g, tex1_env.color.b, tex1_env.color.a);

        glUniform1i(g_color_material_enabled_loc, g_color_material_enabled);
        glUniform1i(g_lighting_enabled_loc, g_lighting_enabled);

        for(const auto& light : g_lights)
        {
            glUniform1i(light.enabled_location, light.enabled);
            glUniform4f(light.position_location, light.position.x, light.position.y, light.position.z, light.position.w);
            glUniform4f(light.ambient_location, light.ambient.x, light.ambient.y, light.ambient.z, light.ambient.w);
            glUniform4f(light.diffuse_location, light.diffuse.x, light.diffuse.y, light.diffuse.z, light.diffuse.w);
            glUniform4f(light.specular_location, light.specular.x, light.specular.y, light.specular.z, light.specular.w);
        }
        glUniform1f(g_material_shininess_loc, g_material_shininess);
        glUniform4f(g_material_specular_loc, g_material_specular.r, g_material_specular.g, g_material_specular.b, g_material_specular.a);
        glUniform4f(g_material_diffuse_loc, g_material_diffuse.r, g_material_diffuse.g, g_material_diffuse.b, g_material_diffuse.a);
        glUniform4f(g_material_ambient_loc, g_material_ambient.r, g_material_ambient.g, g_material_ambient.b, g_material_ambient.a);
        glUniform4f(g_scene_ambient_loc, g_scene_ambient.r, g_scene_ambient.g, g_scene_ambient.b, g_scene_ambient.a);

        if (primitive_mode == GL_QUADS)      primitive_mode = GL_TRIANGLES;
        if (primitive_mode == GL_QUAD_STRIP) primitive_mode = GL_TRIANGLE_STRIP;

        assert(
            primitive_mode == GL_POINTS ||
            primitive_mode == GL_LINE_STRIP ||
            primitive_mode == GL_LINE_LOOP ||
            primitive_mode == GL_LINES ||
            primitive_mode == GL_LINE_STRIP_ADJACENCY ||
            primitive_mode == GL_LINES_ADJACENCY ||
            primitive_mode == GL_TRIANGLE_STRIP ||
            primitive_mode == GL_TRIANGLE_FAN ||
            primitive_mode == GL_TRIANGLES ||
            primitive_mode == GL_TRIANGLE_STRIP_ADJACENCY ||
            primitive_mode == GL_TRIANGLES_ADJACENCY ||
            primitive_mode == GL_PATCHES
        );

        // Draw the primitives
        glBindVertexArray(m_vao);
        glDrawArrays(primitive_mode, first, num_vertices);
    }

    void init()
    {
        int success;
        char infoLog[512];

        GLuint vertex_shader, fragment_shader;

        {
            vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    
            glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
            
            glCompileShader(vertex_shader);
    
            // print compile errors if any
            glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
            if(!success)
            {
                glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
            };
        }

        {
            fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    
            glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
            
            glCompileShader(fragment_shader);
    
            // print compile errors if any
            glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
            if(!success)
            {
                glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
            };
        }

        {
            g_program = glCreateProgram();
            glAttachShader(g_program, vertex_shader);
            glAttachShader(g_program, fragment_shader);
            glLinkProgram(g_program);
            // print linking errors if any
            glGetProgramiv(g_program, GL_LINK_STATUS, &success);
            if(!success)
            {
                glGetProgramInfoLog(g_program, 512, NULL, infoLog);
                std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
            }
        }

        g_lights[0].diffuse = {1.0, 1.0, 1.0, 1.0};
        g_lights[0].specular = {1.0, 1.0, 1.0, 1.0};

        g_texture_env[0].enabled = true;

        g_model_view_location    = glGetUniformLocation(g_program, "u_model_view");
        g_projection_location    = glGetUniformLocation(g_program, "u_projection");
        g_texture0_env_mode_loc  = glGetUniformLocation(g_program, "u_texture0_env_mode");
        g_texture0_env_color_loc = glGetUniformLocation(g_program, "u_texture0_env_color");
        g_texture0_location      = glGetUniformLocation(g_program, "texture0");
        g_texture1_env_mode_loc  = glGetUniformLocation(g_program, "u_texture1_env_mode");
        g_texture1_env_color_loc = glGetUniformLocation(g_program, "u_texture1_env_color");
        g_texture1_location      = glGetUniformLocation(g_program, "texture1");

        g_color_material_enabled_loc  = glGetUniformLocation(g_program, "u_color_material_enabled");
        g_lighting_enabled_loc        = glGetUniformLocation(g_program, "u_lighting_enabled");

        g_lights[0].enabled_location  = glGetUniformLocation(g_program, "u_light0_enabled");
        g_lights[0].position_location = glGetUniformLocation(g_program, "u_light0_position");
        g_lights[0].ambient_location  = glGetUniformLocation(g_program, "u_light0_ambient");
        g_lights[0].diffuse_location  = glGetUniformLocation(g_program, "u_light0_diffuse");
        g_lights[0].specular_location = glGetUniformLocation(g_program, "u_light0_specular");

        g_lights[1].enabled_location  = glGetUniformLocation(g_program, "u_light1_enabled");
        g_lights[1].position_location = glGetUniformLocation(g_program, "u_light1_position");
        g_lights[1].ambient_location  = glGetUniformLocation(g_program, "u_light1_ambient");
        g_lights[1].diffuse_location  = glGetUniformLocation(g_program, "u_light1_diffuse");
        g_lights[1].specular_location = glGetUniformLocation(g_program, "u_light1_specular");

        g_material_shininess_loc      = glGetUniformLocation(g_program, "u_material_shininess");
        g_material_specular_loc       = glGetUniformLocation(g_program, "u_material_specular");
        g_material_diffuse_loc        = glGetUniformLocation(g_program, "u_material_diffuse");
        g_material_ambient_loc        = glGetUniformLocation(g_program, "u_material_ambient");

        g_scene_ambient_loc = glGetUniformLocation(g_program, "u_scene_ambient");

        g_texture0_env_combine_rgb_loc = glGetUniformLocation(g_program, "u_texture0_env_combine_rgb");
        g_texture1_env_combine_rgb_loc = glGetUniformLocation(g_program, "u_texture1_env_combine_rgb");

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        g_model_view.push(glm::mat4(1.0));
        g_projection.push(glm::mat4(1.0));

        g_vertex_buffer.emplace();
    }

    void quit()
    {
        // TODO
    }

    std::span<const VertexData> get_current_vertex_buffer()
    {
        return g_vertex_buffer_data;
    }

    void glBegin(GLenum mode)
    {
        g_primitive_mode = mode;

        g_vertex_buffer_data.clear();
    }

    void glEnd()
    {
        g_vertex_buffer->update(get_current_vertex_buffer());
        g_vertex_buffer->draw_buffer(g_primitive_mode);
    }

    void glMatrixMode(GLenum mode)
    {
        g_matrix_mode = mode;
    }

    void glLoadIdentity()
    {
        get_current_matrix() = glm::mat4(1.0);
    }

    void glMultMatrixf(const GLfloat *m)
    {
        glm::mat4 mat(
            m[0],  m[1],  m[2],  m[3],
            m[4],  m[5],  m[6],  m[7],
            m[8],  m[9],  m[10], m[11],
            m[12], m[13], m[14], m[15]
        );

        get_current_matrix() *= mat;
    }

    void glPushMatrix()
    {
        assert(g_matrix_mode == GL_MODELVIEW || g_matrix_mode == GL_PROJECTION);

        switch (g_matrix_mode)
        {
            case GL_MODELVIEW:
                g_model_view.push(g_model_view.top());
                break;

            case GL_PROJECTION:
                g_projection.push(g_projection.top());
                break;
        }
    }

    void glPopMatrix()
    {
        assert(g_matrix_mode == GL_MODELVIEW || g_matrix_mode == GL_PROJECTION);

        switch (g_matrix_mode)
        {
            case GL_MODELVIEW:
                g_model_view.pop();
                break;
            case GL_PROJECTION:
                g_projection.pop();
                break;
        }
    }

    void glLoadMatrixd(const GLdouble *m)
    {
        get_current_matrix() = glm::mat4(
            m[0],  m[1],  m[2],  m[3],
            m[4],  m[5],  m[6],  m[7],
            m[8],  m[9],  m[10], m[11],
            m[12], m[13], m[14], m[15]
        );
    }

    void glScalef(GLfloat x, GLfloat y, GLfloat z)
    {
        get_current_matrix() = glm::scale(get_current_matrix(), {x, y, z});
    }

    void glTranslatef(GLfloat x, GLfloat y, GLfloat z)
    {
        get_current_matrix() = glm::translate(get_current_matrix(), {x, y, z});
    }

    void glRotatef(GLfloat angle_degrees, GLfloat x, GLfloat y, GLfloat z)
    {
        get_current_matrix() = glm::rotate(get_current_matrix(), glm::radians(angle_degrees), {x,y,z});
    }

    void glGetIntegerv(GLenum pname, GLint *params)
    {
        if(pname == GL_MATRIX_MODE)
        {
            params[0] = g_matrix_mode;
            return;
        }

        ::glGetIntegerv(pname, params);
    }

    void glGetDoublev(GLenum pname, GLdouble *params)
    {
        if(pname == GL_MODELVIEW_MATRIX)
        {
            auto& matrix = g_model_view.top();

            const float* m = glm::value_ptr(matrix);

            params[0]  = m[0];
            params[1]  = m[1];
            params[2]  = m[2];
            params[3]  = m[3];
            params[4]  = m[4]; 
            params[5]  = m[5];
            params[6]  = m[6];
            params[7]  = m[7];
            params[8]  = m[8]; 
            params[9]  = m[9];
            params[10] = m[10];
            params[11] = m[11];
            params[12] = m[12];
            params[13] = m[13];
            params[14] = m[14];
            params[15] = m[15];

            return;
        }

        if(pname == GL_PROJECTION_MATRIX)
        {
            auto& matrix = g_projection.top();

            const float* m = glm::value_ptr(matrix);

            params[0]  = m[0];
            params[1]  = m[1];
            params[2]  = m[2];
            params[3]  = m[3];
            params[4]  = m[4]; 
            params[5]  = m[5];
            params[6]  = m[6];
            params[7]  = m[7];
            params[8]  = m[8]; 
            params[9]  = m[9];
            params[10] = m[10];
            params[11] = m[11];
            params[12] = m[12];
            params[13] = m[13];
            params[14] = m[14];
            params[15] = m[15];

            return;
        }

        ::glGetDoublev(pname, params);
    }

    void gluLookAt(GLdouble pos_x, GLdouble pos_y, GLdouble pos_z,
                   GLdouble forwards_x, GLdouble forwards_y, GLdouble forwards_z,
                   GLdouble up_x, GLdouble up_y, GLdouble up_z)
    {
        glm::vec3 eye{pos_x, pos_y, pos_z};
        glm::vec3 forwards{forwards_x, forwards_y,  forwards_z};

        glm::vec3 up{up_x, up_y, up_z};

        get_current_matrix() *= glm::lookAt(eye, forwards, up);
    }

    void gluPerspective(GLdouble fovy_degrees, GLdouble aspect, GLdouble zNear, GLdouble zFar)
    {
        auto fovy = glm::radians(fovy_degrees);

        get_current_matrix() *= glm::perspective((float)fovy, (float)aspect, (float)zNear, (float)zFar);
    }

    void gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top)
    {
        get_current_matrix() *= glm::ortho((float)left, (float)right, (float)bottom, (float)top);
    }

    void glVertex3f( GLfloat x, GLfloat y, GLfloat z )
    {
        g_current_vertex.x = x;
        g_current_vertex.y = y;
        g_current_vertex.z = z;

        if(g_primitive_mode == GL_QUADS)
        {
            auto triangle_count = g_vertex_buffer_data.size() / 3;

            bool odd_triangle_count = triangle_count % 2 != 0;

            if(odd_triangle_count)
            {
                auto vertex1 = g_vertex_buffer_data[g_vertex_buffer_data.size() - 3];
                auto vertex2 = g_vertex_buffer_data[g_vertex_buffer_data.size() - 1];
    
                g_vertex_buffer_data.push_back(vertex1);
                g_vertex_buffer_data.push_back(vertex2);
            }

        }

        g_vertex_buffer_data.push_back(g_current_vertex);
    }
    void glVertex2f( GLfloat x, GLfloat y )
    {
        glVertex3f(x, y, 0.0);
    }
    void glVertex2i( GLint x, GLint y )
    {
        glVertex3f(static_cast<float>(x), static_cast<float>(y), 0.0);
    }
    void glVertex3d( GLdouble x, GLdouble y, GLdouble z )
    {
        glVertex3f(x, y, z);
    }
    void glVertex2fv( const GLfloat *v )
    {
        glVertex3f(v[0], v[1], 0.0);
    }
    void glVertex3fv( const GLfloat *v )
    {
        glVertex3f(v[0], v[1], v[2]);
    }
    void glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz )
    {
        g_current_vertex.n_x = nx;
        g_current_vertex.n_y = ny;
        g_current_vertex.n_z = nz;
    }
    void glNormal3fv( const GLfloat *v )
    {
        glNormal3f(v[0], v[1], v[2]);
    }
    void glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha )
    {
        g_current_vertex.r = red;
        g_current_vertex.g = green;
        g_current_vertex.b = blue;
        g_current_vertex.a = alpha;
    }
    void glColor3f( GLfloat red, GLfloat green, GLfloat blue )
    {
        glColor4f( red, green, blue, 1.0 );
    }
    void glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
    {
        glColor4f( static_cast<float>(red) / 255.0, static_cast<float>(green) / 255.0, static_cast<float>(blue) / 255.0, static_cast<float>(alpha) / 255.0);
    }
    void glColor3ub( GLubyte red, GLubyte green, GLubyte blue )
    {
        glColor4ub( red, green, blue, 255);
    }
    void glColor3fv( const GLfloat *v )
    {
        glColor4f(v[0], v[1], v[2], 1.0);
    }
    void glColor3ubv( const GLubyte *v )
    {
        glColor4ub(v[0], v[1], v[2], 255);
    }
    void glColor4fv( const GLfloat *v )
    {
        glColor4f(v[0], v[1], v[2], v[3]);
    }
    void glColor4ubv( const GLubyte *v )
    {
        glColor4ub(v[0], v[1], v[2], v[3]);
    }
    void glTexCoord2f( GLfloat u, GLfloat v )
    {
        g_current_vertex.u0 = u;
        g_current_vertex.v0 = v;
    }
    void glTexCoord2i( GLint s, GLint t )
    {
        glTexCoord2f(static_cast<float>(s), static_cast<float>(t));
    }

    void glAlphaFunc(GLenum func, GLclampf ref)
    {
        // TODO
    }

    void glColorMaterial(GLenum face, GLenum mode)
    {
        // TODO
    }

    void glMaterialfv(GLenum face, GLenum pname, const GLfloat *params)
    {
        switch (pname)
        {
            case GL_SPECULAR:  g_material_specular  = {params[0],params[1], params[2], params[3]}; break;
            case GL_DIFFUSE:   g_material_diffuse   = {params[0],params[1], params[2], params[3]}; break;
            case GL_SHININESS: g_material_shininess = params[0]; break;
            case GL_AMBIENT:   g_material_ambient   = {params[0],params[1], params[2], params[3]}; break;
            case GL_AMBIENT_AND_DIFFUSE:
                g_material_ambient = {params[0],params[1], params[2], params[3]};
                g_material_diffuse = {params[0],params[1], params[2], params[3]};
            break;
        }
    }

    void glFogf(GLenum pname, GLfloat param)
    {
        // TODO
    }
    void glFogi(GLenum pname, GLint param)
    {
        // TODO
    }
    void glFogfv(GLenum pname, const GLfloat* params)
    {
        // TODO
    }
    void glFogiv(GLenum pname, const GLint* params)
    {
        // TODO
    }

    void glLightfv(GLenum light_index, GLenum  pname, const GLfloat *params)
    {
        auto& light = g_lights[light_index - GL_LIGHT0];

        switch (pname)
        {
            case GL_POSITION:
            {
                light.position.x = params[0];
                light.position.y = params[1];
                light.position.z = params[2];
                light.position.w = params[3];
    
                bool is_at_infinity = light.position.w == 0.0;

                // We need to change the space (coordinate system) of the position of the light
                // to eye (camera) space. Normally whenever the light position is set, the top
                // of the model-view matrix contains only the view matrix.
                light.position = g_model_view.top() * light.position;

                // If w is 0, this means that the light is considered at infinity, pointed by the
                // xyz direction starting from the origin.
                if(!is_at_infinity)
                    light.position /= light.position.w;
            }
            break;

            case GL_DIFFUSE:
                light.diffuse.x = params[0];
                light.diffuse.y = params[1];
                light.diffuse.z = params[2];
                light.diffuse.w = params[3];
            break;

            case GL_SPECULAR:
                light.specular.x = params[0];
                light.specular.y = params[1];
                light.specular.z = params[2];
                light.specular.w = params[3];
            break;

            case GL_AMBIENT:
                light.ambient.x = params[0];
                light.ambient.y = params[1];
                light.ambient.z = params[2];
                light.ambient.w = params[3];
            break;
        }

    }

    void glLightModelfv(GLenum  pname, const GLfloat *params)
    {
        if(pname == GL_LIGHT_MODEL_AMBIENT)
        {
            g_scene_ambient.r = params[0];
            g_scene_ambient.g = params[1];
            g_scene_ambient.b = params[2];
            g_scene_ambient.a = params[3];
        }
    }

    void glShadeModel(GLenum mode)
    {
        // TODO
    }

    void _glActiveTexture(GLenum texture)
    {
        switch (texture)
        {
            case GL_TEXTURE0: g_active_texture = 0; break;
            case GL_TEXTURE1: g_active_texture = 1; break;
        }

        ::glActiveTexture(texture);
    }

    void glBindTexture(GLenum target, GLuint texture)
    {
        assert(target == GL_TEXTURE_2D);

        ::glBindTexture(target, texture);
    }

    void glTexEnvf(GLenum target, GLenum pname, GLfloat param)
    {
        if(target == GL_TEXTURE_ENV)
        {
            switch (pname)
            {
                case GL_TEXTURE_ENV_MODE:
                    g_texture_env[g_active_texture].mode = param;

                break;

                case GL_COMBINE_RGB: //GL_COMBINE_RGB_EXT
                    g_texture_env[g_active_texture].combine_rgb = param;
                break;
            }
        }
    }

    void glTexEnvi(GLenum target, GLenum pname, GLint param)
    {
        glTexEnvf(target, pname, param);
    }
    void glTexEnviv(GLenum target, GLenum pname, const GLint* params)
    {
        if(target == GL_TEXTURE_ENV)
        {
            switch (pname)
            {
                case GL_TEXTURE_ENV_COLOR:
                    g_texture_env[g_active_texture].color = glm::vec4(
                        static_cast<float>(params[0]) / 255.0,
                        static_cast<float>(params[1]) / 255.0,
                        static_cast<float>(params[2]) / 255.0,
                        static_cast<float>(params[3]) / 255.0
                    );
                break;
            }
        }
    }

    void glEnable(GLenum cap)
    {
        switch (cap)
        {
            case GL_TEXTURE_2D:
                g_texture_env[g_active_texture].enabled = true;
            break;

            case GL_LIGHTING: g_lighting_enabled  = true; break;
            case GL_LIGHT0:   g_lights[0].enabled = true; break;
            case GL_LIGHT1:   g_lights[1].enabled = true; break;

            case GL_COLOR_MATERIAL: g_color_material_enabled = true; break;
        }

        ::glEnable(cap);
    }

    void glDisable(GLenum cap)
    {
        switch (cap)
        {
            case GL_TEXTURE_2D:
                g_texture_env[g_active_texture].enabled = false;
            break;

            case GL_LIGHTING: g_lighting_enabled  = false; break;
            case GL_LIGHT0:   g_lights[0].enabled = false; break;
            case GL_LIGHT1:   g_lights[1].enabled = false; break;

            case GL_COLOR_MATERIAL: g_color_material_enabled = false; break;
        }

        ::glDisable(cap);
    }

    GLboolean glIsEnabled(GLenum cap)
    {
        switch (cap)
        {
            case GL_TEXTURE_2D: return g_texture_env[g_active_texture].enabled;
            case GL_LIGHTING:   return g_lighting_enabled;
            case GL_LIGHT0:     return g_lights[0].enabled;
            case GL_LIGHT1:     return g_lights[1].enabled;

            case GL_COLOR_MATERIAL: return g_color_material_enabled;
        }

        return ::glIsEnabled(cap);
    }


    void glDisableClientState(GLenum array)
    {
        // TODO
    }
    
    void glEnableClientState(GLenum array)
    {
        // TODO
    }
    
    void glVertexPointer(GLint size, GLenum type, GLsizei stride, GLvoid *pointer)
    {
        //glVertexAttribPointer(0, size, type, GL_FALSE, stride, pointer);
        //glEnableVertexAttribArray(0);
    }
    
    void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
    {
        //glVertexAttribPointer(1, 3, type, GL_FALSE, stride, pointer);
        //glEnableVertexAttribArray(1);
    }

    void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
    {
        //glVertexAttribPointer(2, size, type, GL_FALSE, stride, pointer);
        //glEnableVertexAttribArray(2);
    }

    void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
    {
        //glVertexAttribPointer(3, size, type, GL_FALSE, stride, pointer);
        //glEnableVertexAttribArray(3);
    }

    void _glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t)
    {
        switch (target)
        {
            case GL_TEXTURE0:
                g_current_vertex.u0 = s;
                g_current_vertex.v0 = t;
            break;

            case GL_TEXTURE1:
                g_current_vertex.u1 = s;
                g_current_vertex.v1 = t;
            break;
        }
    }
}