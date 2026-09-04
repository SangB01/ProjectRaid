#include "Game.h"
#include <Level/MainLevel.h>
#include <Level/RaidLevel.h>
#include <Level/Menu.h>

Game::Game()
{
    // 두 레벨 생성 및 배열에 추가
    levelList.emplace_back(std::make_shared<MainLevel>());
    levelList.emplace_back(std::make_shared<RaidLevel>());
    levelList.emplace_back(std::make_shared<Menu>());

    // 시작 상태 설정
    state = State::MainMenu;

    // 게임 시작 시 활성화할 레벨 설정
    mainLevel = levelList[(int)state];
}

void Game::StartGame()
{
    state = State::GamePlay;

    levelList[static_cast<int>(State::GamePlay)] = std::make_shared<RaidLevel>();

    mainLevel = levelList[static_cast<int>(State::GamePlay)];
}

void Game::ToggleMenu()
{
    // 게임 → Pause 메뉴
    if (state == State::GamePlay)
    {
        state = State::Menu;
    }

    // Pause 메뉴 → 게임
    else if (state == State::Menu)
    {
        state = State::GamePlay;
    }

    mainLevel = levelList[static_cast<int>(state)];
}
