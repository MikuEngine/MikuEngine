#include "EnginePCH.h"
#include "Renderer.h"

#include "Framework/Object/GameObject/GameObject.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/RenderSystem.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/Component/Animator/SkeletalAnimator.h"

namespace engine
{
	Renderer::Renderer()
	{
		m_systemIndices.fill(-1);
	}

	Renderer::~Renderer()
	{
		SystemManager::Get().GetRenderSystem().Unregister(this);
	}

	void Renderer::Initialize()
	{
		SystemManager::Get().GetRenderSystem().Register(this);
	}

	void Renderer::DrawSocketEditor()
	{
		static Vector3 s_tempEuler = Vector3::Zero;
		static int s_editingIndex = -1;

		if (ImGui::CollapsingHeader("Socket Editor", ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::Button("Add New Socket"))
			{
				Socket* newSocket = new Socket();
				newSocket->name = "New Socket " + std::to_string(m_socketInstances.size());
				newSocket->localScale = Vector3::One;
				newSocket->localRotation = Quaternion::Identity;
				newSocket->localPosition = Vector3::Zero;
				newSocket->UpdateLocalMatrix();

				SocketInstance newInstance;
				newInstance.info = newSocket;
				newInstance.worldMatrix = Matrix::Identity;
				m_socketInstances.push_back(newInstance);
			}

			ImGui::SameLine();
			if (ImGui::Button("Save Socket"))
			{
				SaveSocketData();
			}

			for (size_t i = 0; i < m_socketInstances.size(); ++i)
			{
				auto& instance = m_socketInstances[i];
				Socket* socket = const_cast<Socket*>(instance.info);
				auto animator = GetGameObject()->GetComponent<SkeletalAnimator>();

				ImGui::PushID(static_cast<int>(i));

				bool pendingDelete = false;
				std::string nodeLabel = (socket->name.empty() ? "New Socket" : socket->name) + "###SocketNode_" + std::to_string(i);

				if (ImGui::TreeNode(nodeLabel.c_str()))
				{
					bool changed = false;

					ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 35);
					if (ImGui::InputText("##SocketNameInput", &socket->name)) { changed = true; }

					ImGui::SameLine();
					if (ImGui::Button("X"))
					{
						pendingDelete = true;
					}

					if (!pendingDelete)
					{
						if (animator)
						{
							const char* preview = socket->parentBoneName.empty() ? "<None>" : socket->parentBoneName.c_str();

							if (ImGui::BeginCombo("Select Bone##BoneSelect", preview))
							{
								const auto& skeleton = animator->GetSkeleton();
								for (const auto& bone : skeleton)
								{
									bool isSelected = (socket->parentBoneName == bone.name);

									if (ImGui::Selectable(bone.name.c_str(), isSelected))
									{
										Matrix oldWorld = instance.worldMatrix;
										socket->parentBoneName = bone.name;

										Matrix newBoneWorld = animator->GetBoneWorldMatrix(bone.name);
										socket->localMatrix = oldWorld * newBoneWorld.Invert();

										socket->DecomposeLocalMatrix();

										UpdateSockets();
									}


									if (isSelected)
										ImGui::SetItemDefaultFocus();
								}
								ImGui::EndCombo();
							}
						}
						else
						{
							if (ImGui::InputText("Parent Bone", &socket->parentBoneName))
							{
								changed = true;
								socket->localPosition = Vector3::Zero;
								socket->UpdateLocalMatrix();
								UpdateSockets();
							}
						}
						
						ImGui::Separator();

						changed |= ImGui::DragFloat3("Pos", &socket->localPosition.x, 0.05f);
						ImGui::SameLine();
						if (ImGui::Button("Reset##Pos"))
						{
							socket->localPosition = Vector3::Zero;
							changed = true;
						}

						if (s_editingIndex != static_cast<int>(i))
						{
							s_tempEuler = socket->localRotation.ToEuler() * (180.0f / DirectX::XM_PI);
							s_editingIndex = static_cast<int>(i);
						}

						if (ImGui::DragFloat3("Rot", &s_tempEuler.x, 0.5f))
						{
							socket->localRotation = Quaternion::CreateFromYawPitchRoll(
								ToRadian(s_tempEuler.y),
								ToRadian(s_tempEuler.x),
								ToRadian(s_tempEuler.z)
							);
							changed = true;
						}
						ImGui::SameLine();
						if (ImGui::Button("Reset##Rot"))
						{
							socket->localRotation = Quaternion::Identity;
							s_tempEuler = Vector3::Zero;
							changed = true;
						}

						changed |= ImGui::DragFloat3("Scale", &socket->localScale.x, 0.05f);
						ImGui::SameLine();
						if (ImGui::Button("Reset##Scale"))
						{
							socket->localScale = Vector3::One;
							changed = true;
						}

						if (changed)
						{
							socket->UpdateLocalMatrix();
							UpdateSockets();
						}
					}

					ImGui::TreePop();
				}

				ImGui::PopID();

				if (pendingDelete)
				{
					delete instance.info;
					m_socketInstances.erase(m_socketInstances.begin() + i);
					--i;
					continue;
				}
			}
		}
	}

	void Renderer::SaveSocketData()
	{
		std::string meshPath = GetMeshPath();
		if (meshPath.empty()) return;

		std::string fileName = std::filesystem::path(meshPath).filename().string();
		std::string folderPath = "Resource/Data/Socket/";
		std::string filePath = folderPath + fileName + ".socketdata";

		if (m_socketInstances.empty())
		{
			if (std::filesystem::exists(filePath))
			{
				std::filesystem::remove(filePath);
			}
			return;
		}

		if (!std::filesystem::exists(folderPath))
		{
			std::filesystem::create_directories(folderPath);
		}

		std::vector<Socket> socketsToSave;
		for (const auto& instance : m_socketInstances)
		{
			if (instance.info)
				socketsToSave.push_back(*(instance.info));
		}

		SocketData saver;
		saver.SetSockets(socketsToSave);
		saver.Save(filePath);
	}

	void Renderer::LoadSocketData()
	{
		const std::string& meshPath = GetMeshPath();
		if (meshPath.empty()) return;

		std::string filePath = "Resource/Data/Socket/" + std::filesystem::path(meshPath).filename().string() + ".socketdata";

		SocketData loader;
		loader.Create(filePath);

		const auto& loadedSockets = loader.GetSockets();
		if (loadedSockets.empty())
		{
			ClearSockets();
			return;
		}

		std::vector<SocketInstance> tempInstances;
		tempInstances.reserve(loadedSockets.size());

		for (const auto& s : loadedSockets)
		{
			SocketInstance instance;
			instance.info = new Socket(s);
			instance.worldMatrix = Matrix::Identity;
			tempInstances.push_back(instance);
		}

		ClearSockets();
		m_socketInstances = std::move(tempInstances);

		UpdateSockets();
	}

	void Renderer::UpdateSockets()
	{
		if (this == nullptr) return;

		auto transform = GetTransform();
		if (transform == nullptr) return;

		Matrix world = GetTransform()->GetWorld();

		for (auto& instance : m_socketInstances)
		{
			if (instance.info == nullptr) continue;

			instance.worldMatrix = instance.info->localMatrix * world;
		}
	}

	Matrix Renderer::GetSocketWorldMatrix(const std::string& name) const
	{
		for (const auto& instance : m_socketInstances)
		{
			if (instance.info->name == name)
			{
				return instance.worldMatrix;
			}
		}

		return GetTransform()->GetWorld();
	}
	void Renderer::ClearSockets()
	{
		for (auto& instance : m_socketInstances)
		{
			delete instance.info;
			instance.info = nullptr;
		}
		m_socketInstances.clear();
	}
}