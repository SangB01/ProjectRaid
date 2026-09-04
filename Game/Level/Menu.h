#pragma once
#include <Level/Level.h>
#include <string>

using namespace Craft;
// 메뉴 아이템 구조체
struct MenuItem
{
    // 메뉴 선택 시 실행할 로직 저장을 위한 함수 포인터
    using OnSelected = void (*)();

    // 생성자
    MenuItem(const std::wstring& text, OnSelected onSelected) : text(text), onSelected(onSelected) {}

    // 메뉴 텍스트
    std::wstring text;
    // 메뉴를 선택했을 때 실행할 로직
    OnSelected onSelected = nullptr;
};

class Menu : public Level
{
  public:
    Menu();

  private:
    // 이벤트 함수 오버라이드
    virtual void Tick(float deltaTime) override;
    virtual void Draw() override;

  private:
    // 현재 활성화된 메뉴 아이템 인덱스
    int currentIndex = 0;

    // 선택된 메뉴 아이템 색상
    Color selectedColor = Color::Green;

    // 미선택된 메뉴 아이템의 색상
    Color unselectedColor = Color::White;

    // 메뉴 아이템 배열
    std::vector<std::unique_ptr<MenuItem>> itemList;
};
