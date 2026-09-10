#pragma once

#include <Level/Level.h>
#include <UI/Button.h>

#include <array>

using namespace Craft;

class Menu : public Level
{
  public:
    Menu();

  private:
    enum class PauseAction
    {
        Resume,
        Restart,
        MainMenu
    };

    virtual void Tick(float deltaTime) override;
    virtual void Draw() override;

    void SelectIndex(int index);
    void Activate(PauseAction action);
    int FindButtonAt(const Vector2& position) const;
    void DrawCentered(const std::wstring& text, int y, Color color, int sortingOrder = 10) const;

  private:
    std::array<Button, 3> buttons;
    int selectedIndex = 0;
    bool hasMousePosition = false;
    bool hasRequestedAction = false;
    Vector2 previousMousePosition = Vector2::Zero;

    static constexpr int ScreenWidth = 155;
};
