#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <Windows.h>

// common
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <wrl/client.h>
#include <chrono>
#include <format>
#include <cassert>
#include <array>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <filesystem>


// d3d
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <dxgi1_5.h>
#include <DirectXCollision.h>
#include <directXTK/Mouse.h>
#include <directXTK/Keyboard.h>


// ext
#include <imgui.h>
#include <json.hpp>

// PhysX
#include <PxPhysicsAPI.h>

namespace engine
{
    using Vector2 = DirectX::SimpleMath::Vector2;
    using Vector3 = DirectX::SimpleMath::Vector3;
    using Vector4 = DirectX::SimpleMath::Vector4;
    using Matrix = DirectX::SimpleMath::Matrix;
    using Quaternion = DirectX::SimpleMath::Quaternion;
    using Color = DirectX::SimpleMath::Color;

    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    using json = nlohmann::ordered_json;
}

#include "Common/Utility/Singleton.h"
#include "Common/Utility/StringHelper.h"
#include "Common/Utility/JsonHelper.h"
#include "Common/Math/MathUtility.h"
#include "Common/Debug/Debug.h"

#include "Core/System/MyTime.h"
#include "Core/System/Input.h"

#include "Framework/Object/Ptr.h"
#include "Framework/Object/Component/ComponentFactory.h"

namespace engine
{
    using Keys = DirectX::Keyboard::Keys;
    using Buttons = Input::Buttons;
}