//#include "FFP_emulation.h"
#include <GL/glew.h>

#include <cassert>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/glm.hpp>

#include <GL/glext.h>
#include <cstdint>
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
            float u, v;
        };

        std::vector<VertexData> g_vertex_buffer;

        VertexData g_current_vertex;

        GLuint g_vao = 0;
        GLuint g_vbo;

        GLuint g_program;

        std::stack<glm::mat4> g_model_view;
        std::stack<glm::mat4> g_projection;

        GLint g_model_view_location;
        GLint g_projection_location;

        unsigned int g_vertex_index;

        constexpr const char* vertex_shader_source =
R"(
#version 410

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec4 in_colour;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec2 in_uv_texcoord;

layout (location = 0) out vec4 out_colour;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec2 out_uv_texcoord;

uniform mat4 u_model_view;
uniform mat4 u_projection;

void main()
{
    gl_Position = u_projection * u_model_view * vec4(in_position, 1.0);

    out_colour      = in_colour;
    out_normal      = in_normal;
    out_uv_texcoord = in_uv_texcoord;
}
)";


constexpr const char* fragment_shader_source =
R"(
#version 410

layout (location = 0) in vec4 in_colour;
layout (location = 1) in vec3 in_normal;
layout (location = 2) in vec2 in_uv_texcoord;

layout (location = 0) out vec4 out_colour;

void main()
{
    out_colour = in_colour;
}
)";

    }

    void init()
    {
        glGenVertexArrays(1, &g_vao);
        glBindVertexArray(g_vao);

        {
            glGenBuffers(1, &g_vbo);

            glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), ((std::uint8_t*)nullptr));
            glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), ((std::uint8_t*)nullptr) + 3  * sizeof(float));
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), ((std::uint8_t*)nullptr) + 7  * sizeof(float));
            glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), ((std::uint8_t*)nullptr) + 10 * sizeof(float));

            glEnableVertexAttribArray(0);
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glEnableVertexAttribArray(3);
        }

        glBindVertexArray(0);


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

        g_model_view_location = glGetUniformLocation(g_program, "u_model_view");
        g_projection_location = glGetUniformLocation(g_program, "u_projection");

        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);

        g_model_view.push(glm::mat4(1.0));
        g_projection.push(glm::mat4(1.0));
    }

    void quit()
    {

    }

    void glBegin(GLenum mode)
    {
        g_primitive_mode = mode;

        g_vertex_buffer.clear();

        g_vertex_index = 0;
    }

    void glEnd()
    {
        glUseProgram(g_program);

        // Upload the vertex data buffer
        glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
        glBufferData(GL_ARRAY_BUFFER, g_vertex_buffer.size() * sizeof(VertexData), g_vertex_buffer.data(), GL_DYNAMIC_DRAW);

        // Upload the matrices
        glUniformMatrix4fv(g_model_view_location, 1, GL_FALSE, glm::value_ptr(g_model_view.top()));
        glUniformMatrix4fv(g_projection_location, 1, GL_FALSE, glm::value_ptr(g_projection.top()));

        auto primitive_mode = g_primitive_mode;
        if (primitive_mode == GL_QUADS)      primitive_mode = GL_TRIANGLES;
        if (primitive_mode == GL_QUAD_STRIP) primitive_mode = GL_TRIANGLES;

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
        glBindVertexArray(g_vao);
        glDrawArrays(primitive_mode, 0, g_vertex_buffer.size());
    }

    void glMatrixMode(GLenum mode)
    {
        g_matrix_mode = mode;
    }

    void glLoadIdentity()
    {
        assert(g_matrix_mode == GL_MODELVIEW || g_matrix_mode == GL_PROJECTION);

        switch (g_matrix_mode)
        {
            case GL_MODELVIEW:  g_model_view.top() = glm::mat4(1.0); break;
            case GL_PROJECTION: g_projection.top() = glm::mat4(1.0); break;
        }
    }

    void glMultMatrixf(const GLfloat *m)
    {
        glm::mat4 mat(
            m[0],  m[1],  m[2],  m[3],
            m[4],  m[5],  m[6],  m[7],
            m[8],  m[9],  m[10], m[11],
            m[12], m[13], m[14], m[15]
        );

        assert(g_matrix_mode == GL_MODELVIEW || g_matrix_mode == GL_PROJECTION);

        switch (g_matrix_mode)
        {
            case GL_MODELVIEW:  g_model_view.top() *= mat; break;
            case GL_PROJECTION: g_projection.top() *= mat; break;
        }
    }

    void glPushMatrix()
    {
        assert(g_matrix_mode == GL_MODELVIEW || g_matrix_mode == GL_PROJECTION);

        switch (g_matrix_mode)
        {
            case GL_MODELVIEW:  g_model_view.push(glm::mat4(1.0)); break;
            case GL_PROJECTION: g_projection.push(glm::mat4(1.0)); break;
        }
    }

    void glPopMatrix()
    {
        assert(g_matrix_mode == GL_MODELVIEW || g_matrix_mode == GL_PROJECTION);

        switch (g_matrix_mode)
        {
            case GL_MODELVIEW:  g_model_view.pop(); break;
            case GL_PROJECTION: g_projection.pop(); break;
        }
    }

    void glLoadMatrixd(const GLdouble *m)
    {
        assert(g_matrix_mode == GL_MODELVIEW || g_matrix_mode == GL_PROJECTION);

        switch (g_matrix_mode)
        {
            case GL_MODELVIEW:  g_model_view.top() = glm::mat4(
            m[0],  m[1],  m[2],  m[3],
            m[4],  m[5],  m[6],  m[7],
            m[8],  m[9],  m[10], m[11],
            m[12], m[13], m[14], m[15]
            );
            break;
            case GL_PROJECTION: g_projection.top() = glm::mat4(
            m[0],  m[1],  m[2],  m[3],
            m[4],  m[5],  m[6],  m[7],
            m[8],  m[9],  m[10], m[11],
            m[12], m[13], m[14], m[15]
            );
            break;
        }
    }

    void gluLookAt(GLdouble pos_x, GLdouble pos_y, GLdouble pos_z,
                   GLdouble forwards_x, GLdouble forwards_y, GLdouble forwards_z,
                   GLdouble up_x, GLdouble up_y, GLdouble up_z)
    {
        assert(g_matrix_mode == GL_MODELVIEW || g_matrix_mode == GL_PROJECTION);

        glm::vec3 eye{pos_x, pos_y, pos_z};
        glm::vec3 forwards{forwards_x, forwards_y,  forwards_z};

        glm::vec3 target = eye + forwards;
        glm::vec3 up{up_x, up_y, up_z};

        switch (g_matrix_mode)
        {
            case GL_MODELVIEW:  g_model_view.top() = glm::lookAt(eye, target, up); break;
            case GL_PROJECTION: g_projection.top() = glm::lookAt(eye, target, up); break;
        }
    }

    void gluPerspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar)
    {
        assert(g_matrix_mode == GL_PROJECTION);

        switch (g_matrix_mode)
        {
            case GL_MODELVIEW:  g_model_view.top() = glm::perspective(fovy, aspect, zNear, zFar); break;
            case GL_PROJECTION: g_projection.top() = glm::perspective(fovy, aspect, zNear, zFar); break;
        }
    }

    void gluOrtho2D(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top)
    {
        assert(g_matrix_mode == GL_PROJECTION);

        switch (g_matrix_mode)
        {
            case GL_MODELVIEW:  g_model_view.top() = glm::ortho(left, right, bottom, top); break;
            case GL_PROJECTION: g_projection.top() = glm::ortho(left, right, bottom, top); break;
        }
    }

    void glVertex3f( GLfloat x, GLfloat y, GLfloat z )
    {
        g_current_vertex.x = x;
        g_current_vertex.y = y;
        g_current_vertex.z = z;
        
        if((g_primitive_mode == GL_QUADS || g_primitive_mode == GL_QUAD_STRIP) && ((g_vertex_index - 3) % 4) == 0 )
        {
            auto vertex1 = g_vertex_buffer[g_vertex_buffer.size() - 3];
            auto vertex2 = g_vertex_buffer[g_vertex_buffer.size() - 1];

            g_vertex_buffer.push_back(vertex1);
            g_vertex_buffer.push_back(vertex2);
        }

        else
        
        if(g_primitive_mode == GL_QUAD_STRIP && g_vertex_index > 4)
        {
            if(((g_vertex_index - 4) % 2) == 0)
            {
                auto vertex1 = g_vertex_buffer[g_vertex_buffer.size() - 3];
                auto vertex2 = g_vertex_buffer[g_vertex_buffer.size() - 1];
                g_vertex_buffer.push_back(vertex1);
                g_vertex_buffer.push_back(vertex2);
            }
            
            else

            if(((g_vertex_index - 5) % 2) == 0)
            {
                auto vertex1 = g_vertex_buffer[g_vertex_buffer.size() - 3];
                auto vertex2 = g_vertex_buffer[g_vertex_buffer.size() - 1];
                g_vertex_buffer.push_back(vertex1);
                g_vertex_buffer.push_back(vertex2);
            }
        }

        g_vertex_buffer.push_back(g_current_vertex);

        g_vertex_index++;
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
        g_current_vertex.u = u;
        g_current_vertex.v = v;
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

    GLuint glGenLists(GLsizei range)
    {
        // TODO
        return 0;
    }

    void glNewList(GLuint list, GLenum mode)
    {
        // TODO
    }

    void glEndList()
    {
        // TODO
    }

    void glCallList(GLuint list)
    {
        // TODO
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

    void glTexEnvi(GLenum target, GLenum pname, GLint param)
    {
        // TODO
    }
    void glTexEnviv(GLenum target, GLenum pname, const GLint* params)
    {
        // TODO
    }
    void glTexEnvf(GLenum target, GLenum pname, GLfloat param)
    {
        // TODO
    }
}