#pragma once

#include "Framework/Object/Component/Renderer/Renderer.h"
#include "Core/Graphics/Data/ConstantBufferTypes.h"
#include "Core/Graphics/Resource/Texture.h"

namespace engine
{
	class SkeletalMeshRenderer;
	class SkeletalMeshData;
	class MaterialData;
	class VertexBuffer;
	class IndexBuffer;
	class ConstantBuffer;
	class VertexShader;
	class PixelShader;
	class InputLayout;
	class SamplerState;
	class RasterizerState;

	// A-1: 잔상 한 슬라이스 (위치·회전 + 알파)
	struct AfterimageSlice
	{
		Matrix world;
		float alpha = 1.0f;
	};

	// 알파 감쇠 방식: 동시(모든 슬라이스 동시 감쇠) / 순차(가장 오래된 슬라이스부터 하나씩 감쇠)
	enum class AlphaDecayMode
	{
		Simultaneous,
		Sequential
	};

	// A-2: 최대 슬라이스 수 기본값 (링 버퍼 상한)
	static constexpr size_t AFTERIMAGE_DEFAULT_MAX_SLICES = 24;
	static constexpr size_t AFTERIMAGE_MIN_SLICES = 1;
	static constexpr size_t AFTERIMAGE_MAX_SLICES_CAP = 64;

	class AfterimageRenderer :
		public Renderer
	{
		REGISTER_COMPONENT(AfterimageRenderer, Renderer)

	private:
		// B-1: 소스 SkeletalMeshRenderer (리소스·본 데이터 참조)
		Ptr<SkeletalMeshRenderer> m_source;

		// B-2: 소스와 동일한 메쉬 경로로 로드한 리소스
		std::string m_meshFilePath;
		std::shared_ptr<SkeletalMeshData> m_meshData;
		std::shared_ptr<MaterialData> m_materialData;
		std::shared_ptr<VertexBuffer> m_vertexBuffer;
		std::shared_ptr<IndexBuffer> m_indexBuffer;
		std::shared_ptr<ConstantBuffer> m_objectConstantBuffer;
		std::shared_ptr<ConstantBuffer> m_materialConstantBuffer;
		std::shared_ptr<ConstantBuffer> m_boneConstantBuffer;
		std::shared_ptr<VertexShader> m_vs;
		std::shared_ptr<PixelShader> m_transparentPS;
		std::vector<Textures> m_textures;
		std::shared_ptr<InputLayout> m_inputLayout;
		std::shared_ptr<SamplerState> m_samplerState;
		std::shared_ptr<RasterizerState> m_rasterizerState;
		CbBone m_boneTransformData = {};
		bool m_isRefreshed = false;

		// C-1: 기록 플래그
		bool m_isRecording = false;
		// E-1: 기록 시 초기 알파
		float m_initialAlpha = 0.7f;
		// E-2: 알파 감쇠 (매 프레임 alpha -= m_alphaDecaySpeed * dt, 0 이하면 제거)
		float m_alphaDecaySpeed = 1.5f;
		AlphaDecayMode m_alphaDecayMode = AlphaDecayMode::Simultaneous;
		// 최대 슬라이스 수 (링 버퍼 상한, 에디터/직렬화로 조정 가능)
		size_t m_maxSlices = AFTERIMAGE_DEFAULT_MAX_SLICES;

		// A-2: 링 버퍼 (가장 오래된 것부터 제거)
		std::vector<AfterimageSlice> m_slices;

	private:
		void Refresh();

	public:
		// B-1: 소스 설정 (같은 GameObject면 GetComponent<SkeletalMeshRenderer>() 등)
		void SetSource(SkeletalMeshRenderer* source);

		// Phase C: 기록 API (스크립트 연동)
		void BeginRecording();
		void RecordSample();
		void RecordSample(const Matrix& world);
		void EndRecording();
		/// 순간이동 등에서 기존 잔상을 즉시 제거할 때 호출
		void ClearSlices();
		/// 순간이동 구간(from → to)에 보간 슬라이스를 numSlices개 채움. 기록 상태와 무관하게 호출 가능.
		void RecordTeleportPath(const Matrix& fromWorld, const Matrix& toWorld, size_t numSlices);

		// E-1: 초기 알파 설정
		void SetInitialAlpha(float alpha);
		float GetInitialAlpha() const { return m_initialAlpha; }
		// E-2: 알파 감쇠 속도 (0이면 감쇠 없음)
		void SetAlphaDecaySpeed(float speed);
		float GetAlphaDecaySpeed() const { return m_alphaDecaySpeed; }
		void SetAlphaDecayMode(AlphaDecayMode mode) { m_alphaDecayMode = mode; }
		AlphaDecayMode GetAlphaDecayMode() const { return m_alphaDecayMode; }
		// 최대 슬라이스 수 (1 ~ AFTERIMAGE_MAX_SLICES_CAP)
		void SetMaxSlices(size_t count);
		size_t GetMaxSlices() const { return m_maxSlices; }

		AfterimageRenderer() = default;
		~AfterimageRenderer() override = default;

		void Initialize() override;
		void Update() override;

		// A-4
		bool HasRenderType(RenderType type) const override;
		void Draw(RenderType type) const override;
		DirectX::BoundingBox GetBounds() const override;

		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;

		// F-4: 잔상은 그림자·마스크·피킹 미사용
		void DrawShadow(RenderType, LightType) const override {}
		void DrawMask() const override {}
		void DrawPickingID() const override {}
	};
}
