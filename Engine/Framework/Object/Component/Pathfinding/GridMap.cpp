#include "EnginePCH.h"
#include "GridMap.h"

#include "Framework/System/SystemManager.h"
#include "Framework/System/PathfindingSystem.h"

namespace engine
{
	GridMap::~GridMap()
	{
		SystemManager::Get().GetPathfindingSystem().Unregister(this);
	}

	void GridMap::Initialize()
	{
		SystemManager::Get().GetPathfindingSystem().Register(this);

		if (m_cells.empty())
		{
			ResizeGrid(m_width, m_height);
		}
	}

	void GridMap::WorldToGrid(const Vector3& worldPos, int& outX, int& outZ) const
	{
		Vector3 localPos = worldPos - m_origin;

		// X-Z 평면 사용 (Y는 높이)
		outX = static_cast<int>(std::floor(localPos.x / m_cellSize));
		outZ = static_cast<int>(std::floor(localPos.z / m_cellSize));
	}

	Vector3 GridMap::GridToWorld(int x, int z) const
	{
		// 그리드 좌표를 월드 좌표로 변환 (셀 중심점)
		float worldX = m_origin.x + (x + 0.5f) * m_cellSize;
		float worldZ = m_origin.z + (z + 0.5f) * m_cellSize;

		// Y는 원점의 Y 사용 (2D 평면)
		return Vector3(worldX, m_origin.y, worldZ);
	}

	bool GridMap::IsValid(int x, int z) const
	{
		return x >= 0 && x < m_width && z >= 0 && z < m_height;
	}

	bool GridMap::IsWalkable(int x, int z) const
	{
		if (!IsValid(x, z))
		{
			return false;
		}

		return m_cells[z * m_width + x].walkable;
	}

	void GridMap::SetWalkable(int x, int z, bool walkable)
	{
		if (!IsValid(x, z))
		{
			return;
		}

		m_cells[z * m_width + x].walkable = walkable;
	}

	GridCell& GridMap::GetCell(int x, int z)
	{
		assert(IsValid(x, z) && "Invalid grid coordinates");
		return m_cells[z * m_width + x];
	}

	const GridCell& GridMap::GetCell(int x, int z) const
	{
		assert(IsValid(x, z) && "Invalid grid coordinates");
		return m_cells[z * m_width + x];
	}

	void GridMap::ResizeGrid(int width, int height)
	{
		if (width <= 0 || height <= 0)
			return;

		m_width = width;
		m_height = height;

		// 셀 배열 재할당 (기본값: 모두 walkable)
		m_cells.resize(m_width * m_height);
		for (auto& cell : m_cells)
		{
			cell.walkable = true;
			cell.cost = 1.0f;
		}
	}

	void GridMap::MarkAllWalkable(bool walkable)
	{
		for (auto& cell : m_cells)
		{
			cell.walkable = walkable;
		}
	}

	int GridMap::GetWalkableCellCount() const
	{
		int count = 0;
		for (const auto& cell : m_cells)
		{
			if (cell.walkable)
				count++;
		}
		return count;
	}

	const Vector3& GridMap::GetOrigin() const
	{
		return m_origin;
	}

	void GridMap::SetOrigin(const Vector3& origin)
	{
		m_origin = origin;
	}

	float GridMap::GetCellSize() const
	{
		return m_cellSize;
	}

	void GridMap::SetCellSize(float size)
	{
		if (size <= 0.0f)
		{
			return;
		}

		m_cellSize = size;
	}

	int GridMap::GetWidth() const
	{
		return m_width;
	}

	int GridMap::GetHeight() const
	{
		return m_height;
	}

	void GridMap::DrawGridPreview()
	{
		// 미리보기 크기 설정 (최대 30x30 셀 표시)
		const int maxPreviewSize = 1000;
		const float cellPixelSize = 15.0f;  // 각 셀의 픽셀 크기

		int displayWidth = std::min(maxPreviewSize, m_width);
		int displayHeight = std::min(maxPreviewSize, m_height);

		ImGui::Text("Grid Preview (Click to edit)");

		// 캔버스 영역 계산
		ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		ImVec2 canvasSize(displayWidth * cellPixelSize, displayHeight * cellPixelSize);

		// 캔버스 배경
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
			IM_COL32(50, 50, 50, 255));

		// 셀 그리기
		for (int z = 0; z < displayHeight; z++)
		{
			for (int x = 0; x < displayWidth; x++)
			{
				// Z축 뒤집기: 화면 좌표계에 맞게 (0,0이 왼쪽 위)
				int displayZ = displayHeight - 1 - z;

				ImVec2 cellPos(
					canvasPos.x + x * cellPixelSize,
					canvasPos.y + z * cellPixelSize  // 화면 Y는 아래로 향하므로 z를 그대로 사용
				);

				// 실제 그리드 좌표로 walkable 체크 (displayZ 사용)
				bool walkable = IsWalkable(x, displayZ);
				ImU32 cellColor = walkable
					? IM_COL32(100, 200, 100, 255)  // 녹색 - walkable
					: IM_COL32(200, 100, 100, 255);  // 빨간색 - unwalkable

				// 선택된 셀 강조
				if (m_selectedCellX == x && m_selectedCellZ == displayZ)
				{
					cellColor = IM_COL32(255, 255, 0, 255);  // 노랑 - 선택됨
				}

				// 셀 사각형 그리기
				drawList->AddRectFilled(
					cellPos,
					ImVec2(cellPos.x + cellPixelSize, cellPos.y + cellPixelSize),
					cellColor
				);

				// 셀 경계선
				drawList->AddRect(
					cellPos,
					ImVec2(cellPos.x + cellPixelSize, cellPos.y + cellPixelSize),
					IM_COL32(255, 255, 255, 100)
				);

				// 마우스 호버 및 클릭 처리
				ImVec2 mousePos = ImGui::GetMousePos();
				if (mousePos.x >= cellPos.x && mousePos.x < cellPos.x + cellPixelSize &&
					mousePos.y >= cellPos.y && mousePos.y < cellPos.y + cellPixelSize)
				{
					// 호버 효과
					drawList->AddRect(
						cellPos,
						ImVec2(cellPos.x + cellPixelSize, cellPos.y + cellPixelSize),
						IM_COL32(255, 255, 255, 200),
						0.0f, 0, 2.0f
					);

					// 클릭 처리 (displayZ 사용)
					if (ImGui::IsMouseClicked(0))  // Left Click
					{
						HandleGridCellClick(x, displayZ);
						m_selectedCellX = x;
						m_selectedCellZ = displayZ;
					}
					else if (ImGui::IsMouseClicked(1))  // Right Click
					{
						SetWalkable(x, displayZ, true);
						m_selectedCellX = x;
						m_selectedCellZ = displayZ;
					}
					else if (ImGui::IsMouseClicked(2))  // Middle Click
					{
						SetWalkable(x, displayZ, false);
						m_selectedCellX = x;
						m_selectedCellZ = displayZ;
					}
				}
			}
		}

		// ImGui에 캔버스 영역 등록 (마우스 이벤트 처리용)
		ImGui::Dummy(canvasSize);

		// 그리드가 미리보기보다 크면 스크롤 가능하도록 표시
		if (m_width > maxPreviewSize || m_height > maxPreviewSize)
		{
			ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f),
				"Note: Grid is larger than preview. Only showing %dx%d cells.",
				displayWidth, displayHeight);
		}
	}

	void GridMap::HandleGridCellClick(int x, int z)
	{
		if (!IsValid(x, z))
			return;

		// Walkable 상태 토글
		bool currentWalkable = IsWalkable(x, z);
		SetWalkable(x, z, !currentWalkable);
	}

	void GridMap::OnGui()
	{
		ImGui::Text("Grid Settings");
		ImGui::Separator();

		// Origin
		Vector3 origin = m_origin;
		if (ImGui::DragFloat3("Origin", &origin.x, 0.1f))
		{
			SetOrigin(origin);
		}

		// Cell Size
		float cellSize = m_cellSize;
		if (ImGui::DragFloat("Cell Size", &cellSize, 0.1f, 0.1f, 10.0f))
		{
			SetCellSize(cellSize);
		}

		// Grid Size
		ImGui::Text("Grid Size");
		int width = m_width;
		int height = m_height;
		if (ImGui::DragInt("Width", &width, 1, 1, 1000))
		{
			ResizeGrid(width, m_height);
		}
		if (ImGui::DragInt("Height", &height, 1, 1, 1000))
		{
			ResizeGrid(m_width, height);
		}

		ImGui::Separator();

		// 편집 모드
		ImGui::Checkbox("Edit Grid", &m_editMode);

		if (m_editMode)
		{
			ImGui::Text("Click cells in preview to toggle walkable state");
			ImGui::Text("Left Click: Toggle walkable");
			ImGui::Text("Right Click: Set walkable");
			ImGui::Text("Middle Click: Set unwalkable");

			// 그리드 미리보기
			DrawGridPreview();

			ImGui::Separator();

			// 일괄 편집
			if (ImGui::Button("Mark All Walkable"))
			{
				MarkAllWalkable(true);
			}
			ImGui::SameLine();
			if (ImGui::Button("Mark All Unwalkable"))
			{
				MarkAllWalkable(false);
			}

			// 선택된 셀 정보
			if (m_selectedCellX >= 0 && m_selectedCellZ >= 0)
			{
				ImGui::Text("Selected Cell: (%d, %d)", m_selectedCellX, m_selectedCellZ);
				bool walkable = IsWalkable(m_selectedCellX, m_selectedCellZ);
				if (ImGui::Checkbox("Walkable", &walkable))
				{
					SetWalkable(m_selectedCellX, m_selectedCellZ, walkable);
				}
			}
		}

		ImGui::Separator();

		// 통계
		ImGui::Text("Statistics");
		int walkableCount = GetWalkableCellCount();
		int totalCount = m_width * m_height;
		ImGui::Text("Walkable: %d / %d", walkableCount, totalCount);
		ImGui::Text("Unwalkable: %d / %d", totalCount - walkableCount, totalCount);
	}

	void GridMap::Save(json& j) const
	{
		Object::Save(j);

		j["Origin"] = m_origin;
		j["CellSize"] = m_cellSize;
		j["Width"] = m_width;
		j["Height"] = m_height;

		// 셀 데이터 저장
		json cellsJson = json::array();
		for (const auto& cell : m_cells)
		{
			cellsJson.push_back({
				{"Walkable", cell.walkable},
				{"Cost", cell.cost}
				});
		}
		j["Cells"] = cellsJson;
	}

	void GridMap::Load(const json& j)
	{
		Object::Load(j);

		JsonGet(j, "Origin", m_origin);
		JsonGet(j, "CellSize", m_cellSize);

		int width = 10, height = 10;
		JsonGet(j, "Width", width);
		JsonGet(j, "Height", height);

		ResizeGrid(width, height);

		// 셀 데이터 로드
		if (j.contains("Cells") && j["Cells"].is_array())
		{
			int index = 0;
			for (const auto& cellJson : j["Cells"])
			{
				if (index >= m_cells.size()) break;

				m_cells[index].walkable = cellJson.value("Walkable", true);
				m_cells[index].cost = cellJson.value("Cost", 1.0f);
				index++;
			}
		}
	}
}