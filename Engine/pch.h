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


// d3d
#include <d3d11.h>
#include <directxtk/SimpleMath.h>
#include <dxgi1_5.h>
#include <DirectXCollision.h>

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
}

#include "Common/Utility/Singleton.h"
#include "Common/Utility/StringUtility.h"
#include "Common/Debug/Debug.h"

#include "Core/Graphics/Device/GraphicsDevice.h"
#include "Core/System/MyTime.h"
#include "Core/System/Input.h"

#include "Framework/Object/Ptr.h"