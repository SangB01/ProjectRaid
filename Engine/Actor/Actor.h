#pragma once

#include <Core/Core.h>
#include <Math/Vector2.h>
#include <Math/Color.h>
#include <Core/CraftObject.h>
#include <string>
#include <memory>

namespace Craft
{
class Level;

// 가상 공간에 배치될 모든 액터의 기본 클래스.
class CRAFT_API Actor : public CraftObject
{
    // 매크로 지정할 때 끝에 ; 넣지 않도록 주의
    TYPE_DECLARATIONS(Actor, CraftObject)
  public:
    Actor(const std::wstring& image = L"", const Vector2& position = Vector2::Zero, Color color = Color::White);
    virtual ~Actor();

    // 게임 플레이 이벤트 함수.
    virtual void BeginPlay();
    virtual void Tick(float deltaTime);
    virtual void Draw();

    // 충돌 이벤트 함수
    virtual void OnCollision(const std::shared_ptr<Actor>& other);

    // 액터 제거 함수.
    void Destroy();

    // 게임(엔진) 종료 함수.
    void QuitGame();

    // Getter/Setter.
    inline bool HasBeganPlay() const
    {
        return hasBeganPlay;
    }
    inline bool IsActive() const
    {
        return isActive && !hasExpired;
    }
    inline bool HasExpired() const
    {
        return hasExpired;
    }

    inline std::shared_ptr<Level> GetOwner() const
    {
        return owner.lock();
    }
    inline void SetOwner(std::weak_ptr<Level> newOwner)
    {
        owner = newOwner;
    }

    inline Vector2 GetPosition() const
    {
        return position;
    }
    void SetPosition(const Vector2& newPosition);

    // 이전 위치 반환 함수
    inline Vector2 GetPreviousPosition() const
    {
        return previousPosition;
    }

    // 프레임 종료 후 이전 프레임 위치 저장 함수
    void SavePrevioussState()
    {
        previousPosition = position;
    }

    // 너비 반환 함수
    inline int GetWidth() const
    {
        return width;
    }

    // 액터의 이미지 설정 함수
    inline void ChangeImage(const std::wstring& newImage)
    {
        // 이미지 길이 설정
        width = static_cast<int>(newImage.length());

        // 새로운 글자 값 생성
        image = newImage;
    }

  protected:
    // BeginPlay 이벤트 처리 여부 플래그.
    bool hasBeganPlay = false;

    // 액터 활성화 여부 플래그.
    bool isActive = true;

    // 삭제 요청 여부 플래그.
    bool hasExpired = false;

    std::weak_ptr<Level> owner;

    // 화면에 그릴 글자
    std::wstring image;

    // 글자 색상
    Color color = Color::White;

    // 글자 길이
    int width = 0;

    // 렌더링 순서
    int sortingOrder = 0;

    // 위치
    Vector2 position;

    // 이전 프레임 위치
    Vector2 previousPosition;
};
} // namespace Craft