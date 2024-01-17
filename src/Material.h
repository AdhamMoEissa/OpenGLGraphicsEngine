#ifndef MATERIAL_H
#define MATERIAL_H

#include <unordered_map>
#include <vector>
#include <src/Mesh.h>

struct Texture
{
	unsigned int m_ID;
	std::string m_Type;
	std::string m_Path;
	bool m_GammaCorrection;
	Texture() {}
	Texture(unsigned int id, std::string type, std::string path, bool gammaCorrection = false)
		:m_ID(id), m_Type(type), m_Path(path), m_GammaCorrection(gammaCorrection)
	{

	}
};

class MaterialMask 
{
	enum class RedValue {
		MATERIAL_LAND = 0x30, //#30a040
		MATERIAL_FOLIAGE = 0x60, //#60e060
		MATERIAL_WATER = 0x30, //#30b0e0
		MATERIAL_SKIN = 0xf0, //#f0c080
		MATERIAL_SKY = 0xa0, //#a0e0f0
	};
	enum class GreenValue {
		MATERIAL_LAND = 0xa0, //#30a040
		MATERIAL_FOLIAGE = 0xe0, //#60e060
		MATERIAL_WATER = 0xb0, //#30b0e0
		MATERIAL_SKIN = 0xc0, //#f0c080
		MATERIAL_SKY = 0xe0, //#a0e0f0
	};
	enum class BlueValue {
		MATERIAL_LAND = 0x40, //#30a040
		MATERIAL_FOLIAGE = 0x60, //#60e060
		MATERIAL_WATER = 0xe0, //#30b0e0
		MATERIAL_SKIN = 0x80, //#f0c080
		MATERIAL_SKY = 0xf0, //#a0e0f0
	};

public:
	glm::vec3 m_MaskColor;
	MaterialMask(RedValue r, GreenValue g, BlueValue b)
	{
		float red, green, blue;
		red = (float)r / 255.0f;
		green = (float)g / 255.0f;
		blue = (float)b / 255.0f;
		m_MaskColor = { red, green, blue };
	}
private:

};

class Material
{
public:
	std::vector<Texture> m_Textures;
	MaterialMask m_MaterialMask;

	Material(MaterialMask mask)
		:m_MaterialMask(mask)
	{

	}
	Material(std::string key, MaterialMask mask)
		:m_MaterialMask(mask)
	{
		s_AllMaterials[key] = this;
	}
	~Material()
	{

	}

	void addTexture(std::string type, std::string path, bool gammaCorrection = false)
	{
		Texture text(m_Textures.size(), type, path);
		m_Textures.push_back(text);
	}

	inline static std::unordered_map<std::string, Material*>* getMapOfAllMaterials() { return &s_AllMaterials; }
private:
	static std::unordered_map<std::string, Material*> s_AllMaterials;
};

std::unordered_map<std::string, Material*> s_AllMaterials;

const glm::vec3 materialNone =			{ 1.0f, 1.0f, 1.0f };
const glm::vec3 materialPBR =			{ 0.8f, 0.4f, 0.3f };
const glm::vec3 materialBlinnPhong =	{ 0.5f, 0.1f, 0.9f };
const glm::vec3 materialCellShading =	{ 0.4f, 0.7f, 0.4f };

#endif