#include "Menu.h"
#include <Game/Game.h>
#include <Input/Input.h>
#include <Render/Renderer.h>
#include <cassert>
#include <Level/MainLevel.h>

using namespace Craft;
Menu::Menu()
{
    // 메뉴 아이템 생성
    itemList.emplace_back(std::make_unique<MenuItem>(L"Resume Game",
                                                     []()
                                                     {
                                                         // 메뉴 토글 함수 호출
                                                         Game& game = dynamic_cast<Game&>(Engine::Get());
                                                         game.ToggleMenu();
                                                     }));

    itemList.emplace_back(std::make_unique<MenuItem>(L"Quit Game",
                                                     []()
                                                     {
                                                         // 게임 종료 함수 호출
                                                         Engine::Get().ChangeLevel<MainLevel>();
                                                     }));
}

void Menu::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    // 입력처리(위 아래 방향키, 엔터, esc 키)
    if (Input::Get().GetKeyDown(VK_ESCAPE))
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ToggleMenu();

        // 인덱스 초기화
        currentIndex = 0;
    }
    // 배열의 요소 개수
    const int length = static_cast<int>(itemList.size());
    if (Input::Get().GetKeyDown(VK_UP))
    {
        currentIndex = (currentIndex - 1 + length) % length;
    }
    if (Input::Get().GetKeyDown(VK_DOWN))
    {
        // 인덱스 돌리기 (+방향)
        currentIndex = (currentIndex + 1) % length;
    }

    // 엔터 입력 처리 -> 현재 선택된 메뉴의 로직 실행
    if (Input::Get().GetKeyDown(VK_SPACE))
    {
        assert(currentIndex >= 0 && currentIndex < (int)itemList.size() && itemList[currentIndex]->onSelected);
        // 메뉴 아이템에 저장된 로직 실행
        itemList[currentIndex]->onSelected();
    }
}

void Menu::Draw()
{
    // 제목 그리기
    Renderer::Get().Submit((L"RAIdW"), Vector2::Zero);

    // 메뉴 아이템 그리기
    const int count = static_cast<int>(itemList.size());
    for (int ix = 0; ix < count; ++ix)
    {
        // 선택/미선택된 아이템 색상 처리
        Color textColor = (ix == currentIndex) ? selectedColor : unselectedColor;

        // 아이템 그리기
        Renderer::Get().Submit(itemList[ix]->text, Vector2(0, 2 + ix), textColor);
    }
}
