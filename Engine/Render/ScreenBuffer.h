#pragma once

#include <Math/Vector2.h>
#include <Windows.h>

namespace Craft
{
// 이중 버퍼링 구현을 위한 화면 버퍼 클래스
// 콘솔 핸들을 관리
class ScreenBuffer
{
  public:
    ScreenBuffer(const Vector2& screenSize);
    ~ScreenBuffer();

    // 콘솔 초기화 - 화면 지우기
    void Clear() const;

    // 전달된 글자 값 그리는 함수
    void Draw(const CHAR_INFO* const charInfo) const;

    // Getter
    inline HANDLE GetBuffer() const
    {
        return buffer;
    }
    // HANDLE은 포인터인데 여러 콘솔을 포인터로 해서 관리가 가능

  private:
    // 화면 버퍼 핸들
    HANDLE buffer = nullptr;

    // 화면 크게
    Vector2 size;
};

} // namespace Craft
