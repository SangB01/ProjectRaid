#pragma once
#include <Math/Color.h>
#include <Math/Vector2.h>

#include <string>

using namespace Craft;

class Button
{
  public:
    Button(const std::wstring& text, const Vector2& position, int width, int height);

    void Draw() const;

    bool Contains(const Vector2& point) const;
    bool IsHovered() const;
    bool IsClicked() const;

    void SetEnabled(bool newEnabled);
    bool IsEnabled() const;
    void SetSelected(bool selected);

  private:
    std::wstring text;
    Vector2 position;

    int width = 0;
    int height = 0;

    Color normalColor = Color::White;
    Color hoverColor = Color::Green;
    Color pressedColor = Color::Yellow;
    Color disabledColor = Color::Blue;

    bool enabled = true;
    bool selected = false;
};
