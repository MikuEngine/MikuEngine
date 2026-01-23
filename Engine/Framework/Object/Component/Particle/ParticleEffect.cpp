#include "EnginePCH.h"
#include "ParticleEffect.h"

#include "Framework/System/SystemManager.h"
#include "Framework/System/CameraSystem.h"
#include "Framework/System/ParticleSystem.h"
#include "Framework/Object/Component/Transform.h"

namespace engine
{
	ParticleEffect::~ParticleEffect()
	{
		SystemManager::Get().GetParticleSystem().Unregister(this);
	}

	void ParticleEffect::Initialize()
	{
		SystemManager::Get().GetParticleSystem().Register(this);
	}

	void ParticleEffect::Update()
	{
		if (!m_isPlaying)
		{
			return;
		}

		Vector3 camPos;
		if (auto mainCam = SystemManager::Get().GetCameraSystem().GetMainCamera())
		{
			camPos = mainCam->GetPosition();
		}

		float dt = Time::DeltaTime();

		Vector3 myPos = GetTransform()->GetWorldPosition();

		for (auto& emitter : m_emitters)
		{
			emitter.Update(dt, myPos, camPos);
		}
	}

	void ParticleEffect::AddEmitter()
	{
		auto emitter = ParticleEmitter();
		
		emitter.Initialize(EmitterProps{}); // 기본값으로 초기화

		m_emitters.push_back(std::move(emitter));
	}

	const std::vector<ParticleEmitter>& ParticleEffect::GetEmitters() const
	{
		return m_emitters;
	}

	void ParticleEffect::SortParticles()
	{
		for (auto& emitter : m_emitters)
		{
			emitter.SortParticlesByDistance();
		}
	}

	void ParticleEffect::Play()
	{
		m_isPlaying = true;
	}

	void ParticleEffect::Stop()
	{
		m_isPlaying = false;
	}
	
	void ParticleEffect::OnGui()
	{
		ImGui::SeparatorText("Particle Effect");

		bool isPlaying = m_isPlaying;
		if (ImGui::Checkbox("Is Playing", &isPlaying))
		{
			if (isPlaying)
			{
				Play();
			}
			else
			{
				Stop();
			}
		}

		ImGui::Spacing();

		// Emitter 리스트
		ImGui::Text("Emitters: %zu", m_emitters.size());

		if (ImGui::Button("Add Emitter"))
		{
			AddEmitter();
		}

		ImGui::Spacing();
		ImGui::Separator();

		// 각 Emitter UI
		for (size_t i = 0; i < m_emitters.size(); ++i)
		{
			std::string emitterLabel = std::format("Emitter {}", i);

			if (ImGui::TreeNode(emitterLabel.c_str()))
			{
				DrawEmitterGui(m_emitters[i], static_cast<int>(i));

				ImGui::Spacing();
				if (ImGui::Button(std::format("Remove Emitter {}", i).c_str()))
				{
					m_emitters.erase(m_emitters.begin() + i);
					ImGui::TreePop();
					break; // 인덱스 변경되므로 루프 종료
				}

				ImGui::TreePop();
			}
		}
	}
	
	void ParticleEffect::Save(json& j) const
	{
		Object::Save(j);

		j["IsPlaying"] = m_isPlaying;
		j["Emitters"] = json::array();

		for (const auto& emitter : m_emitters)
		{
			json emitterJson;
			emitter.Save(emitterJson);
			j["Emitters"].push_back(emitterJson);
		}
	}
	
	void ParticleEffect::Load(const json& j)
	{
		Object::Load(j);

		JsonGet(j, "IsPlaying", m_isPlaying);

		// Emitters 로드
		m_emitters.clear();
		JsonArrayForEach(j, "Emitters", [&](const json& emitterJson)
			{
				ParticleEmitter emitter;
				emitter.Load(emitterJson);
				m_emitters.push_back(emitter);
			});

		// Emitter가 없으면 기본 Emitter 추가
		if (m_emitters.empty())
		{
			AddEmitter();
		}
	}

	void ParticleEffect::DrawEmitterGui(ParticleEmitter& emitter, int index)
	{
		auto& props = const_cast<EmitterProps&>(emitter.GetProps());

		// Texture 선택
		ImGui::Text("Texture: %s", std::filesystem::path(emitter.GetTexturePath()).filename().string().c_str());
		std::string selectedTex;
		static std::vector<std::string> texExtensions{ ".png", ".jpg", ".tga", ".dds" };
		if (DrawFileSelector(std::format("Select Texture##{}", index).c_str(), "Resource/Texture", texExtensions, selectedTex))
		{
			emitter.SetTexturePath(selectedTex);
		}

		ImGui::Spacing();
		ImGui::SeparatorText("Emission");

		ImGui::DragFloat3("Position Offset", &props.positionOffset.x, 0.1f);
		ImGui::DragFloat("Emission Rate", &props.emissionRate, 0.1f, 0.0f, 1000.0f);
		ImGui::DragInt("Max Particles", reinterpret_cast<int*>(&props.maxParticles), 1, 1, 10000);
		ImGui::DragFloat("Life Time", &props.lifeTime, 0.1f, 0.01f, 100.0f);

		ImGui::Spacing();
		ImGui::SeparatorText("Velocity");

		ImGui::DragFloat3("Velocity", &props.velocity.x, 0.1f);
		ImGui::DragFloat3("Velocity Variation", &props.velocityVariation.x, 0.1f);
		ImGui::DragFloat3("Gravity", &props.gravity.x, 0.1f);

		ImGui::Spacing();
		ImGui::SeparatorText("Color");

		ImGui::ColorEdit4("Start Color", &props.startColor.x);
		ImGui::ColorEdit4("End Color", &props.endColor.x);

		ImGui::Spacing();
		ImGui::SeparatorText("Size");

		ImGui::DragFloat("Size Begin", &props.sizeBegin, 0.01f, 0.0f, 100.0f);
		ImGui::DragFloat("Size End", &props.sizeEnd, 0.01f, 0.0f, 100.0f);
		ImGui::DragFloat("Size Variation", &props.sizeVariation, 0.01f, 0.0f, 10.0f);

		ImGui::Spacing();
		ImGui::SeparatorText("Shape");

		static const char* shapeNames[] = { "Sphere", "Cone", "Box" };
		int currentShape = static_cast<int>(props.shape);
		if (ImGui::Combo("Shape", &currentShape, shapeNames, IM_ARRAYSIZE(shapeNames)))
		{
			props.shape = static_cast<EmitterShape>(currentShape);
		}

		if (props.shape == EmitterShape::Sphere || props.shape == EmitterShape::Cone)
		{
			ImGui::DragFloat("Radius", &props.radius, 0.1f, 0.0f, 100.0f);
		}

		if (props.shape == EmitterShape::Cone)
		{
			ImGui::DragFloat("Angle", &props.angle, 1.0f, 0.0f, 90.0f);
		}

		if (props.shape == EmitterShape::Box)
		{
			ImGui::DragFloat3("Box Size", &props.boxSize.x, 0.1f);
		}

		ImGui::Checkbox("Random Direction", &props.randomDirection);

		ImGui::Spacing();
		ImGui::SeparatorText("Texture Animation");

		ImGui::DragInt("Tiles X", reinterpret_cast<int*>(&props.textureTilesX), 1, 1, 32);
		ImGui::DragInt("Tiles Y", reinterpret_cast<int*>(&props.textureTilesY), 1, 1, 32);
		ImGui::DragInt("Tile Count", reinterpret_cast<int*>(&props.textureTileCount), 1, 0, props.textureTilesX * props.textureTilesY);
		ImGui::DragFloat("Animation Speed", &props.animationSpeed, 0.1f, 0.0f, 10.0f);
		ImGui::Checkbox("Random Frame", &props.isRandomFrame);

		ImGui::Spacing();
		ImGui::SeparatorText("Bursts");

		// Burst 리스트
		for (size_t i = 0; i < props.bursts.size(); ++i)
		{
			auto& burst = props.bursts[i];
			std::string burstLabel = std::format("Burst {}", i);

			if (ImGui::TreeNode(burstLabel.c_str()))
			{
				ImGui::DragFloat("Time", &burst.time, 0.1f, 0.0f, 100.0f);
				ImGui::DragInt("Count", reinterpret_cast<int*>(&burst.count), 1, 1, 10000);
				ImGui::DragInt("Cycles", reinterpret_cast<int*>(&burst.cycles), 1, 0, 100);
				ImGui::DragFloat("Interval", &burst.interval, 0.1f, 0.0f, 100.0f);

				if (ImGui::Button(std::format("Remove##{}", i).c_str()))
				{
					props.bursts.erase(props.bursts.begin() + i);
					ImGui::TreePop();
					break;
				}

				ImGui::TreePop();
			}
		}

		if (ImGui::Button("Add Burst"))
		{
			Burst newBurst;
			newBurst.time = 0.0f;
			newBurst.count = 10;
			newBurst.cycles = 1;
			newBurst.interval = 0.0f;
			props.bursts.push_back(newBurst);
		}

		// Props 변경 후 적용
		emitter.SetProps(props);
	}
}