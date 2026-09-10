#pragma once
#include <Level/Level.h>
#include <string>
#include <vector>
#include <memory>

using namespace Craft;
struct MainMenuItem
{
    using OnSelected = void (*)();

    MainMenuItem(const std::wstring& text, OnSelected onSelected) : text(text), onSelected(onSelected) {}

    std::wstring text;

    OnSelected onSelected = nullptr;
};

class MainLevel : public Level
{
  public:
    MainLevel();

  private:
    virtual void Tick(float deltaTime) override;
    virtual void Draw() override;

    int FindMenuItemAt(const Vector2& position) const;
    void DrawBox(const Vector2& position, int width, int height, Color color) const;
    void DrawCentered(const std::wstring& text, int y, Color color, int sortingOrder = 10) const;

  private:
    int currentIndex = 0;
    float animationTimer = 0.0f;
    bool hasMousePosition = false;
    Vector2 previousMousePosition = Vector2::Zero;

    Color selectedColor = Color::Green;
    Color unselectedColor = Color::White;

    std::vector<std::unique_ptr<MainMenuItem>> itemList;

    static constexpr int ScreenWidth = 155;
    static constexpr int MenuX = 55;
    static constexpr int MenuY = 28;
    static constexpr int MenuWidth = 45;
    static constexpr int MenuHeight = 5;
    static constexpr int MenuSpacing = 6;
};
