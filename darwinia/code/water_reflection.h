#pragma once

class WaterReflectionEffect
{
public:
	static WaterReflectionEffect* Create();
	void PreRenderWaterReflection();
	bool IsPrerendering() {return m_prerendering;}
	void Start();
	void Stop();

	unsigned int GetProgram() const;

	~WaterReflectionEffect();
private:
	WaterReflectionEffect();

	unsigned int m_program;
	unsigned int m_framebuffer;
	unsigned int m_texture;
	unsigned int m_depth_buffer;

	int m_time_location;
	int m_world_view_projection_loc;
	int m_water_texture_location;

	const unsigned int m_width;
	const unsigned int m_height;

	bool m_prerendering;
};

extern WaterReflectionEffect* g_waterReflectionEffect;
