#include "ResultLevel.h"
#include <Input/Input.h>
#include <Render/Renderer.h>
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cwctype>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <utility>

using namespace Craft;

namespace
{
constexpr const char* RankingFilePath = "../Assets/Ranking.csv";

std::wstring ConvertUTF8ToWide(const std::string& text)
{
    if (text.empty())
    {
        return L"";
    }

    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0)
    {
        return L"";
    }

    std::wstring result(length, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length);
    return result;
}

std::string ConvertWideToUTF8(const std::wstring& text)
{
    if (text.empty())
    {
        return "";
    }

    const int length = WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0,
                                           nullptr, nullptr);
    if (length <= 0)
    {
        return "";
    }

    std::string result(length, '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), result.data(), length, nullptr, nullptr);
    return result;
}
}

ResultLevel::ResultLevel(const BattleResult& result, OnAction onAction)
    : result(result), onAction(std::move(onAction)),
      buttons{Button(L"Restart", Vector2(62, 23), 30, 4), Button(L"Main Menu", Vector2(62, 28), 30, 4),
              Button(L"Ranking", Vector2(62, 33), 30, 4), Button(L"Exit", Vector2 (62, 38), 30 , 4)}
{
    SelectIndex(0);
}

void ResultLevel::SelectIndex(int index)
{
    const int count = static_cast<int>(buttons.size());
    selectedIndex = (index % count + count) % count;
    for (int buttonIndex = 0; buttonIndex < count; ++buttonIndex)
    {
        buttons[buttonIndex].SetSelected(buttonIndex == selectedIndex);
    }
}

int ResultLevel::FindButtonAt(const Vector2& position) const
{
    for (int index = 0; index < static_cast<int>(buttons.size()); ++index)
    {
        if (buttons[index].Contains(position))
        {
            return index;
        }
    }
    return -1;
}

void ResultLevel::Activate(ResultAction action)
{
    if (hasRequestedAction || action < ResultAction::Restart || action > ResultAction::Exit)
    {
        return;
    }

    if (action == ResultAction::Ranking)
    {
        OpenRanking();
        return;
    }

    if (!onAction)
    {
        return;
    }

    hasRequestedAction = true;
    for (Button& button : buttons)
    {
        button.SetEnabled(false);
    }
    onAction(action);
}

void ResultLevel::Tick(float deltaTime)
{
    if (hasRequestedAction)
    {
        return;
    }

    if (viewMode == ViewMode::IdInput)
    {
        TickIdInput();
        return;
    }

    if (viewMode == ViewMode::Ranking)
    {
        TickRanking();
        return;
    }

    TickSummary();
}

void ResultLevel::TickSummary()
{
    const Input& input = Input::Get();
    const Vector2 mousePosition = input.GetMousePosition();
    const int hoveredIndex = FindButtonAt(mousePosition);
    if (!hasMousePosition || mousePosition != previousMousePosition)
    {
        if (hoveredIndex >= 0)
        {
            SelectIndex(hoveredIndex);
        }
        previousMousePosition = mousePosition;
        hasMousePosition = true;
    }

    if (input.GetKeyDown(VK_ESCAPE))
    {
        Activate(ResultAction::MainMenu);
        return;
    }
    if (input.GetKeyDown(VK_UP))
    {
         SelectIndex(selectedIndex - 1);
    }

    else if (input.GetKeyDown(VK_DOWN))
    {
        SelectIndex(selectedIndex + 1);
    }
    if (input.GetKeyDown(VK_LBUTTON) && hoveredIndex >= 0)
    {
        SelectIndex(hoveredIndex);
        Activate(static_cast<ResultAction>(selectedIndex));
    }
    else if (input.GetKeyDown(VK_SPACE) || input.GetKeyDown(VK_RETURN))
    {
        Activate(static_cast<ResultAction>(selectedIndex));
    }
}

void ResultLevel::TickIdInput()
{
    const Input& input = Input::Get();

    if (input.GetKeyDown(VK_ESCAPE))
    {
        rankingMessage.clear();
        viewMode = ViewMode::Summary;
        return;
    }

    if (input.GetKeyDown(VK_BACK) && !idInput.empty())
    {
        idInput.pop_back();
        rankingMessage.clear();
    }

    for (const wchar_t character : input.GetTextInput())
    {
        if (character == L'\b' || character == L'\r' || character == L'\n' ||
            character == L',' || character == L'"' || !std::iswprint(character) ||
            static_cast<int>(idInput.size()) >= MaxIdLength)
        {
            continue;
        }

        idInput.push_back(character);
        rankingMessage.clear();
    }

    if (input.GetKeyDown(VK_RETURN))
    {
        SubmitRanking();
    }
}

void ResultLevel::TickRanking()
{
    const Input& input = Input::Get();
    if (input.GetKeyDown(VK_ESCAPE) || input.GetKeyDown(VK_RETURN) || input.GetKeyDown(VK_SPACE))
    {
        viewMode = ViewMode::Summary;
    }
}

void ResultLevel::OpenRanking()
{
    LoadRankings();

    if (result.outcome == BattleOutcome::Victory && !hasSubmittedRanking)
    {
        idInput.clear();
        rankingMessage.clear();
        viewMode = ViewMode::IdInput;
        return;
    }

    viewMode = ViewMode::Ranking;
}

void ResultLevel::SubmitRanking()
{
    const size_t firstCharacter = idInput.find_first_not_of(L" \t");
    const size_t lastCharacter = idInput.find_last_not_of(L" \t");
    if (firstCharacter == std::wstring::npos)
    {
        rankingMessage = L"Please enter an ID.";
        return;
    }

    idInput = idInput.substr(firstCharacter, lastCharacter - firstCharacter + 1);
    const int clearTimeMilliseconds = (std::max)(0, static_cast<int>(std::lround(result.elapsedTimeSeconds * 1000.0f)));
    rankings.push_back({idInput, clearTimeMilliseconds, true});
    std::stable_sort(rankings.begin(), rankings.end(), [](const RankingEntry& left, const RankingEntry& right) {
        return left.clearTimeMilliseconds < right.clearTimeMilliseconds;
    });

    currentRankingIndex = -1;
    for (int index = 0; index < static_cast<int>(rankings.size()); ++index)
    {
        if (rankings[index].isCurrentResult)
        {
            currentRankingIndex = index;
            break;
        }
    }

    rankingMessage = SaveRankings() ? L"Ranking saved." : L"Failed to save Ranking.csv.";
    hasSubmittedRanking = true;
    viewMode = ViewMode::Ranking;
}

void ResultLevel::LoadRankings()
{
    rankings.clear();
    currentRankingIndex = -1;

    std::ifstream file(RankingFilePath, std::ios::binary);
    if (!file.is_open())
    {
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        if (line.empty() || line == "id,clear_time_ms")
        {
            continue;
        }

        const size_t separator = line.rfind(',');
        if (separator == std::string::npos || separator == 0 || separator + 1 >= line.size())
        {
            continue;
        }

        try
        {
            const int clearTimeMilliseconds = std::stoi(line.substr(separator + 1));
            const std::wstring id = ConvertUTF8ToWide(line.substr(0, separator));
            if (!id.empty() && clearTimeMilliseconds >= 0)
            {
                rankings.push_back({id, clearTimeMilliseconds, false});
            }
        }
        catch (const std::exception&)
        {
            continue;
        }
    }

    std::stable_sort(rankings.begin(), rankings.end(), [](const RankingEntry& left, const RankingEntry& right) {
        return left.clearTimeMilliseconds < right.clearTimeMilliseconds;
    });
}

bool ResultLevel::SaveRankings() const
{
    std::ofstream file(RankingFilePath, std::ios::binary | std::ios::trunc);
    if (!file.is_open())
    {
        return false;
    }

    file << "id,clear_time_ms\n";
    for (const RankingEntry& entry : rankings)
    {
        file << ConvertWideToUTF8(entry.id) << ',' << entry.clearTimeMilliseconds << '\n';
    }

    return file.good();
}

void ResultLevel::DrawCentered(const std::wstring& text, int y, Color color) const
{
    Renderer::Get().Submit(text, Vector2((155 - static_cast<int>(text.size())) / 2, y), color, 10);
}

std::wstring ResultLevel::FormatClearTime(int milliseconds) const
{
    const int totalSeconds = milliseconds / 1000;
    const int minutes = totalSeconds / 60;
    const int seconds = totalSeconds % 60;
    const int hundredths = (milliseconds % 1000) / 10;

    std::wstringstream stream;
    stream << minutes << L':' << std::setfill(L'0') << std::setw(2) << seconds << L'.' << std::setw(2) << hundredths;
    return stream.str();
}

void ResultLevel::Draw()
{
    if (viewMode == ViewMode::IdInput)
    {
        DrawIdInput();
        return;
    }

    if (viewMode == ViewMode::Ranking)
    {
        DrawRanking();
        return;
    }

    DrawSummary();
}

void ResultLevel::DrawSummary() const
{
    const bool victory = result.outcome == BattleOutcome::Victory;
    const Color color = victory ? Color::Green : Color::Red;
    DrawCentered(std::wstring(64, L'='), 6, color);
    DrawCentered(victory ? L"VICTORY" : L"DEFEAT", 9, color);

    const int totalTimes = static_cast<int>(result.elapsedTimeSeconds);

    const int min = totalTimes / 60;
    const int sec = totalTimes % 60;

    const std::wstring timeText = L"Play Time: " + std::to_wstring(min) + L":" + std::to_wstring(sec);
    DrawCentered(timeText, 13, Color::Yellow);

    const auto survivors = std::count_if(result.playerHealth.begin(), result.playerHealth.end(), [](int health) { return health > 0; });
    std::wstring playerStatus;
    for (int index = 0; index < static_cast<int>(result.playerHealth.size()); ++index)
    {
        playerStatus += L"P" + std::to_wstring(index + 1) + L": " + std::to_wstring(result.playerHealth[index]) + L" HP    ";
    }
    DrawCentered(playerStatus, 17, Color::White);
    for (const Button& button : buttons)
    {
        button.Draw();
    }
    Renderer::Get().Submit(L">", Vector2(59, 25 + selectedIndex * 5), Color::Green, 10);
    DrawCentered(std::wstring(64, L'='), 43, color);
}

void ResultLevel::DrawIdInput() const
{
    DrawCentered(std::wstring(64, L'='), 8, Color::Green);
    DrawCentered(L"REGISTER CLEAR TIME", 11, Color::Green);
    DrawCentered(L"Clear Time: " + FormatClearTime(static_cast<int>(std::lround(result.elapsedTimeSeconds * 1000.0f))),
                 15, Color::Yellow);
    DrawCentered(L"ID: " + idInput + L"_", 21, Color::White);
    DrawCentered(L"Up to 12 characters. Commas and quotes are not allowed.", 26, Color::Cyan);
    DrawCentered(L"ENTER: Save    ESC: Cancel", 31, Color::White);
    if (!rankingMessage.empty())
    {
        DrawCentered(rankingMessage, 35, Color::Red);
    }
    DrawCentered(std::wstring(64, L'='), 40, Color::Green);
}

void ResultLevel::DrawRanking() const
{
    DrawCentered(std::wstring(64, L'='), 4, Color::Yellow);
    DrawCentered(L"CLEAR TIME RANKING", 6, Color::Yellow);
    DrawCentered(L"RANK        ID          TIME", 9, Color::White);

    const int displayCount = (std::min)(RankingDisplayCount, static_cast<int>(rankings.size()));
    for (int index = 0; index < displayCount; ++index)
    {
        const RankingEntry& entry = rankings[index];
        std::wstring id = entry.id.substr(0, MaxIdLength);
        id.append(MaxIdLength - id.size(), L' ');
        const std::wstring row = std::to_wstring(index + 1) + L".          " + id + L"  " +
                                 FormatClearTime(entry.clearTimeMilliseconds);
        DrawCentered(row, 11 + index * 2, entry.isCurrentResult ? Color::Green : Color::White);
    }

    if (rankings.empty())
    {
        DrawCentered(L"No ranking records yet.", 15, Color::White);
    }
    else if (currentRankingIndex >= RankingDisplayCount)
    {
        DrawCentered(L"Your Rank: " + std::to_wstring(currentRankingIndex + 1) + L"  " +
                         FormatClearTime(rankings[currentRankingIndex].clearTimeMilliseconds),
                     34, Color::Green);
    }

    if (!rankingMessage.empty())
    {
        DrawCentered(rankingMessage, 37, rankingMessage == L"Ranking saved." ? Color::Green : Color::Red);
    }
    DrawCentered(L"ENTER / SPACE / ESC: Back", 40, Color::Cyan);
    DrawCentered(std::wstring(64, L'='), 43, Color::Yellow);
}
