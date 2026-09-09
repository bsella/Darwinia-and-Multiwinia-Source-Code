//#include "FFP_emulation.h"
#include <GL/glew.h>

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

        struct VertexData
        {
            float x, y, z;
            float r, g, b, a;
            float n_x, n_y, n_z;
            float u0, v0, u1, v1;
        };

        std::vector<VertexData> g_vertex_buffer;

        VertexData g_current_vertex;

        GLuint g_vao = 0;
        GLuint g_vbo;

        GLuint g_program;

        std::stack<glm::mat4> g_model_view;
        std::stack<glm::mat4> g_projection;

        int g_draw_list_model_view_stack_level = 0;
        int g_draw_list_projection_stack_level = 0;

        struct TextureEnvironment
        {
            bool      enabled     = false;
            GLuint    mode        = GL_MODULATE;
            GLuint    combine_rgb = 0;
            glm::vec4 color       = {0.0, 0.0, 0.0, 0.0};
        };

        GLuint g_active_texture = 0;
        std::array<TextureEnvironment, 2> g_texture_env;

        struct DrawList
        {
            GLuint vao;
            GLuint vbo;
            GLenum primitive_mode;

            GLsizei num_vertices;

            glm::mat4 diff_model_view;
            glm::mat4 diff_projection;

            std::array<TextureEnvironment, 2> texture_env;
        };

        std::vector<DrawList> g_draw_lists;

        DrawList* g_current_draw_list = nullptr;

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

uniform mat4 u_model_view;
uniform mat4 u_projection;

void main()
{
    gl_Position = u_projection * u_model_view * vec4(in_position, 1.0);

    out_colour      = in_colour;
    out_normal      = in_normal;
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

layout (location = 0) out vec4 out_colour;

uniform uint u_texture0_env_mode;
uniform uint u_texture0_env_combine_rgb;
uniform vec4 u_texture0_env_color;

uniform uint u_texture1_env_mode;
uniform uint u_texture1_env_combine_rgb;
uniform vec4 u_texture1_env_color;

uniform sampler2D texture0;
uniform sampler2D texture1;

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

void main()
{
    out_colour = in_colour;

    out_colour = apply_texture(out_colour, 0);
    out_colour = apply_texture(out_colour, 1);
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

        void create_vertex_buffers(GLuint& vao, GLuint& vbo)
        {
            glGenVertexArrays(1, &vao);
            glBindVertexArray(vao);

            {
                glGenBuffers(1, &vbo);

                glBindBuffer(GL_ARRAY_BUFFER, vbo);
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

        void draw_list(const DrawList& draw_list)
        {
            glUseProgram(g_program);
    
            // Upload the matrices
            glUniformMatrix4fv(g_model_view_location, 1, GL_FALSE, glm::value_ptr(g_model_view.top() * draw_list.diff_model_view));
            glUniformMatrix4fv(g_projection_location, 1, GL_FALSE, glm::value_ptr(g_projection.top() * draw_list.diff_projection));
    
            // Update the bound texture id
            glUniform1i(g_texture0_location, 0);
            glUniform1i(g_texture1_location, 1);
    
            auto& tex0_env = draw_list.texture_env[0];
            auto& tex1_env = draw_list.texture_env[1];

            glUniform1ui(g_texture0_env_mode_loc, tex0_env.enabled ? tex0_env.mode : 0);
            glUniform1ui(g_texture0_env_combine_rgb_loc, tex0_env.combine_rgb);
            glUniform4f(g_texture0_env_color_loc, tex0_env.color.r, tex0_env.color.g, tex0_env.color.b, tex0_env.color.a);
            glUniform1ui(g_texture1_env_mode_loc, tex1_env.enabled ? tex1_env.mode : 0);
            glUniform1ui(g_texture1_env_combine_rgb_loc, tex1_env.combine_rgb);
            glUniform4f(g_texture1_env_color_loc, tex1_env.color.r, tex1_env.color.g, tex1_env.color.b, tex1_env.color.a);
    
            auto primitive_mode = draw_list.primitive_mode;
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
            glBindVertexArray(draw_list.vao);
            glDrawArrays(primitive_mode, 0, draw_list.num_vertices);
        }

        void draw()
        {
            draw_list(
                DrawList{
                    .vao = g_vao,
                    .vbo = g_vbo,
                    .primitive_mode = g_primitive_mode,
                    .num_vertices = (GLsizei)g_vertex_buffer.size(),
                    .diff_model_view = glm::mat4(1.0),
                    .diff_projection = glm::mat4(1.0),
                    .texture_env = g_texture_env,
            });
        }
    }

    void init()
    {
        create_vertex_buffers(g_vao, g_vbo);

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

        g_texture_env[0].enabled = true;

        g_model_view_location    = glGetUniformLocation(g_program, "u_model_view");
        g_projection_location    = glGetUniformLocation(g_program, "u_projection");
        g_texture0_env_mode_loc  = glGetUniformLocation(g_program, "u_texture0_env_mode");
        g_texture0_env_color_loc = glGetUniformLocation(g_program, "u_texture0_env_color");
        g_texture0_location      = glGetUniformLocation(g_program, "texture0");
        g_texture1_env_mode_loc  = glGetUniformLocation(g_program, "u_texture1_env_mode");
        g_texture1_env_color_loc = glGetUniformLocation(g_program, "u_texture1_env_color");
        g_texture1_location      = glGetUniformLocation(g_program, "texture1");

        g_texture0_env_combine_rgb_loc = glGetUniformLocation(g_program, "u_texture0_env_combine_rgb");
        g_texture1_env_combine_rgb_loc = glGetUniformLocation(g_program, "u_texture1_env_combine_rgb");

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        g_model_view.push(glm::mat4(1.0));
        g_projection.push(glm::mat4(1.0));
    }

    void quit()
    {
        // TODO
    }

    void glBegin(GLenum mode)
    {
        if(g_current_draw_list)
        {
            g_current_draw_list->primitive_mode = mode;
            g_current_draw_list->diff_model_view = glm::mat4(1.0);
            g_current_draw_list->diff_projection = glm::mat4(1.0);
        }
        else
            g_primitive_mode = mode;

        g_vertex_buffer.clear();
    }

    void glEnd()
    {
        // Upload the vertex data buffer
        if(g_current_draw_list)
        {
            glBindBuffer(GL_ARRAY_BUFFER, g_current_draw_list->vbo);
            glBufferData(GL_ARRAY_BUFFER, g_vertex_buffer.size() * sizeof(VertexData), g_vertex_buffer.data(), GL_STATIC_DRAW);

            g_current_draw_list->num_vertices = g_vertex_buffer.size();

            glm::mat4 diff_matrix = glm::mat4(1.0);
            std::vector<glm::mat4> diff_matrices;

            {
                for(int i = 0; i < g_draw_list_model_view_stack_level; i++)
                {
                    diff_matrix *= diff_matrices.emplace_back(g_model_view.top());

                    g_model_view.pop();
                }
                for(const auto& m : diff_matrices)
                {
                    g_model_view.push(m);
                }
            }

            g_current_draw_list->diff_model_view *= diff_matrix;

            diff_matrix = glm::mat4(1.0);
            diff_matrices.clear();

            {
                for(int i = 0; i < g_draw_list_projection_stack_level; i++)
                {
                    diff_matrix *= diff_matrices.emplace_back(g_projection.top());

                    g_projection.pop();
                }
                for(const auto& m : diff_matrices)
                {
                    g_projection.push(m);
                }
            }

            g_current_draw_list->diff_projection *= diff_matrix;
        }
        else
        {
            glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
            glBufferData(GL_ARRAY_BUFFER, g_vertex_buffer.size() * sizeof(VertexData), g_vertex_buffer.data(), GL_DYNAMIC_DRAW);
        }

        if(!g_current_draw_list)
        {
            draw();
        }
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

        switch (g_matrix_mode)
        {
            case GL_MODELVIEW:
                if(g_current_draw_list)
                    g_current_draw_list->diff_model_view *= mat;
                break;

            case GL_PROJECTION:
                if(g_current_draw_list)
                    g_current_draw_list->diff_projection *= mat;
                break;
        }
    }

    void glPushMatrix()
    {
        assert(g_matrix_mode == GL_MODELVIEW || g_matrix_mode == GL_PROJECTION);

        switch (g_matrix_mode)
        {
            case GL_MODELVIEW:
                if(g_current_draw_list)
                    g_draw_list_model_view_stack_level++;
                g_model_view.push(g_model_view.top());
                break;

            case GL_PROJECTION:
                if(g_current_draw_list)
                    g_draw_list_projection_stack_level++;
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
                if(g_current_draw_list)
                    g_draw_list_model_view_stack_level--;
                break;
            case GL_PROJECTION:
                g_projection.pop();
                if(g_current_draw_list)
                    g_draw_list_projection_stack_level--;
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
            auto triangle_count = g_vertex_buffer.size() / 3;

            bool odd_triangle_count = triangle_count % 2 != 0;

            if(odd_triangle_count)
            {
                auto vertex1 = g_vertex_buffer[g_vertex_buffer.size() - 3];
                auto vertex2 = g_vertex_buffer[g_vertex_buffer.size() - 1];
    
                g_vertex_buffer.push_back(vertex1);
                g_vertex_buffer.push_back(vertex2);
            }

        }

        g_vertex_buffer.push_back(g_current_vertex);
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
        // TODO
    }

    GLuint glGenLists(GLsizei range)
    {
        auto previous_list_index = g_draw_lists.size();

        for(GLsizei i = 0; i < range; i++)
        {
            auto& new_list = g_draw_lists.emplace_back();

            create_vertex_buffers(new_list.vao, new_list.vbo);
        }

        return previous_list_index;
    }

    void glNewList(GLuint list, GLenum mode)
    {
        if(mode == GL_COMPILE)
            g_current_draw_list = &g_draw_lists[list];

        g_draw_list_model_view_stack_level = 0;
        g_draw_list_projection_stack_level = 0;
    }

    void glEndList()
    {
        g_current_draw_list = nullptr;
    }

    void glCallList(GLuint list)
    {
        draw_list(g_draw_lists[list]);
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

    void glLightfv(GLenum  light, GLenum  pname, const GLfloat *params)
    {
        // TODO
    }
    void glLightModelf(GLenum  pname, GLfloat *param)
    {
        // TODO
    }
    void glLightModelfv(GLenum  pname, const GLfloat *params)
    {
        // TODO
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
                    if(g_current_draw_list)
                        g_current_draw_list->texture_env[g_active_texture].mode = param;
                    else
                        g_texture_env[g_active_texture].mode = param;

                break;

                case GL_COMBINE_RGB: //GL_COMBINE_RGB_EXT
                    if(g_current_draw_list)
                        g_current_draw_list->texture_env[g_active_texture].combine_rgb = param;
                    else
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
                    if(g_current_draw_list)
                        g_current_draw_list->texture_env[g_active_texture].color = glm::vec4(
                            static_cast<float>(params[0]) / 255.0,
                            static_cast<float>(params[1]) / 255.0,
                            static_cast<float>(params[2]) / 255.0,
                            static_cast<float>(params[3]) / 255.0
                        );
                    else
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
        if(cap == GL_TEXTURE_2D)
        {
            if(g_current_draw_list)
                g_current_draw_list->texture_env[g_active_texture].enabled = true;
            else
                g_texture_env[g_active_texture].enabled = true;
        }

        ::glEnable(cap);
    }

    void glDisable(GLenum cap)
    {
        if(cap == GL_TEXTURE_2D)
        {
            if(g_current_draw_list)
                g_current_draw_list->texture_env[g_active_texture].enabled = false;
            else
                g_texture_env[g_active_texture].enabled = false;
        }

        ::glDisable(cap);
    }

    void glColorPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
    {
        // TODO
    }

    void glDisableClientState(GLenum array)
    {
        // TODO
    }

    void glEnableClientState(GLenum array)
    {
        // TODO
    }

    void glNormalPointer(GLenum type, GLsizei stride, const GLvoid *pointer)
    {
        // TODO
    }

    void glTexCoordPointer(GLint size, GLenum type, GLsizei stride, const GLvoid *pointer)
    {
        // TODO
    }

    void glVertexPointer(GLint size, GLenum type, GLsizei stride, GLvoid *pointer)
    {
        // TODO
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