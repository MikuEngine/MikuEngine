# PhysX를 D3D 게임 엔진에 통합하기 - 완벽 가이드

> 초보 프로그래머를 위한 PhysX SDK 설치부터 프로젝트 통합까지

**작성 기준**: Windows 11, Visual Studio 2022, PhysX 5.6.1

---

## 목차
1. [PhysX란?](#physx란)
2. [준비 사항](#준비-사항)
3. [PhysX SDK 다운로드](#1단계-physx-sdk-다운로드)
4. [CUDA 설치 (선택)](#2단계-cuda-설치-선택사항)
5. [PhysX 빌드하기](#3단계-physx-빌드하기)
6. [프로젝트에 PhysX 통합](#4단계-프로젝트에-physx-통합)
7. [기본 사용 예제](#5단계-기본-사용-예제)
8. [문제 해결](#문제-해결)

---

## PhysX란?

**NVIDIA PhysX**는 실시간 물리 시뮬레이션 엔진입니다.

### 주요 기능
- ✅ **강체 물리**: 충돌, 중력, 마찰
- ✅ **관절(Joints)**: 경첩, 스프링 등
- ✅ **캐릭터 컨트롤러**: 게임 캐릭터 이동
- ✅ **트리거**: 영역 감지
- ✅ **레이캐스트**: 광선 충돌 검사

### 사용 게임
- Borderlands 시리즈
- Warframe
- Rocket League
- 많은 언리얼 엔진 게임

---

## 준비 사항

### 필수 소프트웨어

| 항목 | 버전 | 용도 |
|------|------|------|
| **Visual Studio** | 2019/2022 | C++ 컴파일 |
| **CMake** | 3.18+ | 프로젝트 생성 |
| **Git** | 최신 | SDK 다운로드 |
| **Python** | 3.7+ | 빌드 스크립트 |

### 선택 사항

| 항목 | 버전 | 용도 |
|------|------|------|
| **CUDA Toolkit** | 12.x | GPU 가속 (대부분 불필요) |

---

## 1단계: PhysX SDK 다운로드

### 방법 1: GitHub에서 다운로드 (권장)

#### 1-1. GitHub 저장소 접속
1. 웹 브라우저 열기
2. 주소창에 입력:
```
https://github.com/NVIDIA-Omniverse/PhysX
```

#### 1-2. 릴리즈 버전 다운로드
1. 페이지에서 **"Releases"** 클릭 (오른쪽)
2. 최신 버전 찾기 (예: **107.3-omni-and-physx-5.6.1**)
3. **"Source code (zip)"** 클릭하여 다운로드

#### 1-3. 압축 풀기
1. 다운로드한 ZIP 파일을 찾기 (보통 `다운로드` 폴더)
2. 파일 우클릭 → **"압축 풀기..."**
3. 압축 풀 위치 선택:
```
C:\Dev\PhysX-107.3-omni-and-physx-5.6.1
```
4. **확인** 클릭

> 💡 **팁**: `C:\Dev` 폴더가 없으면 먼저 만들어주세요.

---

## 2단계: CUDA 설치 (선택사항)

### CUDA가 필요한 경우
- ❌ 일반 게임 엔진: **불필요**
- ✅ GPU 가속 물리 시뮬레이션: 필요
- ✅ 수천 개의 물체 시뮬레이션: 필요

> ⚠️ **대부분의 게임 엔진은 CUDA 없이도 PhysX를 사용할 수 있습니다.**

### CUDA 설치 방법 (필요한 경우만)

#### 2-1. CUDA Toolkit 다운로드
1. 브라우저 열기
2. 주소 입력:
```
https://developer.nvidia.com/cuda-downloads
```
3. 옵션 선택:
   - Operating System: **Windows**
   - Architecture: **x86_64**
   - Version: **11** (또는 10)
   - Installer Type: **exe (local)**
4. **Download** 클릭

#### 2-2. 설치
1. 다운로드한 파일 실행
2. **"동의함"** 체크 → **다음**
3. 설치 유형: **"사용자 지정"** 선택
4. 필수 구성 요소만 선택:
   - ✅ CUDA Toolkit
   - ❌ Visual Studio Integration (선택 해제)
   - ❌ Nsight (선택 해제)
5. **다음** → 설치 완료 대기 (10-20분)

---

## 3단계: PhysX 빌드하기

### 3-1. Visual Studio 솔루션 생성

#### 폴더 찾기
1. **Windows 키 + E** (탐색기 열기)
2. 주소창에 입력:
```
C:\Dev\PhysX-107.3-omni-and-physx-5.6.1\physx
```

#### 프로젝트 생성 스크립트 실행
1. `generate_projects.bat` 파일 찾기
2. **더블클릭**하여 실행
3. 검은 창이 나타나고 자동으로 닫힘 (정상)

#### 생성된 솔루션 확인
1. 같은 폴더에서 `compiler\vc17win64` 폴더 열기
2. `PhysXSDK.sln` 파일 확인

---

### 3-2. 디버그 버전 빌드

#### Visual Studio 열기
1. `PhysXSDK.sln` 파일 **더블클릭**
2. Visual Studio가 자동으로 열림

#### 빌드 설정
1. 상단 툴바에서 구성 확인:
   - **Configuration**: `Debug` (기본값)
   - **Platform**: `x64`

#### 필요한 프로젝트만 빌드

> ⚠️ **중요**: 전체 빌드는 오류가 발생할 수 있습니다. 필요한 것만 빌드하세요.

**솔루션 탐색기**에서 아래 프로젝트를 **하나씩** 빌드:

1. **PhysXFoundation** 프로젝트 찾기
   - 우클릭 → **"빌드"** 클릭
   - 하단 출력 창에서 완료 확인 (약 10초)

2. **PhysXCommon** 빌드
   - 우클릭 → **"빌드"**

3. **PhysX** 빌드
   - 우클릭 → **"빌드"** (약 30초)

4. **PhysXCooking** 빌드

5. **PhysXExtensions** 빌드 (약 1-2분)

6. **PhysXCharacterKinematic** 빌드

7. **PhysXPvdSDK** 빌드

✅ **빌드 순서를 지켜야 합니다!**

#### 빌드 완료 확인
- 하단 출력 창에서 확인:
```
========== 빌드: 성공 1개, 실패 0개, 최신 0개, 생략 0개 ==========
```

---

### 3-3. 릴리즈 버전 빌드

#### 구성 변경
1. 상단 툴바에서 **Configuration 드롭다운** 클릭
2. **Release** 선택

#### 동일한 순서로 빌드
- 위의 **3-2단계**와 동일한 순서로 7개 프로젝트 빌드

---

### 3-4. 빌드된 파일 확인

#### 파일 위치
1. 탐색기에서 아래 경로 열기:
```
C:\Dev\PhysX-107.3-omni-and-physx-5.6.1\physx\bin\win.x86_64.vc143.md
```

2. 폴더 구조:
```
win.x86_64.vc143.md/
├── debug/
│   ├── PhysX_64.lib
│   ├── PhysX_64.dll
│   └── ... (기타 파일들)
└── release/
    ├── PhysX_64.lib
    ├── PhysX_64.dll
    └── ... (기타 파일들)
```

---

## 4단계: 프로젝트에 PhysX 통합

### 4-1. Vendor 폴더 구조 만들기

#### 프로젝트 폴더 열기
1. 탐색기로 본인의 게임 엔진 프로젝트 폴더 열기
   - 예: `C:\Users\User\Documents\GitHub\MikuEngine`

#### PhysX 폴더 만들기
1. 프로젝트 폴더에서 `Vendor` 폴더 열기 (없으면 만들기)
2. `Vendor` 안에 `PhysX` 폴더 만들기
3. `PhysX` 안에 아래 폴더들 만들기:
```
PhysX/
├── include/
├── lib/
│   ├── debug/
│   └── release/
└── bin/
    ├── debug/
    └── release/
```

---

### 4-2. 헤더 파일 복사

#### 원본 폴더 열기
1. 탐색기 새 창 열기
2. 경로 입력:
```
C:\Dev\PhysX-107.3-omni-and-physx-5.6.1\physx\include
```

#### 복사
1. `include` 폴더 안의 **모든 폴더** 선택:
   - `PxConfig.h`
   - `foundation` 폴더
   - `geometry` 폴더
   - `characterkinematic` 폴더
   - 기타 모든 파일/폴더
2. **Ctrl + C** (복사)

#### 붙여넣기
1. 프로젝트의 `Vendor\PhysX\include` 폴더 열기
2. **Ctrl + V** (붙여넣기)

---

### 4-3. 라이브러리 파일 복사

#### Debug 라이브러리 복사

**원본 폴더 열기**:
```
C:\Dev\PhysX-107.3-omni-and-physx-5.6.1\physx\bin\win.x86_64.vc143.md\debug
```

**복사할 LIB 파일 (7개)** - Ctrl 키 누른 채로 선택:
1. `PhysX_64.lib`
2. `PhysXCommon_64.lib`
3. `PhysXCooking_64.lib`
4. `PhysXFoundation_64.lib`
5. `PhysXExtensions_static_64.lib`
6. `PhysXCharacterKinematic_static_64.lib`
7. `PhysXPvdSDK_static_64.lib`

**복사 → 붙여넣기**:
```
[본인 프로젝트]\Vendor\PhysX\lib\debug\
```

**복사할 DLL 파일 (4개)**:
1. `PhysX_64.dll`
2. `PhysXCommon_64.dll`
3. `PhysXCooking_64.dll`
4. `PhysXFoundation_64.dll`

**복사 → 붙여넣기**:
```
[본인 프로젝트]\Vendor\PhysX\bin\debug\
```

#### Release 라이브러리 복사

**원본 폴더**를 `release`로 변경하고 동일한 파일들을 복사:
```
원본: C:\Dev\PhysX-107.3-omni-and-physx-5.6.1\physx\bin\win.x86_64.vc143.md\release
대상 LIB: [본인 프로젝트]\Vendor\PhysX\lib\release\
대상 DLL: [본인 프로젝트]\Vendor\PhysX\bin\release\
```

---

### 4-4. Visual Studio 프로젝트 설정

#### 프로젝트 속성 열기
1. Visual Studio에서 본인 프로젝트 열기
2. **솔루션 탐색기**에서 프로젝트 우클릭
3. **"속성"** 클릭

---

#### 포함 디렉터리 설정

1. 좌측 트리에서:
   - **구성 속성** → **C/C++** → **일반**
2. **추가 포함 디렉터리** 더블클릭
3. 새 줄 추가 버튼 클릭
4. 경로 입력:
```
$(SolutionDir)Vendor\PhysX\include
```
5. **확인**

---

#### 라이브러리 디렉터리 설정

1. 좌측 트리:
   - **구성 속성** → **링커** → **일반**
2. **추가 라이브러리 디렉터리** 더블클릭
3. 상단 드롭다운에서 **"구성"**: `Debug` 선택
4. 새 줄 추가:
```
$(SolutionDir)Vendor\PhysX\lib\debug
```
5. 드롭다운 **"구성"**: `Release` 선택
6. 새 줄 추가:
```
$(SolutionDir)Vendor\PhysX\lib\release
```
7. **확인**

---

#### 링커 입력 설정

1. 좌측 트리:
   - **구성 속성** → **링커** → **입력**
2. **추가 종속성** 더블클릭
3. 맨 위에 아래 내용 추가:
```
PhysX_64.lib;
PhysXCommon_64.lib;
PhysXCooking_64.lib;
PhysXFoundation_64.lib;
PhysXExtensions_static_64.lib;
PhysXCharacterKinematic_static_64.lib;
PhysXPvdSDK_static_64.lib;
```
4. **확인** → **확인**

---

### 4-5. DLL 자동 복사 설정 (선택)

#### 빌드 후 이벤트 설정
1. 프로젝트 속성 열기
2. **구성 속성** → **빌드 이벤트** → **빌드 후 이벤트**
3. **명령줄** 더블클릭
4. 아래 내용 입력:

**Debug 구성**:
```
xcopy /Y /D "$(SolutionDir)Vendor\PhysX\bin\debug\*.dll" "$(OutDir)"
```

**Release 구성**:
```
xcopy /Y /D "$(SolutionDir)Vendor\PhysX\bin\release\*.dll" "$(OutDir)"
```

5. **확인**

---

## 5단계: 기본 사용 예제

### 5-1. PhysicsSystem 클래스 만들기

#### PhysicsSystem.h 생성

프로젝트에 새 헤더 파일 생성:

```cpp
#pragma once
#include <PxPhysicsAPI.h>

using namespace physx;

class PhysicsSystem
{
public:
    PhysicsSystem();
    ~PhysicsSystem();

    bool Initialize();
    void Update(float deltaTime);
    void Shutdown();

    PxPhysics* GetPhysics() { return mPhysics; }
    PxScene* GetScene() { return mScene; }

private:
    PxFoundation* mFoundation = nullptr;
    PxPhysics* mPhysics = nullptr;
    PxDefaultCpuDispatcher* mDispatcher = nullptr;
    PxScene* mScene = nullptr;
    PxMaterial* mMaterial = nullptr;
    PxPvd* mPvd = nullptr;
};
```

---

#### PhysicsSystem.cpp 생성

```cpp
#include "PhysicsSystem.h"
#include <iostream>

// 에러 콜백
class ErrorCallback : public PxErrorCallback
{
public:
    virtual void reportError(PxErrorCode::Enum code, const char* message, 
                           const char* file, int line) override
    {
        std::cout << "PhysX Error: " << message << std::endl;
    }
};

static ErrorCallback gErrorCallback;
static PxDefaultAllocator gAllocator;

PhysicsSystem::PhysicsSystem()
{
}

PhysicsSystem::~PhysicsSystem()
{
    Shutdown();
}

bool PhysicsSystem::Initialize()
{
    // 1. Foundation 생성
    mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, 
                                     gErrorCallback);
    if (!mFoundation)
    {
        std::cout << "PxCreateFoundation failed!" << std::endl;
        return false;
    }

    // 2. Physics 생성
    mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, 
                                PxTolerancesScale());
    if (!mPhysics)
    {
        std::cout << "PxCreatePhysics failed!" << std::endl;
        return false;
    }

    // 3. 기본 Material 생성 (정지 마찰, 동적 마찰, 반발력)
    mMaterial = mPhysics->createMaterial(0.5f, 0.5f, 0.6f);

    // 4. Scene 생성
    PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
    sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f); // 중력
    
    mDispatcher = PxDefaultCpuDispatcherCreate(2); // CPU 스레드 2개
    sceneDesc.cpuDispatcher = mDispatcher;
    sceneDesc.filterShader = PxDefaultSimulationFilterShader;

    mScene = mPhysics->createScene(sceneDesc);
    if (!mScene)
    {
        std::cout << "createScene failed!" << std::endl;
        return false;
    }

    std::cout << "PhysX Initialized Successfully!" << std::endl;
    return true;
}

void PhysicsSystem::Update(float deltaTime)
{
    if (mScene)
    {
        // 물리 시뮬레이션 실행
        mScene->simulate(deltaTime);
        mScene->fetchResults(true);
    }
}

void PhysicsSystem::Shutdown()
{
    if (mScene) mScene->release();
    if (mDispatcher) mDispatcher->release();
    if (mMaterial) mMaterial->release();
    if (mPhysics) mPhysics->release();
    if (mPvd) mPvd->release();
    if (mFoundation) mFoundation->release();

    std::cout << "PhysX Shutdown" << std::endl;
}
```

---

### 5-2. Main.cpp에서 사용하기

```cpp
#include "PhysicsSystem.h"
#include <iostream>
#include <chrono>
#include <thread>

int main()
{
    // PhysX 초기화
    PhysicsSystem physics;
    if (!physics.Initialize())
    {
        return -1;
    }

    // 바닥 생성 (Static Actor)
    PxPhysics* physicsSDK = physics.GetPhysics();
    PxScene* scene = physics.GetScene();
    
    PxMaterial* material = physicsSDK->createMaterial(0.5f, 0.5f, 0.6f);
    
    // 바닥 (Static)
    PxRigidStatic* ground = PxCreatePlane(*physicsSDK, 
                                          PxPlane(0, 1, 0, 0), 
                                          *material);
    scene->addActor(*ground);

    // 박스 생성 (Dynamic Actor) - 10미터 위에서 떨어뜨리기
    PxShape* shape = physicsSDK->createShape(
        PxBoxGeometry(0.5f, 0.5f, 0.5f), 
        *material
    );
    
    PxTransform transform(PxVec3(0.0f, 10.0f, 0.0f));
    PxRigidDynamic* box = physicsSDK->createRigidDynamic(transform);
    box->attachShape(*shape);
    PxRigidBodyExt::updateMassAndInertia(*box, 10.0f); // 질량 10kg
    scene->addActor(*box);
    
    shape->release();

    // 시뮬레이션 루프
    std::cout << "Simulating box falling..." << std::endl;
    
    for (int i = 0; i < 300; i++) // 5초 시뮬레이션 (60fps)
    {
        physics.Update(1.0f / 60.0f);

        // 박스 위치 출력
        PxTransform boxTransform = box->getGlobalPose();
        PxVec3 position = boxTransform.p;
        
        if (i % 60 == 0) // 1초마다 출력
        {
            std::cout << "Time: " << (i / 60.0f) << "s, "
                      << "Y Position: " << position.y << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    // 정리
    box->release();
    ground->release();
    material->release();
    physics.Shutdown();

    return 0;
}
```

---

### 5-3. 빌드 및 실행

1. **Ctrl + Shift + B** (빌드)
2. **F5** (실행)

**예상 출력**:
```
PhysX Initialized Successfully!
Simulating box falling...
Time: 0s, Y Position: 10
Time: 1s, Y Position: 5.095
Time: 2s, Y Position: 0.38
Time: 3s, Y Position: 0
Time: 4s, Y Position: 0
PhysX Shutdown
```

---

## 문제 해결

### 1. LNK2038 에러 (RuntimeLibrary 불일치)

**증상**:
```
error LNK2038: 'RuntimeLibrary'에 대해 불일치가 검색되었습니다. 
'MDd_DynamicDebug' 값이 'MD_DynamicRelease'
```

**원인**: Debug/Release 라이브러리 혼용

**해결**:
1. 프로젝트 속성 열기
2. **구성 속성** → **링커** → **입력** → **추가 라이브러리 디렉터리**
3. Debug 구성일 때 `lib\debug` 사용하는지 확인
4. Release 구성일 때 `lib\release` 사용하는지 확인

---

### 2. 헤더 파일을 찾을 수 없음

**증상**:
```
fatal error C1083: 포함 파일을 열 수 없습니다. 'PxPhysicsAPI.h'
```

**해결**:
1. 프로젝트 속성 → **C/C++** → **일반**
2. **추가 포함 디렉터리**에 PhysX include 경로 확인

---

### 3. DLL을 찾을 수 없음

**증상**:
```
PhysX_64.dll을 찾을 수 없어 코드 실행을 진행할 수 없습니다.
```

**해결 방법 1** - 수동 복사:
1. `Vendor\PhysX\bin\debug` 폴더 열기
2. 모든 DLL 파일 복사
3. 프로젝트의 실행 파일이 있는 폴더에 붙여넣기
   - 보통: `Bin\Debug\` 또는 `x64\Debug\`

**해결 방법 2** - 빌드 후 이벤트 (위 4-5 참조)

---

### 4. CUDA 관련 오류

**증상**:
```
'cuCtxCreate_v4': 함수는 3개의 인수를 사용하지 않습니다.
```

**원인**: GPU 프로젝트 빌드 시도

**해결**: 
- GPU 프로젝트를 빌드하지 마세요
- 필요한 프로젝트만 빌드 (위 3-2 참조)

---

### 5. 빌드는 되는데 실행 시 크래시

**확인 사항**:

1. **초기화 순서**:
   - Foundation → Physics → Scene 순서로 생성

2. **릴리즈 순서**:
   - Actor → Scene → Physics → Foundation 순서로 해제

3. **Null 체크**:
```cpp
if (!mPhysics)
{
    // 에러 처리
    return false;
}
```

---

## 추가 학습 자료

### 공식 문서
- [PhysX Documentation](https://nvidia-omniverse.github.io/PhysX/physx/5.4.0/)
- [PhysX Guide](https://nvidia-omniverse.github.io/PhysX/physx/5.4.0/docs/Guide.html)

### 튜토리얼
- GitHub의 `PhysX/snippets` 폴더에 많은 예제 코드
- 언리얼 엔진 소스코드의 PhysX 통합 참고

---

## 체크리스트

프로젝트 통합 완료 확인:

- [ ] PhysX SDK 다운로드 및 압축 해제
- [ ] Visual Studio에서 PhysX 빌드 (Debug + Release)
- [ ] 헤더 파일 복사 완료
- [ ] 라이브러리 파일 복사 완료 (lib + dll)
- [ ] Visual Studio 프로젝트 설정 완료
  - [ ] 포함 디렉터리
  - [ ] 라이브러리 디렉터리
  - [ ] 링커 입력
- [ ] 기본 예제 코드 빌드 성공
- [ ] 실행 테스트 성공

---

## 마치며

이제 PhysX를 프로젝트에서 사용할 준비가 완료되었습니다!

**다음 단계**:
1. 게임 오브젝트와 PhysX Actor 연결
2. 충돌 콜백 구현
3. 레이캐스트로 월드 쿼리
4. 캐릭터 컨트롤러 구현

행운을 빕니다! 🚀
