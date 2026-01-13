#pragma once

#include <vector>
#include <cstdint>

#include "Framework/Object/Ptr.h"

namespace engine
{
    class Collider;
    class Rigidbody;
    class GameObject;

    // ═══════════════════════════════════════════════════════════════
    // 접촉점 정보
    // ═══════════════════════════════════════════════════════════════

    struct ContactPoint
    {
        Vector3 point;          // 접촉 위치 (월드 좌표)
        Vector3 normal;         // 접촉 표면 노말 (A → B 방향)
        float separation;       // 침투 깊이 (음수면 겹침)
        float impulse;          // 충격량
    };

    // ═══════════════════════════════════════════════════════════════
    // 충돌 정보 (스크립트 콜백에 전달)
    // ═══════════════════════════════════════════════════════════════

    struct CollisionInfo
    {
        Ptr<Collider> collider;             // 충돌한 상대 콜라이더
        Ptr<Rigidbody> rigidbody;           // 상대 리지드바디 (있으면)
        Ptr<GameObject> gameObject;         // 상대 게임오브젝트
        std::vector<ContactPoint> contacts; // 접촉점들
        Vector3 relativeVelocity;           // 상대 속도
        float totalImpulse = 0.0f;          // 총 충격량
    };

    // ═══════════════════════════════════════════════════════════════
    // 충돌 이벤트 타입
    // ═══════════════════════════════════════════════════════════════

    enum class CollisionEventType
    {
        Enter,  // 충돌 시작
        Stay,   // 충돌 유지
        Exit    // 충돌 종료
    };

    enum class TriggerEventType
    {
        Enter,  // 트리거 진입
        Stay,   // 트리거 내부 (PhysX 미지원, 직접 추적)
        Exit    // 트리거 퇴장
    };

    // ═══════════════════════════════════════════════════════════════
    // 충돌 우선순위 (전투 시스템용)
    // 숫자가 높을수록 우선 처리
    // ═══════════════════════════════════════════════════════════════

    enum class CollisionPriority : int32_t
    {
        Lowest = -100,
        
        Default = 0,
        
        // 환경/아이템
        Pickup = 10,
        Hazard = 30,
        
        // 전투
        Attack = 50,
        Dodge = 80,
        Block = 90,
        Parry = 100,
        
        Highest = 1000
    };

    // ═══════════════════════════════════════════════════════════════
    // 내부 이벤트 구조체 (CollisionSystem용)
    // ═══════════════════════════════════════════════════════════════

    struct CollisionEvent
    {
        CollisionEventType type;
        Ptr<Collider> colliderA;
        Ptr<Collider> colliderB;
        std::vector<ContactPoint> contacts;
        
        // 우선순위 시스템용
        CollisionPriority priority = CollisionPriority::Default;
        uint64_t sourceId = 0;      // 공격 인스턴스 ID (같은 공격 그룹화)
        bool consumed = false;      // 처리 완료 플래그
    };

    struct TriggerEvent
    {
        TriggerEventType type;
        Ptr<Collider> trigger;      // 트리거 콜라이더
        Ptr<Collider> other;        // 진입한 콜라이더
    };

    // ═══════════════════════════════════════════════════════════════
    // Raycast 결과
    // ═══════════════════════════════════════════════════════════════

    struct RaycastHit
    {
        bool hasHit = false;
        
        Vector3 point;              // 충돌 지점
        Vector3 normal;             // 충돌 표면 노말
        float distance = 0.0f;      // Ray 시작점으로부터 거리
        
        Ptr<Collider> collider;
        Ptr<Rigidbody> rigidbody;
        Ptr<GameObject> gameObject;
    };
}
