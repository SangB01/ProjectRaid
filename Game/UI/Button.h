#pragma once
#include <Math/Color.h>
#include <Math/Vector2.h>

#include <string>

class Button
{
  public:
    Button(const std::wstring& text, const Craft::Vector2& position, int width, int height);

    void Draw() const;

    bool Contains(const Craft::Vector2& point) const;
    bool IsHovered() const;
    bool IsClicked() const;

    void SetEnabled(bool newEnabled);
    bool IsEnabled() const;
    void SetSelected(bool selected);

  private:
    std::wstring text;
    Craft::Vector2 position;

    int width = 0;
    int height = 0;

    Craft::Color normalColor = Craft::Color::White;
    Craft::Color hoverColor = Craft::Color::Green;
    Craft::Color pressedColor = Craft::Color::Yellow;
    Craft::Color disabledColor = Craft::Color::Blue;

    bool enabled = true;
    bool selected = false;
};
