#ifndef MODEL_H
#define MODEL_H

#include <Glad/glad.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <stb_image/stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <src/Mesh.h>
#include <src/shader.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>

unsigned int TextureFromFile(const char* path, const std::string &m_Directory, bool gamma);

class Model
{
public:
	//Model Data
	std::vector<glm::mat4> m_ModelMatrices;
	std::vector<Texture> textures_loaded;
	std::vector<Mesh> m_Meshes;
	std::string m_Directory;
	bool m_GammaCorrection;

	//constructor that takes in a filepath to a 3D model
	Model(std::string const &path, unsigned int instances, bool gamma = false)
		:m_GammaCorrection(gamma), m_Instances(instances)
	{
		loadModel(path);
	}
	//~Model();//TO DO: add a destructor to destroy all unneeded objects and variables or just to prevent memory leak

	void Draw(Shader &shader)
	{
		if(m_Instances == 0)
		{
			for(unsigned int i = 0; i < m_Meshes.size(); i++)
				m_Meshes[i].Draw(shader);
		}
		else
		{
			for(unsigned int i = 0; i < m_Meshes.size(); i++)
				m_Meshes[i].Draw(m_ModelMatrices, m_Instances);
		}
	}

private:
	unsigned int m_Instances;
	//loads model using file path
	void loadModel(std::string const& path)
	{
		//read file via ASSIMP
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		//checks for errors
		if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)//if not zero
		{
			std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return;
		}
		//retrieve the m_Directory path of the fileppath
		m_Directory = path.substr(0, path.find_last_of('\\') == std::string::npos ? path.find_last_of('/') : path.find_last_of('\\'));

		//process ASSIMP's root node recursively
		processNode(scene->mRootNode, scene);
	}

	//processes a node recursively. Processes each mesh located at the node and repeats this for any child nodes
	void processNode(aiNode* node, const aiScene* scene)
	{
		//process each mesh
		for(unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			// the node object only contains indices to index the actual objects in the scene. 
			// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			//m_Meshes.push_back(processMesh(mesh, scene));
		}

		for(unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}
	}

	/*Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		//data to fill
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<Texture> textures;

		//go through each of the mesh's vertices
		for(unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			glm::vec3 vector;// we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this one first.
			//positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.m_Position = vector;
			//normals
			if(mesh->HasNormals())
			{
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.m_Normal = vector;
			}
			//texture
			if(mesh->mTextureCoords[0])//if the mesh has texture coordinates
			{
				glm::vec2 vec;
				//texture coordinates
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.m_TexCoords = vec;

			}
			else
				vertex.m_TexCoords = glm::vec2(0.0f, 0.0f);

			vertices.push_back(vertex);
		}

		//go through each of the m_Meshes faces and get the corresponding indices
		for(unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			//retrieve all indices of the face
			for(unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		//process materials
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
		// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
		// Same applies to other texture as the following list summarizes:
		// diffuse: texture_diffuseN
		// specular: texture_specularN

		//diffuse maps
		std::vector<Texture> diffuseMaps = loadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
		textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());
		//specular maps
		std::vector<Texture> specularMaps = loadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		//normal maps
		std::vector<Texture> normalMaps = loadMaterialTextures(material, aiTextureType_HEIGHT, "texture_normal");
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		//height maps
		std::vector<Texture> heightMaps = loadMaterialTextures(material, aiTextureType_AMBIENT, "texture_height");
		textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

		return Mesh(vertices, indices, textures, Blinn_Phong_Mesh);
	}*/

	// checks all material textures of a given type and loads the textures if they're not loaded yet.
	// the required info is returned as a Texture struct.
	std::vector<Texture> loadMaterialTextures(aiMaterial* mat, aiTextureType type, std::string typeName)
	{
		std::vector<Texture> textures;
		for(unsigned int i = 0; i < mat->GetTextureCount(type); i++)
		{
			aiString str;
			mat->GetTexture(type, i, &str);
			//check if texture was loaded before and if so, continue to next interation: skip loading texture
			bool skip = false;
			for(unsigned int j = 0; j < textures_loaded.size(); j++)
			{
				if(std::strcmp(textures_loaded[j].m_Path.data(), str.C_Str()) == 0)
				{
					textures.push_back(textures_loaded[j]);
					skip = true;// a texture with the same filepath has already been loaded, continue to next one. (optimization)
					break;
				}
			}
			if(!skip)
			{	// if texture hasn't been loaded already, load it
				Texture texture;
				texture.m_ID = TextureFromFile(str.C_Str(), this->m_Directory, m_GammaCorrection);
				texture.m_Type = typeName;
				texture.m_Path = str.C_Str();
				textures.push_back(texture);
				textures_loaded.push_back(texture); // store it as texture loaded for entire model, to ensure we won't unnecesery load duplicate textures.
			}
		}
		return textures;
	}
};

class PBRModel
{
public:
	//Model Data
	std::vector<Texture> textures_loaded;
	std::vector<Mesh> m_Meshes;
	std::string m_Directory;
	std::string m_FileName;
	bool m_GammaCorrection;

	//constructor that takes in a filepath to a 3D model
	PBRModel(std::string const& path, bool gamma = false)
		:m_GammaCorrection(gamma)
	{
		loadModel(path);
	}
	//~PBRModel();//TO DO: add a destructor to destroy all unneeded objects and variables or just to prevent memory leak

	void Draw(Shader& shader)
	{
		for(unsigned int i = 0; i < m_Meshes.size(); i++)
			m_Meshes[i].Draw(shader);
	}

private:
	//loads model using file path
	void loadModel(std::string const& path)
	{
		if(path.find(".obj") == std::string::npos && path.find(".OBJ") == std::string::npos)
		{
			std::cout << "ERROR::MODEL:: This file format is supported with PBR" << std::endl;
			return;
		}

		//read file via ASSIMP
		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
		//checks for errors
		if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)//if not zero
		{
			std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
			return;
		}
		//retrieve the m_Directory path of the fileppath
		m_Directory = path.substr(0, path.find_last_of('/\\'));
		m_FileName = path;

		//process ASSIMP's root node recursively
		processNode(scene->mRootNode, scene);
	}

	//processes a node recursively. Processes each mesh located at the node and repeats this for any child nodes
	void processNode(aiNode* node, const aiScene* scene)
	{
		//process each mesh
		for(unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			// the node object only contains indices to index the actual objects in the scene. 
			// the scene contains all the data, node is just to keep stuff organized (like relations between nodes).
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			//m_Meshes.push_back(processMesh(mesh, scene));
		}

		for(unsigned int i = 0; i < node->mNumChildren; i++)
		{
			processNode(node->mChildren[i], scene);
		}
	}

	/*Mesh processMesh(aiMesh* mesh, const aiScene* scene)
	{
		//data to fill
		std::vector<Vertex> vertices;
		std::vector<unsigned int> indices;
		std::vector<Texture> textures;

		//go through each of the mesh's vertices
		for(unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex;
			glm::vec3 vector;// we declare a placeholder vector since assimp uses its own vector class that doesn't directly convert to glm's vec3 class so we transfer the data to this one first.
			//positions
			vector.x = mesh->mVertices[i].x;
			vector.y = mesh->mVertices[i].y;
			vector.z = mesh->mVertices[i].z;
			vertex.m_Position = vector;
			//normals
			if(mesh->HasNormals())
			{
				vector.x = mesh->mNormals[i].x;
				vector.y = mesh->mNormals[i].y;
				vector.z = mesh->mNormals[i].z;
				vertex.m_Normal = vector;
			}
			//texture
			if(mesh->mTextureCoords[0])//if the mesh has texture coordinates
			{
				glm::vec2 vec;
				//texture coordinates
				vec.x = mesh->mTextureCoords[0][i].x;
				vec.y = mesh->mTextureCoords[0][i].y;
				vertex.m_TexCoords = vec;
			}
			else
				vertex.m_TexCoords = glm::vec2(0.0f, 0.0f);

			vertices.push_back(vertex);
		}

		//go through each of the m_Meshes faces and get the corresponding indices
		for(unsigned int i = 0; i < mesh->mNumFaces; i++)
		{
			aiFace face = mesh->mFaces[i];
			//retrieve all indices of the face
			for(unsigned int j = 0; j < face.mNumIndices; j++)
				indices.push_back(face.mIndices[j]);
		}
		//process materials
		aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
		// we assume a convention for sampler names in the shaders. Each diffuse texture should be named
		// as 'texture_diffuseN' where N is a sequential number ranging from 1 to MAX_SAMPLER_NUMBER. 
		// Same applies to other texture as the following list summarizes:
		// diffuse: texture_diffuseN
		// specular: texture_specularN

		//diffuse maps
		std::vector<Texture> albedoMaps = loadMaterialTextures("albedoMap");
		textures.insert(textures.end(), albedoMaps.begin(), albedoMaps.end());
		//specular maps
		std::vector<Texture> specularMaps = loadMaterialTextures("metallicMap");
		textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
		//normal maps
		std::vector<Texture> normalMaps = loadMaterialTextures("normalMap");
		textures.insert(textures.end(), normalMaps.begin(), normalMaps.end());
		//height maps
		std::vector<Texture> heightMaps = loadMaterialTextures("roughnessMap");
		textures.insert(textures.end(), heightMaps.begin(), heightMaps.end());

		return Mesh(vertices, indices, textures, PBR_Mesh);
	}*/

	// checks all material textures of a given type and loads the textures if they're not loaded yet.
	// the required info is returned as a Texture struct.
	std::vector<Texture> loadMaterialTextures(std::string typeName)
	{
		std::vector<Texture> textures;
		std::string materialFileName;
		if(m_FileName.find("obj") != std::string::npos)
			materialFileName = m_FileName.substr(0, m_FileName.find("obj"));
		else if(m_FileName.find("OBJ") != std::string::npos)
			materialFileName = m_FileName.substr(0, m_FileName.find("OBJ"));
		materialFileName.append("mtl");

		std::fstream materialFile;
		materialFile.open(materialFileName, std::ios::in);

		std::string materialFileString;
		std::string temp;
		std::string textureFilename;
		while(std::getline(materialFile, temp))
		{
			materialFileString.append(temp);
			bool hasFound = 0;
			if(typeName == "albedoMap" && temp.find("map_Kd") != std::string::npos)
			{
				textureFilename = temp.substr(temp.find_last_of(" ") + 1);
				hasFound = 1;
			}
			else if(typeName == "emissiveMap" && temp.find("map_Ke") != std::string::npos)
			{
				textureFilename = temp.substr(temp.find_last_of(" ") + 1);
				hasFound = 1;
			}
			else if(typeName == "normalMap" && temp.find("norm") != std::string::npos)
			{
				textureFilename = temp.substr(temp.find_last_of(" ") + 1);
				hasFound = 1;
			}
			else if(typeName == "normalMap" && temp.find("map_Bump") != std::string::npos)
			{
				textureFilename = temp.substr(temp.find_last_of(" ") + 1);
				hasFound = 1;
			}
			else if(typeName == "roughnessMap" && temp.find("map_Pr") != std::string::npos)
			{
				textureFilename = temp.substr(temp.find_last_of(" ") + 1);
				hasFound = 1;
			}
			else if(typeName == "roughnessMap" && temp.find("map_Ns") != std::string::npos)
			{
				textureFilename = temp.substr(temp.find_last_of(" ") + 1);
				hasFound = 1;
			}
			else if(typeName == "metallicMap" && temp.find("map_Pm") != std::string::npos)
			{
				textureFilename = temp.substr(temp.find_last_of(" ") + 1);
				hasFound = 1;
			}
			else if(typeName == "metallicMap" && temp.find("refl") != std::string::npos)
			{
				textureFilename = temp.substr(temp.find_last_of(" ") + 1);
				hasFound = 1;
			}

			if(hasFound)
			{
				Texture t;
				t.m_ID = TextureFromFile(textureFilename.c_str(), m_Directory, m_GammaCorrection);
				t.m_Type = typeName;
				t.m_Path = textureFilename;
				textures.push_back(t);
			}
		}
		materialFile.close();
			
		return textures;
	}
};


unsigned int TextureFromFile(const char* path, const std::string& m_Directory, bool gamma = false)
{
	std::string m_Filename = std::string(path);
	m_Filename = m_Directory + '\\' + m_Filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char* data = stbi_load(m_Filename.c_str(), &width, &height, &nrComponents, 0);
	if(data)
	{
		GLenum internalFormat = gamma ? GL_SRGB_ALPHA : GL_RGBA;
		GLenum dataFormat = GL_RGBA;
		if(nrComponents == 1)
			internalFormat = dataFormat = GL_RED;
		else if(nrComponents == 3)
		{
			internalFormat = gamma ? GL_SRGB : GL_RGB;
			dataFormat = GL_RGB;
		}

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, dataFormat, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << m_Filename << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

#endif