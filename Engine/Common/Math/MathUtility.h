#pragma once

namespace engine
{
    constexpr float ToRadian(float degree)
    {
        return degree * (DirectX::XM_PI / 180.0f);
    }

    constexpr float ToDegree(float radian)
    {
        return radian * (180.0f / DirectX::XM_PI);
    }

    // radian
    Vector3 ToEulerAngles(const Quaternion& q);
}
