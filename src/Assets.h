#ifndef ASSETS_H
#define ASSETS_H

#include <iostream>
#include <fstream>
#include <vector>
#include <unordered_map>

#include "src/Scene.h"
#include "src/shader.h"
#include "src/Model.h"
#include "src/Material.h"

class Assets
{
public:
	Assets(const char* filePath, Scene& scene)
		:m_Models(scene.m_Models), m_Lights(scene.m_Lights), m_Materials(scene.m_Materials)
	{
		std::fstream assetFile(filePath, std::ios::out | std::ios::in | std::ios::binary);
		if(!assetFile)
		{
			debug::log("ASSETS::Cannot open asset file", debug::Error);
		}
		assetFile.seekg(0, std::ios::end);
		unsigned int totalSize = assetFile.tellg();

		scene.data = new unsigned char[totalSize];
		assetFile.read((char*)scene.data, totalSize);
		if(!assetFile.good())
		{
			debug::log("ASSETS::error when reading asset file", debug::Error);
		}
		unsigned int objectOffset = 0;
		for(auto iter = scene.m_Models->begin(); iter != scene.m_Models->end(); iter++)
		{
			unsigned char* objectKey = &scene.data[objectOffset];
			unsigned int sizeOfString = 0;
			while(objectKey[sizeOfString] != NULL)
				sizeOfString++;

			(*scene.m_Models)[(char*)objectKey] = ((Model*)scene.data) + objectOffset + sizeOfString;
			objectOffset += sizeof(Model);
		}
		for(auto iter = scene.m_Lights->begin(); iter != scene.m_Lights->end(); iter++)
		{
			iter->second = ((Light*)scene.data) + objectOffset;
			objectOffset += sizeof(Light);
		}
		for(auto iter = scene.m_Materials->begin(); iter != scene.m_Materials->end(); iter++)
		{
			iter->second = ((Material*)scene.data) + objectOffset;
			objectOffset += sizeof(Material);
		}
		assetFile.close();
	}
private:
	std::unordered_map<std::string, Model*>* m_Models;
	std::unordered_map<std::string, Light*>* m_Lights;
	std::unordered_map<std::string, Material*>* m_Materials;
};

#endif