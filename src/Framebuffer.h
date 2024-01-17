#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <Glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image/stb_image.h>

#include <iostream>
#include <vector>
#include <tuple>

extern unsigned int wWidth, wHeight;

struct FramebufferAttachment
{
	FramebufferAttachment* m_NextAttachment;
	unsigned int m_Type;

	FramebufferAttachment()
	{
		m_NextAttachment = nullptr;
		m_Type = GL_COLOR_ATTACHMENT0;
	}
};

class Framebuffer
{
public:
	unsigned int m_Id;
	unsigned int m_QuadVAO;
	unsigned int m_Renderbuffer;//stores the depth-stencil renderbuffer attachment for the framebuffer
	std::vector<unsigned int> m_Textures;//stores the color texture attachments for the framebuffer


	Framebuffer(unsigned int width, unsigned int height, unsigned char nrSamples = 1, bool isPostProcessing = NULL)
		:m_Width(width), m_Height(height)
	{
		if(nrSamples > 0 && nrSamples < 9)
			this->m_NrOfSamples = nrSamples;
		else
			this->m_NrOfSamples = 1;
		
		this->m_IsPostProcessing = isPostProcessing;
		if(m_IsPostProcessing)//if this framebuffer will be used for post-processing, then set up the screen quad to render the framebuffer onto
			setUpQuad();

		glGenFramebuffers(1, &m_Id);

		m_Attachments = new FramebufferAttachment();
	}
	//~Framebuffer();

	//draws the framebuffer to the default framebuffer (the window) using a shader and one of the texture attachments of the framebuffer
	void Draw(Shader shader, unsigned int textureIndex)
	{
		if(m_IsPostProcessing)
		{
			//initializes everything to get ready for rendering
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT);
			glDisable(GL_DEPTH_TEST);

			//draws screen quad with texture
			shader.use();
			glBindVertexArray(m_QuadVAO);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, m_Textures[textureIndex]);
			glDrawArrays(GL_TRIANGLES, 0, 6);

		}
		else
			std::cout << "This framebuffer was not set up for post-processing!\n";
	}

	//attaches a texture object to the framebuffer as a color buffer
	void addTextureAttachment(unsigned int internalFormat = GL_RGB, unsigned int format = GL_RGB, unsigned int minFilter = GL_LINEAR, unsigned int magFilter = GL_LINEAR, unsigned int typeOfAttachment = GL_COLOR_ATTACHMENT0)
	{
		if(typeOfAttachment != GL_COLOR_ATTACHMENT0 && typeOfAttachment != GL_DEPTH_ATTACHMENT && typeOfAttachment != GL_STENCIL_ATTACHMENT && typeOfAttachment != GL_DEPTH_STENCIL_ATTACHMENT)
		{
			std::cout << "ERROR::FRAMEBUFFER:: tried to create and attach a texture attachment with invalid attachment type" << std::endl;
			return;
		}

		unsigned int attachmentType = typeOfAttachment;

		unsigned int dataType = 0;
		if(internalFormat == GL_R16F || internalFormat == GL_RG16F || internalFormat == GL_RGB16F || internalFormat == GL_RGBA16F || internalFormat == GL_DEPTH_COMPONENT16)
		{
			dataType = GL_FLOAT;
		}
		else
		{
			dataType = GL_UNSIGNED_BYTE;
		}
		if(m_NrOfSamples == 1)//default: if multisampling is off
		{
			unsigned int texId;
			use();
			glGenTextures(1, &texId);//creates a texture
			glBindTexture(GL_TEXTURE_2D, texId);
			if(attachmentType == GL_COLOR_ATTACHMENT0)
			{
				attachmentType = GL_COLOR_ATTACHMENT0 + m_NrOfAttachments;
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, format, dataType, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, texId, 0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			else if(attachmentType == GL_DEPTH_ATTACHMENT)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, format, dataType, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, texId, 0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			else if(attachmentType == GL_STENCIL_ATTACHMENT)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, format, dataType, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, texId, 0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			else if(attachmentType == GL_DEPTH_STENCIL_ATTACHMENT)
			{
				glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, m_Width, m_Height, 0, format, dataType, NULL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
				glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D, texId, 0);
				glBindTexture(GL_TEXTURE_2D, 0);
			}
			unbind();
			m_Textures.push_back(texId);

		}
		else//if multisampling is on
		{
				unsigned int size = m_Textures.size();//gets the index of the vector array
				m_Textures.push_back(0);//initializes the memory that openGL can store the texture id in
				unsigned int* idptr = &m_Textures[size];//creates the pointer to the id value of the texture in the vector array
				use();
				glGenTextures(1, idptr);//creates a texture
				glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, *idptr);
				if(attachmentType == GL_COLOR_ATTACHMENT0)
				{
					attachmentType = GL_COLOR_ATTACHMENT0 + m_NrOfAttachments;
					glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_NrOfSamples, internalFormat, m_Width, m_Height, GL_TRUE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
					glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D_MULTISAMPLE, *idptr, 0);
				}
				else if(attachmentType == GL_DEPTH_ATTACHMENT)
				{
					glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_NrOfSamples, internalFormat, m_Width, m_Height, GL_TRUE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
					glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D_MULTISAMPLE, *idptr, 0);
				}
				else if(attachmentType == GL_STENCIL_ATTACHMENT)
				{
					glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D_MULTISAMPLE, *idptr, 0);
					glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_NrOfSamples, internalFormat, m_Width, m_Height, GL_TRUE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
					glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D_MULTISAMPLE, *idptr, 0);
				}
				else if(attachmentType == GL_DEPTH_STENCIL_ATTACHMENT)
				{
					glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D_MULTISAMPLE, *idptr, 0);
					glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_NrOfSamples, internalFormat, m_Width, m_Height, GL_TRUE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
					glFramebufferTexture2D(GL_FRAMEBUFFER, attachmentType, GL_TEXTURE_2D_MULTISAMPLE, *idptr, 0);
				}

				glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
				unbind();
		}

		if(m_NrOfAttachments == 0)
		{
			m_Attachments->m_Type = attachmentType;
		}
		else //iteratively goes through the singly linked list to add new framebuffer attachment at the end
		{
			FramebufferAttachment* newAttachment = new FramebufferAttachment();
			FramebufferAttachment* temp = m_Attachments;
			for(; temp->m_NextAttachment != nullptr;)
			{
				temp = temp->m_NextAttachment;
			}
			temp->m_NextAttachment = newAttachment;
			newAttachment->m_Type = attachmentType;
		}
		m_NrOfAttachments++;
	}
	//attaches a renderbuffer object to the framebuffer as a depth and stencil buffer
	void addRenderbufferAttachment(unsigned int internalFormat = GL_DEPTH24_STENCIL8, unsigned int typeOfAttachment = GL_DEPTH_STENCIL_ATTACHMENT)
	{
		if(typeOfAttachment != GL_DEPTH_ATTACHMENT && typeOfAttachment != GL_STENCIL_ATTACHMENT && typeOfAttachment != GL_DEPTH_STENCIL_ATTACHMENT)
		{
			std::cout << "ERROR::FRAMEBUFFER:: tried to create and attach a renderbuffer attachment with invalid attachment type" << std::endl;
			return;
		}

		if(m_NrOfSamples == 1)//default: if multisampling is off
		{
			unsigned int id;//creates the id value of the renderbuffer
			use();
			glGenRenderbuffers(1, &id);//creates a renderbuffer
			glBindRenderbuffer(GL_RENDERBUFFER, id);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, wWidth, wHeight);
			glBindTexture(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, typeOfAttachment, GL_RENDERBUFFER, m_RenderBufferID);//attaches the renderbuffer to the framebuffer
			unbind();
		}
		else//if multisampling is on
		{
			unsigned int id;//creates the id value of the renderbuffer
			use();
			glGenRenderbuffers(1, &id);//creates a renderbuffer
			glBindRenderbuffer(GL_RENDERBUFFER, id);
			glRenderbufferStorageMultisample(GL_RENDERBUFFER, m_NrOfSamples, GL_DEPTH24_STENCIL8, wWidth, wHeight);
			glBindRenderbuffer(GL_RENDERBUFFER, 0);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, typeOfAttachment, GL_RENDERBUFFER, m_RenderBufferID);
			unbind();
		}

		if(m_NrOfAttachments == 0)
		{
			m_Attachments->m_Type = typeOfAttachment;
		}
		else //iteratively goes through the singly linked list to add new framebuffer attachment at the end
		{
			FramebufferAttachment* newAttachment = new FramebufferAttachment();
			FramebufferAttachment* temp = m_Attachments;
			for(; temp->m_NextAttachment != nullptr;)
			{
				temp = temp->m_NextAttachment;
			}
			temp->m_NextAttachment = newAttachment;
			newAttachment->m_Type = typeOfAttachment;
		}
		m_NrOfAttachments++;
	}

	//binds the framebuffer to type
	void use()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_Id);
	}
	//unbinds the framebuffer to type
	void unbind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	inline unsigned int getWidth() const { return m_Width; }
	inline unsigned int getHeight() const { return m_Height; }

	//copies or blits the colorbuffer of the current framebuffer to the argument framebuffer
	const void copyTo(Framebuffer other) const
	{
		//blits the multisampled buffers to the normal color buffer of the intermediate fbo
		glBindFramebuffer(GL_READ_FRAMEBUFFER, this->m_Id);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, other.m_Id);
		glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, other.getWidth(), other.getHeight(), GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}
	//copies or blits the colorbuffer of the current framebuffer to the argument framebuffer
	const void copyTo(unsigned int otherFramebuffer, unsigned int width, unsigned int height) const
	{
		//blits the multisampled buffers to the normal color buffer of the intermediate fbo
		glBindFramebuffer(GL_READ_FRAMEBUFFER, this->m_Id);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, otherFramebuffer);
		glBlitFramebuffer(0, 0, m_Width, m_Height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_NEAREST);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	}
	//checks if the framebuffer is complete. Returns 1 if it's complete, returns 0 otherwise
	bool checkStatus()
	{
		use();
		unsigned int* atts = new unsigned int[m_NrOfAttachments];
		FramebufferAttachment* temp = m_Attachments;
		unsigned int NrOfAtts = 0;
		for(NrOfAtts = 0; NrOfAtts < m_NrOfAttachments && temp != nullptr; NrOfAtts++)
		{
			if(temp->m_Type == GL_DEPTH_ATTACHMENT || temp->m_Type == GL_STENCIL_ATTACHMENT || temp->m_Type == GL_DEPTH_STENCIL_ATTACHMENT)
			{
				temp = temp->m_NextAttachment;
				NrOfAtts--;
				continue;
			}
			atts[NrOfAtts] = temp->m_Type;
			temp = temp->m_NextAttachment;
		}
		glDrawBuffers(NrOfAtts, atts);

		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE)
		{
			unbind();
			return 1;
		}
		else
		{
			std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!\n";
			unbind();
			return 0;
		}
		delete[] atts;
	}

private:
	unsigned int m_Width, m_Height;
	FramebufferAttachment* m_Attachments;
	unsigned int m_NrOfAttachments;
	unsigned int m_RenderBufferID;
	unsigned char m_NrOfSamples;//number of samples for multisampling, default is 1 meaning multisampling is off
	bool m_IsPostProcessing;//is true if framebuffer will be used for post processing
	void setUpQuad()
	{
		float quadVertices[] = {
		-1.0f,  1.0f,	//top left
		-1.0f, -1.0f,	//bottom left
		 1.0f, -1.0f,	//bottom right

		-1.0f,  1.0f,	//top left
		 1.0f, -1.0f,	//bottom right
		 1.0f,  1.0f	//top right
		};

		unsigned int quadVBO;
		glGenVertexArrays(1, &m_QuadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(m_QuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
};


#endif