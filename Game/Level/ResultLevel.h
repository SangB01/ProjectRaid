#pragma once
#include <Game/BattleResult.h>
#include <Level/Level.h>
#include <UI/Button.h>
#include <array>
#include <functional>

using namespace Craft;

enum class ResultAction { Restart, MainMenu, Ranking, Exit };

class ResultLevel : public Level
{
    friend class RaidLevelCardTests;

  public:
    using OnAction = std::function<void(ResultAction)>;

    ResultLevel(const BattleResult& result, OnAction onAction);
    virtual void Tick(float deltaTime) override;
    virtual void Draw() override;

  private:
    void SelectIndex(int index);
    void Activate(ResultAction action);
    int FindButtonAt(const Vector2& position) const;
    void DrawCentered(const std::wstring& text, int y, Color color) const;

    BattleResult result;
    OnAction onAction;
    std::array<Button, 4> buttons;
    int selectedIndex = 0;
    bool hasRequestedAction = false;
    bool hasMousePosition = false;
    Vector2 previousMousePosition = Craft::Vector2::Zero;
};
