#pragma once

#include <Actor/Player.h>
#include <Level/Level.h>
#include <Level/RaidMap.h>
#include <Math/Vector2.h>
#include <UI/Button.h>

#include <memory>
#include <vector>

using namespace Craft;

class RaidLevel : public Level
{
    TYPE_DECLARATIONS(RaidLevel, Level)

  private:
    enum class TurnState
    {
        PlayerPlanning,
        PlayerMoving,
        Boss
    };

  public:
    RaidLevel();
    virtual ~RaidLevel() = default;

    virtual void OnInitialized() override;
    virtual void Tick(float deltaTime) override;
    virtual void Draw() override;

  private:
    void ProcessMovementInput();
    void ProcessTurn(float deltaTime);
    void BeginPlayerTurn();
    void BeginPlayerMovement();
    void BeginBossTurn();
    void ExecuteReservedMoves();

    void DrawReservedPaths() const;
    void DrawPathPreview() const;
    void DrawTargetCursor() const;

    void Interface();

    void DrawBox(const Vector2& position, int width, int height, Color color = Color::White);

    void CardArea();
    void SpawnPlayers();

    std::shared_ptr<Player> FindPlayerAt(const Vector2& position) const;

    void SelectPlayer(const std::shared_ptr<Player>& player);

    bool IsOccupiedByOtherPlayer(const Vector2& position, const Player* ignorePlayer = nullptr) const;

    std::vector<Vector2> BuildBlockedPositions(const Player* ignorePlayer) const;

    bool IsAnyPlayerMoving() const;

  private:
    std::unique_ptr<RaidMap> raidMap;

    std::unique_ptr<Button> turnEndButton;

    std::vector<std::shared_ptr<Player>> players;

    std::shared_ptr<Player> selectedPlayer;

    Vector2 targetPosition = Vector2::Zero;

    Vector2 previewStartPosition = Vector2::Zero;

    bool hasPreviewTarget = false;

    std::vector<Vector2> previewPath;

    TurnState turnState = TurnState::PlayerPlanning;
    float bossTurnTimer = 0.0f;

    static constexpr float BossTurnDuration = 0.75f;
};
