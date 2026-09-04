#include "MainLevel.h"
#include <Game/Game.h>
#include <Input/Input.h>
#include <Render/Renderer.h>
#include <Engine/Engine.h>
#include <cassert>

using namespace Craft;

MainLevel::MainLevel()
{
    // 게임 시작
    itemList.emplace_back(std::make_unique<MainMenuItem>(L"Game Start",
                                                         []()
                                                         {
                                                             Game& game = dynamic_cast<Game&>(Engine::Get());

                                                             game.StartGame();
                                                         }));

    // 게임 종료
    itemList.emplace_back(std::make_unique<MainMenuItem>(L"Exit", []() { Engine::Get().Quit(); }));
}

void MainLevel::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    const int length = static_cast<int>(itemList.size());

    // 위
    if (Input::Get().GetKeyDown(VK_UP))
    {
        currentIndex = (currentIndex - 1 + length) % length;
    }

    // 아래
    if (Input::Get().GetKeyDown(VK_DOWN))
    {
        currentIndex = (currentIndex + 1) % length;
    }

    // 선택
    if (Input::Get().GetKeyDown(VK_SPACE))
    {
        assert(currentIndex >= 0 && currentIndex < length && itemList[currentIndex]->onSelected);

        itemList[currentIndex]->onSelected();
    }
}

void MainLevel::Draw()
{
    // 게임 제목
    Renderer::Get().Submit(L"RAId", Vector2(40, 8), Color::White);

    // 메뉴
    const int count = static_cast<int>(itemList.size());

    for (int ix = 0; ix < count; ++ix)
    {
        Color textColor = ix == currentIndex ? selectedColor : unselectedColor;

        std::wstring text;

        if (ix == currentIndex)
        {
            text = L"> " + itemList[ix]->text;
        }
        else
        {
            text = L"  " + itemList[ix]->text;
        }

        Renderer::Get().Submit(text, Vector2(38, 12 + ix), textColor);
    }
}