#include <GL/glew.h>

#include "deform.h"

#include "FFP_VertexData.h"
#include "app.h"
#include "camera.h"
#include "explosion.h"
#include "location.h"
#include "renderer.h"
#include "lib/profiler.h"
#include "lib/resource.h"
#include "lib/vector3.h"

#include "FFP_emulation.h"

#include <GL/glext.h>
#include <array>
#include <chrono>

#define STRETCH_RECT // should make FSAA possible (for small penalty with non-FSAA setups)

// conserved for future testing several deformation types
//#define SOUL_TYPES 10
//float soulSize [SOUL_TYPES] = {0.37f,0.37f,0.37f,0.32f,0.6f,0.4f,0.60f,0.50f,0.40f};
//float soulDepth[SOUL_TYPES] = {0.10f,0.05f,0.15f,0.12f,0.1f,0.1f,0.05f,0.15f,0.15f};
//int soulType = 0;

static unsigned long long GetTickCount()
{
	using namespace std::chrono;
	return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}


struct PunchParams
{
	const char* punchTable;
	float seconds;
	int fadeType;
};

enum PunchFade
{
	PF_OLD,
	PF_NEW,
};

// sucks inwards and returns back
//PunchParams punchParams = {"textures/deform3.bmp",1,PF_OLD};
// ripples
PunchParams punchParams = {"textures/deform36c.bmp",1,PF_NEW};

DeformEffect* DeformEffect::Create()
{
	return new DeformEffect();
}

DeformEffect::DeformEffect()
	: m_deform_texture_width(g_app->m_renderer->ScreenW()/2)
	, m_deform_texture_height(g_app->m_renderer->ScreenH()/2)
{
constexpr const char* vertex_shader_source = R"(
#version 410

uniform mat4 u_model_view_projection;

layout (location = 0) in vec3 in_position;
layout (location = 1) in vec4 in_colour;
layout (location = 2) in vec3 in_normal;
layout (location = 3) in vec2 in_uv0_texcoord;
layout (location = 4) in vec2 in_uv1_texcoord;

layout (location = 0) out vec2 out_uv;

void main()
{
	gl_Position = vec4(in_position, 1.0);
	
	out_uv.x = 0.5*(1+in_position.x)+0.0003;
	out_uv.y = 0.5*(1-in_position.y)+0.0003;
}
)";

constexpr const char* fullscreen_source =
R"(
#version 410

uniform sampler2D screen_sampler;
uniform sampler2D deform_sampler;

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_colour;

// greater = higher precision (use against banding),
//  but max deformation limited to 1/DEFORM_PRECISION of screen
const uint DEFORM_PRECISION = 10;

void main()
{
	vec2 uv = in_uv;

	uv.y = 1 - in_uv.y;

	vec4 delta = texture(deform_sampler, uv);
	out_colour = texture(screen_sampler, uv + (delta.xy-delta.zw)/DEFORM_PRECISION);
}
)";

constexpr const char* sphere_source =
R"(
#version 410

uniform sampler2D screen_sampler;
uniform sampler2D deform_sampler;

uniform float u_time;
uniform vec2  u_center;
uniform float u_inv_size;
uniform float u_height;
uniform float u_aspect;

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_colour;

// greater = higher precision (use against banding),
//  but max deformation limited to 1/DEFORM_PRECISION of screen
const uint DEFORM_PRECISION = 10;

void main()
{
	vec2 dir = (in_uv - u_center);
	float dist = length(dir * vec2(u_aspect, 1.0));
	vec2 delta = texture(deform_sampler, vec2(u_time, dist * u_inv_size)).x * u_height/ dist * dir;
	delta *= DEFORM_PRECISION;
	out_colour = vec4(delta.x,delta.y,-delta.x,-delta.y);
}
)";

constexpr const char* diamond_source =
R"(
#version 410

uniform sampler2D deform_sampler;

uniform float u_time;
uniform vec2  u_center;
uniform float u_inv_size;
uniform float u_height;
uniform float u_aspect;

layout (location = 0) in vec2 in_uv;

layout (location = 0) out vec4 out_colour;

// greater = higher precision (use against banding),
//  but max deformation limited to 1/DEFORM_PRECISION of screen
const uint DEFORM_PRECISION = 10;

void main()
{
	vec2 dir = in_uv - u_center;
	float dist = abs(dir.x) * u_aspect + abs(dir.y);
	vec2 delta = -texture(deform_sampler, vec2(u_time, 1 - dist * u_inv_size)).x * u_height / dist * dir;
	delta *= DEFORM_PRECISION;
	if(delta.x == 0.0 && delta.y == 0.0)
		discard;
	out_colour = vec4(delta.x, delta.y, -delta.x, -delta.y);
}
)";

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
		{
			fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	
			glShaderSource(fragment_shader, 1, &fullscreen_source, NULL);
			
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
			m_deform_fullscreen_program = glCreateProgram();
			glAttachShader(m_deform_fullscreen_program, vertex_shader);
			glAttachShader(m_deform_fullscreen_program, fragment_shader);
			glLinkProgram(m_deform_fullscreen_program);
			// print linking errors if any
			glGetProgramiv(m_deform_fullscreen_program, GL_LINK_STATUS, &success);
			if(!success)
			{
				glGetProgramInfoLog(m_deform_fullscreen_program, 512, NULL, infoLog);
				std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
			}
		}

		glDeleteShader(fragment_shader);
	}

	{
		{
			fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	
			glShaderSource(fragment_shader, 1, &sphere_source, NULL);
			
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
			m_deform_sphere_program = glCreateProgram();
			glAttachShader(m_deform_sphere_program, vertex_shader);
			glAttachShader(m_deform_sphere_program, fragment_shader);
			glLinkProgram(m_deform_sphere_program);
			// print linking errors if any
			glGetProgramiv(m_deform_sphere_program, GL_LINK_STATUS, &success);
			if(!success)
			{
				glGetProgramInfoLog(m_deform_sphere_program, 512, NULL, infoLog);
				std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
			}
		}

		glDeleteShader(fragment_shader);
	}

	{
		{
			fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	
			glShaderSource(fragment_shader, 1, &diamond_source, NULL);
			
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
			m_deform_diamond_program = glCreateProgram();
			glAttachShader(m_deform_diamond_program, vertex_shader);
			glAttachShader(m_deform_diamond_program, fragment_shader);
			glLinkProgram(m_deform_diamond_program);
			// print linking errors if any
			glGetProgramiv(m_deform_diamond_program, GL_LINK_STATUS, &success);
			if(!success)
			{
				glGetProgramInfoLog(m_deform_diamond_program, 512, NULL, infoLog);
				std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
			}
		}

		glDeleteShader(fragment_shader);
	}

	glDeleteShader(vertex_shader);

	{
		GLint old_fbo = 0;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &old_fbo);

		glGenFramebuffers(1, &m_deform_framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_deform_framebuffer);
	
		// The texture we're going to render to
		glGenTextures(1, &m_deform_texture);
	
		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, m_deform_texture);
	
		// Give an empty image to OpenGL ( the last "0" )
		glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, m_deform_texture_width, m_deform_texture_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	
		// Poor filtering. Needed !
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		// Set "renderedTexture" as our colour attachement #0
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_deform_texture, 0);
	
		// Set the list of draw buffers.
		GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

		glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
	}

	{
		GLint old_fbo = 0;
		glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &old_fbo);

		glGenFramebuffers(1, &m_screen_framebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, m_screen_framebuffer);
	
		// The texture we're going to render to
		glGenTextures(1, &m_screen_texture);
	
		// "Bind" the newly created texture : all future texture functions will modify this texture
		glBindTexture(GL_TEXTURE_2D, m_screen_texture);
	
		// Poor filtering. Needed !
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

		// Give an empty image to OpenGL ( the last "0" )
		glTexImage2D(GL_TEXTURE_2D, 0,GL_RGBA, g_app->m_renderer->ScreenW(), g_app->m_renderer->ScreenH(), 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

		{
			glGenRenderbuffers(1, &m_screen_depth_buffer);
			glBindRenderbuffer(GL_RENDERBUFFER, m_screen_depth_buffer);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, g_app->m_renderer->ScreenW(), g_app->m_renderer->ScreenH());
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_screen_depth_buffer);
		}

		// Set "renderedTexture" as our colour attachement #0
		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, m_screen_texture, 0);
	
		// Set the list of draw buffers.
		GLenum DrawBuffers[1] = {GL_COLOR_ATTACHMENT0};
		glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

		glBindFramebuffer(GL_FRAMEBUFFER, old_fbo);
	}
}

//#define PUNCH_FIRST 10
//#define PUNCH_LAST 36
//static unsigned punchtest = PUNCH_LAST;

void DeformEffect::AddPunch(const Vector3& pos, float range)
{
	Punch punch;
	punch.m_pos = pos;
	punch.m_range = range;
	punch.m_birth = GetTickCount();
	m_punchList.PutData(punch);

//	punchtest++;
//	if(punchtest>PUNCH_LAST) punchtest=PUNCH_FIRST;
//	DebugOut("Doing punch %d.\n",punchtest);
}

void DeformEffect::Start()
{
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &m_old_render_target);
#ifndef STRETCH_RECT
	glBindFramebuffer(GL_FRAMEBUFFER, m_screen_framebuffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
#endif
}

void DeformEffect::RenderSpritesBegin(unsigned int program, int deform_texture)
{
	glUseProgram(program);
	
	glActiveTexture(GL_TEXTURE0);
	m_old_is_texture0_enabled = glIsEnabled(GL_TEXTURE_2D);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_old_texture0_binding);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, deform_texture);

	glUniform1i(glGetUniformLocation(program, "deform_sampler"), 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT ); //time
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE ); // distance
	
	glActiveTexture(GL_TEXTURE1);
	m_old_is_texture1_enabled = glIsEnabled(GL_TEXTURE_2D);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &m_old_texture1_binding);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_screen_texture);

	glUniform1i(glGetUniformLocation(program, "screen_sampler"), 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//OpenGLD3D::g_pd3dDevice->SetSamplerState(screenSampler,D3DSAMP_MIPFILTER,D3DTEXF_NONE);
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri ( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	// seems to be partially ignored on my card,
	//  verify that it's my card's bug or fix it
}

// differs from Camera::Get2DScreenPos by viewport, this one returns coords in range 0..1
static float Get2DScreenPos(Vector3 const &_vector, float& _screenX, float& _screenY)
{
	double outX, outY, outZ;

	int viewport[4] = {0,0,1,1};
	double viewMatrix[16];
	double projMatrix[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, viewMatrix);
	glGetDoublev(GL_PROJECTION_MATRIX, projMatrix);
	gluProject(_vector.x, _vector.y, _vector.z,
		viewMatrix,
		projMatrix,
		viewport,
		&outX,
		&outY,
		&outZ);

	_screenX = outX;
	_screenY = outY;
	return outZ;
}

// depth != 0 -> render sprite with given width/depth
// depth == 0 -> render fullscreen
void DeformEffect::RenderSprite(unsigned int program, const Vector3& posWorld, float width, float depth, float redux)
{
	Vector2 posScreen = Vector2(0,0);
	Vector2 posScreen2 = Vector2(0,0);
	//Vector3 lookDir = (posWorld-g_app->m_camera->GetPos()).Normalise();
	float size = 0.001f;
	float aspect = g_app->m_renderer->ScreenW()/(float)g_app->m_renderer->ScreenH();
	if(Get2DScreenPos(posWorld, posScreen.x, posScreen.y)<1)
	{
		//Vector3 posWorld2 = posWorld + Vector3(lookDir.z,0,-lookDir.x).Normalise();
		//float z = Get2DScreenPos(posWorld2, posScreen2.x, posScreen2.y);
		if(!(posScreen.x<-1 || posScreen.x>2 || posScreen.y<-1 || posScreen.y>2))
		{
			Vector2 tmp = posScreen-posScreen2;
			size = 10*Vector2(tmp.x*aspect,tmp.y).Mag();
			if(size>0.4f) size = 0.4f;
		}
	}

	glUniform2f(glGetUniformLocation(program, "u_center"), posScreen.x, 1-posScreen.y);
	glUniform1f(glGetUniformLocation(program, "u_inv_size"), 1/(size*width));
	glUniform1f(glGetUniformLocation(program, "u_height"), (size*width)*depth*4);

	posScreen = posScreen*2-Vector2(1,1);
	Vector2 size2 = Vector2(size*width,size*width*aspect)*redux;
	if(!depth)
	{
		posScreen = Vector2(0,0);
		size2 = Vector2(1,1);
	}
	
	m_sprite_vertex_buffer.update(
		std::array{
		ffp_emulation::VertexData{posScreen.x-size2.x, posScreen.y-size2.y, 0, 0, 0, 0, 0,0,0,0,0,0,0,0},
		ffp_emulation::VertexData{posScreen.x-size2.x, posScreen.y+size2.y, 0, 0, 0, 0, 0,0,0,0,0,0,0,0},
		ffp_emulation::VertexData{posScreen.x+size2.x, posScreen.y+size2.y, 0, 0, 0, 0, 0,0,0,0,0,0,0,0},
		ffp_emulation::VertexData{posScreen.x-size2.x, posScreen.y-size2.y, 0, 0, 0, 0, 0,0,0,0,0,0,0,0},
		ffp_emulation::VertexData{posScreen.x+size2.x, posScreen.y+size2.y, 0, 0, 0, 0, 0,0,0,0,0,0,0,0},
		ffp_emulation::VertexData{posScreen.x+size2.x, posScreen.y-size2.y, 0, 0, 0, 0, 0,0,0,0,0,0,0,0},
	});
	m_sprite_vertex_buffer.draw_buffer(GL_TRIANGLES, program);
}

void DeformEffect::RenderSpritesEnd([[maybe_unused]] unsigned int program)
{
	glActiveTexture(GL_TEXTURE0);
	m_old_is_texture0_enabled ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_old_texture0_binding);

	glActiveTexture(GL_TEXTURE1);
	m_old_is_texture1_enabled ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, m_old_texture1_binding);

	glActiveTexture(GL_TEXTURE0);
}

struct SoulDistance
{
	Spirit* m_spirit;
	float   m_distance;
};

static int CompareSoulDistance(const void * elem1, const void * elem2 )
{
	SoulDistance *s1 = (SoulDistance*) elem1;
	SoulDistance *s2 = (SoulDistance*) elem2;

	if      ( s1->m_distance < s2->m_distance )     return +1;
	else if ( s1->m_distance > s2->m_distance )     return -1;
	else                                            return 0;
}

void DeformEffect::Stop()
{
	// possible optimization: immediately return when deformations are not visible

	START_PROFILE(g_app->m_profiler, "Deform Pre-render");

	// conserved for future testing several deformation types
	//extern signed char g_keyDeltas[256];
	//if(g_keyDeltas[' ']==1)
	//{
	//	extern int soulType;
	//	++soulType %= SOUL_TYPES;
	//	DebugOut("Soul effect=%d.\n",soulType);
	//}

	glDepthMask(0);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

#ifdef STRETCH_RECT
	// copy screen
	glBindFramebuffer(GL_FRAMEBUFFER, m_old_render_target);

	glBindTexture(GL_TEXTURE_2D, m_screen_texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 0, 0, g_app->m_renderer->ScreenW(),g_app->m_renderer->ScreenH(), 0 );
#endif

	// build deforem table
	glBindFramebuffer(GL_FRAMEBUFFER, m_deform_framebuffer);
	{
		glViewport( 0, 0, m_deform_texture_width, m_deform_texture_height );
		glClear(GL_COLOR_BUFFER_BIT);
	
		// - spirits
		//   sort them
		// Note : The spirit/soul deform effect does not look very good
		//        and can be disturbing when applied on multiple souls.
		if (0)
		{
			std::vector<SoulDistance> sorted(g_app->m_location->m_spirits.Size());
			unsigned numSouls = 0;
			for(int i=0;i<g_app->m_location->m_spirits.Size();i++)
			{
				if( g_app->m_location->m_spirits.ValidIndex(i) )
				{
					Spirit *r = g_app->m_location->m_spirits.GetPointer(i);
					if(r->m_state!=Spirit::StateAttached)
					{
						sorted[numSouls].m_spirit = r;
						sorted[numSouls].m_distance = (r->m_pos-g_app->m_camera->GetPos()).MagSquared();
						numSouls++;
					}
				}
			}
			qsort(sorted.data(),numSouls,sizeof(SoulDistance),CompareSoulDistance);
			//   render them sorted
			RenderSpritesBegin(m_deform_diamond_program,g_app->m_resource->GetTexture("textures/deform1c.bmp", false, false));
			glUniform1f(glGetUniformLocation(m_deform_diamond_program, "u_aspect"), g_app->m_renderer->ScreenW()/(float)g_app->m_renderer->ScreenH());
			glUniform1f(glGetUniformLocation(m_deform_diamond_program, "u_time"), (GetTickCount()%10000)/1000.0f);
			for(unsigned i=0;i<numSouls;i++)
			{
				RenderSprite(m_deform_diamond_program,sorted[i].m_spirit->m_pos,0.32f,0.12f,1.3f);
			}
			RenderSpritesEnd(m_deform_diamond_program);
		}
	
		// - explosions
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE,GL_ONE);
		//char buf[100];sprintf(buf,"textures/deform%d.bmp",punchtest);
		RenderSpritesBegin(m_deform_sphere_program,g_app->m_resource->GetTexture(punchParams.punchTable, false, false));
		glUniform1f(glGetUniformLocation(m_deform_sphere_program, "u_aspect"), g_app->m_renderer->ScreenW()/(float)g_app->m_renderer->ScreenH());
		auto now = GetTickCount();
		for(unsigned i=m_punchList.Size();i--;)
		{
			if( m_punchList.ValidIndex(i) )
			{
				const Punch* p = m_punchList.GetPointer(i);
				float age = (now-p->m_birth)/1000.0f;
				const float maxage = punchParams.seconds;
				if(age>=maxage)
				{
					m_punchList.RemoveData(i);
				}
				else
				{
					float width;
					float depth;
					switch(punchParams.fadeType)
					{
						case PF_OLD:
							width = 10*(maxage-(maxage-age)*(maxage-age)/maxage);
							depth = 0.15f*(1-age/maxage);
							break;
						case PF_NEW:
							width = 10;
							// depth fadeout curve is hardcoded here rather than painted in .bmp,
							// to avoid banding from low precision (8bit)
							depth = 0.03*(1-age/maxage)*(1-age/maxage);
							break;
					}
					float time = age/maxage;
					if(time<0.03f) time=0.03f; // clamp on CPU
					glUniform1f(glGetUniformLocation(m_deform_sphere_program, "u_time"), sqrtf(time));
	
					RenderSprite(m_deform_sphere_program,p->m_pos,width,depth,0.8f);
				}
			}
		}
		RenderSpritesEnd(m_deform_sphere_program);
		glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_BLEND);
	}

	END_PROFILE(g_app->m_profiler, "Deform Pre-render");

	START_PROFILE(g_app->m_profiler, "Deform Apply");

	// deform screen according to deform table
	glBindFramebuffer(GL_FRAMEBUFFER, m_old_render_target);
	{
		glViewport( 0, 0, g_app->m_renderer->ScreenW(), g_app->m_renderer->ScreenH() );
		RenderSpritesBegin(m_deform_fullscreen_program, m_deform_texture);
		RenderSprite(m_deform_fullscreen_program,Vector3(0,0,0),1,0,1);
		RenderSpritesEnd(m_deform_fullscreen_program);
	}

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(1);

	END_PROFILE(g_app->m_profiler, "Deform Apply");
}

DeformEffect::~DeformEffect()
{
	glDeleteProgram(m_deform_diamond_program);
	glDeleteProgram(m_deform_sphere_program);
	glDeleteProgram(m_deform_fullscreen_program);
	glDeleteRenderbuffers(1, &m_screen_depth_buffer);
	glDeleteTextures(1, &m_deform_texture);
	glDeleteTextures(1, &m_screen_texture);
	glDeleteFramebuffers(1, &m_deform_framebuffer);
	glDeleteFramebuffers(1, &m_screen_framebuffer);
}

DeformEffect* g_deformEffect = nullptr;
