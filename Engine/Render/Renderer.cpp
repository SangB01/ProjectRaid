#include "Renderer.h"
#include "ScreenBuffer.h"

#include <cassert>
#include <iostream>
#include <Windows.h>

namespace Craft
{
Renderer::Frame::Frame(int bufferCount)
{
    charInfoArray = std::make_unique<CHAR_INFO[]>(bufferCount);

    sortingOrderArray = std::make_unique<int[]>(bufferCount);
}

Renderer::Frame::~Frame() {}

void Renderer::Frame::Clear(const Vector2& screenSize)
{
    const int width = screenSize.x;
    const int height = screenSize.y;

    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x)
        {
            const int index = (y * width) + x;

            CHAR_INFO& info = charInfoArray[index];

            // Unicode 문자 사용
            info.Char.UnicodeChar = L' ';

            info.Attributes = 0;

            sortingOrderArray[index] = -1;
        }
    }
}

Renderer* Renderer::instance = nullptr;

Renderer::Renderer(const Vector2& screenSize) : screenSize(screenSize)
{
    assert(!instance);
    instance = this;

    const int bufferCount = screenSize.x * screenSize.y;

    frame = std::make_unique<Frame>(bufferCount);

    frame->Clear(screenSize);

    screenBufferArray[0] = std::make_unique<ScreenBuffer>(screenSize);

    screenBufferArray[0]->Clear();

    screenBufferArray[1] = std::make_unique<ScreenBuffer>(screenSize);

    screenBufferArray[1]->Clear();

    SetConsoleActiveScreenBuffer(screenBufferArray[0]->GetBuffer());
}

Renderer::~Renderer()
{
    instance = nullptr;

    SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE));
}

void Renderer::Submit(const std::wstring& image, const Vector2& position, Color color, int sortingOrder)
{
    RenderCommand command;

    command.image = image;
    command.position = position;
    command.color = color;
    command.sortingOrder = sortingOrder;

    renderQueue.emplace_back(command);
}

void Renderer::Draw()
{
    Clear();

    DrawRenderQueue();

    Present();
}

Renderer& Renderer::Get()
{
    assert(instance && "instance should not be null");

    return *instance;
}

void Renderer::Clear()
{
    frame->Clear(screenSize);

    GetCurrentBuffer()->Clear();
}

void Renderer::DrawRenderQueue()
{
    for (const RenderCommand& command : renderQueue)
    {
        if (command.image.empty())
        {
            continue;
        }

        if (command.position.y < 0 || command.position.y >= screenSize.y)
        {
            continue;
        }

        const int length = static_cast<int>(command.image.length());

        const int startX = command.position.x;

        const int endX = startX + length - 1;

        if (endX < 0 || startX >= screenSize.x)
        {
            continue;
        }

        const int visibleStart = startX < 0 ? 0 : startX;

        const int visibleEnd = endX >= screenSize.x ? screenSize.x - 1 : endX;

        for (int x = visibleStart; x <= visibleEnd; ++x)
        {
            const int sourceIndex = x - startX;

            const int index = (command.position.y * screenSize.x) + x;

            if (frame->sortingOrderArray[index] > command.sortingOrder)
            {
                continue;
            }

            frame->charInfoArray[index].Char.UnicodeChar = command.image[sourceIndex];

            frame->charInfoArray[index].Attributes = static_cast<WORD>(command.color);

            frame->sortingOrderArray[index] = command.sortingOrder;
        }
    }

    GetCurrentBuffer()->Draw(frame->charInfoArray.get());

    renderQueue.clear();

    SetConsoleTextAttribute(GetCurrentBuffer()->GetBuffer(), static_cast<WORD>(Color::White));
}

void Renderer::Present()
{
    SetConsoleActiveScreenBuffer(GetCurrentBuffer()->GetBuffer());

    currentBufferIndex = 1 - currentBufferIndex;
}

const ScreenBuffer* const Renderer::GetCurrentBuffer() const
{
    return screenBufferArray[currentBufferIndex].get();
}
} // namespace Craft
