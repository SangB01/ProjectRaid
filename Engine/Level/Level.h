#pragma once
#include <Actor/Actor.h>
#include <Core/Core.h>
#include <Core/CraftObject.h>
#include <memory>
#include <vector>

namespace Craft
{
class CRAFT_API Level : public CraftObject, public std::enable_shared_from_this<Level>
{
    TYPE_DECLARATIONS(Level, CraftObject)
    friend class Engine;

  public:
    Level();
    virtual ~Level();

    // 초기화 함수
    virtual void OnInitialized();

    // 게임 플레이 이벤트 함수
    virtual void BeginPlay();
    virtual void Tick(float deltaTime);
    virtual void Draw();

    template <typename T, typename... Args, typename = std::enable_if_t<std::is_base_of<Actor, T>::value>>
    std::shared_ptr<T> SpawnActor(Args&&... args)
    {
        // 새로운 액터 생성.
        std::shared_ptr<T> newActor = std::make_shared<T>(std::forward<Args>(args)...);

        // 추가 요청 목록에 포함.
        addRequestedActorList.emplace_back(newActor);

        // 오너십 설정.
        newActor->SetOwner(weak_from_this());

        // 생성한 액터 반환.
        return newActor;
    }

    // 액터 검색 함수(템플릿)
    template <typename T, typename = std::enable_if_t<std::is_base_of<Actor, T>::value>> std::shared_ptr<T> FindActor()
    {
        for (const auto& actor : actorList)
        {
            std::shared_ptr<T> targetActor = std::dynamic_pointer_cast<T>(actor);
            if (targetActor)
            {
                return targetActor;
            }
        }
        return nullptr;
    }

    // Getter
    inline bool HasInitialized() const
    {
        return hasInitialized;
    }

  protected:
    // 이전 프레임에 추가/ 제거 요청된 액터 처리 함수
    void ProcessAddAndDestroyActors();

    // 액터의 이전 상태 처리 함수
    void SavePreviousActorStates();

  protected:
    // 초기화 처리 여부 플래그
    bool hasInitialized = false;

    // 레벨에 배치된 모든 액터
    std::vector<std::shared_ptr<Actor>> actorList;

    std::vector<std::shared_ptr<Actor>> addRequestedActorList;
};
} // namespace Craft
