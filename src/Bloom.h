#ifndef BLOOM_H
#define BLOOM_H

#define NR_BLOOM_MIPS 5
void renderQuad();

struct BloomMip
{
	glm::vec2 size;
	glm::ivec2 intSize;
	unsigned int texture;
};

class BloomFBO
{
public:
	unsigned int m_ID;
	BloomFBO()
		:m_Init(0), m_ID(0)
	{

	}
	~BloomFBO()
	{

	}

	//Used to initialize the Bloom Framebuffer object
	bool Init(unsigned int windowWidth, unsigned int windowHeight, unsigned int mipChainLength = NR_BLOOM_MIPS)
	{
		if(m_Init) return 1;

		glGenFramebuffers(1, &m_ID);
		glBindFramebuffer(GL_FRAMEBUFFER, m_ID);

		glm::vec2 mipSize((float)windowWidth, (float)windowHeight);
		glm::ivec2 mipIntSize((int)windowWidth, (int)windowHeight);

		if(windowWidth > (unsigned int)INT_MAX || windowHeight > (unsigned int)INT_MAX) //SAFETY CHECK
		{
			std::cerr << "WINDOW SIZE TOO LARGE; CANNOT BUILD FRAMEBUFFER OBJECT!" << std::endl;
			return 0;
		}

		for(unsigned int i = 0; i < mipChainLength; i++)
		{
			BloomMip mip;

			mipSize *= 0.5f;
			mipIntSize /= 2;
			mip.size = mipSize;
			mip.intSize = mipIntSize;

			glGenTextures(1, &mip.texture);
			glBindTexture(GL_TEXTURE_2D, mip.texture);

			//we are downscaling an HDR color buffer so we need to use floating point buffers and we set the internal format to GL_R11F_G11F_B10F because we do not need the extra alpha component (more color precision)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, (int)mipSize.x, (int)mipSize.y, 0, GL_RGB, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			m_MipChain.emplace_back(mip);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_MipChain[0].texture, 0);

		unsigned int attachments[1] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, attachments);

		int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if(status != GL_FRAMEBUFFER_COMPLETE)
		{
			printf("BLOOM FRAMEBUFFER ERROR! \nStatus: 0x%x\n", status);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return 0;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		m_Init = 1;
		return 1;
	}
	void Destroy()
	{
		for(unsigned int i = 0; i < (int)m_MipChain.size(); i++)
		{
			glDeleteTextures(1, &m_MipChain[i].texture);
			m_MipChain[i].texture = 0;
		}
		glDeleteFramebuffers(1, &m_ID);
		m_ID= 0;
		m_Init = 0;
	}
	void use()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_ID);
	}
	void unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	const std::vector<BloomMip>& MipChain() const
	{
		return m_MipChain;
	}

private:
	bool m_Init;
	std::vector<BloomMip> m_MipChain;

};

class BloomRenderer
{
public:
	BloomFBO m_FBO;
	unsigned int m_NrMips;
	BloomRenderer(bool KarisAverage = true)
		:m_Init(0), m_KarisAverage(KarisAverage)
	{

	}
	~BloomRenderer()
	{

	}

	bool Init(unsigned int windowWidth, unsigned int windowheight, const char* lensDirtDirectory = "\0", unsigned int nrMips = NR_BLOOM_MIPS)
	{
		if(m_Init) return 1;

		m_NrMips = nrMips;

		if(lensDirtDirectory == "\0")
			m_LensDirt = 0;
		else
		{
			m_LensDirt = 1;
			loadLensDirtImage(lensDirtDirectory);
		}


		m_IntSrcViewPortSize = glm::ivec2(windowWidth, windowheight);
		m_SrcViewPortSize = glm::vec2((float)windowWidth, (float)windowheight);

		bool status = m_FBO.Init(windowWidth, windowheight, nrMips);
		if(!status)
		{
			std::cerr << "ERROR CREATING BLOOM FRAMEBUFFER OBJECT!\nBLOOM RENDERER CANNOT BE CREATED!" << std::endl;
			return 0;
		}

		m_DownSampleShader = new Shader("ProgramFiles\\Resources\\Shaders\\Bloom\\DownSample.V.shader", "ProgramFiles\\Resources\\Shaders\\Bloom\\DownSample.F.shader");
		m_UpSampleShader = new Shader("ProgramFiles\\Resources\\Shaders\\Bloom\\UpSample.V.shader", "ProgramFiles\\Resources\\Shaders\\Bloom\\UpSample.F.shader");

		m_DownSampleShader->use();
		m_DownSampleShader->setInt("srcTexture", 0);
		m_UpSampleShader->use();
		m_UpSampleShader->setInt("srcTexture", 0);

		glUseProgram(0);

		return 1;
	}
	void Destroy()
	{
		m_FBO.Destroy();
		delete m_DownSampleShader;
		delete m_UpSampleShader;
	}
	void RenderBloomTexture(unsigned int srcTexture, float filterRadius)
	{
		m_FBO.use();

		this->RenderDownSamples(srcTexture);
		this->RenderUpSamples(filterRadius);

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		//restore viewport to default
		glViewport(0, 0, m_SrcViewPortSize.x, m_SrcViewPortSize.y);
	}
	unsigned int BloomTexture(unsigned int index = 0)
	{
		const std::vector<BloomMip>& mipChain = m_FBO.MipChain();
		int size = mipChain.size();
		return mipChain[(index > size - 1) ? size - 1 : (index < 0) ? 0 : index].texture;
	}
	unsigned int LensDirtTexture()
	{
		if(m_LensDirt)
			return m_LensDirtTexture;
		else
			return -1;
	}
private:
	void RenderDownSamples(unsigned int srcTexture)
	{
		const std::vector<BloomMip>& mipChain = m_FBO.MipChain();

		m_DownSampleShader->use();
		m_DownSampleShader->setVec2("srcResolution", m_SrcViewPortSize);
		if(m_KarisAverage)
			m_DownSampleShader->setInt("mipLevel", 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, srcTexture);

		for(unsigned int i = 0; i < mipChain.size() - 1; i++)
		{
			const BloomMip& mip = mipChain[i];
			glViewport(0, 0, mip.size.x, mip.size.y);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mip.texture, 0);

			//renders current mip onto screen quad
			renderQuad();
			
			m_DownSampleShader->setVec2("srcResolution", mip.size);

			glBindTexture(GL_TEXTURE_2D, mip.texture);

			//disable karis average in shader for all downsamples except initial downsample
			if(i == 0) m_DownSampleShader->setInt("mipLevel", 1);
		}

		glUseProgram(0);
	}
	void RenderUpSamples(float filterRadius)
	{
		const std::vector<BloomMip>& mipChain = m_FBO.MipChain();

		m_UpSampleShader->use();

		//additive blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		glBlendEquation(GL_FUNC_ADD);

		for(int i = (int)mipChain.size() - 1; i > 0; i--)
		{
			m_UpSampleShader->setFloat("filterRadius", pow(2, mipChain.size() - i) * filterRadius);
			const BloomMip& mip = mipChain[i];
			const BloomMip& nextMip = mipChain[i - 1];

			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, mip.texture);

			//set size of the viewport to nextMip because we are rendering to this resolution (we are upscaling)
			glViewport(0, 0, nextMip.size.x, nextMip.size.y);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, nextMip.texture, 0);

			//renders current mip onto screen quad
			renderQuad();
		}

		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

		glUseProgram(0);
	}
	unsigned int loadLensDirtImage(const char* path, bool gammaCorrection = false)
	{
		glGenTextures(1, &m_LensDirtTexture);

		int width, height, nrComponents;
		unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
		if(data)
		{
			GLenum internalFormat = gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA;
			GLenum dataFormat = GL_RGBA;
			if(nrComponents == 1)
				internalFormat = dataFormat = GL_RED;
			else if(nrComponents == 3)
			{
				internalFormat = gammaCorrection ? GL_SRGB : GL_RGB;
				dataFormat = GL_RGB;
			}

			glBindTexture(GL_TEXTURE_2D, m_LensDirtTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			//this makes the textures repeat if texCoords are larger than 1 or less than 0 and tells openGL to use linear upscaling
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);//this frees the image from system memory since it should be in VRAM by now
		}
		else
		{   //this is if the image wasn't read for any reason
			std::cout << "Texture failed to load at path: " << path << std::endl;
			stbi_image_free(data);
		}

		return m_LensDirtTexture;
	}

	bool m_Init;
	bool m_LensDirt;
	unsigned int m_LensDirtTexture;
	glm::ivec2 m_IntSrcViewPortSize;
	glm::vec2 m_SrcViewPortSize;
	Shader* m_DownSampleShader;
	Shader* m_UpSampleShader;

	bool m_KarisAverage;
};

#endif 