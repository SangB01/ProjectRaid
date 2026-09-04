#include "Input.h"
#include <cassert>
#include <Windows.h>

namespace Craft
{
Input* Input::instance = nullptr;

Input::Input()
{
    assert(!instance);
    instance = this;

    // Quick Edit가 켜져 있으면 마우스 클릭이 텍스트 선택으로
    // 소비될 수 있으므로 끄고, 콘솔 마우스 이벤트를 활성화한다.
    inputHandle = GetStdHandle(STD_INPUT_HANDLE);

    if (inputHandle != INVALID_HANDLE_VALUE && GetConsoleMode(inputHandle, &originalConsoleMode))
    {
        DWORD consoleMode = originalConsoleMode;
        consoleMode |= ENABLE_EXTENDED_FLAGS;
        consoleMode |= ENABLE_MOUSE_INPUT;
        consoleMode |= ENABLE_WINDOW_INPUT;
        consoleMode &= ~ENABLE_QUICK_EDIT_MODE;

        shouldRestoreConsoleMode = SetConsoleMode(inputHandle, consoleMode) == TRUE;
    }
}

Input::~Input()
{
    if (shouldRestoreConsoleMode && inputHandle != INVALID_HANDLE_VALUE)
    {
        SetConsoleMode(inputHandle, originalConsoleMode);
    }

    instance = nullptr;
}

bool Input::GetKeyDown(int keyCode) const
{
    const bool keyPressed = !keyStates[keyCode].wasKeyDown && keyStates[keyCode].isKeyDown;

    if (keyCode == VK_LBUTTON)
    {
        return keyPressed || leftMousePressed;
    }
    if (keyCode == VK_RBUTTON)
    {
        return keyPressed || rightMousePressed;
    }

    return keyPressed;
}

bool Input::GetKeyUp(int keyCode) const
{
    return keyStates[keyCode].wasKeyDown && !keyStates[keyCode].isKeyDown;
}

bool Input::GetKey(int keyCode) const
{
    return keyStates[keyCode].isKeyDown;
}

const Vector2& Input::GetMousePosition() const
{
    return mousePosition;
}

Input& Input::Get()
{
    assert(instance);
    return *instance;
}

void Input::ProcessInput()
{
    // 현재 프레임에 키 입력이 발생하였는지 확인
    for (int ix = 0; ix < keyCount; ++ix)
    {
        keyStates[ix].isKeyDown = ((GetAsyncKeyState(ix) & 0x8000) != 0);
    }

    UpdateMousePosition();
}
void Input::SavePreviousStates()
{
    // 이전 프레임 입력 값 저장
    for (KeyState& state : keyStates)
    {
        // 현재 프레임 입력 값을 이전 프레임 값으로 저장
        state.wasKeyDown = state.isKeyDown;
    }

    leftMousePressed = false;
    rightMousePressed = false;
}
void Input::UpdateMousePosition()
{
    // 콘솔이 제공하는 좌표는 이미 문자 셀 기준이므로 가장 정확하다.
    if (UpdateMousePositionFromConsoleInput())
    {
        return;
    }

    // 콘솔 입력 이벤트를 사용할 수 없는 환경을 위한 폴백.
    POINT cursorPosition;

    // 예외 처리
    if (!GetCursorPos(&cursorPosition))
    {
        return;
    }

    HWND consoleWindow = GetConsoleWindow();

    if (!consoleWindow)
    {
        return;
    }
    // 화면 좌표 → 콘솔 창 내부 좌표
    if (!ScreenToClient(consoleWindow, &cursorPosition))
    {
        return;
    }

    // 콘솔 출력 핸들
    HANDLE outputHandle = GetStdHandle(STD_OUTPUT_HANDLE);

    if (outputHandle == INVALID_HANDLE_VALUE)
    {
        return;
    }

    // 현재 콘솔 글꼴 정보
    CONSOLE_FONT_INFO fontInfo = {};

    if (!GetCurrentConsoleFont(outputHandle, FALSE, &fontInfo))
    {
        return;
    }

    if (fontInfo.dwFontSize.X <= 0 || fontInfo.dwFontSize.Y <= 0)
    {
        return;
    }

    // 현재 콘솔 버퍼 정보
    CONSOLE_SCREEN_BUFFER_INFO bufferInfo = {};

    if (!GetConsoleScreenBufferInfo(outputHandle, &bufferInfo))
    {
        return;
    }

    // 픽셀 좌표를 콘솔 셀 좌표로 변환
    const int x = cursorPosition.x / fontInfo.dwFontSize.X + bufferInfo.srWindow.Left;

    const int y = cursorPosition.y / fontInfo.dwFontSize.Y + bufferInfo.srWindow.Top;

    mousePosition = Vector2(x, y);
}

bool Input::UpdateMousePositionFromConsoleInput()
{
    if (!shouldRestoreConsoleMode || inputHandle == INVALID_HANDLE_VALUE)
    {
        return false;
    }

    DWORD pendingEventCount = 0;
    if (!GetNumberOfConsoleInputEvents(inputHandle, &pendingEventCount))
    {
        return false;
    }

    bool receivedMouseEvent = false;
    INPUT_RECORD inputRecords[128] = {};

    while (pendingEventCount > 0)
    {
        const DWORD readRequestCount = pendingEventCount < 128 ? pendingEventCount : 128;

        DWORD readEventCount = 0;
        if (!ReadConsoleInputW(inputHandle, inputRecords, readRequestCount, &readEventCount))
        {
            return false;
        }

        for (DWORD ix = 0; ix < readEventCount; ++ix)
        {
            if (inputRecords[ix].EventType != MOUSE_EVENT)
            {
                continue;
            }

            const MOUSE_EVENT_RECORD& mouseEvent = inputRecords[ix].Event.MouseEvent;

            mousePosition = Vector2(mouseEvent.dwMousePosition.X, mouseEvent.dwMousePosition.Y);

            const bool isLeftMouseDown = (mouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) != 0;
            const bool isRightMouseDown = (mouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED) != 0;

            if (isLeftMouseDown && !consoleLeftMouseDown)
            {
                leftMousePressed = true;
            }

            consoleLeftMouseDown = isLeftMouseDown;

            if (isRightMouseDown && !consoleRightMouseDown)
            {
                rightMousePressed = true;
            }

            consoleRightMouseDown = isRightMouseDown;

            receivedMouseEvent = true;
        }

        if (!GetNumberOfConsoleInputEvents(inputHandle, &pendingEventCount))
        {
            break;
        }
    }

    // 이벤트가 없더라도 콘솔 이벤트 모드는 정상 동작 중이다.
    // 마지막으로 받은 좌표를 유지하고 픽셀 기반 폴백은 사용하지 않는다.
    return receivedMouseEvent || shouldRestoreConsoleMode;
}

} // namespace Craft
