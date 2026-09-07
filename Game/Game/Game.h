#pragma once
#include <Engine/Engine.h>
#include "BattleResult.h"
#include <vector>

// 레벨 관리에 사용할 상태 열겨형
enum class State
{
    MainMenu = 0,
    GamePlay = 1,
    Menu = 2,
    Result = 3
};

using namespace Craft;
// 메뉴 레벨 및 게임 레벨을 관리하는 객체
class Game : public Engine
{
    friend class RaidLevelCardTests;
  public:
    Game();
    ~Game() = default;

    void StartGame();
    void ReturnToMainMenu();
    void ShowBattleResult(const BattleResult& result);

    // 메뉴 / 게임 레벨을 전환하는 함수
    void ToggleMenu();

  private:
    void QueueLevel(State newState, const std::shared_ptr<Level>& level);

    // 메뉴 레벨과 게임 레벨을 관리할 배열
    std::vector<std::shared_ptr<Level>> levelList;

    // 현재 활성화된 레벨의 상태를 나타내는 변수
    State state = State::MainMenu;
};
