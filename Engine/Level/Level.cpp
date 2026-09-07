#include "Level.h"

namespace Craft
{
Level::Level() {}
Level::~Level() {}
void Level::OnInitialized()
{
    // 초기화 됐다고 설정.
    hasInitialized = true;
}
void Level::BeginPlay()
{
    for (std::shared_ptr<Actor>& actor : actorList)
    {
        if (actor->HasBeganPlay())
        {
            continue;
        }
        actor->BeginPlay();
    }
}
void Level::Tick(float deltaTime)
{
    for (const std::shared_ptr<Actor>& actor : actorList)
    {
        if (!actor->IsActive())
        {
            continue;
        }

        // Tick 호출
        actor->Tick(deltaTime);
    }
}
void Level::Draw()
{
    for (std::shared_ptr<Actor>& actor : actorList)
    {
        // 검증
        if (!actor->IsActive())
        {
            continue;
        }

        // Draw 호출
        actor->Draw();
    }
}
void Level::ProcessAddAndDestroyActors()
{
    // 액터 제거 처리
    for (auto iterator = actorList.begin(); iterator != actorList.end();)
    {
        // 제거 요청된 액터인지 확인
        auto actor = *iterator;
        if (actor->HasExpired())
        {
            iterator = actorList.erase(iterator);
            continue;
        }
        // 다음 순번을 처리하기 위해 이터레이터 증가 처리
        ++iterator;
    }

    // 추가 처리
    // 추가 요청된 목록이 없으면 종료
    if (addRequestedActorList.empty())
    {
        return;
    }
    for (const auto& actor : addRequestedActorList)
    {
        actorList.emplace_back(actor);
    }

    // 추가 처리된 목록 정리
    addRequestedActorList.clear();
}
void Level::SavePreviousActorStates()
{
    // 액터 순회하면서 이전 상태 저장 처리
    for (const auto& actor : actorList)
    {
        // 액터가 활성화 되지 않았으면 무시
        if (!actor->IsActive())
        {
            continue;
        }

        actor->SavePrevioussState();
    }
}
}
