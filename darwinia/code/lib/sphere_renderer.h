#ifndef SPHERE_RENDERER_H
#define SPHERE_RENDERER_H

#include "FFP_VertexData.h"
#include "lib/vector3.h"
#include <optional>


class Triangle
{
public:
	Vector3 m_corner[3];

	Triangle() {}
	Triangle(Vector3 const &c1, Vector3 const &c2, Vector3 const &c3 );
};


class Sphere
{
public:
	Sphere();
	void Render(Vector3 const &pos, float radius);
	void RenderLong();

private:
	Triangle	m_topLevelTriangle[20];
	std::optional<ffp_emulation::StaticVertexBuffer> m_vertex_buffer;

	void ConsiderTriangle(int level, Vector3 const &a, Vector3 const &b, Vector3 const &c);
};


#endif
