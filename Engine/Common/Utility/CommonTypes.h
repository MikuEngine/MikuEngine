#pragma once

enum class LifeScope
{
	Global, // 게임 시작 - 종료
	Scene, // 씬 시작 - 종료
	Owning, // 소유자 한명이라도 있음 - 없음

	Count
};

enum class GeometryType
{
	Quad,
	Cube,
	Sphere,
	Plane,
	Cone,

	Count
};