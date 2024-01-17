#ifndef SHADOWS_H
#define SHADOWS_H

#define SHADOW_SIZE 10.0f
#define DEFAULT_SHADOW_PERSPECTIVE_FOV 60.0f //FOV angle in degrees for perspective projection matrix
#define SHADOW_NEAR_PLANE 0.1f
#define SHADOW_FAR_PLANE 100.0f
#define MAX_SHADOWMAPS 8
#define MAX_SHADOW_MAP_RESOLUTION (4096 * 4096)

#include <src/light.h>

extern const float Pi;
extern void renderQuad();

class ShadowMap
{
public:
	unsigned int m_ID;
	float m_Width, m_Height;
	Shader* m_ShadowShader;
	Light* m_Light;
	glm::mat4* m_TransformMatrix;

	ShadowMap()
		:m_Init(0), m_Width(0), m_Height(0)
	{

	}
	~ShadowMap()
	{

	}

	bool Init(unsigned int shadowMapWidth, unsigned int shadowMapHeight, glm::vec3 position, glm::vec3 color, LightType type = POINT_LIGHT, glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f))
	{
		if(m_Init == 1)
			return 1;

		m_Init = 1;

		if(shadowMapWidth * shadowMapHeight > MAX_SHADOW_MAP_RESOLUTION)
		{
			shadowMapWidth = 4096;
			shadowMapHeight = 4096;
		}

		m_Width = shadowMapWidth;
		m_Height = shadowMapHeight;
		
		m_Light = new Light();

		m_Light->m_Pos = position;
		m_Light->m_Color = color;
		m_Light->m_Dir = direction;

		m_Light->m_Type = type;

		if(type == DIRECTIONAL_LIGHT)
		{
			m_TransformMatrix = (glm::mat4*) malloc(sizeof(glm::mat4));
			createDirectionalShadowMap();
		}
		else if(type == PERSPECTIVE_LIGHT)
		{
			m_TransformMatrix = (glm::mat4*) malloc(sizeof(glm::mat4));
			createPerspectiveShadowMap();
		}
		else if(type == POINT_LIGHT)
		{
			m_TransformMatrix = (glm::mat4*) malloc(6 * sizeof(glm::mat4));
			createPointShadowMap();
		}
	}
	bool Init(unsigned int shadowMapWidth, unsigned int shadowMapHeight, Light light)
	{
		if(m_Init == 1)
			return 1;

		m_Init = 1;

		if(shadowMapWidth * shadowMapHeight > MAX_SHADOW_MAP_RESOLUTION)
		{
			shadowMapWidth = 4096;
			shadowMapHeight = 4096;
		}

		m_Width = shadowMapWidth;
		m_Height = shadowMapHeight;

		m_Light = &light;

		if(m_Light->m_Type == DIRECTIONAL_LIGHT)
		{
			m_TransformMatrix = (glm::mat4*)malloc(sizeof(glm::mat4));
			createDirectionalShadowMap();
		}
		else if(m_Light->m_Type == PERSPECTIVE_LIGHT)
		{
			m_TransformMatrix = (glm::mat4*)malloc(sizeof(glm::mat4));
			createPerspectiveShadowMap();
		}
		else if(m_Light->m_Type == POINT_LIGHT)
		{
			m_TransformMatrix = (glm::mat4*)malloc(6 * sizeof(glm::mat4));
			createPointShadowMap();
		}
	}
	void destroy()
	{
		glDeleteTextures(1, &m_ID);
		free(m_TransformMatrix);
		m_Init = 0;
		delete m_Light;
	}

	void use()
	{
		if(m_Light->m_Type == PERSPECTIVE_LIGHT)
		{
			glBindTexture(GL_TEXTURE_2D, m_ID);
		}
		else if(m_Light->m_Type == DIRECTIONAL_LIGHT)
		{
			glBindTexture(GL_TEXTURE_2D, m_ID);
		}
		else if(m_Light->m_Type == POINT_LIGHT)
		{
			glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);
		}
	}
	void unbind()
	{
		if(m_Light->m_Type == PERSPECTIVE_LIGHT)
			glBindTexture(GL_TEXTURE_2D, 0);
		else if(m_Light->m_Type == DIRECTIONAL_LIGHT)
			glBindTexture(GL_TEXTURE_2D, 0);
		else if(m_Light->m_Type == POINT_LIGHT)
			glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	}

	void updateShadowMap(glm::vec3 position, glm::vec3 color, glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f))
	{
		m_Light->m_Pos = position;
		m_Light->m_Color = color;
		m_Light->m_Dir = direction;

		if(m_Light->m_Type == DIRECTIONAL_LIGHT)
		{
			glm::mat4 proj = glm::ortho(-SHADOW_SIZE, SHADOW_SIZE, -SHADOW_SIZE, SHADOW_SIZE, SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);
			glm::mat4 view = glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + m_Light->m_Dir, glm::vec3(0.0f, 0.0f, 1.0f));

			*m_TransformMatrix = proj * view;
		}
		else if(m_Light->m_Type == PERSPECTIVE_LIGHT)
		{
			glm::mat4 proj = glm::perspective(glm::radians(m_Light->m_FOV == 0 ? DEFAULT_SHADOW_PERSPECTIVE_FOV : m_Light->m_FOV), float(m_Width) / float(m_Height), SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);
			glm::mat4 view = glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + m_Light->m_Dir, glm::vec3(0.0f, 0.0f, 1.0f));

			*m_TransformMatrix = proj * view;
		}
		else if(m_Light->m_Type == POINT_LIGHT)
		{
			glm::mat4 proj = glm::perspective(glm::radians(90.0f), (float)m_Width / (float)m_Height, SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);

			m_TransformMatrix[0] = proj * glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
			m_TransformMatrix[1] = proj * glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
			m_TransformMatrix[2] = proj * glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f));
			m_TransformMatrix[3] = proj * glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f));
			m_TransformMatrix[4] = proj * glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
			m_TransformMatrix[5] = proj * glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
		}
	}

private:
	bool m_Init;
	void createDirectionalShadowMap()
	{
		glGenTextures(1, &m_ID);
		glBindTexture(GL_TEXTURE_2D, m_ID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, m_Width, m_Height, 0, GL_RED, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glm::mat4 proj = glm::ortho(-SHADOW_SIZE, SHADOW_SIZE, -SHADOW_SIZE, SHADOW_SIZE, SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);
		glm::mat4 view = glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + m_Light->m_Dir, glm::vec3(0.0f, 0.0f, 1.0f));

		*m_TransformMatrix = proj * view;
	}
	void createPerspectiveShadowMap()
	{
		glGenTextures(1, &m_ID);
		glBindTexture(GL_TEXTURE_2D, m_ID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, m_Width, m_Height, 0, GL_RED, GL_FLOAT, NULL);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
		glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

		glm::mat4 proj = glm::perspective(glm::radians(m_Light->m_FOV == 0 ? DEFAULT_SHADOW_PERSPECTIVE_FOV : m_Light->m_FOV), float(m_Width) / float(m_Height), SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);
		glm::mat4 view = glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + m_Light->m_Dir, glm::vec3(0.0f, 0.0f, 1.0f));

		*m_TransformMatrix = proj * view;
	}
	void createPointShadowMap()
	{
		std::cout << glGetError() << std::endl;
		glGenTextures(1, &m_ID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, m_ID);
		for(unsigned int i = 0; i < 6; i++)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_R16F, m_Width, m_Height, 0, GL_RED, GL_FLOAT, NULL);
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);


		glm::mat4 proj = glm::perspective(glm::radians(90.0f), (float)m_Width / (float)m_Height, SHADOW_NEAR_PLANE, SHADOW_FAR_PLANE);

		m_TransformMatrix[0] = proj * glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
		m_TransformMatrix[1] = proj * glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f));
		m_TransformMatrix[2] = proj * glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f));
		m_TransformMatrix[3] = proj * glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f));
		m_TransformMatrix[4] = proj * glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
		m_TransformMatrix[5] = proj * glm::lookAt(m_Light->m_Pos, m_Light->m_Pos + glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f));
	}
};

class ShadowRenderer
{
public:
	unsigned int m_ID;
	Shader* m_CurrentShader;
	ShadowMap** shadowMaps;
	ShadowRenderer()
		:m_Init(0), m_ID(0), m_NrOfShadowMaps(0)
	{
		shadowMaps = (ShadowMap**) malloc(sizeof(ShadowMap**));
		for(unsigned int i = 0; i < MAX_SHADOWMAPS; i++)
		{
			m_ShadowMapsCreated[i] = 0;
		}
	}
	~ShadowRenderer()
	{
		
	}

	//initializes the shadow renderer object
	bool Init()
	{
		if(m_Init == 1)
			return 1;
		m_Init = 1;

		shadowMaps = (ShadowMap**) malloc(MAX_SHADOWMAPS * sizeof(ShadowMap*));

		glGenFramebuffers(1, &m_ID);
		if(m_SimpleDepthShader == nullptr || m_PointDepthShader == nullptr || m_DebugShader == nullptr)
		{
			m_SimpleDepthShader = new Shader("ProgramFiles\\Resources\\Shaders\\Shadows\\simpleDepth.V.shader", "ProgramFiles\\Resources\\Shaders\\Shadows\\simpleDepth.F.shader");
			m_PointDepthShader = new Shader("ProgramFiles\\Resources\\Shaders\\Shadows\\pointDepth.V.shader", "ProgramFiles\\Resources\\Shaders\\Shadows\\pointDepth.G.shader", "ProgramFiles\\Resources\\Shaders\\Shadows\\pointDepth.F.shader");
			m_DebugShader = new Shader("ProgramFiles\\Resources\\Shaders\\Shadows\\debug.V.shader", "ProgramFiles\\Resources\\Shaders\\Shadows\\debug.F.shader");
			m_DebugShader->use();
			m_DebugShader->setInt("depthMap", 0);
		}
	}
	void Destroy()
	{
		if(m_Init == 0)
			return;
		for(int i = 0; i < m_NrOfShadowMaps; i++)
		{
			if(m_ShadowMapsCreated[i] == 1)
			{
				shadowMaps[i]->m_ShadowShader = nullptr;
				shadowMaps[i]->destroy();
			}
			free(shadowMaps[i]);
		}
		m_SimpleDepthShader->destroy();
		m_PointDepthShader->destroy();
		m_DebugShader->destroy();
		delete m_SimpleDepthShader;
		delete m_PointDepthShader;
		delete m_DebugShader;
		free(shadowMaps);
		glDeleteFramebuffers(1, &m_ID);
		m_Init = 0;
	}

	void use(int index = -1)
	{
		if(index > -1)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
			glViewport(0, 0, shadowMaps[index]->m_Width, shadowMaps[index]->m_Height);
			useShadowMap(index);
			m_CurrentShader = shadowMaps[index]->m_ShadowShader;
			if(shadowMaps[index]->m_Light->m_Type == DIRECTIONAL_LIGHT)
			{
				m_CurrentShader->setInt("doLinerizeDepth", 1);
				m_CurrentShader->setMat4("lightSpaceMatrix", *shadowMaps[index]->m_TransformMatrix);
				m_CurrentShader->setVec3("lightPos", shadowMaps[index]->m_Light->m_Pos);
				m_CurrentShader->setFloat("farPlane", SHADOW_FAR_PLANE);
			}
			else  if(shadowMaps[index]->m_Light->m_Type == PERSPECTIVE_LIGHT)
			{
				m_CurrentShader->setInt("doLinerizeDepth", 0);
				m_CurrentShader->setMat4("lightSpaceMatrix", *shadowMaps[index]->m_TransformMatrix);
				m_CurrentShader->setVec3("lightPos", shadowMaps[index]->m_Light->m_Pos);
				m_CurrentShader->setFloat("farPlane", SHADOW_FAR_PLANE);
			}
			else if(shadowMaps[index]->m_Light->m_Type == POINT_LIGHT)
			{
				for(unsigned int i = 0; i < 6; i++)
				{
					m_CurrentShader->setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowMaps[index]->m_TransformMatrix[i]);
				}
				m_CurrentShader->setVec3("lightPos", shadowMaps[index]->m_Light->m_Pos);
				m_CurrentShader->setFloat("farPlane", SHADOW_FAR_PLANE);
			}
		}
		else
			glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
	}
	void unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glUseProgram(0);
	}


	//creates and initializes a new shadow map (*the direction parameter should only be set if the shadow map type is not point)
	void createShadowMap(unsigned int index, unsigned int width, unsigned int height, glm::vec3 position, glm::vec3 color, LightType type, glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f))
	{
		if(index < MAX_SHADOWMAPS && m_ShadowMapsCreated[index] != 1)
		{
			shadowMaps[index] = (ShadowMap*) malloc(sizeof(ShadowMap));
			if(shadowMaps[index] == NULL)
				std::cerr << "ERROR::CREATE_SHADOWMAP:: Error allocating memory for shadow map, malloc() failed" << std::endl;
			shadowMaps[index]->Init(width, height, position, color, type, direction);
			if(type == DIRECTIONAL_LIGHT)
			{
				shadowMaps[index]->m_ShadowShader = m_SimpleDepthShader;
			}
			else if(type == PERSPECTIVE_LIGHT)
			{
				shadowMaps[index]->m_ShadowShader = m_SimpleDepthShader;
			}
			else if(type == POINT_LIGHT)
			{
				shadowMaps[index]->m_ShadowShader = m_PointDepthShader;
			}
			m_CurrentShader = shadowMaps[index]->m_ShadowShader;
			m_ShadowMapsCreated[index] = 1;
			m_NrOfShadowMaps++;
		}
		else
		{
			std::cerr << "ERROR::CREATE_SHADOWMAP:: Error trying to create shadow map, Shadow map overload" << std::endl;
		}
	}
	//deletes a shadow map
	void deleteShadowMap(unsigned int index)
	{
		if(index > m_NrOfShadowMaps || m_ShadowMapsCreated[index] == 0)
		{
			std::cerr << "ERROR::DELETE_SHADOWMAP:: Error deleting shadow map, index out of range/this shadow map is already deleted" << std::endl;
			return;
		}
		shadowMaps[index]->destroy();
		free(shadowMaps[index]);
		m_ShadowMapsCreated[index] = 0;
		m_CurrentShader = nullptr;
		m_NrOfShadowMaps--;
	}
	//updates a shadow map's properties
	void updateShadowMap(unsigned int index, glm::vec3 position, glm::vec3 color, glm::vec3 direction = glm::vec3(0.0f, 0.0f, 0.0f))
	{
		shadowMaps[index]->updateShadowMap(position, color, direction);
	}


	//returns a shadow map's texture value
	unsigned int shadowMapTexture(unsigned int index)
	{
		if(m_ShadowMapsCreated[index] == 1)
			return shadowMaps[index]->m_ID;
		std::cerr << "ERROR::SHADOWMAP_TEXTURE:: Tried accessing shadow map texture that hasn't been initialized" << std::endl;
	}
	glm::mat4* lightSpaceMatrix(unsigned int index)
	{
		if(m_ShadowMapsCreated[index] == 0)
		{
			std::cerr << "ERROR::LIGHT_SPACE_MATRIX:: Tried accessing a shadow map that hasn't been initialized" << std::endl;
			return nullptr;
		}
		return shadowMaps[index]->m_TransformMatrix;
	}
	unsigned int inline getNrOfShadowMaps() const {	return m_NrOfShadowMaps; }

	void debugShadowMap(unsigned int index)
	{
		m_DebugShader->use();
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, shadowMapTexture(index));
		renderQuad();
		m_DebugShader->unbind();
	}

private:
	bool m_Init;
	bool m_ShadowMapsCreated[MAX_SHADOWMAPS];
	unsigned int m_NrOfShadowMaps;
	static Shader* m_SimpleDepthShader;
	static Shader* m_PointDepthShader;
	static Shader* m_DebugShader;
	void useShadowMap(unsigned int index)
	{
		if(m_ShadowMapsCreated[index] == 1)
		{
			shadowMaps[index]->use();
			if(shadowMaps[index]->m_Light->m_Type == DIRECTIONAL_LIGHT)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMaps[index]->m_ID, 0);
			}
			else if(shadowMaps[index]->m_Light->m_Type == PERSPECTIVE_LIGHT)
			{
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, shadowMaps[index]->m_ID, 0);
			}
			else if(shadowMaps[index]->m_Light->m_Type == POINT_LIGHT)
			{
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, shadowMaps[index]->m_ID, 0);
			}
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glReadBuffer(GL_NONE);
			
			int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if(status != GL_FRAMEBUFFER_COMPLETE)
			{
				std::cerr << "ERROR::USE_SHADOWMAP:: Framebuffer object is not complete,\n" << status << std::endl;
			}
			shadowMaps[index]->m_ShadowShader->use();
		}
		else
			std::cerr << "ERROR::USE_SHADOWMAP:: tried using shadow map that hasn't been initialized" << std::endl;
	}
	void unbindShadowMap(unsigned int index)
	{
		if(m_ShadowMapsCreated[index] == 1)
		{
			shadowMaps[index]->unbind();
			if(shadowMaps[index]->m_Light->m_Type == DIRECTIONAL_LIGHT)
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
			else if(shadowMaps[index]->m_Light->m_Type == PERSPECTIVE_LIGHT)
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
			else if(shadowMaps[index]->m_Light->m_Type == POINT_LIGHT)
				glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 0, 0);
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glReadBuffer(GL_NONE);
			shadowMaps[index]->m_ShadowShader->unbind();
		}
		else
			std::cerr << "ERROR::UNBIND_SHADOWMAP:: tried unbinding shadow map that hasn't been initialized" << std::endl;
	}
};

Shader* ShadowRenderer::m_SimpleDepthShader = nullptr;
Shader* ShadowRenderer::m_PointDepthShader  = nullptr;
Shader* ShadowRenderer::m_DebugShader		= nullptr;

#endif 