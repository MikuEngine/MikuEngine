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

    inline Vector3 GetTranslation(const Matrix& m)
    {
        return Vector3(m._41, m._42, m._43);
    }

    inline Vector3 GetForward(const Matrix& m)
    {
        return Vector3(m._31, m._32, m._33);
    }

    inline Vector3 GetRight(const Matrix& m)
    {
        return Vector3(m._11, m._12, m._13);
    }

    inline Vector3 GetUp(const Matrix& m)
    {
        return Vector3(m._21, m._22, m._23);
    }
}
