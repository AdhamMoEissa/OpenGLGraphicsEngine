#ifndef SCENE_H
#define SCENE_H

#include <src/Model.h>
#include <src/Light.h>
#include <src/Material.h>
#include <iterator>
#include <unordered_map>
#include <map>
#include <vector>
#include <tuple>

struct SceneNode
{
	std::unordered_map<unsigned int, SceneNode*>* m_ChildNodes;
	std::string* m_ModelKey;
	std::string* m_LightKey;
	std::string* m_MaterialKey;
	glm::vec3 m_Pos;
	glm::vec3 m_Scale;
	glm::quat m_Rot;

	SceneNode(unsigned int id)
	{
		s_AllNodes[id] = this;
		m_ChildNodes = nullptr;

		m_ModelKey = nullptr;
		m_LightKey = nullptr;
		m_MaterialKey = nullptr;

		m_Pos = { 0.0f, 0.0f, 0.0f };
		m_Scale = { 0.0f, 0.0f, 0.0f };
		m_Rot = { 0.0f, 0.0f, 0.0f, 0.0f };
	}
	SceneNode(unsigned int id, std::string&& modelKey, std::string&& lightKey, std::string&& materialKey)
	{
		s_AllNodes[id] = this;
		m_ChildNodes = nullptr;

		m_ModelKey    = new std::string(std::move(modelKey));
		m_LightKey    = new std::string(std::move(lightKey));
		m_MaterialKey = new std::string(std::move(materialKey));

		m_Pos   = { 0.0f, 0.0f, 0.0f };
		m_Scale = { 0.0f, 0.0f, 0.0f };
		m_Rot   = { 0.0f, 0.0f, 0.0f, 0.0f };
	}
	SceneNode(unsigned int id, std::string&& modelKey, std::string&& lightKey, std::string&& materialKey, glm::vec3 pos, glm::vec3 scale, glm::quat rot)
	{
		s_AllNodes[id] = this;
		m_ChildNodes = nullptr;

		m_ModelKey    = new std::string(std::move(modelKey));
		m_LightKey    = new std::string(std::move(lightKey));
		m_MaterialKey = new std::string(std::move(materialKey));

		m_Pos = pos;
		m_Scale = scale;
		m_Rot = rot;
	}
	SceneNode(unsigned int id, SceneNode& node)
	{
		s_AllNodes[id] = this;
		m_ChildNodes = nullptr;
		if(!node.m_ChildNodes->empty())
		{
			m_ChildNodes = new std::unordered_map<unsigned int, SceneNode*>;
			auto map = getAllChildren(*node.m_ChildNodes);
			for(auto iter = map.begin(); iter != map.end(); iter++)
			{
				unsigned int id = getFreeID();
				SceneNode* node = new SceneNode(id, std::move(*(*iter).second));
				(*m_ChildNodes)[id] = node;
			}
		}

		m_ModelKey    = new std::string(*node.m_ModelKey);
		m_LightKey    = new std::string(*node.m_LightKey);
		m_MaterialKey = new std::string(*node.m_MaterialKey);
		
		m_Pos = node.m_Pos;
		m_Scale = node.m_Scale;
		m_Rot = node.m_Rot;
	}
	SceneNode(unsigned int id, SceneNode&& node)
	{
		s_AllNodes[id] = this;

		m_ChildNodes = std::move(node.m_ChildNodes);
		node.m_ChildNodes = nullptr;

		m_ModelKey    = new std::string(std::move(*node.m_ModelKey));
		m_LightKey    = new std::string(std::move(*node.m_LightKey));
		m_MaterialKey = new std::string(std::move(*node.m_MaterialKey));
		node.m_ModelKey    = nullptr;
		node.m_LightKey    = nullptr;
		node.m_MaterialKey = nullptr;

		m_Pos   = std::move(node.m_Pos);
		m_Scale = std::move(node.m_Scale);
		m_Rot   = std::move(node.m_Rot);
	}

	~SceneNode()
	{
		destroy();
	}

	//destroys the Scene Node object and all of its child nodes
	void destroy()
	{
		if(!m_ChildNodes->empty())
		{
			auto map = getAllChildren(*m_ChildNodes);
			for(auto iter = map.begin(); iter != map.end(); iter++)
			{
				(*iter).second->destroy();
				(*iter).second->m_ChildNodes = nullptr;
				s_AllNodes.erase((*iter).first);
			}
		}
		m_ChildNodes = nullptr;

		m_ModelKey = nullptr;
		m_LightKey = nullptr;
		m_MaterialKey = nullptr;
	}
	//Use to add a child node to current node
	void addChildNode(unsigned int id, SceneNode& node)
	{
		if(m_ChildNodes == nullptr)
			m_ChildNodes = new std::unordered_map<unsigned int, SceneNode*>;
		(*m_ChildNodes)[id] = &node;
	}
	//Use to remove a Child node from both this current node and all scene nodes
	//returns 0 if the child node has been successfully erased
	//returns 1 otherwise
	bool destroyChildNode(unsigned int id)
	{
		if(s_AllNodes.find(id) != s_AllNodes.end() && m_ChildNodes->find(id) != m_ChildNodes->end())
		{
			(*m_ChildNodes)[id]->destroy();
			return 0;
		}
		else
		{
			return 1;
		}
	}

	void updatePosition(glm::vec3 pos)
	{
		m_Pos = pos;
	}
	void updateScale(glm::vec3 scale)
	{
		m_Scale = scale;
	}
	void updateRotation(glm::quat rot)
	{
		m_Rot = rot;
	}
	void updateRotation(glm::mat4 rot)
	{
		m_Rot = glm::quat_cast(rot);
	}
	void updateRotation(float angle, glm::vec3 vector)
	{
		m_Rot = glm::quat_cast(glm::rotate(glm::mat4(1.0f), angle, vector));
	}

	void changeModelKey(std::string key)
	{
		if(m_ModelKey == nullptr)
			m_ModelKey = new std::string(key);
		else
			*m_ModelKey = key;
	}
	void changeLightKey(std::string key)
	{
		if(m_LightKey == nullptr)
			m_LightKey = new std::string(key);
		else
			*m_LightKey = key;
	}
	void changeMaterialKey(std::string key)
	{
		if(m_MaterialKey == nullptr)
			m_MaterialKey = new std::string(key);
		else
			*m_MaterialKey = key;
	}

	glm::mat4 getMatrix()
	{
		glm::mat4 matrix(1.0f);
		matrix = glm::translate(matrix, m_Pos);
		matrix = matrix * glm::mat4_cast(m_Rot);
		matrix = glm::scale(matrix, m_Scale);
		return matrix;
	}
	inline unsigned int getID()
	{
		std::unordered_map<unsigned int, SceneNode*>::iterator iter;
		for(iter = s_AllNodes.begin(); iter != s_AllNodes.end(); iter++)
		{
			if(this == (*iter).second)
				return (*iter).first;
		}
		return -1;
	}
	inline unsigned int getNrOfChildren()
	{
		if(m_ChildNodes == nullptr)
			return 0;
		else
			return m_ChildNodes->size();
	}

	inline SceneNode* operator[](unsigned int id) { return SceneNode::getNodeFromID(id); }
	inline bool operator<(SceneNode other) { return this->getID() < other.getID(); }
	inline bool operator==(SceneNode other) { return this->getID() == other.getID(); }
	inline bool operator>(SceneNode other) { return this->getID() > other.getID(); }
	inline static std::unordered_map<unsigned int, SceneNode*>* getMapOfAllNodes() { return &s_AllNodes;  }
	inline static SceneNode* getNodeFromID(unsigned int id) { return s_AllNodes[id]; }
	inline static unsigned int getNrOfNodes() { return s_AllNodes.size(); }
	static unsigned int getFreeID()
	{
		for(unsigned int i = 1; i < INT_MAX; i++)
		{
			bool isUsed = 0;
			for(auto iter = s_AllNodes.begin(); iter != s_AllNodes.end(); iter++)
			{
				if((*iter).first == i)
					isUsed = 1;
			}
			if(!isUsed)
				return i;
		}
	}
private:
	static std::unordered_map<unsigned int, SceneNode*> s_AllNodes; //The map of all Scene Nodes
	std::unordered_map<unsigned int, SceneNode*>& getAllChildren(std::unordered_map<unsigned int, SceneNode*>& map)
	{
		if(map.empty())
			return map;
		std::unordered_map<unsigned int, SceneNode*> resultantMap;
		for(auto iter = map.begin(); iter != map.end(); iter++)
		{
			if((*iter).second->getNrOfChildren() != 0)
			{
				auto temp = getAllChildren(*(*iter).second->m_ChildNodes);
				resultantMap.insert(temp.begin(), temp.end());
			}
			resultantMap.insert(*iter);
		}
		return resultantMap;
	}
};

std::unordered_map<unsigned int, SceneNode*> SceneNode::s_AllNodes;

class Scene
{
public:
	unsigned char* data;
	std::unordered_map<std::string, Model*>* m_Models;
	std::unordered_map<std::string, Light*>* m_Lights;
	std::unordered_map<std::string, Material*>* m_Materials;
	Scene()
	{
		m_Nodes = SceneNode::getMapOfAllNodes();
		(*m_Nodes)[0] = new SceneNode(0);
		m_Materials = Material::getMapOfAllMaterials();
		//m_Models = Model::getMapOfAllModels();
		//m_Models = Model::getMapOfAllLights();
	}
	~Scene()
	{
		for(auto iter = m_Nodes->begin(); iter != m_Nodes->end(); iter++)
		{
			delete (*iter).second;
		}
	}
private:
	std::unordered_map<unsigned int, SceneNode*>* m_Nodes;
};

#endif