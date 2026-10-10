#ifndef _included_particle_system_h
#define _included_particle_system_h

#include "lib/rgb_colour.h"
#include "lib/vector3.h"
#include "lib/vector_with_options.hpp"
#include <cstdint>

#include "FFP_VertexData.h"

// ****************************************************************************
// ParticleType
// ****************************************************************************

class ParticleType
{
public:
	float m_spin;			// Rate of spin in radians per second (positive is clockwise)
	float m_fadeInTime;
	float m_fadeOutTime;
	float m_life;
	float m_size;
	float m_friction;		// Amount of friction to apply (0=none 1=a huge amount)
	float m_gravity;		// Amount of gravity to apply (0=none 1=quite a lot)
	RGBAColour m_colour1;		// Start of colour range
	RGBAColour m_colour2;		// End of colour range

	ParticleType();
};


// ****************************************************************************
// Particle
// ****************************************************************************

class Particle
{
public:
	enum Type : std::uint32_t
	{
		TypeNone = 0, // Marks the particle as disabled/deleted
		TypeRocketTrail,
		TypeExplosionCore,
		TypeExplosionDebris,
		TypeMuzzleFlash,
        TypeFire,
        TypeControlFlash,
        TypeSpark,
        TypeBlueSpark,
        TypeBrass,
        TypeMissileTrail,
        TypeMissileFire,
        TypeDarwinianFire,
		TypeLeaf,
		TypeNumTypes
	};

private:
	static ParticleType m_types[TypeNumTypes];

public:
	Vector3         m_pos;
	Vector3         m_vel;
	float           m_birthTime;
	float           m_size;
	RGBAColour      m_colour;
	Type            m_typeId;

	Particle(const Vector3& pos, const Vector3& vel, Type, float _size=-1.0f);

	operator bool() const;

	Particle&       operator*();
	const Particle& operator*() const;
	
	void reset();

	bool Advance();
	void Render(float _predictionTime);

    static void SetupParticles();
};


template<>
struct OptionalTraits<Particle>
{
	using Type = Particle;

	static Particle make(auto&& ... args)
	{
		return Particle(std::forward<decltype(args)>(args)...);
	}
};

// ****************************************************************************
// ParticleSystem
// ****************************************************************************

class ParticleSystem
{
private:
	VectorWithOptionals<Particle> m_particles;
	
	ffp_emulation::Uniforms m_uniforms;
	
	int m_game_time_location;
	int m_prediction_time_location;
	
	unsigned int m_vao;
	unsigned int m_vbo;
	std::optional<ffp_emulation::DynamicVertexBuffer> m_vertex_buffer;

	// The boolean tells if the particle batch is missile trails or not
	std::vector<std::tuple<std::size_t, bool>> m_draw_batches;

public:
	ParticleSystem();

	void CreateParticle(Vector3 const &_pos, Vector3 const &_vel, Particle::Type, float _size=-1.0f, RGBAColour col = 0);

	void Advance();
	void Render();
	void Empty();
};

#endif
