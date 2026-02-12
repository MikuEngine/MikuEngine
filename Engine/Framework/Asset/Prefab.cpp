#include "EnginePCH.h"
#include "Prefab.h"

#include <unordered_set>

#include "Framework/Asset/AssetManager.h"
#include "Framework/Asset/PrefabData.h"
#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Scene/SceneManager.h"
#include "Framework/Scene/Scene.h"

namespace engine
{
	void Prefab::Create(GameObject* target, const std::string& name)
	{
		if (target == nullptr)
		{
			return;
		}

		std::vector<GameObject*> flatList;
		flatList.reserve(128);

		std::vector<Transform*> stack;
		stack.push_back(target->GetTransform());

		std::unordered_set<Transform*> visited;
		visited.reserve(128);

		while (!stack.empty())
		{
			Transform* tr = stack.back();
			stack.pop_back();

			if (tr == nullptr)
			{
				continue;
			}

			if (!visited.insert(tr).second)
			{
				continue;
			}

			flatList.push_back(tr->GetGameObject());

			const auto& children = tr->GetChildren();
			// 기존 DFS(재귀) 순서를 유지하려고 역순 push
			for (int i = static_cast<int>(children.size()) - 1; i >= 0; --i)
			{
				stack.push_back(children[i]);
			}
		}

		std::unordered_map<GameObject*, int> ptrToId;
		for (int i = 0; i < flatList.size(); ++i)
		{
			ptrToId[flatList[i]] = i;
		}

		json root;
		root["Name"] = target->GetName();
		root["GameObjects"] = json::array();
		for (auto go : flatList)
		{
			json goJson;
			goJson["ID"] = ptrToId[go];

			Transform* parent = go->GetTransform()->GetParent();
			if (go != target && parent && ptrToId.find(parent->GetGameObject()) != ptrToId.end())
			{
				goJson["ParentID"] = ptrToId[parent->GetGameObject()];
			}
			else
			{
				goJson["ParentID"] = -1;
			}

			go->Save(goJson);
			root["GameObjects"].push_back(goJson);
		}

		auto prefabData = AssetManager::Get().GetOrCreatePrefabData(name);

		prefabData->SetData(std::move(root));
		prefabData->Save("Resource/Prefab/" + name + ".prefab");
	}

	GameObject* Prefab::Instantiate(const std::string& name)
	{
		Scene* scene = SceneManager::Get().GetScene();
		if (!scene)
		{
			return nullptr;
		}
		
		auto prefabData = AssetManager::Get().GetOrCreatePrefabData(name);
		if (!prefabData)
		{
			return nullptr;
		}
		
		const json& root = prefabData->GetData();
		const json& gameObjectsJson = root["GameObjects"];
		
		std::vector<GameObject*> createdObjects;
		std::unordered_map<int, GameObject*> idToObj;

		createdObjects.reserve(gameObjectsJson.size());
		
		for (const auto& goJson : gameObjectsJson)
		{
			std::string name = goJson.value("Name", "GameObject");
			GameObject* go = scene->CreateGameObject(name);

			int id = goJson.value("ID", -1);

			if (id != -1)
			{
				if (idToObj.find(id) != idToObj.end())
				{
					LOG_ERROR("[Prefab] Duplicate ID({}) found while instantiating '{}'. Later entry overrides earlier entry.", id, name);
				}
				idToObj[id] = go;
			}

			go->Load(goJson);
			createdObjects.push_back(go);
		}
		
		GameObject* rootInstance = nullptr;
		for (const auto& goJson : gameObjectsJson)
		{
			int id = goJson.value("ID", -1);
			int parentId = goJson.value("ParentID", -1);

			if (id == -1)
			{
				continue;
			}

			auto currentIter = idToObj.find(id);
			if (currentIter == idToObj.end() || currentIter->second == nullptr)
			{
				continue;
			}

			GameObject* current = currentIter->second;
			if (parentId != -1)
			{
				auto parentIter = idToObj.find(parentId);
				if (parentIter != idToObj.end() && parentIter->second != nullptr)
				{
					current->GetTransform()->SetParent(parentIter->second->GetTransform(), false);
				}
			}
			else
			{
				if (!rootInstance)
				{
					rootInstance = current;
				}
			}
		}

		return rootInstance;
	}
}