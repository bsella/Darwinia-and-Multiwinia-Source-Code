#pragma once

#include "lib/llist.h"
#include "lib/vector3.h"

#include "FFP_VertexData.h"

class DeformEffect
{
public:
	static DeformEffect* Create();
	void AddPunch(const Vector3& pos, float range); ///< Creates punch effect, adds it to internal list of active deformations.
	void Start(); ///< Step 1: call before rendering scene with deform effects.
	void Stop(); ///< Step 2: call after rendering scene with deform effects.
	~DeformEffect();
private:
	DeformEffect();
	void RenderSpritesBegin(unsigned int program, int deform_texture);
	void RenderSprite(unsigned int program, const Vector3& posWorld, float width, float depth, float redux);
	void RenderSpritesEnd(unsigned int program);

	unsigned int m_screen_framebuffer;
	unsigned int m_screen_texture;
	unsigned int m_screen_depth_buffer;
	unsigned int m_deform_framebuffer;
	unsigned int m_deform_texture;

	unsigned int m_deform_diamond_program;
	unsigned int m_deform_sphere_program;
	unsigned int m_deform_fullscreen_program;

	ffp_emulation::DynamicVertexBuffer m_sprite_vertex_buffer;

	bool m_old_is_texture0_enabled;
	bool m_old_is_texture1_enabled;
	int m_old_texture0_binding;
	int m_old_texture1_binding;

	int m_old_render_target;
	struct Punch
	{
		Vector3 m_pos;
		float m_range;
		unsigned long long m_birth;
	};
	LList<Punch> m_punchList;

	const unsigned int m_deform_texture_width;
	const unsigned int m_deform_texture_height;
};

extern DeformEffect* g_deformEffect;

