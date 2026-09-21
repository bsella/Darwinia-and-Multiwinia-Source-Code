#include <GL/glew.h>

#include "lib/profiler.h"
#include "app.h"
#include "camera.h"
#include "location.h"
#include "renderer.h"
#include "water_reflection.h"

#include "FFP_emulation.h"

#include <iostream>
#include <chrono>

static unsigned long long GetTickCount()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

WaterReflectionEffect* WaterReflectionEffect::Create()
{
	return new WaterReflectionEffect();
}

WaterReflectionEffect::WaterReflectionEffect()
	: m_width (g_app->m_renderer->ScreenW()/2)
	, m_height(g_app->m_renderer->ScreenH()/2)
{
	{
		GLint old_fbo = 0;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &old_fbo);

		glGenFramebuffers(1, &m_framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	
		// The texture we're going to render to
		glGenTextures(1, &m_texture);
	
		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, m_texture);
	
		// Give an empty image to OpenGL ( the last "0" )
		glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	
		// Poor filtering. Needed !
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
		glGenRenderbuffers(1, &m_depth_buffer);
		glBindRenderbuffer(GL_RENDERBUFFER, m_depth_buffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth_buffer);

		// Set "renderedTexture" as our colour attachement #0
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_texture, 0);
	
		// Set the list of draw buffers.
		GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

		glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
	}


	GLuint vertex_shader, fragment_shader;
	
	constexpr const char* vertex_shader_source =
R"(
#version 410

uniform mat4 u_model_view_projection;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec4 in_colour;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec2 in_uv0_texcoord;
layout (location = 4) in vec2 in_uv1_texcoord;

layout (location = 0) out vec4 out_pos_world;
layout (location = 1) out vec4 out_refl_coord;
layout (location = 2) out vec4 out_colour;

void main()
{
	out_pos_world  = vec4(in_position, 1.0);
	gl_Position    = u_model_view_projection * vec4(in_position, 1.0);
	out_refl_coord = u_model_view_projection * vec4(in_position + in_normal, 1.0);
	out_colour     = in_colour;
}
)";

constexpr const char* fragment_shader_source =
R"(
#version 410

uniform sampler2D water;
uniform vec2 u_time; // x=time in seconds, y=small wave amplitude (eg. 0.003)

layout (location = 0) in vec4 in_pos_world;  //xz
layout (location = 1) in vec4 in_refl_coord; //xyw
layout (location = 2) in vec4 in_colour;     //xyz

layout (location = 0) out vec4 out_colour;

void main()
{
	// big mesh waves
	vec2 uv = 0.5*( vec2(1.0, 1.0)-in_refl_coord.xy/in_refl_coord.w );
	// small shader waves
	vec2 tmp = abs(dFdx(in_pos_world.xz));
	uv += u_time.y*sin((0.2*in_pos_world.yy+in_pos_world.xz)*0.25+u_time.xx*4)/(tmp.x+tmp.y);
	// fix 1pix seam between water and land
	uv.y += 0.004;
	// blend

	uv.y = -uv.y;

	out_colour = 0.7 * in_colour + 0.7 * (in_colour.r+in_colour.g+in_colour.b) * texture(water,uv);
	out_colour.a = 0.7;
}
)";

	int success;
	char infoLog[512];

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
		m_program = glCreateProgram();
		glAttachShader(m_program, vertex_shader);
		glAttachShader(m_program, fragment_shader);
		glLinkProgram(m_program);
		// print linking errors if any
		glGetProgramiv(m_program, GL_LINK_STATUS, &success);
		if(!success)
		{
			glGetProgramInfoLog(m_program, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		}
	}

	m_time_location               = glGetUniformLocation(m_program, "u_time");
	m_world_view_projection_loc   = glGetUniformLocation(m_program, "u_model_view_projection");
	m_water_texture_location      = glGetUniformLocation(m_program, "water");

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);
}

void WaterReflectionEffect::PreRenderWaterReflection()
{
	START_PROFILE(g_app->m_profiler, "Water Pre-render");

	//
	// Prepare render target

	GLint old_fbo = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &old_fbo);

	// Render to our framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
	glViewport( 0, 0, m_width, m_height );

	GLdouble plane[4] = {0,-1,0,0};
	glClipPlane(GL_CLIP_PLANE2,plane); // planes 0 and 1 used by radar
	glEnable(GL_CLIP_PLANE2);

	//
	// Clipping hack start

	glMatrixMode(GL_MODELVIEW);

	//
	// Clear

	START_PROFILE(g_app->m_profiler, "Render Clear");
	glClearColor(0.3f,0.3f,0.3f,0);
	glClear	(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glClearColor(0,0,0,0);
	END_PROFILE(g_app->m_profiler, "Render Clear");

	//
	// Render mirrored scene without water, spiritreceiver, details

	m_prerendering = true;
	/*int buildingDetail = g_prefsManager->GetInt( "RenderBuildingDetail", 1 );
	g_prefsManager->SetInt( "RenderBuildingDetail", 3 );
	int entityDetail = g_prefsManager->GetInt( "RenderEntityDetail", 1 );
	g_prefsManager->SetInt( "RenderEntityDetail", 3 );
	int landscapeDetail = g_prefsManager->GetInt( "RenderLandscapeDetail", 1 );
	g_prefsManager->SetInt( "RenderLandscapeDetail", 3 );*/
	g_app->m_camera->WaterReflect();
	g_app->m_location->WaterReflect();
	g_app->m_location->SetupLights();
	g_app->m_renderer->SetupMatricesFor3D();
	g_app->m_renderer->UpdateTotalMatrix();
	g_app->m_location->Render( false );
	// optional: reflect particles
	//g_app->m_particleSystem->Render();
	g_app->m_camera->WaterReflect();
	g_app->m_location->WaterReflect();
	g_app->m_renderer->SetupMatricesFor3D();
	g_app->m_renderer->UpdateTotalMatrix();
	g_app->m_location->SetupLights();
	/*g_prefsManager->SetInt( "RenderLandscapeDetail", landscapeDetail );
	g_prefsManager->SetInt( "RenderEntityDetail", entityDetail );
	g_prefsManager->SetInt( "RenderBuildingDetail", buildingDetail );*/
	m_prerendering = false;

	//
	// Clipping hack end

	//
	// Restore render target

	glEnable(GL_CULL_FACE);
	glDisable(GL_CLIP_PLANE2);

	glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
	glViewport( 0, 0, g_app->m_renderer->ScreenW(), g_app->m_renderer->ScreenH() );

	CHECK_OPENGL_STATE();
	END_PROFILE(g_app->m_profiler, "Water Pre-render");
}

void WaterReflectionEffect::Start()
{
	glUseProgram(m_program);

	glUniform2f(m_time_location, (GetTickCount()%10000000)/1000.0f, 0.001f*1024/g_app->m_renderer->ScreenW());

	glUniformMatrix4fv(m_world_view_projection_loc, 1, GL_FALSE, g_app->m_renderer->GetTotalMatrix());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_texture);

	glUniform1i(m_water_texture_location, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

void WaterReflectionEffect::Stop()
{
}

WaterReflectionEffect::~WaterReflectionEffect()
{
	glDeleteProgram(m_program);
}

WaterReflectionEffect* g_waterReflectionEffect = nullptr;

unsigned int WaterReflectionEffect::GetProgram() const
{
	return m_program;
}