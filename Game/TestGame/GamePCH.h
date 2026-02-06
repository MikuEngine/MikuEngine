#pragma once

#include <Engine/EnginePCH.h>

#include <Engine/Framework/Object/GameObject/GameObject.h>
#include <Engine/Framework/Object/Component/Transform.h>

namespace game
{
    // csv reader game namespace에서 특수화를 위한 기본 템플릿
    template <typename T>
    bool FromFields(const std::vector<std::string>& fields, T& out)
    {
        return false;
    }
}