#pragma once

#include <type_traits>
#include <random>

namespace engine
{
    namespace
    {
        inline std::mt19937& GetRandomGenerator()
        {
            static thread_local std::mt19937 gen{ std::random_device{}() };
            return gen;
        }
    }

    inline constexpr float EPSILON = 0.00001f;

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

    class Random
    {
    public:
        // 정수 타입 랜덤 (int 전용, 하위 호환성)
        static int Int(int min, int max);
        
        // 정수 타입 랜덤 (템플릿, 모든 정수 타입 지원)
        template<typename T>
        static T Int(T min, T max)
        {
            static_assert(std::is_integral_v<T>, "Random::Int requires an integral type");
            return std::uniform_int_distribution<T>(min, max)(GetRandomGenerator());
        }

        static float Float(float min, float max);

        // 0.0f ~ 1.0f -> 0% ~ 100%
        static bool Chance(float probability);

        static Vector3 InsideUnitCircle();

        static Vector3 OnUnitSphere();

        static Vector3 InsideUnitSphere();

        static Vector2 Direction(float minDegree = 0.0f, float maxDegree = 360.0f);
    };
}
