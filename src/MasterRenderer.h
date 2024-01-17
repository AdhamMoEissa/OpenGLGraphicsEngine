#ifndef MASTER_RENDERER_H
#define MASTER_RENDERER_H

#include <src/Camera.h>
#include <src/Scene.h>
#include <src/Shadows.h>
#include <src/Bloom.h>

/*
enum RenderMode
{
	NO_RENDERING = 0,
	BLINN_PHONG_RENDERING = 1,
	PHYSICALLY_BASED_RENDERING = 2
};
*/

struct RenderingFlags
{
	void* windowPtr; //the pointer of the window object used to render to
	unsigned int x, y, width, height; //the area to be rendered to on the window
	
};

class MasterRenderer
{
public:
	MasterRenderer();
	MasterRenderer(RenderingFlags flags)
	{

	}

	void render(Scene scene, Camera camera)
	{

	}
private:
	RenderingFlags m_RenderingFlags;
};

#endif