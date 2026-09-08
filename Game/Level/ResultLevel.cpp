#include "ResultLevel.h"
#include <Input/Input.h>
#include <Render/Renderer.h>
#include <algorithm>
#include <utility>

using namespace Craft;

ResultLevel::ResultLevel(const BattleResult& result, OnAction onAction)
    : result(result), onAction(std::move(onAction)),
      buttons{Button(L"Restart", Vector2(62, 23), 30, 4), Button(L"Main Menu", Vector2(62, 28), 30, 4),
              Button(L"Ranking", Vector2(62, 33), 30, 4), Button(L"Exit", Vector2 (62, 38), 30 , 4)}
{
    SelectIndex(0);
}

void ResultLevel::SelectIndex(int index)
{
    const int count = static_cast<int>(buttons.size());
    selectedIndex = (index % count + count) % count;
    for (int buttonIndex = 0; buttonIndex < count; ++buttonIndex)
    {
        buttons[buttonIndex].SetSelected(buttonIndex == selectedIndex);
    }
}

int ResultLevel::FindButtonAt(const Vector2& position) const
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

void ResultLevel::Activate(ResultAction action)
{
    if (hasRequestedAction || !onAction || action < ResultAction::Restart || action > ResultAction::Exit)
    {
        return;
    }
    hasRequestedAction = true;
    for (Button& button : buttons)
    {
        button.SetEnabled(false);
    }
    onAction(action);
}

void ResultLevel::Tick(float deltaTime)
{
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

    if (input.GetKeyDown(VK_ESCAPE))
    {
        Activate(ResultAction::MainMenu);
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
        Activate(static_cast<ResultAction>(selectedIndex));
    }
    else if (input.GetKeyDown(VK_SPACE) || input.GetKeyDown(VK_RETURN))
    {
        Activate(static_cast<ResultAction>(selectedIndex));
    }
}

void ResultLevel::DrawCentered(const std::wstring& text, int y, Color color) const
{
    Renderer::Get().Submit(text, Vector2((155 - static_cast<int>(text.size())) / 2, y), color, 10);
}

void ResultLevel::Draw()
{
    const bool victory = result.outcome == BattleOutcome::Victory;
    const Color color = victory ? Color::Green : Color::Red;
    DrawCentered(std::wstring(64, L'='), 6, color);
    DrawCentered(victory ? L"VICTORY" : L"DEFEAT", 9, color);

    const int totalTimes = static_cast<int>(result.elapsedTimeSeconds);

    const int min = totalTimes / 60;
    const int sec = totalTimes % 60;

    const std::wstring timeText = L"Play Time: " + std::to_wstring(min) + L":" + std::to_wstring(sec);
    DrawCentered(timeText, 13, Color::Yellow);

    const auto survivors = std::count_if(result.playerHealth.begin(), result.playerHealth.end(), [](int health) { return health > 0; });
    std::wstring playerStatus;
    for (int index = 0; index < static_cast<int>(result.playerHealth.size()); ++index)
    {
        playerStatus += L"P" + std::to_wstring(index + 1) + L": " + std::to_wstring(result.playerHealth[index]) + L" HP    ";
    }
    DrawCentered(playerStatus, 17, Color::White);
    for (const Button& button : buttons)
    {
        button.Draw();
    }
    Renderer::Get().Submit(L">", Vector2(59, 25 + selectedIndex * 5), Color::Green, 10);
    DrawCentered(std::wstring(64, L'='), 43, color);
}
