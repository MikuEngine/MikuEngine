Preload.json은 리소스를 미리 로드해서 캐싱하기 위한 파일입니다

{
  "Global": [
    { "Type": "StaticMesh", "Path": "Resource/Model/Cube.fbx" },
    { "Type": "StaticMesh", "Path": "Resource/Model/Sphere.fbx" },
    { "Type": "StaticMesh", "Path": "Resource/Model/Quad.fbx" }
  ],
  "Scenes": {
    "StaticMeshTest": [
        { "Type": "StaticMesh", "Path": "Resource/Model/Girl.fbx" },
        { "Type": "StaticMesh", "Path": "Resource/Model/HighPolySphere.fbx" },
        { "Type": "StaticMesh", "Path": "Resource/Model/Floor.fbx" },
        { "Type": "StaticMesh", "Path": "Resource/Model/char.fbx" }
    ]
  }
}

"Global"의 목록은 게임 시작부터 종료될때까지 캐싱됩니다.
"Scenes"의 목록은 실제 Scene 이름과 일치시켜야 합니다.
"Scenes"의 "Scene"내의 목록은 해당 씬의 시작부터 종료될때까지 캐싱됩니다.

"Type"은 리소스 타입입니다.
현재 리소스 타입에는

"Texture"
"StaticMesh"
"SkeletalMesh"
"Animation"
"SpriteData"
"SpriteAnimation"

이 있습니다.

리소스 타입을 추가하고 싶다면 PreloadManager의 LoadAsset 멤버함수에 추가하면 됩니다.

"Path"는 리소스 경로입니다.
"Bin"폴더가 작업 디렉토리이자 실행 디렉토리라서 Bin폴더를 루트로 보고 작성하시면 됩니다.
웬만하면 Resource 폴더 내의 경로로 설정하면 됩니다.