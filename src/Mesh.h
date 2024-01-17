#ifndef MESH_H
#define MESH_H

#include <Glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <src/shader.h>
#include <src/Material.h>

#include <iostream>
#include <string>
#include <vector>

struct Vertex
{
	glm::vec3 m_Position;
	glm::vec3 m_Normal;
	glm::vec2 m_TexCoords;
	glm::vec3 m_MaterialMask;
	//TO DO: add vertex attributes for skeletal animation
};

//Used to define whether the mesh is supposed to be drawn using a Blinn-Phong shader or a PBR shader
enum MeshType
{
	Blinn_Phong_Mesh = 0,
	PBR_Mesh = 1
};

class Mesh
{
public:
	//mesh data
	std::vector<Vertex> m_Vertices;
	std::vector<unsigned int> m_Indices;
	std::string m_MaterialID;
	unsigned int m_VAO;
	MeshType m_Type;
	Shader* m_Shader;

	//constructor that takes in vector array of vertices, indices and textures of the mesh
	Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::string materialID, MeshType type, bool instanced = 0)
		: m_Type(type), isInstanced(instanced)
	{
		this->m_Vertices = vertices;
		this->m_Indices = indices;
		this->m_MaterialID = materialID;
		isSetup = 0;
	}

	~Mesh()//TO DO: add a destructor to destroy all unneeded objects and variables or just to prevent memory leaks
	{
		//glDeleteBuffers(1, &this->m_VBO);
		//glDeleteBuffers(1, &this->m_EBO);
		//glDeleteVertexArrays(1, &this->VAO);
	}

	//Renders the mesh
	void Draw(Shader &shader)
	{
		if(m_Type == Blinn_Phong_Mesh)
		{
			// bind appropriate textures
			unsigned int diffuseNr = 1;
			unsigned int specularNr = 1;
			unsigned int normalNr = 1;
			unsigned int heightNr = 1;
			for(unsigned int i = 0; i < m_Material->m_Textures.size(); i++)
			{
				glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
				// retrieve texture number (the N in diffuse_textureN)
				std::string number;
				std::string name = m_Material->m_Textures[i].m_Type;
				if(name == "texture_diffuse")
					number = std::to_string(diffuseNr++);
				else if(name == "texture_specular")
					number = std::to_string(specularNr++); // transfer unsigned int to string
				else if(name == "texture_normal")
					number = std::to_string(normalNr++); // transfer unsigned int to string
				else if(name == "texture_height")
					number = std::to_string(heightNr++); // transfer unsigned int to string

				// now set the sampler to the correct texture unit
				glUniform1i(glGetUniformLocation(shader.m_ID, (name + number).c_str()), i);
				// and finally bind the texture
				glBindTexture(GL_TEXTURE_2D, m_Material->m_Textures[i].m_ID);
			}

			// draw mesh
			glBindVertexArray(m_VAO);
			glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, 0);

			// always good practice to set everything back to defaults once configured.
			glBindVertexArray(0);
			glActiveTexture(GL_TEXTURE0);
		}
		else if(m_Type == PBR_Mesh)
		{
						//bind appropriate textures
			unsigned int albedoNr = 1;
			unsigned int metallicNr = 1;
			unsigned int normalNr = 1;
			unsigned int roughnessNr = 1;
			unsigned int emissiveNr = 1;
			for(unsigned int i = 0; i < m_Material->m_Textures.size(); i++)
			{
				glActiveTexture(GL_TEXTURE0 + i);

				std::string number;
				std::string name = m_Material->m_Textures[i].m_Type;
				if(name == "albedoMap")
					number = std::to_string(albedoNr++);
				else if(name == "metallicMap")
					number = std::to_string(metallicNr++);
				else if(name == "normalMap")
					number = std::to_string(normalNr++);
				else if(name == "roughnessMap")
					number = std::to_string(roughnessNr++);
				else if(name == "emissiveMap")
					number = std::to_string(emissiveNr++);


				glUniform1i(glGetUniformLocation(shader.m_ID, (name + number).c_str()), i);//sends the texture slot to the correct sampler2D
				glBindTexture(GL_TEXTURE_2D, m_Material->m_Textures[i].m_ID);
			}

			//this renders the mesh
			glBindVertexArray(m_VAO);
			glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, 0);

			//It's always good to put things back after you're done with them
			glBindVertexArray(0);
			glActiveTexture(GL_TEXTURE0);
		}
	}
	void Draw(std::vector<glm::mat4>& modelMatrices, unsigned int instances)
	{
		setupInstancedMesh(modelMatrices);
		if(m_Type == Blinn_Phong_Mesh)
		{
			// bind appropriate textures
			unsigned int diffuseNr = 1;
			unsigned int specularNr = 1;
			unsigned int normalNr = 1;
			unsigned int heightNr = 1;
			for(unsigned int i = 0; i < m_Material->m_Textures.size(); i++)
			{
				glActiveTexture(GL_TEXTURE0 + i); // active proper texture unit before binding
				// retrieve texture number (the N in diffuse_textureN)
				std::string number;
				std::string name = m_Material->m_Textures[i].m_Type;
				if(name == "texture_diffuse")
					number = std::to_string(diffuseNr++);
				else if(name == "texture_specular")
					number = std::to_string(specularNr++); // transfer unsigned int to string
				else if(name == "texture_normal")
					number = std::to_string(normalNr++); // transfer unsigned int to string
				else if(name == "texture_height")
					number = std::to_string(heightNr++); // transfer unsigned int to string

				// now set the sampler to the correct texture unit
				glUniform1i(glGetUniformLocation(m_Shader->m_ID, (name + number).c_str()), i);
				// and finally bind the texture
				glBindTexture(GL_TEXTURE_2D, m_Material->m_Textures[i].m_ID);
			}

			// draw mesh
			glBindVertexArray(m_VAO);
			glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, 0);

			// always good practice to set everything back to defaults once configured.
			glBindVertexArray(0);
			glActiveTexture(GL_TEXTURE0);
		}
		else if(m_Type == PBR_Mesh)
		{
						//bind appropriate textures
			unsigned int albedoNr = 1;
			unsigned int metallicNr = 1;
			unsigned int normalNr = 1;
			unsigned int roughnessNr = 1;
			unsigned int emissiveNr = 1;
			for(unsigned int i = 0; i < m_Material->m_Textures.size(); i++)
			{
				glActiveTexture(GL_TEXTURE0 + i);

				std::string number;
				std::string name = m_Material->m_Textures[i].m_Type;
				if(name == "albedoMap")
					number = std::to_string(albedoNr++);
				else if(name == "metallicMap")
					number = std::to_string(metallicNr++);
				else if(name == "normalMap")
					number = std::to_string(normalNr++);
				else if(name == "roughnessMap")
					number = std::to_string(roughnessNr++);
				else if(name == "emissiveMap")
					number = std::to_string(emissiveNr++);


				glUniform1i(glGetUniformLocation(m_Shader->m_ID, (name + number).c_str()), i);//sends the texture slot to the correct sampler2D
				glBindTexture(GL_TEXTURE_2D, m_Material->m_Textures[i].m_ID);
			}

			//this renders the mesh
			glBindVertexArray(m_VAO);
			glDrawElements(GL_TRIANGLES, m_Indices.size(), GL_UNSIGNED_INT, 0);

			//It's always good to put things back after you're done with them
			glBindVertexArray(0);
			glActiveTexture(GL_TEXTURE0);
		}
	}


private:
	Material* m_Material;
	//data for rendering
	unsigned int m_VBO, m_EBO;
	bool isInstanced, isSetup;

	//initializes all the mesh data to be ready for rendering
	void setupMesh()
	{
		if(isSetup)
			return;
		isSetup = 1;
		glGenVertexArrays(1, &m_VAO);
		glGenBuffers(1, &m_VBO);
		glGenBuffers(1, &m_EBO);

		glBindVertexArray(m_VAO);

		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
		glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(Vertex), &m_Vertices[0], GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int), &m_Indices[0], GL_DYNAMIC_DRAW);

		//Vertex position attributes
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		//Vertex normal attributes
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Normal));
		//Vertex texture coordinates attributes
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_TexCoords));
		//Mesh material mask attribute (for material masking)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_MaterialMask));

		//TO DO: vertex attributes for skeletal animations

		glBindVertexArray(0);
	}
	//initializes all the mesh data to be ready for instanced rendering
	void setupInstancedMesh(std::vector<glm::mat4>& modelMatrices)
	{
		if(isSetup)
			return;
		isSetup = 1;

		glGenVertexArrays(1, &m_VAO);
		glGenBuffers(1, &m_VBO);
		glGenBuffers(1, &m_EBO);

		glBindVertexArray(m_VAO);

		glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
		glBufferData(GL_ARRAY_BUFFER, m_Vertices.size() * sizeof(Vertex), &m_Vertices[0], GL_DYNAMIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_Indices.size() * sizeof(unsigned int), &m_Indices[0], GL_DYNAMIC_DRAW);

		glm::mat4* matrices = modelMatrices.data();
		float* tempPtr;
		float* data = (float*)malloc(sizeof(glm::mat4) * modelMatrices.size());
		for(unsigned int i = 0; i < modelMatrices.size(); i++)
		{
			tempPtr = glm::value_ptr(matrices[i]);
			for(unsigned int j = 0; j < sizeof(glm::mat4); j++)
				data[i * sizeof(glm::mat4) + j] = tempPtr[j];
		}

		unsigned int instanceVBO;
		glGenBuffers(1, &instanceVBO);
		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		glBufferData(GL_ARRAY_BUFFER, modelMatrices.size() * sizeof(glm::mat4), data, GL_DYNAMIC_DRAW);

		//Vertex position attributes
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		//Vertex normal attributes
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_Normal));
		//Vertex texture coordinates attributes
		glEnableVertexAttribArray(2);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_TexCoords));
		//Mesh material mask attribute (for material masking)
		glEnableVertexAttribArray(3);
		glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, m_MaterialMask));
		//Instanced array attributes
		glEnableVertexAttribArray(4);
		glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glVertexAttribDivisor(2, 1);
		//TO DO: vertex attributes for skeletal animations

		glBindVertexArray(0);
	}
};

#endif