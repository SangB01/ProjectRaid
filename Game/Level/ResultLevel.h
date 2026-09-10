#pragma once
#include <Game/BattleResult.h>
#include <Level/Level.h>
#include <UI/Button.h>
#include <array>
#include <functional>
#include <string>
#include <vector>

using namespace Craft;

enum class ResultAction { Restart, MainMenu, Ranking, Exit };

class ResultLevel : public Level
{
    friend class RaidLevelCardTests;

  public:
    using OnAction = std::function<void(ResultAction)>;

    ResultLevel(const BattleResult& result, OnAction onAction);
    virtual void Tick(float deltaTime) override;
    virtual void Draw() override;

  private:
    enum class ViewMode
    {
        Summary,
        IdInput,
        Ranking
    };

    struct RankingEntry
    {
        std::wstring id;
        int clearTimeMilliseconds = 0;
        bool isCurrentResult = false;
    };

    void SelectIndex(int index);
    void Activate(ResultAction action);
    int FindButtonAt(const Vector2& position) const;
    void DrawCentered(const std::wstring& text, int y, Color color) const;
    void TickSummary();
    void TickIdInput();
    void TickRanking();
    void OpenRanking();
    void SubmitRanking();
    void LoadRankings();
    bool SaveRankings() const;
    void DrawSummary() const;
    void DrawIdInput() const;
    void DrawRanking() const;
    std::wstring FormatClearTime(int milliseconds) const;

    BattleResult result;
    OnAction onAction;
    std::array<Button, 4> buttons;
    int selectedIndex = 0;
    bool hasRequestedAction = false;
    bool hasMousePosition = false;
    Vector2 previousMousePosition = Craft::Vector2::Zero;
    ViewMode viewMode = ViewMode::Summary;
    std::wstring idInput;
    std::wstring rankingMessage;
    std::vector<RankingEntry> rankings;
    int currentRankingIndex = -1;
    bool hasSubmittedRanking = false;

    static constexpr int MaxIdLength = 12;
    static constexpr int RankingDisplayCount = 10;
};
