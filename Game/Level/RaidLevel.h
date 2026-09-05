#pragma once

#include <Actor/Boss.h>
#include <Actor/Minion.h>
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
    void PlanBossAction();
    void PlanMinionActions();
    void ExecuteBossAction();
    void ExecuteBossAttack();
    void ExecuteMinionActions();
    void ExecuteMinionAttacks();
    void ResolveBossMovementHit();
    void ResolveMinionMovementBlock();

    void DrawBossPlannedPath() const;
    void DrawBossAttackWarning() const;
    void DrawMinionWarnings() const;
    void DrawReservedPaths() const;
    void DrawPathPreview() const;
    void DrawTargetCursor() const;

    void Interface();

    void DrawBox(const Vector2& position, int width, int height, Color color = Color::White);

    void CardArea();
    void SpawnPlayers();
    void SpawnBoss();
    void SpawnMinions(const std::vector<Vector2>& spawnPositions);

    bool FindBossSpawnPosition(Vector2& outPosition) const;
    bool FindBossMovePlan(std::vector<Vector2>& outPath, Vector2& outDestination,
                          std::shared_ptr<Player>& outTargetPlayer) const;
    bool FindTeleportDestination(const Player& targetPlayer, Vector2& outDestination) const;
    bool FindSpecialDestination(Vector2& outDestination) const;

    Boss::ActionType ChooseNormalBossAction() const;

    std::vector<Vector2> BuildLaserAttackPositions(const Vector2& center) const;
    std::vector<Vector2> BuildNearbyAttackPositions(const Vector2& center) const;
    std::vector<Vector2> BuildConeAttackPositions(const Vector2& center, const Vector2& target) const;
    std::vector<Vector2> BuildSummonPositions(int count, const Vector2& bossDestination) const;
    std::vector<Vector2> BuildSpecialAttackPositions(const Vector2& center) const;

    bool HasLineOfSight(const Vector2& start, const Vector2& end) const;
    bool ContainsPosition(const std::vector<Vector2>& positions, const Vector2& position) const;

    std::shared_ptr<Player> FindPlayerAt(const Vector2& position) const;

    void SelectPlayer(const std::shared_ptr<Player>& player);

    bool IsOccupiedByActor(const Vector2& position, const Actor* ignoreActor = nullptr) const;

    std::vector<Vector2> BuildBlockedPositions(const Actor* ignoreActor) const;

    bool IsAnyPlayerMoving() const;
    bool IsAnyMinionMoving() const;
    int GetActiveMinionCount() const;

  private:
    std::unique_ptr<RaidMap> raidMap;

    std::unique_ptr<Button> turnEndButton;

    std::vector<std::shared_ptr<Player>> players;
    std::vector<std::shared_ptr<Minion>> minions;

    std::shared_ptr<Boss> boss;

    std::shared_ptr<Player> selectedPlayer;

    Vector2 targetPosition = Vector2::Zero;

    Vector2 previewStartPosition = Vector2::Zero;

    bool hasPreviewTarget = false;

    std::vector<Vector2> previewPath;

    TurnState turnState = TurnState::PlayerPlanning;
    float bossTurnTimer = 0.0f;
    bool hasBossAttackExecuted = false;
    bool hasMinionActionsStarted = false;
    bool hasMinionAttacksExecuted = false;

    Boss::ActionType lastNormalBossAction = Boss::ActionType::None;
    bool hasUsedSpecialAt60 = false;
    bool hasUsedSpecialAt30 = false;

    static constexpr float BossTurnDuration = 0.75f;
    static constexpr int BossLaserDamage = 3;
    static constexpr int BossNearbyDamage = 3;
    static constexpr int BossConeDamage = 4;
    static constexpr int BossTeleportDamage = 3;
    static constexpr int BossSpecialDamage = 7;
    static constexpr int MinionChargeDamage = 2;
};
