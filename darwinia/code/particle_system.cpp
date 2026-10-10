#include <GL/glew.h>
#include <cstddef>
#include <math.h>

#include "FFP_VertexData.h"
#include "lib/debug_utils.h"
#include "lib/math_utils.h"
#include "lib/profiler.h"
#include "lib/resource.h"

#include "app.h"
#include "camera.h"
#include "lib/rgb_colour.h"
#include "main.h"
#include "particle_system.h"
#include "location.h"

#include "FFP_emulation.h"

// ****************************************************************************
// ParticleType
// ****************************************************************************

// *** Constructor
ParticleType::ParticleType()
:	m_spin(0.0f),
	m_fadeInTime(0.0f),
	m_fadeOutTime(0.0f),
	m_life(10.0f),
	m_size(1000.0f),
	m_friction(0.0f),
	m_gravity(0.0f),
	m_colour1(0.5f, 0.5f, 0.5f),
	m_colour2(0.95f, 0.95f, 0.95f)
{
}


// ****************************************************************************
// Particle
// ****************************************************************************

ParticleType Particle::m_types[TypeNumTypes];

// *** Constructor
Particle::Particle(Vector3 const &_pos, Vector3 const &_vel, Type _typeId, float _size)
{
	DarwiniaDebugAssert(_typeId < Particle::TypeNumTypes);

	m_pos = _pos;
	m_vel = _vel;
	m_typeId = _typeId;

	ParticleType const &type = m_types[_typeId];
	float amount = frand();
	m_colour = type.m_colour1 * amount + type.m_colour2 * (1.0f - amount);

	if( _size == -1 )
	{
		m_size = type.m_size;
	}
	else
	{
		m_size = _size;
	}

	m_birthTime = g_gameTime;
}

Particle::operator bool() const
{
	return m_typeId != Type::TypeNone;
}

Particle& Particle::operator*()
{
	return *this;
}

const Particle& Particle::operator*() const
{
	return *this;
}

void Particle::reset()
{
	m_typeId = Type::TypeNone;
}

// *** Advance
// Returns true if this particle should be removed from the particle list
bool Particle::Advance()
{
	ParticleType const &type = m_types[m_typeId];
	double timeToDie = m_birthTime + m_types[m_typeId].m_life;
	if (g_gameTime > timeToDie)
	{
		return true;
	}

	m_pos += m_vel * SERVER_ADVANCE_PERIOD;

	float amount = SERVER_ADVANCE_PERIOD * type.m_friction;
	m_vel *= (1.0f - amount);

	if (type.m_gravity != 0.0f)
	{
		float gravityScale = m_size / m_types[m_typeId].m_size;
		m_vel.y -= SERVER_ADVANCE_PERIOD * type.m_gravity * gravityScale;
	}


	//
	// Particles that spawn particles

	bool particleCreated = false;

	if( m_typeId == TypeExplosionDebris )
	{
		if( frand() < 0.3f )
		{
			// *** WARNING ***
			// CreateParticle might have to resize the particles array, which
			// might invalidate what "p" points to, so we need to make local
			// copies of the bits of p that we need
			Vector3 pos = m_pos;
			Vector3 vel = m_vel / 5.0f;
			g_app->m_particleSystem->CreateParticle( pos, vel, Particle::TypeRocketTrail, m_size/2.0f );
			particleCreated = true;
		}
	}


	//
	// Particles that bounce

	if( !particleCreated )
	{
		if( m_typeId == TypeSpark ||
			m_typeId == TypeBrass ||
			m_typeId == TypeBlueSpark ||
			m_typeId == TypeLeaf )
		{
			float landHeight = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);
			if( m_pos.y <= landHeight )
			{
				Vector3 lastPos = m_pos;// - m_vel * g_advanceTime;
				Vector3 impactPos = (m_pos + lastPos) * 0.5f;
				m_pos = impactPos;
				m_pos.y = g_app->m_location->m_landscape.m_heightMap->GetValue(m_pos.x, m_pos.z);

				Vector3 normal = g_app->m_location->m_landscape.m_normalMap->GetValue(m_pos.x, m_pos.z);
				float dotProd = normal * m_vel;
				m_vel -= normal * 2.0f * dotProd * 0.7f;
			}
		}
	}

	return false;
}


// *** Render
void Particle::Render(float _predictionTime)
{
	double birthTime = m_birthTime;
	double deathTime = m_birthTime + m_types[m_typeId].m_life;
	float startFade = birthTime + m_types[m_typeId].m_life * 0.75f;
	int alpha;
	if( g_gameTime < startFade )
	{
		alpha = 90;
	}
	else
	{
		float fractionFade = (g_gameTime - startFade) / (deathTime - startFade);
		alpha = 90 - 90 * fractionFade;
		if( alpha < 0 ) alpha = 0;
	}

	Vector3 predictedPos = m_pos + _predictionTime * m_vel;
	float size = m_size/16.0f;
	Vector3 up(g_app->m_camera->GetUp() * size);
	Vector3 right(g_app->m_camera->GetRight() * size);

	if( m_typeId == TypeMissileTrail )
	{
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
		float fraction = (float) alpha / 100.0f;
		glColor4ub(m_colour.r*fraction, m_colour.g*fraction, m_colour.b*fraction, 0.0f );
	}
	else
	{
		glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
		glColor4ub(m_colour.r, m_colour.g, m_colour.b, alpha );
	}

	glBegin( GL_QUADS );
		glTexCoord2i(0, 0);
		glVertex3fv( (predictedPos - up).GetData() );

		glTexCoord2i(0, 1);
		glVertex3fv( (predictedPos + right).GetData() );

		glTexCoord2i(1, 1);
		glVertex3fv( (predictedPos + up).GetData() );

		glTexCoord2i(1, 0);
		glVertex3fv( (predictedPos - right).GetData() );
	glEnd();
}


// *** SetupParticles
void Particle::SetupParticles()
{
	m_types[TypeRocketTrail].m_life = 2.0f;
	m_types[TypeRocketTrail].m_size = 15.0f;
	m_types[TypeRocketTrail].m_gravity = 15.0f;
	m_types[TypeRocketTrail].m_friction = 0.6f;
	m_types[TypeRocketTrail].m_colour1.Set( 128, 128, 128 );
	m_types[TypeRocketTrail].m_colour2.Set( 200, 200, 200 );

	m_types[TypeExplosionCore].m_life = 2.0f;
	m_types[TypeExplosionCore].m_size = 150.0f;
	m_types[TypeExplosionCore].m_gravity = 10.0f;
	m_types[TypeExplosionCore].m_friction = 0.2f;
	m_types[TypeExplosionCore].m_colour1.Set( 200, 100, 100 );
	m_types[TypeExplosionCore].m_colour2.Set( 255, 120, 120 );

	m_types[TypeExplosionDebris].m_life = 6.0f;
	m_types[TypeExplosionDebris].m_size = 40.0f;
	m_types[TypeExplosionDebris].m_gravity = 20.0f;
	m_types[TypeExplosionDebris].m_friction = 0.2f;
	m_types[TypeExplosionDebris].m_colour1.Set( 200, 128, 128 );
	m_types[TypeExplosionDebris].m_colour2.Set( 250, 200, 200 );

	m_types[TypeMuzzleFlash].m_life = 1.0f;
	m_types[TypeMuzzleFlash].m_size = 10.0f;
	m_types[TypeMuzzleFlash].m_gravity = 0.0f;
	m_types[TypeMuzzleFlash].m_friction = 0.2f;
	m_types[TypeMuzzleFlash].m_colour1.Set( 255, 128, 128 );
	m_types[TypeMuzzleFlash].m_colour2.Set( 200, 100, 100 );

	m_types[TypeFire].m_life = 4.0f;
	m_types[TypeFire].m_size = 50.0f;
	m_types[TypeFire].m_gravity = -4.0f;
	m_types[TypeFire].m_friction = 0.0f;
	m_types[TypeFire].m_colour1.Set( 150, 50, 50 );
	m_types[TypeFire].m_colour2.Set( 150, 120, 50 );

	m_types[TypeControlFlash].m_life = 1.0f;
	m_types[TypeControlFlash].m_size = 30.0f;
	m_types[TypeControlFlash].m_gravity = -2.0f;
	m_types[TypeControlFlash].m_friction = 0.0f;
	m_types[TypeControlFlash].m_colour1.Set( 50, 50, 150 );
	m_types[TypeControlFlash].m_colour2.Set( 50, 50, 250 );

	m_types[TypeSpark].m_life = 4.0f;
	m_types[TypeSpark].m_size = 15.0f;
	m_types[TypeSpark].m_gravity = 15.0f;
	m_types[TypeSpark].m_friction = 1.5f;
	m_types[TypeSpark].m_colour1.Set( 250, 200, 0 );
	m_types[TypeSpark].m_colour2.Set( 250, 200, 50 );

	m_types[TypeBlueSpark].m_life = 4.0f;
	m_types[TypeBlueSpark].m_size = 15.0f;
	m_types[TypeBlueSpark].m_gravity = 15.0f;
	m_types[TypeBlueSpark].m_friction = 1.5f;
	m_types[TypeBlueSpark].m_colour1.Set( 50, 50, 200 );
	m_types[TypeBlueSpark].m_colour2.Set( 50, 70, 255 );

	m_types[TypeBrass].m_life = 2.0f;
	m_types[TypeBrass].m_size = 4.0f;
	m_types[TypeBrass].m_gravity = 30.0f;
	m_types[TypeBrass].m_friction = 0.5f;
	m_types[TypeBrass].m_colour1.Set( 250, 200, 0 );
	m_types[TypeBrass].m_colour2.Set( 250, 200, 50 );

	m_types[TypeMissileTrail].m_life = 5.0f;
	m_types[TypeMissileTrail].m_size = 200.0f;
	m_types[TypeMissileTrail].m_gravity = 1.0f;
	m_types[TypeMissileTrail].m_friction = 0.0f;
	m_types[TypeMissileTrail].m_colour1.Set( 100, 100, 100 );
	m_types[TypeMissileTrail].m_colour2.Set( 200, 200, 200 );

	m_types[TypeMissileFire].m_life = 0.5f;
	m_types[TypeMissileFire].m_size = 300.0f;
	m_types[TypeMissileFire].m_gravity = 0.0f;
	m_types[TypeMissileFire].m_friction = 0.0f;
	m_types[TypeMissileFire].m_colour1.Set( 150, 50, 50 );
	m_types[TypeMissileFire].m_colour2.Set( 150, 120, 50 );

	m_types[TypeDarwinianFire].m_life = 1.0f;
	m_types[TypeDarwinianFire].m_size =25.0f;
	m_types[TypeDarwinianFire].m_gravity = -10.0f;
	m_types[TypeDarwinianFire].m_friction = 0.0f;
	m_types[TypeDarwinianFire].m_colour1.Set( 150, 50, 50 );
	m_types[TypeDarwinianFire].m_colour2.Set( 150, 120, 50 );

	m_types[TypeLeaf].m_life = 60.0f;
	m_types[TypeLeaf].m_size = 25.0f;
	m_types[TypeLeaf].m_gravity = 5.0f;
	m_types[TypeLeaf].m_friction = 1.5f;
	m_types[TypeLeaf].m_colour1.Set( 50, 150, 50 );
	m_types[TypeLeaf].m_colour2.Set( 50, 200, 50 );
}

namespace
{
constexpr const char* vertex_shader_source =
R"(
#version 410
layout (location = 0) in vec3  in_position;
layout (location = 1) in vec3  in_velocity;
layout (location = 2) in float in_birth_time;
layout (location = 3) in float in_size;
layout (location = 4) in uint  in_colour;
layout (location = 5) in uint  in_type;

layout (location = 0) out float out_size;
layout (location = 1) out vec4  out_colour;

uniform float u_prediction_time;
uniform float u_game_time;

const uint TypeNone            = 0;
const uint TypeRocketTrail     = 1;
const uint TypeExplosionCore   = 2;
const uint TypeExplosionDebris = 3;
const uint TypeMuzzleFlash     = 4;
const uint TypeFire            = 5;
const uint TypeControlFlash    = 6;
const uint TypeSpark           = 7;
const uint TypeBlueSpark       = 8;
const uint TypeBrass           = 9;
const uint TypeMissileTrail    = 10;
const uint TypeMissileFire     = 11;
const uint TypeDarwinianFire   = 12;
const uint TypeLeaf            = 13;

float getLifeTime(uint typeID)
{
	switch(typeID)
	{
		case TypeRocketTrail:     return  2.0f;
		case TypeExplosionCore:   return  2.0f;
		case TypeExplosionDebris: return  6.0f;
		case TypeMuzzleFlash:     return  1.0f;
		case TypeFire:            return  4.0f;
		case TypeControlFlash:    return  1.0f;
		case TypeSpark:           return  4.0f;
		case TypeBlueSpark:       return  4.0f;
		case TypeBrass:           return  2.0f;
		case TypeMissileTrail:    return  5.0f;
		case TypeMissileFire:     return  0.5f;
		case TypeDarwinianFire:   return  1.0f;
		case TypeLeaf:            return  60.0f;
	}
	return 0.0;
}

void main()
{
	if(in_type == TypeNone)
	{
		gl_Position = vec4(0);
		out_size = 0.0;
		return;
	}

	float life_time  = getLifeTime(in_type);
	float death_time = in_birth_time + life_time;
	float start_fade = in_birth_time + life_time * 0.75f;
	int alpha;
	if( u_game_time < start_fade )
	{
		alpha = 90;
	}
	else
	{
		float fraction_fade = (u_game_time - start_fade) / (death_time - start_fade);
		alpha = 90 - int(90 * fraction_fade);
		if( alpha < 0 ) alpha = 0;
	}

	gl_Position    = vec4(in_position + u_prediction_time * in_velocity, 1.0);
	out_size       = in_size/16.0f;
	
	float red   = float(bitfieldExtract(in_colour, 0,  8)) / 255.0f;
	float green = float(bitfieldExtract(in_colour, 8,  8)) / 255.0f;
	float blue  = float(bitfieldExtract(in_colour, 16, 8)) / 255.0f;

	if(in_type == TypeMissileTrail)
	{
		float fraction = alpha / 100.0f;
		red   *= fraction;
		green *= fraction;
		blue  *= fraction;
		alpha = 0;
	}

	out_colour = vec4(red, green, blue, alpha / 255.0f);
}
)";

constexpr const char* geometry_shader_source =
R"(
#version 410

layout (points) in;
layout (triangle_strip, max_vertices = 4) out;

layout (location = 0) in float in_size[];
layout (location = 1) in vec4  in_colour[];

layout (location = 0) out vec4 out_colour;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec2 out_uv0_texcoord;
layout (location = 3) out vec2 out_uv1_texcoord;
layout (location = 4) out vec4 out_vertex_position;

uniform mat4 u_projection;
uniform mat4 u_model_view;

void main()
{
	out_colour = in_colour[0];

	out_uv1_texcoord = vec2(0, 0);

	out_normal = vec3(0, 0, 1);

	float size = in_size[0];

	vec4 up    = inverse(u_model_view) * vec4(0, size, 0, 0);
	vec4 right = inverse(u_model_view) * vec4(size, 0, 0, 0);

	out_vertex_position = gl_in[0].gl_Position - up;
	gl_Position = u_projection * u_model_view * out_vertex_position;
	out_uv0_texcoord = vec2(0, 0);
	EmitVertex();

	out_vertex_position = gl_in[0].gl_Position + right;
	gl_Position = u_projection * u_model_view * out_vertex_position;
	out_uv0_texcoord = vec2(0, 1);
	EmitVertex();

	out_vertex_position = gl_in[0].gl_Position - right;
	gl_Position = u_projection * u_model_view * out_vertex_position;
	out_uv0_texcoord = vec2(1, 0);
	EmitVertex();
	
	out_vertex_position = gl_in[0].gl_Position + up;
	gl_Position = u_projection * u_model_view * out_vertex_position;
	out_uv0_texcoord = vec2(1, 1);
	EmitVertex();

	EndPrimitive();
}
)";

GLuint make_particle_program()
{
	int success;
	char infoLog[512];

	GLuint vertex_shader, geometry_shader;

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
		geometry_shader = glCreateShader(GL_GEOMETRY_SHADER);

		glShaderSource(geometry_shader, 1, &geometry_shader_source, NULL);
		
		glCompileShader(geometry_shader);

		// print compile errors if any
		glGetShaderiv(geometry_shader, GL_COMPILE_STATUS, &success);
		if(!success)
		{
			glGetShaderInfoLog(geometry_shader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::GEOMETRY::COMPILATION_FAILED\n" << infoLog << std::endl;
		};
	}

	{
		auto program = glCreateProgram();
		glAttachShader(program, vertex_shader);
		glAttachShader(program, geometry_shader);
		glAttachShader(program, ffp_emulation::get_fragment_shader());
		glLinkProgram(program);
		// print linking errors if any
		glGetProgramiv(program, GL_LINK_STATUS, &success);
		if(!success)
		{
			glGetProgramInfoLog(program, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		}

		glDeleteShader(vertex_shader);
		glDeleteShader(geometry_shader);

		return program;
	}
}
}

// ****************************************************************************
// ParticleSystem
// ****************************************************************************

// *** Constructor
ParticleSystem::ParticleSystem()
	: m_uniforms(make_particle_program())
{
	Particle::SetupParticles();

	m_game_time_location       = glGetUniformLocation(m_uniforms.program, "u_game_time");
	m_prediction_time_location = glGetUniformLocation(m_uniforms.program, "u_prediction_time");

	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);

	
	glBindVertexArray(m_vao);

		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

		glVertexAttribPointer(0, 3, GL_FLOAT,        GL_FALSE, sizeof(Particle), ((std::uint8_t*)nullptr) + offsetof(Particle, m_pos));
		glVertexAttribPointer(1, 3, GL_FLOAT,        GL_FALSE, sizeof(Particle), ((std::uint8_t*)nullptr) + offsetof(Particle, m_vel));
		glVertexAttribPointer(2, 1, GL_FLOAT,        GL_FALSE, sizeof(Particle), ((std::uint8_t*)nullptr) + offsetof(Particle, m_birthTime));
		glVertexAttribPointer(3, 1, GL_FLOAT,        GL_FALSE, sizeof(Particle), ((std::uint8_t*)nullptr) + offsetof(Particle, m_size));
		glVertexAttribIPointer(4, 1, GL_UNSIGNED_INT,  sizeof(Particle), ((std::uint8_t*)nullptr) + offsetof(Particle, m_colour));
		glVertexAttribIPointer(5, 1, GL_UNSIGNED_INT,  sizeof(Particle), ((std::uint8_t*)nullptr) + offsetof(Particle, m_typeId));

		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);
		glEnableVertexAttribArray(3);
		glEnableVertexAttribArray(4);
		glEnableVertexAttribArray(5);

	glBindVertexArray(0);

	m_vertex_buffer.emplace(m_vao, m_vbo);
}

// *** CreateParticle
void ParticleSystem::CreateParticle(Vector3 const &_pos, Vector3 const &_vel,
									Particle::Type _typeId, float _size, RGBAColour col)
{
	auto [_, aParticle] = m_particles.MakeOrReplaceFirstNull(_pos, _vel, _typeId, _size);

	if( col != 0)
	{
		aParticle.m_colour = col;
	}
}


// *** Advance
void ParticleSystem::Advance()
{
	START_PROFILE(g_app->m_profiler, "Advance Particles");

	m_draw_batches.clear();

	enum BatchType
	{
		Unknown,
		MissileTrail,
		Other,
	};

	BatchType current_batch_type = Unknown;
	
	std::size_t current_batch_size = -1;

	for( auto& particle : m_particles.Optionals() )
	{
		current_batch_size++;
		
		if(!bool(particle))
		{
			continue;
		}

		BatchType batch_type =
		particle.m_typeId == Particle::Type::TypeMissileTrail
		? BatchType::MissileTrail
		: BatchType::Other;

		if(batch_type != current_batch_type)
		{
			if(current_batch_type != Unknown)
			{
				m_draw_batches.emplace_back(current_batch_size, current_batch_type == MissileTrail);
				current_batch_size = 0;
			}

			current_batch_type = batch_type;
		}
		
		if (particle.Advance())
		{
			particle.reset();
		}
	}

	END_PROFILE(g_app->m_profiler, "Advance Particles");
}


// *** Render
void ParticleSystem::Render()
{
	START_PROFILE(g_app->m_profiler, "Render Particles");

	glDisable   ( GL_CULL_FACE );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE );

	glEnable    ( GL_BLEND );
	glEnable	( GL_TEXTURE_2D );
	glBindTexture(GL_TEXTURE_2D, g_app->m_resource->GetTexture("textures/particle.bmp"));
	glTexParameteri	(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glDepthMask ( false );

	// Update the vertex buffer
	m_vertex_buffer->update(m_particles.data(), m_particles.size() * sizeof(Particle), m_particles.size());

	glUseProgram(m_uniforms.program);

	glUniform1f(m_game_time_location, g_gameTime);
	glUniform1f(m_prediction_time_location, g_predictionTime);

	{
		std::size_t offset = 0;
		for(const auto& [batch_size, is_missile_trail] : m_draw_batches)
		{
			if(is_missile_trail)
			{
				glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_COLOR );
			}
			else
			{
				glBlendFunc ( GL_SRC_ALPHA, GL_ONE );
			}

			m_vertex_buffer->draw(GL_POINTS, offset, batch_size, m_uniforms);

			offset += batch_size;
		}
	}

	glDepthMask ( true );
	glDisable	( GL_TEXTURE_2D );
	glTexParameteri	( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glDisable   ( GL_BLEND );
	glBlendFunc ( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glEnable    ( GL_CULL_FACE );

	END_PROFILE(g_app->m_profiler, "Render Particles");
}


// *** Empty
void ParticleSystem::Empty()
{
	m_particles.clear();
}
