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
		std::shared_ptr<PixelShader> m_transparentPS;   // 알파 레이어 (라이팅)
		std::shared_ptr<PixelShader> m_emissivePS;       // 솔리드 레이어 (언릿/이미시브)
		std::vector<Textures> m_textures;
		std::shared_ptr<InputLayout> m_inputLayout;
		std::shared_ptr<SamplerState> m_samplerState;
		std::shared_ptr<RasterizerState> m_rasterizerState;
		CbBone m_boneTransformData = {};
		bool m_isRefreshed = false;

		// C-1: 기록 플래그
		bool m_isRecording = false;

		// 솔리드 레이어 전용 (알파 없음, emissive/언릿)
		bool m_drawSolidLayer = false;
		Vector4 m_solidColor = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		size_t m_solidMaxSlices = AFTERIMAGE_DEFAULT_MAX_SLICES;
		float m_solidSampleInterval = 0.0f;
		float m_solidLastSampleTime = 0.0f;
		std::vector<AfterimageSlice> m_slicesSolid;

		// 알파 레이어 전용 (감쇠 + 틴트, 라이팅)
		bool m_drawAlphaLayer = true;
		Vector4 m_alphaTint = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
		float m_alphaInitialAlpha = 0.7f;
		float m_alphaDecaySpeed = 1.5f;
		AlphaDecayMode m_alphaDecayMode = AlphaDecayMode::Simultaneous;
		size_t m_alphaMaxSlices = AFTERIMAGE_DEFAULT_MAX_SLICES;
		float m_alphaSampleInterval = 0.0f;
		float m_alphaLastSampleTime = 0.0f;
		std::vector<AfterimageSlice> m_slicesAlpha;

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

		// 솔리드 레이어 전용
		void SetSolidMaxSlices(size_t count);
		size_t GetSolidMaxSlices() const { return m_solidMaxSlices; }
		void SetSolidSampleInterval(float seconds);
		float GetSolidSampleInterval() const { return m_solidSampleInterval; }
		// 알파 레이어 전용
		void SetAlphaInitialAlpha(float alpha);
		float GetAlphaInitialAlpha() const { return m_alphaInitialAlpha; }
		void SetAlphaDecaySpeed(float speed);
		float GetAlphaDecaySpeed() const { return m_alphaDecaySpeed; }
		void SetAlphaDecayMode(AlphaDecayMode mode) { m_alphaDecayMode = mode; }
		AlphaDecayMode GetAlphaDecayMode() const { return m_alphaDecayMode; }
		void SetAlphaMaxSlices(size_t count);
		size_t GetAlphaMaxSlices() const { return m_alphaMaxSlices; }
		void SetAlphaSampleInterval(float seconds);
		float GetAlphaSampleInterval() const { return m_alphaSampleInterval; }
		// 하위 호환: 알파 레이어에 대응
		void SetInitialAlpha(float alpha) { SetAlphaInitialAlpha(alpha); }
		float GetInitialAlpha() const { return m_alphaInitialAlpha; }
		void SetMaxSlices(size_t count) { SetAlphaMaxSlices(count); }
		size_t GetMaxSlices() const { return m_alphaMaxSlices; }
		void SetSampleInterval(float seconds) { SetAlphaSampleInterval(seconds); }
		float GetSampleInterval() const { return m_alphaSampleInterval; }
		/// 솔리드 레이어 (알파 없음, 단색 실루엣)
		void SetDrawSolidLayer(bool draw) { m_drawSolidLayer = draw; }
		bool GetDrawSolidLayer() const { return m_drawSolidLayer; }
		void SetSolidColor(const Vector4& color);
		const Vector4& GetSolidColor() const { return m_solidColor; }
		/// 알파 레이어 (감쇠 + 틴트)
		void SetDrawAlphaLayer(bool draw) { m_drawAlphaLayer = draw; }
		bool GetDrawAlphaLayer() const { return m_drawAlphaLayer; }
		void SetAlphaTint(const Vector4& color);
		const Vector4& GetAlphaTint() const { return m_alphaTint; }

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
