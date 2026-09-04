#include "RaidLevel.h"
#include <Game/Game.h>
#include <Math/AStar.h>
#include <Engine/Engine.h>
#include <Input/Input.h>
#include <Render/Renderer.h>
#include <Windows.h>
#include <iostream>

using namespace Craft;

RaidLevel::RaidLevel()
{
    raidMap = std::make_unique<RaidMap>();

    if (!raidMap->Load("../Assets/BaseMap.txt"))
    {
        std::cout << "RaidMap Load Failed\n";
    }
}

void RaidLevel::OnInitialized()
{
    super::OnInitialized();

    if (!raidMap)
    {
        return;
    }

    SpawnPlayers();

    const int interfaceX = raidMap->GetPosition().x + raidMap->GetWidth() + 3;
    const int interfaceY = raidMap->GetPosition().y;
    const int interfaceWidth = 30;
    const int interfaceHeight = raidMap->GetHeight();
    const int turnButtonY = interfaceY + interfaceHeight - 6;

    turnEndButton = std::make_unique<Button>(L"Turn End", Vector2(interfaceX + 5, turnButtonY), interfaceWidth - 10, 4);

    BeginPlayerTurn();
}

void RaidLevel::Tick(float deltaTime)
{
    if (Input::Get().GetKeyDown(VK_ESCAPE))
    {
        Game& game = dynamic_cast<Game&>(Engine::Get());
        game.ToggleMenu();
        return;
    }

    super::Tick(deltaTime);

    if (!raidMap)
    {
        return;
    }

    ProcessTurn(deltaTime);
}

void RaidLevel::ProcessTurn(float deltaTime)
{
    if (turnState == TurnState::Boss)
    {
        bossTurnTimer += deltaTime;

        if (bossTurnTimer >= BossTurnDuration)
        {
            BeginPlayerTurn();
        }

        return;
    }

    if (turnState == TurnState::PlayerMoving)
    {
        if (!IsAnyPlayerMoving())
        {
            BeginBossTurn();
        }

        return;
    }

    if (turnEndButton && turnEndButton->IsClicked())
    {
        ExecuteReservedMoves();

        if (IsAnyPlayerMoving())
        {
            BeginPlayerMovement();
        }
        else
        {
            BeginBossTurn();
        }

        return;
    }

    ProcessMovementInput();
}

void RaidLevel::BeginPlayerTurn()
{
    turnState = TurnState::PlayerPlanning;
    bossTurnTimer = 0.0f;

    if (turnEndButton)
    {
        turnEndButton->SetEnabled(true);
    }
}

void RaidLevel::BeginPlayerMovement()
{
    turnState = TurnState::PlayerMoving;

    previewPath.clear();
    hasPreviewTarget = false;

    if (turnEndButton)
    {
        turnEndButton->SetEnabled(false);
    }
}

void RaidLevel::BeginBossTurn()
{
    turnState = TurnState::Boss;
    bossTurnTimer = 0.0f;

    previewPath.clear();
    hasPreviewTarget = false;

    if (turnEndButton)
    {
        turnEndButton->SetEnabled(false);
    }
}

void RaidLevel::ExecuteReservedMoves()
{
    for (const std::shared_ptr<Player>& player : players)
    {
        if (player)
        {
            player->ExecuteReservedPath();
        }
    }
}

void RaidLevel::Draw()
{
    if (!raidMap)
    {
        return;
    }

    raidMap->Draw();
    DrawReservedPaths();
    DrawPathPreview();
    super::Draw();
    DrawTargetCursor();
    Interface();
    CardArea();
}

void RaidLevel::SpawnPlayers()
{
    players.clear();

    int playerIndex = 1;
    const Vector2 mapPosition = raidMap->GetPosition();

    for (int y = 0; y < raidMap->GetHeight(); ++y)
    {
        for (int x = 0; x < raidMap->GetWidth(); ++x)
        {
            Vector2 spawnPosition(mapPosition.x + x, mapPosition.y + y);

            if (!raidMap->IsWalkable(spawnPosition))
            {
                continue;
            }

            if (IsOccupiedByOtherPlayer(spawnPosition))
            {
                continue;
            }

            std::shared_ptr<Player> newPlayer = SpawnActor<Player>(playerIndex, spawnPosition);

            if (!newPlayer)
            {
                continue;
            }

            players.emplace_back(newPlayer);
            ++playerIndex;

            if (players.size() >= 4)
            {
                SelectPlayer(players[0]);
                return;
            }
        }
    }
}

void RaidLevel::ProcessMovementInput()
{
    if (!raidMap)
    {
        return;
    }

    if (IsAnyPlayerMoving())
    {
        previewPath.clear();
        hasPreviewTarget = false;
        return;
    }

    if (Input::Get().GetKeyDown(VK_LBUTTON))
    {
        Vector2 mousePosition = Input::Get().GetMousePosition();

        std::shared_ptr<Player> clickedPlayer = FindPlayerAt(mousePosition);

        if (clickedPlayer)
        {
            SelectPlayer(clickedPlayer);
            return;
        }
    }

    if (!selectedPlayer)
    {
        previewPath.clear();
        hasPreviewTarget = false;
        return;
    }

    const Vector2 mousePosition = Input::Get().GetMousePosition();

    const Vector2 playerPosition = selectedPlayer->GetPosition();

    // 같은 셀에 커서가 머무는 동안에는 A*를 다시 계산하지 않는다.
    const bool shouldUpdatePreview =
        !hasPreviewTarget || targetPosition != mousePosition || previewStartPosition != playerPosition;

    if (shouldUpdatePreview)
    {
        hasPreviewTarget = true;
        targetPosition = mousePosition;
        previewStartPosition = playerPosition;
        previewPath.clear();

        if (raidMap->IsInside(targetPosition) && raidMap->IsWalkable(targetPosition) &&
            targetPosition != playerPosition && !IsOccupiedByOtherPlayer(targetPosition, selectedPlayer.get()))
        {
            const std::vector<Vector2> blockedPositions = BuildBlockedPositions(selectedPlayer.get());

            std::vector<Vector2> path = AStar::FindPath(*raidMap, playerPosition, targetPosition, blockedPositions);

            if (!path.empty() && static_cast<int>(path.size()) <= Player::MaxMoveDistance)
            {
                previewPath = path;
            }
        }
    }

    // 예약한 목적지를 다시 우클릭하면 해당 예약을 취소한다.
    if (!Input::Get().GetKeyDown(VK_RBUTTON))
    {
        return;
    }

    if (selectedPlayer->HasReservedPath() && selectedPlayer->GetReservedPath().back() == targetPosition)
    {
        selectedPlayer->ClearReservedPath();
        previewPath.clear();
        hasPreviewTarget = false;
        return;
    }

    if (previewPath.empty())
    {
        return;
    }

    selectedPlayer->ReservePath(previewPath);
    previewPath.clear();
    hasPreviewTarget = false;
}

void RaidLevel::SelectPlayer(const std::shared_ptr<Player>& player)
{
    if (!player)
    {
        return;
    }

    if (selectedPlayer)
    {
        selectedPlayer->SetSelected(false);
    }

    selectedPlayer = player;
    selectedPlayer->SetSelected(true);

    targetPosition = selectedPlayer->GetPosition();

    previewStartPosition = targetPosition;
    hasPreviewTarget = false;
    previewPath.clear();
}

std::shared_ptr<Player> RaidLevel::FindPlayerAt(const Vector2& position) const
{
    for (const std::shared_ptr<Player>& player : players)
    {
        if (!player)
        {
            continue;
        }

        if (player->GetPosition() == position)
        {
            return player;
        }
    }

    return nullptr;
}

bool RaidLevel::IsOccupiedByOtherPlayer(const Vector2& position, const Player* ignorePlayer) const
{
    for (const std::shared_ptr<Player>& player : players)
    {
        if (!player)
        {
            continue;
        }

        if (player.get() == ignorePlayer)
        {
            continue;
        }

        if (player->GetPosition() == position)
        {
            return true;
        }
    }

    return false;
}

std::vector<Vector2> RaidLevel::BuildBlockedPositions(const Player* ignorePlayer) const
{
    std::vector<Vector2> blockedPositions;

    for (const std::shared_ptr<Player>& player : players)
    {
        if (!player)
        {
            continue;
        }

        if (player.get() == ignorePlayer)
        {
            continue;
        }

        blockedPositions.emplace_back(player->GetPosition());

        for (const Vector2& reservedPosition : player->GetReservedPath())
        {
            blockedPositions.emplace_back(reservedPosition);
        }
    }

    return blockedPositions;
}

bool RaidLevel::IsAnyPlayerMoving() const
{
    for (const std::shared_ptr<Player>& player : players)
    {
        if (player && player->HasPath())
        {
            return true;
        }
    }

    return false;
}

void RaidLevel::DrawReservedPaths() const
{
    for (const std::shared_ptr<Player>& player : players)
    {
        if (!player || !player->HasReservedPath())
        {
            continue;
        }

        const std::vector<Vector2>& reservedPath = player->GetReservedPath();

        for (const Vector2& pathPosition : reservedPath)
        {
            Renderer::Get().Submit(L"·", pathPosition, Color::Cyan, 9);
        }

        Renderer::Get().Submit(L"X", reservedPath.back(), Color::Cyan, 11);
    }
}

void RaidLevel::DrawPathPreview() const
{
    for (const Vector2& pathPosition : previewPath)
    {
        Renderer::Get().Submit(L"·", pathPosition, Color::Green, 10);
    }
}

void RaidLevel::DrawTargetCursor() const
{
    if (!raidMap || !selectedPlayer || !hasPreviewTarget)
    {
        return;
    }

    if (!raidMap->IsInside(targetPosition))
    {
        return;
    }

    if (targetPosition == selectedPlayer->GetPosition())
    {
        return;
    }

    const Color cursorColor = previewPath.empty() ? Color::Red : Color::Green;

    Renderer::Get().Submit(L"X", targetPosition, cursorColor, 30);
}

void RaidLevel::Interface()
{

    if (!raidMap)
    {
        return;
    }

    int interfaceX = raidMap->GetPosition().x + raidMap->GetWidth() + 3;

    int interfaceY = raidMap->GetPosition().y;

    int interfaceWidth = 30;
    int interfaceHeight = raidMap->GetHeight();

    DrawBox(Vector2(interfaceX, interfaceY), interfaceWidth, interfaceHeight);

    Renderer::Get().Submit(L"Interface", Vector2(interfaceX + 10, interfaceY + 2), Color::White, 10);

    std::wstring turnText = L"Turn : Boss";
    Color turnColor = Color::Yellow;

    if (turnState == TurnState::PlayerPlanning)
    {
        turnText = L"Turn : Planning";
        turnColor = Color::Green;
    }
    else if (turnState == TurnState::PlayerMoving)
    {
        turnText = L"Turn : Moving";
        turnColor = Color::Cyan;
    }

    Renderer::Get().Submit(turnText, Vector2(interfaceX + 8, interfaceY + 3), turnColor, 10);

    DrawBox(Vector2(interfaceX + 3, interfaceY + 5), interfaceWidth - 6, 3);

    Renderer::Get().Submit(L"Boss HP", Vector2(interfaceX + 5, interfaceY + 6), Color::White, 10);

    for (int ix = 0; ix < 4; ++ix)
    {
        int boxY = interfaceY + 9 + (ix * 4);
        std::shared_ptr<Player> interfacePlayer;

        if (ix < static_cast<int>(players.size()))
        {
            interfacePlayer = players[ix];
        }

        const int playerIndex = interfacePlayer ? interfacePlayer->GetPlayerIndex() : ix + 1;
        const Color borderColor = interfacePlayer && interfacePlayer->IsSelected() ? Color::Blue : Color::White;

        DrawBox(Vector2(interfaceX + 3, boxY), interfaceWidth - 6, 3, borderColor);

        std::wstring text = L"Player" + std::to_wstring(playerIndex) + L" HP";

        if (interfacePlayer)
        {
            text += L" : " + std::to_wstring(interfacePlayer->GetHealth());
        }

        Renderer::Get().Submit(text, Vector2(interfaceX + 5, boxY + 1), Color::White, 10);
    }

    if (turnEndButton)
    {
        turnEndButton->Draw();
    }
}

void RaidLevel::CardArea()
{
    if (!raidMap)
    {
        return;
    }

    int cardX = raidMap->GetPosition().x + 5;

    int cardY = raidMap->GetPosition().y + raidMap->GetHeight() + 2;

    int cardWidth = raidMap->GetWidth() + 28;

    int cardHeight = 8;

    DrawBox(Vector2(cardX, cardY), cardWidth, cardHeight);

    Renderer::Get().Submit(L"Card Area", Vector2(cardX + (cardWidth / 2) - 4, cardY + 2), Color::White, 10);

    int cardCount = 8;
    int startX = cardX + 5;
    int cardSpacing = 15;

    for (int ix = 0; ix < cardCount; ++ix)
    {
        std::wstring cardText = L"[ Card " + std::to_wstring(ix + 1) + L" ]";

        Renderer::Get().Submit(cardText, Vector2(startX + (ix * cardSpacing), cardY + 5), Color::White, 10);
    }
}

void RaidLevel::DrawBox(const Vector2& position, int width, int height, Color color)
{
    if (width < 2 || height < 2)
    {
        return;
    }

    std::wstring top = L"┌";
    top.append(width - 2, L'─');
    top += L"┐";

    Renderer::Get().Submit(top, position, color, 5);

    for (int y = 1; y < height - 1; ++y)
    {
        Renderer::Get().Submit(L"│", Vector2(position.x, position.y + y), color, 5);

        Renderer::Get().Submit(L"│", Vector2(position.x + width - 1, position.y + y), color, 5);
    }

    std::wstring bottom = L"└";
    bottom.append(width - 2, L'─');
    bottom += L"┘";

    Renderer::Get().Submit(bottom, Vector2(position.x, position.y + height - 1), color, 5);
}
