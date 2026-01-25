#pragma once

#include "Framework/Object/Component/Component.h"

namespace engine
{
	struct GridCell
	{
		bool walkable = true;
		float cost = 1.0f;
	};

	class GridMap :
		public Component
	{
		REGISTER_COMPONENT(GridMap, Component)

	private:
		Vector3 m_origin{ 0.0f, 0.0f, 0.0f };
		float m_cellSize = 1.0f;
		int m_width = 10;
		int m_height = 10;
		std::vector<GridCell> m_cells;

		// 에디터 편집용
		bool m_editMode = false;
		int m_selectedCellX = -1;
		int m_selectedCellZ = -1;

	public:
		~GridMap();

	public:
		void Initialize() override;

		void WorldToGrid(const Vector3& worldPos, int& outX, int& outZ) const;
		Vector3 GridToWorld(int x, int z) const;

		bool IsValid(int x, int z) const;
		bool IsWalkable(int x, int z) const;
		void SetWalkable(int x, int z, bool walkable);
		GridCell& GetCell(int x, int z);
		const GridCell& GetCell(int x, int z) const;

		void ResizeGrid(int width, int height);
		void MarkAllWalkable(bool walkable);
		int GetWalkableCellCount() const;

		const Vector3& GetOrigin() const;
		void SetOrigin(const Vector3& origin);

		float GetCellSize() const;
		void SetCellSize(float size);

		int GetWidth() const;
		int GetHeight() const;

	private:
		void DrawGridPreview();
		void HandleGridCellClick(int x, int z);

	public:
		void OnGui() override;
		void Save(json& j) const override;
		void Load(const json& j) override;
	};
}