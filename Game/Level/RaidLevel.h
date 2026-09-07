#pragma once

#include <Actor/Boss.h>
#include <Actor/Deployable.h>
#include <Actor/Minion.h>
#include <Actor/Player.h>
#include <Card/CardHand.h>
#include <Game/BattleResult.h>
#include <Level/Level.h>
#include <Level/RaidMap.h>
#include <Math/Vector2.h>
#include <UI/Button.h>

#include <memory>
#include <functional>
#include <vector>

using namespace Craft;

class RaidLevel : public Level
{
    TYPE_DECLARATIONS(RaidLevel, Level)
    friend class RaidLevelCardTests;

  private:
    enum class TurnState
    {
        PlayerPlanning,
        PlayerMoving,
        PlayerCards,
        Boss,
        Victory,
        Defeat
    };

  public:
    using OnBattleEnded = std::function<void(const BattleResult&)>;
    explicit RaidLevel(OnBattleEnded onBattleEnded = {});
    virtual ~RaidLevel() = default;

    virtual void OnInitialized() override;
    virtual void Tick(float deltaTime) override;
    virtual void Draw() override;

  private:
    void ProcessMovementInput();
    void ProcessTurn(float deltaTime);
    void BeginPlayerTurn();
    void BeginPlayerMovement();
    void BeginPlayerCards();
    void ProcessPlayerCards(float deltaTime);
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
    void DrawCardTargets() const;
    void DrawCardEffect() const;

    bool ProcessCardInput();
    bool TryReserveSelectedCard(const std::shared_ptr<Actor>& target, const std::optional<Vector2>& position = std::nullopt);
    std::shared_ptr<Player> FindCardOwner(int cardId) const;
    std::shared_ptr<Actor> FindEnemyAt(const Vector2& position) const;
    bool IsValidCardTarget(const Card& card, const std::shared_ptr<Actor>& target) const;
    bool CanUseCard(const Card& card, const Player& caster, const std::shared_ptr<Actor>& target,
                    const std::optional<Vector2>& position, bool planning) const;
    bool ApplyCardEffect(const Card& card, Player& caster, const std::shared_ptr<Actor>& target,
                         const std::optional<Vector2>& position);
    bool IsAdjacent(const Vector2& first, const Vector2& second) const;
    Vector2 GetPlannedPlayerPosition(const Player& player) const;
    bool IsOnReservedRoute(const Vector2& position, const Player* ignorePlayer) const;
    bool CanPlaceCardAt(const Vector2& position, const Player& caster) const;
    std::shared_ptr<Player> FindPlayerChoiceAt(const Vector2& position) const;
    void HandleCardTargetClick(const Vector2& position);
    void ResetCardTargeting();
    std::wstring GetTargetingHint(const Card& card) const;
    std::wstring GetCardTargetLabel(const Player& player) const;
    void ExecuteTurretAttacks();
    void CompleteTurnEffects();
    void SyncTemporaryObstacles();
    int GetTurretCount(const Player* ignoreReservationOwner = nullptr, bool includeReservations = false) const;
    int FindCardSlotAt(const Vector2& position) const;
    Vector2 GetCardSlotPosition(int index) const;
    void ClearMovementPreview();
    bool CheckBattleEnd();

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
    std::vector<Vector2> BuildPlayerMovePath(const Player& player, const Vector2& destination) const;

    bool IsAnyPlayerMoving() const;
    bool IsAnyMinionMoving() const;
    int GetActiveMinionCount() const;

  private:
    std::unique_ptr<RaidMap> raidMap;
    OnBattleEnded onBattleEnded;

    std::unique_ptr<Button> turnEndButton;

    std::vector<std::shared_ptr<Player>> players;
    std::vector<std::shared_ptr<Minion>> minions;
    std::vector<std::shared_ptr<Deployable>> deployables;

    std::shared_ptr<Boss> boss;

    std::shared_ptr<Player> selectedPlayer;

    Vector2 targetPosition = Vector2::Zero;

    Vector2 previewStartPosition = Vector2::Zero;

    bool hasPreviewTarget = false;

    std::vector<Vector2> previewPath;

    CardHand cardHand;
    int selectedCardIndex = 0;
    bool isSelectingCardTarget = false;
    std::weak_ptr<Actor> pendingCardTarget;
    std::wstring cardMessage;

    int nextCardPlayerIndex = 0;
    bool isCardInFlight = false;
    float cardEffectTimer = 0.0f;
    Card activeCard;
    std::weak_ptr<Actor> activeCardTarget;
    std::weak_ptr<Player> activeCardCaster;
    std::optional<Vector2> activeCardPosition;
    Vector2 cardEffectStart = Vector2::Zero;
    Vector2 cardEffectEnd = Vector2::Zero;

    static constexpr int CardSlotWidth = 16;
    static constexpr int CardSlotHeight = 5;
    static constexpr int CardSlotSpacing = 17;
    static constexpr float CardEffectDuration = 0.3f;

    TurnState turnState = TurnState::PlayerPlanning;
    float bossTurnTimer = 0.0f;
    bool hasBossAttackExecuted = false;
    bool hasTurretsAttacked = false;
    bool hasMinionActionsStarted = false;
    bool hasMinionAttacksExecuted = false;

    Boss::ActionType lastNormalBossAction = Boss::ActionType::None;
    bool hasUsedSpecialAt60 = false;
    bool hasUsedSpecialAt30 = false;

    static constexpr float BossTurnDuration = 0.35f;
    static constexpr int BossLaserDirectionCount = 4;
    static constexpr int BossNearbyRadius = 2;
    static constexpr int BossConeRange = 8;
    static constexpr int BossLaserDamage = 3;
    static constexpr int BossNearbyDamage = 3;
    static constexpr int BossConeDamage = 4;
    static constexpr int BossTeleportDamage = 3;
    static constexpr int BossSpecialDamage = 7;
    static constexpr int MinionChargeDamage = 2;
};
