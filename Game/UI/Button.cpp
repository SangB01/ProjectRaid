#include "Button.h"

#include <Input/Input.h>
#include <Render/Renderer.h>

#include <Windows.h>

using namespace Craft;

Button::Button(const std::wstring& text, const Vector2& position, int width, int height)
    : text(text), position(position), width(width), height(height)
{
}

void Button::Draw() const
{
    if (width < 2 || height < 2)
    {
        return;
    }

    const bool isHovered = IsHovered();
    const bool isPressed = isHovered && Input::Get().GetKeyDown(VK_LBUTTON);
    const Color drawColor = !enabled ? disabledColor : (isPressed ? pressedColor : (isHovered || selected ? hoverColor : normalColor));

    std::wstring top = L"\u250C";
    top.append(width - 2, L'\u2500');
    top += L"\u2510";

    Renderer::Get().Submit(top, position, drawColor, 5);

    for (int y = 1; y < height - 1; ++y)
    {
        Renderer::Get().Submit(L"\u2502", Vector2(position.x, position.y + y), drawColor, 5);
        Renderer::Get().Submit(L"\u2502", Vector2(position.x + width - 1, position.y + y), drawColor, 5);
    }

    std::wstring bottom = L"\u2514";
    bottom.append(width - 2, L'\u2500');
    bottom += L"\u2518";

    Renderer::Get().Submit(bottom, Vector2(position.x, position.y + height - 1), drawColor, 5);

    const int innerWidth = width - 2;
    const std::wstring visibleText = text.substr(0, innerWidth);
    const int textX = position.x + 1 + (innerWidth - static_cast<int>(visibleText.size())) / 2;
    const int textY = position.y + height / 2;

    Renderer::Get().Submit(visibleText, Vector2(textX, textY), drawColor, 10);
}

bool Button::Contains(const Vector2& point) const
{
    return point.x >= position.x && point.x < position.x + width && point.y >= position.y &&
           point.y < position.y + height;
}

bool Button::IsHovered() const
{
    return enabled && Contains(Input::Get().GetMousePosition());
}

bool Button::IsClicked() const
{
    return IsHovered() && Input::Get().GetKeyDown(VK_LBUTTON);
}

void Button::SetEnabled(bool newEnabled)
{
    enabled = newEnabled;
}

bool Button::IsEnabled() const
{
    return enabled;
}

void Button::SetSelected(bool selected)
{
    this->selected = selected;
}
