#include "MainLevel.h"
#include <Game/Game.h>
#include <Input/Input.h>
#include <Render/Renderer.h>
#include <Engine/Engine.h>
#include <array>
#include <cassert>
#include <cmath>

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

    animationTimer = std::fmod(animationTimer + deltaTime, 1000.0f);

    const int length = static_cast<int>(itemList.size());
    const Input& input = Input::Get();
    const Vector2 mousePosition = input.GetMousePosition();
    const int hoveredIndex = FindMenuItemAt(mousePosition);

    if (!hasMousePosition || mousePosition != previousMousePosition)
    {
        if (hoveredIndex >= 0)
        {
            currentIndex = hoveredIndex;
        }

        previousMousePosition = mousePosition;
        hasMousePosition = true;
    }

    // 위
    if (input.GetKeyDown(VK_UP))
    {
        currentIndex = (currentIndex - 1 + length) % length;
    }

    // 아래
    if (input.GetKeyDown(VK_DOWN))
    {
        currentIndex = (currentIndex + 1) % length;
    }

    if (input.GetKeyDown(VK_LBUTTON) && hoveredIndex >= 0)
    {
        currentIndex = hoveredIndex;
    }

    // 선택
    if ((input.GetKeyDown(VK_LBUTTON) && hoveredIndex >= 0) || input.GetKeyDown(VK_SPACE) ||
        input.GetKeyDown(VK_RETURN))
    {
        assert(currentIndex >= 0 && currentIndex < length && itemList[currentIndex]->onSelected);

        itemList[currentIndex]->onSelected();
    }
}

int MainLevel::FindMenuItemAt(const Vector2& position) const
{
    for (int index = 0; index < static_cast<int>(itemList.size()); ++index)
    {
        const int itemY = MenuY + index * MenuSpacing;
        if (position.x >= MenuX && position.x < MenuX + MenuWidth &&
            position.y >= itemY && position.y < itemY + MenuHeight)
        {
            return index;
        }
    }

    return -1;
}

void MainLevel::DrawBox(const Vector2& position, int width, int height, Color color) const
{
    if (width < 2 || height < 2)
    {
        return;
    }

    std::wstring top = L"┌";
    top.append(width - 2, L'─');
    top += L"┐";
    Renderer::Get().Submit(top, position, color, 5);

    for (int y = 1; y < height - 1; ++y)
    {
        Renderer::Get().Submit(L"│", position + Vector2(0, y), color, 5);
        Renderer::Get().Submit(L"│", position + Vector2(width - 1, y), color, 5);
    }

    std::wstring bottom = L"└";
    bottom.append(width - 2, L'─');
    bottom += L"┘";
    Renderer::Get().Submit(bottom, position + Vector2(0, height - 1), color, 5);
}

void MainLevel::DrawCentered(const std::wstring& text, int y, Color color, int sortingOrder) const
{
    Renderer::Get().Submit(text, Vector2((ScreenWidth - static_cast<int>(text.size())) / 2, y), color,
                           sortingOrder);
}

void MainLevel::Draw()
{
    const bool pulse = static_cast<int>(animationTimer * 3.0f) % 2 == 0;
    const Color pulseColor = pulse ? Color::Green : Color::Cyan;

    DrawBox(Vector2(8, 3), 139, 43, Color::Blue);
    Renderer::Get().Submit(L"[ RAID CONTROL SYSTEM // NODE 01 ]", Vector2(13, 4), Color::Cyan, 10);
    Renderer::Get().Submit(L"SECURE LINK", Vector2(132, 4), pulseColor, 10);
    Renderer::Get().Submit(L"+", Vector2(10, 5), pulseColor, 10);
    Renderer::Get().Submit(L"+", Vector2(144, 43), pulseColor, 10);

    static const std::array<std::wstring, 6> titleArt = {
        L"██████╗   █████╗  ██╗         ██╗",
        L"██╔══██╗ ██╔══██╗ ██║         ██║",
        L"██████╔╝ ███████║ ██║    ███████║",
        L"██╔══██╗ ██╔══██║ ██║   ██╔═══██║",
        L"██║  ██║ ██║  ██║ ██║   ██║   ██║",
        L"╚═╝  ╚═╝ ╚═╝  ╚═╝ ╚═╝   ╚███████║"};

    for (int row = 0; row < static_cast<int>(titleArt.size()); ++row)
    {
        DrawCentered(titleArt[row], 8 + row, row == 2 ? Color::BrightWhite : Color::Cyan, 10);
    }

    DrawCentered(L"R  A  I  d", 15, Color::BrightWhite);
    DrawCentered(L"──────────────  TACTICAL CONSOLE RAID  ──────────────", 17, Color::Blue);
    DrawCentered(L"FOUR OPERATIVES  //  ONE TARGET  //  NO RETREAT", 20, Color::White);

    // 메뉴
    const int count = static_cast<int>(itemList.size());

    for (int ix = 0; ix < count; ++ix)
    {
        const bool selected = ix == currentIndex;
        const Color textColor = selected ? pulseColor : unselectedColor;
        const Vector2 itemPosition(MenuX, MenuY + ix * MenuSpacing);
        DrawBox(itemPosition, MenuWidth, MenuHeight, textColor);

        const std::wstring label = ix == 0 ? L"GAME START" : L"EXIT";
        Renderer::Get().Submit(label,
                               Vector2(itemPosition.x + (MenuWidth - static_cast<int>(label.size())) / 2,
                                       itemPosition.y + 2),
                               textColor, 10);

        if (selected)
        {
            Renderer::Get().Submit(L">>", itemPosition + Vector2(-4, 2), selectedColor, 10);
            Renderer::Get().Submit(L"<<", itemPosition + Vector2(MenuWidth + 2, 2), selectedColor, 10);
        }
    }

    DrawCentered(L"[ ↑ / ↓ ] SELECT     [ SPACE / ENTER ] CONFIRM     [ MOUSE ] ENABLED", 41, Color::Cyan);
    DrawCentered(L"PROJECT RAID // BUILD 05", 43, Color::Blue);
}
