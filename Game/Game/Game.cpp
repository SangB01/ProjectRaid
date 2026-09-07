#include "Game.h"
#include <Level/MainLevel.h>
#include <Level/RaidLevel.h>
#include <Level/Menu.h>
#include <Level/ResultLevel.h>
#include <Util/Util.h>

Game::Game()
{
    Util::SetRandomSeed();

    levelList.resize(4);
    levelList[static_cast<int>(State::MainMenu)] = std::make_shared<MainLevel>();

    // 시작 상태 설정
    state = State::MainMenu;

    // 게임 시작 시 활성화할 레벨 설정
    mainLevel = levelList[(int)state];
}

void Game::StartGame()
{
    QueueLevel(State::GamePlay, std::make_shared<RaidLevel>([this](const BattleResult& result) { ShowBattleResult(result); }));
    levelList[static_cast<int>(State::Result)].reset();
    levelList[static_cast<int>(State::Menu)].reset();
}

void Game::ReturnToMainMenu()
{
    QueueLevel(State::MainMenu, std::make_shared<MainLevel>());
    levelList[static_cast<int>(State::GamePlay)].reset();
    levelList[static_cast<int>(State::Menu)].reset();
    levelList[static_cast<int>(State::Result)].reset();
}

void Game::ShowBattleResult(const BattleResult& result)
{
    if (state != State::GamePlay)
    {
        return;
    }
    QueueLevel(State::Result, std::make_shared<ResultLevel>(result, [this](ResultAction action) {
        switch (action)
        {
        case ResultAction::Restart:
            StartGame();
            break;
        case ResultAction::MainMenu:
            ReturnToMainMenu();
            break;
        case ResultAction::Exit:
            Quit();
            break;
        }
    }));
    levelList[static_cast<int>(State::GamePlay)].reset();
    levelList[static_cast<int>(State::Menu)].reset();
}

void Game::QueueLevel(State newState, const std::shared_ptr<Level>& level)
{
    state = newState;
    levelList[static_cast<int>(state)] = level;
    nextLevel = level;
}

void Game::ToggleMenu()
{
    // 게임 → Pause 메뉴
    if (state == State::GamePlay)
    {
        QueueLevel(State::Menu, std::make_shared<Menu>());
    }

    // Pause 메뉴 → 게임
    else if (state == State::Menu)
    {
        QueueLevel(State::GamePlay, levelList[static_cast<int>(State::GamePlay)]);
    }

}
