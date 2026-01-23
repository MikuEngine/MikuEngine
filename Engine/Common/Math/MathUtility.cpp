#include "EnginePCH.h"
#include "MathUtility.h"

#include <random>

namespace engine
{
    constexpr float PI = 3.1415926535f;
    constexpr float TWO_PI = PI * 2.0f;

	namespace
	{
		std::mt19937 gen{ std::random_device{}() };
	}

	int Random::Int(int min, int max)
	{
		return std::uniform_int_distribution<int>(min, max)(gen);
	}

	float Random::Float(float min, float max)
	{
		return std::uniform_real_distribution<float>(min, max)(gen);
	}

	bool Random::Chance(float probability)
	{
		assert((probability >= 0.0f && probability <= 1.0f) && "확률을 0 ~ 1 사이로 해주세요");

		return std::uniform_real_distribution<float>(0.0f, 1.0f)(gen) <= probability;
	}

    // x z
    Vector3 Random::InsideUnitCircle()
    {
        float theta = std::uniform_real_distribution<float>(0.0f, TWO_PI)(gen);

        float u = std::uniform_real_distribution<float>(0.0f, 1.0f)(gen);
        float r = std::sqrt(u);

        float x = r * std::cos(theta);
        float z = r * std::sin(theta);

        return Vector3(x, 0.0f, z); // Y-Up 기준 바닥
    }

    Vector3 Random::OnUnitSphere()
    {
        float z = std::uniform_real_distribution<float>(-1.0f, 1.0f)(gen);
        float theta = std::uniform_real_distribution<float>(0.0f, TWO_PI)(gen);

        float planar_r = std::sqrt(1.0f - z * z);

        float x = planar_r * std::cos(theta);
        float y = planar_r * std::sin(theta);

        return Vector3(x, z, y);
    }

    Vector3 Random::InsideUnitSphere()
    {
        Vector3 v;
        while (true)
        {
            v.x = std::uniform_real_distribution<float>(-1.0f, 1.0f)(gen);
            v.y = std::uniform_real_distribution<float>(-1.0f, 1.0f)(gen);
            v.z = std::uniform_real_distribution<float>(-1.0f, 1.0f)(gen);

            // LengthSquared()는 sqrt를 안 쓰므로 매우 빠름.
            // 반지름 1.0 이내인지 체크
            if (v.LengthSquared() <= 1.0f)
            {
                return v;
            }
        }
    }

	Vector2 Random::Direction(float minDegree, float maxDegree)
	{
		float degree = std::uniform_real_distribution<float>(minDegree, maxDegree)(gen);
		float radian = ToRadian(degree);

		return Vector2(std::cos(radian), std::sin(radian));
	}
}