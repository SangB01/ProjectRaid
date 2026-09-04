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

  private:
    int currentIndex = 0;

    Color selectedColor = Color::Green;
    Color unselectedColor = Color::White;

    std::vector<std::unique_ptr<MainMenuItem>> itemList;
};
