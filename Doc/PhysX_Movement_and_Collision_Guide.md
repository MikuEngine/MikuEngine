# PhysX 기반 캐릭터 이동과 충돌 처리 가이드

## 개요

이 문서는 PhysX 물리 엔진을 사용한 게임에서 캐릭터 이동 시 발생할 수 있는 충돌 문제와 해결 방법을 설명합니다.

---

## 1. 문제: ForceMode와 Mass의 불균형

### 1.1 ForceMode 종류

PhysX에서 힘을 적용할 때 사용하는 ForceMode는 4가지가 있습니다:

| ForceMode | 공식 | 특징 |
|-----------|------|------|
| **Force** | `a = F / m` | 질량에 반비례하는 가속도 |
| **Acceleration** | `a = F` | **질량 무시**, 직접 가속도 적용 |
| **Impulse** | `Δv = I / m` | 질량에 반비례하는 속도 변화 |
| **VelocityChange** | `Δv = I` | 질량 무시, 직접 속도 변화 |

### 1.2 문제 발생 원인

게임에서 캐릭터 이동 시 `ForceMode::Acceleration`을 사용하면:

```
이동 가속도 = 입력 * 가속도계수  (질량 무관)
```

반면, PhysX의 충돌 응답(임펄스)은 질량에 반비례합니다:

```
밀리는 속도 = 충돌 임펄스 / 질량
```

### 1.3 결과

| Mass | 이동 가속도 | 충돌로 밀림 | 결과 |
|------|------------|------------|------|
| 1 | 80 | 임펄스/1 = 큼 | 벽에 막힘 ✓ |
| 500 | 80 (동일) | 임펄스/500 = 매우 작음 | 벽 통과 ✗ |

**Mass가 커질수록 충돌 응답이 약해지고, 이동 가속도는 그대로이므로 벽을 뚫게 됩니다.**

---

## 2. 해결 방안

### 2.1 방안 비교

| 방안 | 설명 | 장점 | 단점 |
|------|------|------|------|
| **Mass 조정** | Mass를 1~10으로 유지 | 간단 | 게임 디자인 제약 |
| **ForceMode::Force** | F=ma 사용 | PhysX와 일관성 | 무거운 캐릭터가 느려짐 |
| **충돌 방향 체크** | 충돌 노말 방향 힘 제외 | Mass 무관, 현실적 | 구현 복잡도 |
| **Kinematic 활용** | 움직이면 안 되는 물체 | PhysX 자동 처리 | 제한적 사용 |

### 2.2 권장: 충돌 방향 힘 제외

충돌 중인 표면의 노말(법선) 방향으로 힘을 가하지 않으면, Mass와 무관하게 벽을 통과할 수 없습니다.

```cpp
Vector3 RemoveBlockedDirection(Vector3 force, Vector3 collisionNormal)
{
    float dot = force.Dot(collisionNormal);
    
    if (dot < 0.0f)  // 벽 쪽으로 가려는 힘
    {
        // 해당 방향 성분 제거
        force -= collisionNormal * dot;
    }
    
    return force;
}
```

#### 시각적 설명

```
벽 노말: →  (벽의 바깥쪽 방향)

이동 힘: ↙ (벽 쪽으로 가려는 힘)
         ↓
분해:    ← (노말 반대 방향 성분) + ↓ (벽과 평행한 성분)
         
결과:    ↓ (벽과 평행한 성분만 적용 → 벽을 따라 미끄러짐)
```

---

## 3. Rigidbody 타입 활용

### 3.1 타입별 특성

| 타입 | 물리 시뮬레이션 | 다른 물체를 밀음 | 다른 물체에 밀림 |
|------|----------------|-----------------|-----------------|
| **Dynamic** | O | O | O |
| **Kinematic** | X (스크립트 제어) | O | X |
| **Static** | X (고정) | - | - |

### 3.2 Kinematic의 활용

Kinematic은 "플레이어가 조작하는 물체"뿐만 아니라 **"물리적으로 밀리면 안 되는 물체"**에도 사용할 수 있습니다.

#### 예시: 정지한 몬스터

```
이동 몬스터 (Dynamic) → 정지 몬스터 (Kinematic)
                        ↳ 이동 몬스터만 밀림
                        ↳ 정지 몬스터는 그대로
```

---

## 4. 게임 시나리오별 설정

### 4.1 탑다운 슈터 예시

| 오브젝트 | 타입 | Mass | 비고 |
|----------|------|------|------|
| 플레이어 | Dynamic | 1 | 가벼움 (밀리기 쉬움) |
| 이동 몬스터 | Dynamic | 5~10 | 플레이어보다 무거움 |
| 정지 몬스터 | Kinematic | - | 절대 안 밀림 |
| 지형/벽 | Static | - | 절대 안 밀림 |

### 4.2 충돌 규칙

1. **플레이어 → 몬스터**: 플레이어가 밀기 어려움 (mass 차이)
2. **몬스터 → 플레이어**: 플레이어가 밀림 (mass 차이)
3. **이동 몬스터 → 정지 몬스터**: 이동 몬스터만 밀림 (Kinematic)
4. **모두 → 지형/벽**: 막힘 (충돌 방향 체크)

---

## 5. SetLinearVelocity vs AddForce

### 5.1 문제: SetLinearVelocity 남용

```cpp
// 매 프레임 속도를 직접 설정
rigidbody->SetLinearVelocity(direction * speed);
```

이 방식은 PhysX의 충돌 응답을 **완전히 무시**합니다. 충돌로 밀려나도 다음 프레임에 즉시 덮어씁니다.

### 5.2 해결: AddForce 사용

```cpp
// 목표 속도와 현재 속도의 차이만큼 힘 적용
Vector3 velDiff = targetVel - currentVel;
rigidbody->AddForce(velDiff * acceleration, ForceMode::Acceleration);
```

AddForce는 힘을 **누적**하므로 PhysX의 충돌 응답과 함께 작동합니다.

### 5.3 비교

| 방식 | 충돌 응답 | 물리적 현실감 | 제어 용이성 |
|------|----------|--------------|------------|
| SetLinearVelocity | 무시됨 | 낮음 | 높음 |
| AddForce | 보존됨 | 높음 | 중간 |

---

## 6. 총알/프로젝타일 처리

### 6.1 Trigger Collider

총알은 일반적으로 **Dynamic + Trigger**로 설정합니다:

- 물리적으로 통과 (다른 물체를 밀지 않음)
- 이벤트만 발생 (OnTriggerEnter)

### 6.2 Trigger와 Rigidbody 타입

Trigger는 상대방의 Rigidbody 타입과 무관하게 이벤트가 발생합니다:

| 총알 (Trigger) | 상대 타입 | 이벤트 |
|----------------|-----------|--------|
| Dynamic + Trigger | Dynamic | ✓ OnTriggerEnter |
| Dynamic + Trigger | Kinematic | ✓ OnTriggerEnter |
| Dynamic + Trigger | Static | ✓ OnTriggerEnter |

### 6.3 빠른 총알의 터널링

빠른 물체는 한 프레임에 다른 물체를 완전히 통과할 수 있습니다 (터널링).

해결 방법:
1. **CCD (Continuous Collision Detection)** 활성화
2. **Raycast 기반 피격 판정** 사용

---

## 7. 요약

### 핵심 원칙

1. **ForceMode::Acceleration**은 질량을 무시하므로, 충돌 응답과 불균형 발생 가능
2. **충돌 방향 체크**로 Mass와 무관하게 벽 통과 방지
3. **Kinematic**은 "움직이면 안 되는 물체"에 활용
4. **AddForce**를 사용하여 충돌 응답 보존
5. **Trigger**는 Rigidbody 타입과 무관하게 동작

### 트러블슈팅 체크리스트

- [ ] 캐릭터가 벽을 통과하나요? → 충돌 방향 체크 추가 또는 Mass 조정
- [ ] SetLinearVelocity를 매 프레임 호출하나요? → AddForce로 변경
- [ ] Mass가 매우 크나요? → 1~10 범위로 유지
- [ ] 정지 상태의 물체가 밀리나요? → Kinematic으로 전환
- [ ] 총알이 적을 통과하나요? → CCD 활성화 또는 Raycast 사용

---

## 참고 자료

- [NVIDIA PhysX Documentation](https://gameworksdocs.nvidia.com/PhysX/4.1/documentation/physxguide/Index.html)
- [PhysX ForceMode Reference](https://gameworksdocs.nvidia.com/PhysX/4.1/documentation/physxapi/files/structPxForceMode.html)
- [Continuous Collision Detection](https://gameworksdocs.nvidia.com/PhysX/4.1/documentation/physxguide/Manual/RigidBodyCollision.html#continuous-collision-detection)
