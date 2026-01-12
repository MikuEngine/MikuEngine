#pragma once

// 엔진 전용 Geometry 생성
// 엔진 내에서 사용할 메쉬임 quad, cube, sphere, cone 등
// 필수 요소들 vertex, index 만 만들어 줌

#include "Core/Graphics/Data/Vertex.h"

namespace engine
{
	class GeometryGenerator
	{
	public:
		struct Data
		{
			std::vector<CommonVertex> vertices;
			std::vector<DWORD> indices;
		};

	public:
		static Data MakeQuad(float width, float height);
		static Data MakeCube(float size);
		static Data MakeSphere(float radius, int sliceCount, int stackCount);
		static Data MakePlane(float width, float depth);
		static Data MakeCone(float radius, float height, int sliceCount);
	};
}