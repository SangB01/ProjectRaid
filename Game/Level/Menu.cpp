#include "Menu.h"

#include <Game/Game.h>
#include <Input/Input.h>
#include <Render/Renderer.h>

#include <Windows.h>

using namespace Craft;

namespace
{
constexpr int ButtonX = 60;
constexpr int ButtonY = 19;
constexpr int ButtonWidth = 35;
constexpr int ButtonHeight = 5;
constexpr int ButtonSpacing = 6;
}

Menu::Menu()
    : buttons{Button(L"RESUME", Vector2(ButtonX, ButtonY), ButtonWidth, ButtonHeight),
              Button(L"RESTART", Vector2(ButtonX, ButtonY + ButtonSpacing), ButtonWidth, ButtonHeight),
              Button(L"MAIN MENU", Vector2(ButtonX, ButtonY + ButtonSpacing * 2), ButtonWidth, ButtonHeight)}
{
    SelectIndex(0);
}

void Menu::Tick(float deltaTime)
{
    Level::Tick(deltaTime);

    if (hasRequestedAction)
    {
        return;
    }

    const Input& input = Input::Get();
    const Vector2 mousePosition = input.GetMousePosition();
    const int hoveredIndex = FindButtonAt(mousePosition);

    if (!hasMousePosition || mousePosition != previousMousePosition)
    {
        if (hoveredIndex >= 0)
        {
            SelectIndex(hoveredIndex);
        }

        previousMousePosition = mousePosition;
        hasMousePosition = true;
    }

    // ESC always closes the pause menu.
    if (input.GetKeyDown(VK_ESCAPE))
    {
        Activate(PauseAction::Resume);
        return;
    }

    if (input.GetKeyDown(VK_UP))
    {
        SelectIndex(selectedIndex - 1);
    }
    else if (input.GetKeyDown(VK_DOWN))
    {
        SelectIndex(selectedIndex + 1);
    }

    if (input.GetKeyDown(VK_LBUTTON) && hoveredIndex >= 0)
    {
        SelectIndex(hoveredIndex);
        Activate(static_cast<PauseAction>(selectedIndex));
    }
    else if (input.GetKeyDown(VK_SPACE) || input.GetKeyDown(VK_RETURN))
    {
        Activate(static_cast<PauseAction>(selectedIndex));
    }
}

void Menu::SelectIndex(int index)
{
    const int count = static_cast<int>(buttons.size());
    selectedIndex = (index % count + count) % count;

    for (int buttonIndex = 0; buttonIndex < count; ++buttonIndex)
    {
        buttons[buttonIndex].SetSelected(buttonIndex == selectedIndex);
    }
}

void Menu::Activate(PauseAction action)
{
    if (hasRequestedAction)
    {
        return;
    }

    hasRequestedAction = true;
    for (Button& button : buttons)
    {
        button.SetEnabled(false);
    }

    Game& game = dynamic_cast<Game&>(Engine::Get());
    switch (action)
    {
    case PauseAction::Resume:
        game.ToggleMenu();
        break;
    case PauseAction::Restart:
        game.StartGame();
        break;
    case PauseAction::MainMenu:
        game.ReturnToMainMenu();
        break;
    }
}

int Menu::FindButtonAt(const Vector2& position) const
{
    for (int index = 0; index < static_cast<int>(buttons.size()); ++index)
    {
        if (buttons[index].Contains(position))
        {
            return index;
        }
    }

    return -1;
}

void Menu::DrawCentered(const std::wstring& text, int y, Color color, int sortingOrder) const
{
    Renderer::Get().Submit(text, Vector2((ScreenWidth - static_cast<int>(text.size())) / 2, y), color,
                           sortingOrder);
}

void Menu::Draw()
{
    DrawCentered(std::wstring(64, L'='), 7, Color::Cyan);
    DrawCentered(L"GAME PAUSED", 10, Color::Green);
    DrawCentered(L"Select an action", 14, Color::White);

    for (const Button& button : buttons)
    {
        button.Draw();
    }

    Renderer::Get().Submit(L">>", Vector2(ButtonX - 4, ButtonY + selectedIndex * ButtonSpacing + 2),
                           Color::Green, 10);
    Renderer::Get().Submit(L"<<", Vector2(ButtonX + ButtonWidth + 2, ButtonY + selectedIndex * ButtonSpacing + 2),
                           Color::Green, 10);

    DrawCentered(std::wstring(64, L'='), 39, Color::Cyan);
    DrawCentered(L"[ UP / DOWN ] SELECT     [ ENTER ] CONFIRM     [ ESC ] RESUME", 42, Color::White);
}
