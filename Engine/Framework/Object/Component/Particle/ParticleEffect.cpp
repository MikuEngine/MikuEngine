#include "EnginePCH.h"
#include "ParticleEffect.h"

#include "Common/Utility/StaticMemoryPool.h"
#include "Framework/System/SystemManager.h"
#include "Framework/System/CameraSystem.h"
#include "Framework/System/ParticleSystem.h"
#include "Framework/Object/Component/Transform.h"
#include "Framework/Object/GameObject/GameObject.h"

namespace engine
{
	namespace
	{
		StaticMemoryPool<ParticleEffect, 256> g_particleEffectPool;
	}

	ParticleEffect::~ParticleEffect()
	{
		SystemManager::Get().GetParticleSystem().Unregister(this);
	}

	void* ParticleEffect::operator new(size_t size)
	{
		return g_particleEffectPool.Allocate(size);
	}

	void ParticleEffect::operator delete(void* ptr)
	{
		g_particleEffectPool.Deallocate(ptr);
	}

	void ParticleEffect::Initialize()
	{
		SystemManager::Get().GetParticleSystem().Register(this);
	}

	void ParticleEffect::Update()
	{
		Vector3 camPos;
		if (auto mainCam = SystemManager::Get().GetCameraSystem().GetMainCamera())
		{
			camPos = mainCam->GetPosition();
		}

		float dt = Time::DeltaTime();

		Vector3 myPos = GetTransform()->GetWorldPosition();

		// 재생 중일 때만 재생 시간 업데이트
		if (m_isPlaying)
		{
			// 재생 시간 업데이트
			m_playTime += dt;

			// Duration이 설정되어 있고 시간이 지났으면 자동 Stop
			if (m_duration > 0.0f && m_playTime >= m_duration)
			{
				Stop();
			}
		}

		// Stop() 후에도 기존 파티클이 사라질 수 있도록 항상 업데이트
		// isPlaying 상태를 전달하여 재생 중일 때만 새 파티클 생성
		for (auto& emitter : m_emitters)
		{
			emitter.Update(dt, myPos, camPos, m_isPlaying);
		}

		// 자동 삭제가 활성화되어 있고 파티클이 모두 끝났으면 삭제
		if (m_autoDestroy && IsFinished())
		{
			GetGameObject()->Destroy();
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
		m_playTime = 0.0f; // 재생 시간 리셋
	}

	void ParticleEffect::Stop()
	{
		m_isPlaying = false;
	}

	bool ParticleEffect::IsFinished() const
	{
		// 모든 emitter가 끝났는지 확인
		for (const auto& emitter : m_emitters)
		{
			if (!emitter.IsFinished(m_isPlaying))
			{
				return false;
			}
		}
		return true;
	}

	void ParticleEffect::SetAutoDestroy(bool autoDestroy)
	{
		m_autoDestroy = autoDestroy;
	}

	bool ParticleEffect::GetAutoDestroy() const
	{
		return m_autoDestroy;
	}

	void ParticleEffect::SetDuration(float duration)
	{
		m_duration = duration;
		if (duration < 0.0f)
		{
			m_duration = 0.0f; // 음수는 0으로 클램프
		}
	}

	float ParticleEffect::GetDuration() const
	{
		return m_duration;
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

		ImGui::Checkbox("Auto Destroy", &m_autoDestroy);
		if (m_autoDestroy)
		{
			ImGui::SameLine();
			bool isFinished = IsFinished();
			ImGui::TextColored(isFinished ? ImVec4(1.0f, 0.0f, 0.0f, 1.0f) : ImVec4(0.0f, 1.0f, 0.0f, 1.0f),
				isFinished ? " (Finished)" : " (Active)");
		}

		ImGui::Spacing();

		// Duration 설정
		ImGui::DragFloat("Duration", &m_duration, 0.1f, 0.0f, 1000.0f);
		if (m_duration <= 0.0f)
		{
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(Infinite)");
		}
		else
		{
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(%.2fs)", m_duration);
			if (m_isPlaying)
			{
				ImGui::Text("Play Time: %.2f / %.2f", m_playTime, m_duration);
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
		j["AutoDestroy"] = m_autoDestroy;
		j["Duration"] = m_duration;
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
		JsonGet(j, "AutoDestroy", m_autoDestroy);
		JsonGet(j, "Duration", m_duration);
		m_playTime = 0.0f; // 로드 시 재생 시간 리셋

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
		ImGui::DragInt("Max Particles", reinterpret_cast<int*>(&props.maxParticles), 1, 1, 10000);
		ImGui::DragFloat("Life Time", &props.lifeTime, 0.1f, 0.01f, 100.0f);
		ImGui::Checkbox(std::format("Use Distance Emission##{}", index).c_str(), &props.useDistanceEmission);
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("ON: 거리 간격 설정. 너무 작으면 위험하므로 최소치를 0.001f 정도로 제한합니다. \nOFF: Emission Rate 그대로 사용.");
		}

		if (props.useDistanceEmission)
		{
			ImGui::DragFloat(std::format("Emit Spacing##{}", index).c_str(), &props.emitSpacing, 0.001f, 0.001f, 2.0f, "%.3f m");
		}
		else
		{
			ImGui::DragFloat(std::format("Emission Rate##{}", index).c_str(), &props.emissionRate, 0.1f, 0.0f, 10000.0f);
		}

		ImGui::Spacing();
		ImGui::SeparatorText("Velocity");

		ImGui::DragFloat3("Velocity", &props.velocity.x, 0.1f);
		ImGui::DragFloat3("Velocity Variation", &props.velocityVariation.x, 0.1f);
		ImGui::DragFloat3("Gravity", &props.gravity.x, 0.1f);
		ImGui::Checkbox("Use Shape Direction", &props.useShapeDirection);
		ImGui::SameLine();
		ImGui::TextDisabled("(?)");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("ON: Shape(Cone/Sphere) 방향으로 velocity 크기만 적용.\nOFF: Velocity (x,y,z) 그대로 사용.");
		}

		ImGui::Spacing();
		ImGui::SeparatorText("Rotation");

		ImGui::DragFloat("Rotation Min", &props.rotationMin, 0.1f);
		ImGui::DragFloat("Rotation Max", &props.rotationMax, 0.1f);

		if (props.rotationMin > props.rotationMax)
		{
			std::swap(props.rotationMin, props.rotationMax);
		}

		ImGui::DragFloat("Rotation Speed Min", &props.rotationSpeedMin, 0.1f);
		ImGui::DragFloat("Rotation Speed Max", &props.rotationSpeedMax, 0.1f);

		if (props.rotationSpeedMin > props.rotationSpeedMax)
		{
			std::swap(props.rotationSpeedMin, props.rotationSpeedMax);
		}

		ImGui::Spacing();
		ImGui::SeparatorText("Color");

		ImGui::ColorEdit4("Start Color", &props.startColor.x);
		ImGui::ColorEdit4("End Color", &props.endColor.x);
		int type = static_cast<int>(props.blend);
		const char* blendItems[]{ "Additive", "Blend" };
		if (ImGui::Combo("Blend Type", &type, blendItems, IM_ARRAYSIZE(blendItems)))
		{
			props.blend = static_cast<EmitterBlend>(type);
		}

		ImGui::Spacing();
		ImGui::SeparatorText("Emissive");

		ImGui::ColorEdit3("Start Emissive", &props.startEmissive.x);
		ImGui::ColorEdit3("End Emissive", &props.endEmissive.x);
		ImGui::DragFloat("Start Emissive Intensity", &props.startEmissiveIntensity, 0.01f, 0.0f, 100.0f, "%.2f");
		ImGui::DragFloat("End Emissive Intensity", &props.endEmissiveIntensity, 0.01f, 0.0f, 100.0f, "%.2f");

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