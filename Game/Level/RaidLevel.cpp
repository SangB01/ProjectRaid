#include "RaidLevel.h"
#include <Game/Game.h>
#include <Math/AStar.h>
#include <Engine/Engine.h>
#include <Input/Input.h>
#include <Render/Renderer.h>
#include <Util/Util.h>
#include <Windows.h>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <iostream>
#include <utility>
#include <array>

using namespace Craft;

RaidLevel::RaidLevel(OnBattleEnded onBattleEnded) : onBattleEnded(std::move(onBattleEnded))
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
	SpawnBoss();

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
	if (CheckBattleEnd())
	{
		return;
	}

	if (Input::Get().GetKeyDown(VK_ESCAPE))
	{
		Game& game = dynamic_cast<Game&>(Engine::Get());
		game.ToggleMenu();
		return;
	}

	elapsedTime += deltaTime;

	super::Tick(deltaTime);

	if (!raidMap)
	{
		return;
	}

	ResolveBossMovementHit();
	ResolveMinionMovementBlock();
	ProcessTurn(deltaTime);
}

void RaidLevel::ProcessTurn(float deltaTime)
{
	if (CheckBattleEnd())
	{
		return;
	}

	if (turnState == TurnState::PlayerCards)
	{
		ProcessPlayerCards(deltaTime);
		return;
	}

	if (turnState == TurnState::Boss)
	{
		if (boss && boss->HasPath())
		{
			return;
		}

		const bool hasBlinkingWarning = boss &&
			boss->GetPlannedActionType() != Boss::ActionType::None &&
			boss->GetPlannedActionType() != Boss::ActionType::SummonMinions &&
			!boss->GetAttackWarningPositions().empty();

		if (hasBlinkingWarning && !hasBossWarningBlinked)
		{
			bossWarningBlinkTimer += deltaTime;

			if (bossWarningBlinkTimer < BossWarningBlinkDuration)
			{
				return;
			}

			hasBossWarningBlinked = true;
		}

		ExecuteBossAttack();

		if (CheckBattleEnd())
		{
			return;
		}

		ExecuteTurretAttacks();
		if (CheckBattleEnd())
		{
			return;
		}

		if (!hasMinionActionsStarted)
		{
			ExecuteMinionActions();
			hasMinionActionsStarted = true;
		}

		if (IsAnyMinionMoving())
		{
			return;
		}

		ExecuteMinionAttacks();

		if (CheckBattleEnd())
		{
			return;
		}
		bossTurnTimer += deltaTime;

		if (bossTurnTimer >= BossTurnDuration)
		{
			CompleteTurnEffects();
			BeginPlayerTurn();
		}

		return;
	}

	if (turnState == TurnState::PlayerMoving)
	{
		if (!IsAnyPlayerMoving())
		{
			BeginPlayerCards();
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
			BeginPlayerCards();
		}

		return;
	}

	if (!ProcessCardInput())
	{
		ProcessMovementInput();
	}
}

void RaidLevel::BeginPlayerTurn()
{
	turnState = TurnState::PlayerPlanning;
	bossTurnTimer = 0.0f;
	hasBossAttackExecuted = false;
	hasTurretsAttacked = false;
	hasMinionActionsStarted = false;
	hasMinionAttacksExecuted = false;
	ResetCardTargeting();
	ClearMovementPreview();

	for (const std::shared_ptr<Player>& player : players)
	{
		if (player)
		{
			player->ClearReservedCard();
		}
	}

	cardHand.DrawCards();
	selectedCardIndex = 0;

	minions.erase(std::remove_if(minions.begin(), minions.end(), [](const std::shared_ptr<Minion>& minion) {
		return !minion || !minion->IsActive();
		}),
		minions.end());

	if (turnEndButton)
	{
		turnEndButton->SetEnabled(true);
	}

	PlanBossAction();
	PlanMinionActions();
}

void RaidLevel::BeginPlayerMovement()
{
	turnState = TurnState::PlayerMoving;
	ResetCardTargeting();

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
	bossWarningBlinkTimer = 0.0f;
	hasBossAttackExecuted = false;
	hasBossWarningBlinked = false;
	hasTurretsAttacked = false;
	hasMinionActionsStarted = false;
	hasMinionAttacksExecuted = false;

	previewPath.clear();
	hasPreviewTarget = false;

	if (turnEndButton)
	{
		turnEndButton->SetEnabled(false);
	}

	ExecuteBossAction();
}

void RaidLevel::ExecuteReservedMoves()
{
	for (const std::shared_ptr<Player>& player : players)
	{
		if (player && player->IsActive())
		{
			player->ExecuteReservedPath();
		}
	}
}

void RaidLevel::PlanBossAction()
{
	if (!raidMap || !boss || !boss->IsActive())
	{
		return;
	}

	boss->ClearPlannedAction();

	const bool shouldUseSpecialAt60 = boss->GetHealth() <= 60 && !hasUsedSpecialAt60;
	const bool shouldUseSpecialAt30 = boss->GetHealth() <= 30 && hasUsedSpecialAt60 && !hasUsedSpecialAt30;

	if (shouldUseSpecialAt60 || shouldUseSpecialAt30)
	{
		Vector2 specialDestination = boss->GetPosition();
		std::vector<Vector2> specialPath;

		if (FindSpecialDestination(specialDestination) && specialDestination != boss->GetPosition())
		{
			specialPath = AStar::FindPath(*raidMap, boss->GetPosition(), specialDestination,
				BuildBlockedPositions(boss.get()));

			if (specialPath.empty())
			{
				specialDestination = boss->GetPosition();
			}
		}

		boss->PlanAction(Boss::ActionType::SpecialAttack, specialPath, specialDestination,
			BuildSpecialAttackPositions(specialDestination));

		if (shouldUseSpecialAt60)
		{
			hasUsedSpecialAt60 = true;
		}
		else
		{
			hasUsedSpecialAt30 = true;
		}

		return;
	}

	const Boss::ActionType actionType = ChooseNormalBossAction();

	if (actionType == Boss::ActionType::None)
	{
		return;
	}

	if (actionType == Boss::ActionType::TeleportAttack)
	{
		std::vector<std::shared_ptr<Player>> livingPlayers;

		for (const std::shared_ptr<Player>& player : players)
		{
			if (player && player->IsActive())
			{
				livingPlayers.emplace_back(player);
			}
		}

		if (livingPlayers.empty())
		{
			return;
		}

		const int targetIndex = Util::RandomRange(0, static_cast<int>(livingPlayers.size()) - 1);
		Vector2 teleportDestination = boss->GetPosition();

		FindTeleportDestination(*livingPlayers[targetIndex], teleportDestination);

		boss->PlanAction(actionType, {}, teleportDestination, BuildNearbyAttackPositions(teleportDestination));
		lastNormalBossAction = actionType;
		return;
	}

	std::vector<Vector2> movePath;
	Vector2 plannedDestination = boss->GetPosition();
	std::shared_ptr<Player> targetPlayer;

	if (!FindBossMovePlan(movePath, plannedDestination, targetPlayer))
	{
		return;
	}

	std::vector<Vector2> warningPositions;

	if (actionType == Boss::ActionType::LaserAttack)
	{
		warningPositions = BuildLaserAttackPositions(plannedDestination);
	}
	else if (actionType == Boss::ActionType::NearbyAttack)
	{
		warningPositions = BuildNearbyAttackPositions(plannedDestination);
	}
	else if (actionType == Boss::ActionType::ConeAttack && targetPlayer)
	{
		warningPositions = BuildConeAttackPositions(plannedDestination, targetPlayer->GetPosition());
	}
	else if (actionType == Boss::ActionType::SummonMinions)
	{
		warningPositions = BuildSummonPositions(3, plannedDestination);
	}

	boss->PlanAction(actionType, movePath, plannedDestination, warningPositions);
	lastNormalBossAction = actionType;
}

void RaidLevel::PlanMinionActions()
{
	if (!raidMap)
	{
		return;
	}

	for (const std::shared_ptr<Minion>& minion : minions)
	{
		if (!minion || !minion->IsActive())
		{
			continue;
		}

		std::shared_ptr<Player> closestPlayer;
		int closestDistance = INT_MAX;

		for (const std::shared_ptr<Player>& player : players)
		{
			if (!player || !player->IsActive())
			{
				continue;
			}

			const int distanceX = std::abs(player->GetPosition().x - minion->GetPosition().x);
			const int distanceY = std::abs(player->GetPosition().y - minion->GetPosition().y);
			const int distance = distanceX + distanceY;

			if (distance < closestDistance)
			{
				closestDistance = distance;
				closestPlayer = player;
			}
		}

		if (!closestPlayer)
		{
			minion->ClearCharge();
			continue;
		}

		const Vector2 difference = closestPlayer->GetPosition() - minion->GetPosition();
		const int absoluteX = std::abs(difference.x);
		const int absoluteY = std::abs(difference.y);
		Vector2 direction(difference.x == 0 ? 0 : (difference.x > 0 ? 1 : -1),
			difference.y == 0 ? 0 : (difference.y > 0 ? 1 : -1));

		// 22.5도 경계로 가장 가까운 8방향을 선택하고 돌진 경로 전체에 고정한다.
		constexpr double diagonalThreshold = 0.414213562373095;

		if (absoluteY < absoluteX * diagonalThreshold)
		{
			direction.y = 0;
		}
		else if (absoluteX < absoluteY* diagonalThreshold)
		{
			direction.x = 0;
		}

		const int maximumSteps = absoluteX > absoluteY ? absoluteX : absoluteY;
		std::vector<Vector2> movePath;
		std::vector<Vector2> warningPositions;
		const std::vector<Vector2> blockedPositions = BuildBlockedPositions(minion.get());
		Vector2 currentPosition = minion->GetPosition();

		for (int step = 0; step < maximumSteps; ++step)
		{
			const Vector2 nextPosition = currentPosition + direction;

			if (!raidMap->IsWalkable(nextPosition))
			{
				break;
			}

			if (direction.x != 0 && direction.y != 0)
			{
				const Vector2 horizontalPosition = currentPosition + Vector2(direction.x, 0);
				const Vector2 verticalPosition = currentPosition + Vector2(0, direction.y);

				if (!raidMap->IsWalkable(horizontalPosition) || !raidMap->IsWalkable(verticalPosition))
				{
					break;
				}
			}

			warningPositions.emplace_back(nextPosition);

			if (FindPlayerAt(nextPosition) || ContainsPosition(blockedPositions, nextPosition))
			{
				break;
			}

			movePath.emplace_back(nextPosition);
			currentPosition = nextPosition;
		}

		minion->ReserveCharge(movePath, warningPositions);
	}
}

void RaidLevel::ExecuteBossAction()
{
	if (!boss || !boss->IsActive())
	{
		return;
	}

	if (boss->GetPlannedActionType() == Boss::ActionType::TeleportAttack)
	{
		const Vector2 destination = boss->GetPlannedDestination();

		if (!IsOccupiedByActor(destination, boss.get()))
		{
			boss->SetPosition(destination);
		}

		return;
	}

	if (boss->HasReservedPath())
	{
		boss->ExecuteReservedPath();
	}
}

void RaidLevel::ExecuteBossAttack()
{
	if (hasBossAttackExecuted || !boss || !boss->IsActive() ||
		boss->GetPlannedActionType() == Boss::ActionType::None)
	{
		return;
	}

	hasBossAttackExecuted = true;
	const Boss::ActionType actionType = boss->GetPlannedActionType();

	if (actionType == Boss::ActionType::SummonMinions)
	{
		SpawnMinions(boss->GetAttackWarningPositions());
		return;
	}

	int damage = 0;

	if (actionType == Boss::ActionType::LaserAttack)
	{
		damage = BossLaserDamage;
	}
	else if (actionType == Boss::ActionType::NearbyAttack)
	{
		damage = BossNearbyDamage;
	}
	else if (actionType == Boss::ActionType::ConeAttack)
	{
		damage = BossConeDamage;
	}
	else if (actionType == Boss::ActionType::TeleportAttack)
	{
		damage = BossTeleportDamage;
	}
	else if (actionType == Boss::ActionType::SpecialAttack)
	{
		damage = BossSpecialDamage;
	}

	for (const std::shared_ptr<Player>& player : players)
	{
		if (!player || !player->IsActive() ||
			!ContainsPosition(boss->GetAttackWarningPositions(), player->GetPosition()) ||
			!HasLineOfSight(boss->GetPlannedDestination(), player->GetPosition()))
		{
			continue;
		}

		player->TakeDamage(damage);

		if (player->IsDead() && selectedPlayer == player)
		{
			selectedPlayer.reset();
			previewPath.clear();
			hasPreviewTarget = false;
		}
	}
}

void RaidLevel::ExecuteMinionActions()
{
	for (const std::shared_ptr<Minion>& minion : minions)
	{
		if (minion && minion->IsActive() && minion->WasPlannedThisTurn())
		{
			minion->ExecuteReservedCharge();
		}
	}
}

void RaidLevel::ExecuteMinionAttacks()
{
	if (hasMinionAttacksExecuted)
	{
		return;
	}

	hasMinionAttacksExecuted = true;

	for (const std::shared_ptr<Minion>& minion : minions)
	{
		if (!minion || !minion->IsActive() || !minion->WasPlannedThisTurn())
		{
			continue;
		}

		for (const std::shared_ptr<Player>& player : players)
		{
			if (!player || !player->IsActive() ||
				!ContainsPosition(minion->GetWarningPositions(), player->GetPosition()))
			{
				continue;
			}

			player->TakeDamage(MinionChargeDamage);

			if (player->IsDead() && selectedPlayer == player)
			{
				selectedPlayer.reset();
				previewPath.clear();
				hasPreviewTarget = false;
			}
		}

		minion->CompleteTurn();
	}
}

void RaidLevel::ResolveBossMovementHit()
{
	if (turnState != TurnState::Boss || !boss || boss->GetPosition() == boss->GetPreviousPosition())
	{
		return;
	}

	for (const std::shared_ptr<Player>& player : players)
	{
		if (!player || !player->IsActive() || player->GetPosition() != boss->GetPosition())
		{
			continue;
		}

		player->TakeDamage(1);

		if (player->IsDead() && selectedPlayer == player)
		{
			selectedPlayer.reset();
			previewPath.clear();
			hasPreviewTarget = false;
		}
	}
}

void RaidLevel::ResolveMinionMovementBlock()
{
	if (turnState != TurnState::Boss)
	{
		return;
	}

	for (const std::shared_ptr<Minion>& minion : minions)
	{
		if (!minion || !minion->IsActive() || minion->GetPosition() == minion->GetPreviousPosition())
		{
			continue;
		}

		if (FindPlayerAt(minion->GetPosition()))
		{
			minion->SetPosition(minion->GetPreviousPosition());
			minion->StopMovement();
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
	DrawBossPlannedPath();
	DrawBossAttackWarning();
	DrawMinionWarnings();
	DrawReservedPaths();
	DrawPathPreview();
	super::Draw();
	DrawCardTargets();
	DrawCardEffect();
	DrawTargetCursor();
	Interface();
	CardArea();
}

void RaidLevel::SpawnPlayers()
{
	players.clear();
	/* (25, 13), (36, 27), (94, 20), (77, 5) 이걸 어떻게 두느냐*/
	const Vector2 mapPosition = raidMap->GetPosition();

	const std::array<Vector2, 4> startPosition =
	{
		Vector2(25,13), Vector2(36,27), Vector2(94,27), Vector2(77,5)
	};

	for (int ix = 0; ix < startPosition.size(); ++ix)
	{
		const int playerIndex = static_cast<int>(ix) + 1; 
		const Vector2 spawnPosition = mapPosition + startPosition[ix];

		if (!raidMap->IsWalkable(spawnPosition))
		{
			continue;
		}

		if (IsOccupiedByActor(spawnPosition))
		{
			continue;
		}

		std::shared_ptr<Player> newPlayer = SpawnActor<Player>(playerIndex, spawnPosition);

		if (!newPlayer)
		{
			continue;
		}

		players.emplace_back(newPlayer);

		if (players.size() >= 4)
		{
			SelectPlayer(players[0]);
			return;
		}
	}
}

void RaidLevel::SpawnBoss()
{
	Vector2 spawnPosition;

	if (!FindBossSpawnPosition(spawnPosition))
	{
		std::cout << "Boss Spawn Failed\n";
		return;
	}

	boss = SpawnActor<Boss>(spawnPosition);
}

void RaidLevel::SpawnMinions(const std::vector<Vector2>& spawnPositions)
{
	int availableCount = 4 - GetActiveMinionCount();

	for (const Vector2& spawnPosition : spawnPositions)
	{
		if (availableCount <= 0)
		{
			return;
		}

		if (!raidMap->IsWalkable(spawnPosition) || IsOccupiedByActor(spawnPosition))
		{
			continue;
		}

		minions.emplace_back(SpawnActor<Minion>(spawnPosition));
		--availableCount;
	}
}

bool RaidLevel::FindBossSpawnPosition(Vector2& outPosition) const
{
	if (!raidMap)
	{
		return false;
	}

	const Vector2 mapPosition = raidMap->GetPosition();
	const Vector2 mapCenter(mapPosition.x + raidMap->GetWidth() / 2, mapPosition.y + raidMap->GetHeight() / 2);

	int nearestDistance = INT_MAX;
	bool foundPosition = false;

	for (int y = 0; y < raidMap->GetHeight(); ++y)
	{
		for (int x = 0; x < raidMap->GetWidth(); ++x)
		{
			const Vector2 candidate(mapPosition.x + x, mapPosition.y + y);

			if (!raidMap->IsWalkable(candidate) || IsOccupiedByActor(candidate))
			{
				continue;
			}

			const int distance = std::abs(candidate.x - mapCenter.x) + std::abs(candidate.y - mapCenter.y);

			if (distance >= nearestDistance)
			{
				continue;
			}

			nearestDistance = distance;
			outPosition = candidate;
			foundPosition = true;
		}
	}

	return foundPosition;
}

bool RaidLevel::FindBossMovePlan(std::vector<Vector2>& outPath, Vector2& outDestination,
	std::shared_ptr<Player>& outTargetPlayer) const
{
	if (!raidMap || !boss)
	{
		return false;
	}

	const Vector2 bossPosition = boss->GetPosition();
	const Vector2 directions[] = { Vector2(0, -1), Vector2(0, 1),  Vector2(-1, 0), Vector2(1, 0),
								  Vector2(-1, -1), Vector2(1, -1), Vector2(-1, 1), Vector2(1, 1) };
	const std::vector<Vector2> blockedPositions = BuildBlockedPositions(boss.get());

	int shortestPathLength = INT_MAX;
	bool foundDestination = false;

	for (const std::shared_ptr<Player>& player : players)
	{
		if (!player || !player->IsActive())
		{
			continue;
		}

		const Vector2 playerPosition = player->GetPosition();
		const int distanceX = std::abs(playerPosition.x - bossPosition.x);
		const int distanceY = std::abs(playerPosition.y - bossPosition.y);

		if (distanceX <= 1 && distanceY <= 1)
		{
			outPath.clear();
			outDestination = bossPosition;
			outTargetPlayer = player;
			return true;
		}

		for (const Vector2& direction : directions)
		{
			const Vector2 destination = playerPosition + direction;

			if (!raidMap->IsWalkable(destination) || IsOccupiedByActor(destination, boss.get()))
			{
				continue;
			}

			std::vector<Vector2> path = AStar::FindPath(*raidMap, bossPosition, destination, blockedPositions);

			if (path.empty() || static_cast<int>(path.size()) >= shortestPathLength)
			{
				continue;
			}

			shortestPathLength = static_cast<int>(path.size());
			outPath = path;
			outDestination = destination;
			outTargetPlayer = player;
			foundDestination = true;
		}
	}

	return foundDestination;
}

bool RaidLevel::FindTeleportDestination(const Player& targetPlayer, Vector2& outDestination) const
{
	if (!raidMap || !boss)
	{
		return false;
	}

	std::vector<Vector2> candidates;

	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			if (x == 0 && y == 0)
			{
				continue;
			}

			const Vector2 candidate = targetPlayer.GetPosition() + Vector2(x, y);

			if (raidMap->IsWalkable(candidate) && !IsOccupiedByActor(candidate, boss.get()))
			{
				candidates.emplace_back(candidate);
			}
		}
	}

	if (candidates.empty())
	{
		return false;
	}

	std::shuffle(candidates.begin(), candidates.end(), Util::GetRandomEngine());
	outDestination = candidates.front();
	return true;
}

bool RaidLevel::FindSpecialDestination(Vector2& outDestination) const
{
	if (!raidMap || !boss)
	{
		return false;
	}

	const Vector2 mapPosition = raidMap->GetPosition();
	const Vector2 mapCenter(mapPosition.x + raidMap->GetWidth() / 2, mapPosition.y + raidMap->GetHeight() / 2);
	int shortestDistance = INT_MAX;
	bool foundDestination = false;

	for (int y = 0; y < raidMap->GetHeight(); ++y)
	{
		for (int x = 0; x < raidMap->GetWidth(); ++x)
		{
			const Vector2 candidate(mapPosition.x + x, mapPosition.y + y);

			if (!raidMap->IsWalkable(candidate) || IsOccupiedByActor(candidate, boss.get()))
			{
				continue;
			}

			const int distance = std::abs(candidate.x - mapCenter.x) + std::abs(candidate.y - mapCenter.y);

			if (distance >= shortestDistance)
			{
				continue;
			}

			shortestDistance = distance;
			outDestination = candidate;
			foundDestination = true;
		}
	}

	return foundDestination;
}

Boss::ActionType RaidLevel::ChooseNormalBossAction() const
{
	std::vector<Boss::ActionType> candidates = { Boss::ActionType::LaserAttack, Boss::ActionType::NearbyAttack,
												 Boss::ActionType::ConeAttack, Boss::ActionType::TeleportAttack };

	if (GetActiveMinionCount() < 2)
	{
		candidates.emplace_back(Boss::ActionType::SummonMinions);
	}

	candidates.erase(std::remove(candidates.begin(), candidates.end(), lastNormalBossAction), candidates.end());

	if (candidates.empty())
	{
		return Boss::ActionType::None;
	}

	return candidates[Util::RandomRange(0, static_cast<int>(candidates.size()) - 1)];
}

std::vector<Vector2> RaidLevel::BuildLaserAttackPositions(const Vector2& center) const
{
	std::vector<Vector2> attackPositions;

	if (!raidMap)
	{
		return attackPositions;
	}

	std::vector<Vector2> directions = { Vector2(0, -1), Vector2(0, 1),  Vector2(-1, 0), Vector2(1, 0),
									   Vector2(-1, -1), Vector2(1, -1), Vector2(-1, 1), Vector2(1, 1) };
	std::shuffle(directions.begin(), directions.end(), Util::GetRandomEngine());

	for (int directionIndex = 0; directionIndex < BossLaserDirectionCount; ++directionIndex)
	{
		Vector2 currentPosition = center;

		while (true)
		{
			currentPosition = currentPosition + directions[directionIndex];

			if (!raidMap->IsWalkable(currentPosition))
			{
				break;
			}

			attackPositions.emplace_back(currentPosition);
		}
	}

	return attackPositions;
}

std::vector<Vector2> RaidLevel::BuildNearbyAttackPositions(const Vector2& center) const
{
	std::vector<Vector2> attackPositions;

	if (!raidMap)
	{
		return attackPositions;
	}

	for (int y = -BossNearbyRadius; y <= BossNearbyRadius; ++y)
	{
		for (int x = -BossNearbyRadius; x <= BossNearbyRadius; ++x)
		{
			if (x == 0 && y == 0)
			{
				continue;
			}

			const Vector2 attackPosition = center + Vector2(x, y);

			if (raidMap->IsWalkable(attackPosition))
			{
				attackPositions.emplace_back(attackPosition);
			}
		}
	}

	return attackPositions;
}

std::vector<Vector2> RaidLevel::BuildConeAttackPositions(const Vector2& center, const Vector2& target) const
{
	std::vector<Vector2> attackPositions;

	if (!raidMap)
	{
		return attackPositions;
	}

	const Vector2 difference = target - center;
	Vector2 forward;

	if (std::abs(difference.x) >= std::abs(difference.y))
	{
		forward = Vector2(difference.x >= 0 ? 1 : -1, 0);
	}
	else
	{
		forward = Vector2(0, difference.y >= 0 ? 1 : -1);
	}

	const Vector2 lateral(-forward.y, forward.x);

	for (int depth = 1; depth <= BossConeRange; ++depth)
	{
		const int halfWidth = (depth - 1) / 2;

		for (int offset = -halfWidth; offset <= halfWidth; ++offset)
		{
			const Vector2 attackPosition = center + Vector2(forward.x * depth + lateral.x * offset,
				forward.y * depth + lateral.y * offset);

			if (raidMap->IsWalkable(attackPosition))
			{
				attackPositions.emplace_back(attackPosition);
			}
		}
	}

	return attackPositions;
}

std::vector<Vector2> RaidLevel::BuildSummonPositions(int count, const Vector2& bossDestination) const
{
	std::vector<Vector2> candidates;

	if (!raidMap || count <= 0)
	{
		return candidates;
	}

	const Vector2 mapPosition = raidMap->GetPosition();

	for (int y = 0; y < raidMap->GetHeight(); ++y)
	{
		for (int x = 0; x < raidMap->GetWidth(); ++x)
		{
			const Vector2 candidate(mapPosition.x + x, mapPosition.y + y);

			if (candidate != bossDestination && raidMap->IsWalkable(candidate) && !IsOccupiedByActor(candidate))
			{
				candidates.emplace_back(candidate);
			}
		}
	}

	std::shuffle(candidates.begin(), candidates.end(), Util::GetRandomEngine());

	if (static_cast<int>(candidates.size()) > count)
	{
		candidates.resize(count);
	}

	return candidates;
}

std::vector<Vector2> RaidLevel::BuildSpecialAttackPositions(const Vector2& center) const
{
	std::vector<Vector2> attackPositions;

	if (!raidMap)
	{
		return attackPositions;
	}

	const Vector2 mapPosition = raidMap->GetPosition();

	for (int y = 0; y < raidMap->GetHeight(); ++y)
	{
		for (int x = 0; x < raidMap->GetWidth(); ++x)
		{
			const Vector2 candidate(mapPosition.x + x, mapPosition.y + y);

			if (candidate != center && raidMap->IsWalkable(candidate) && HasLineOfSight(center, candidate))
			{
				attackPositions.emplace_back(candidate);
			}
		}
	}

	return attackPositions;
}

bool RaidLevel::HasLineOfSight(const Vector2& start, const Vector2& end) const
{
	if (!raidMap || !raidMap->IsInside(start) || !raidMap->IsInside(end))
	{
		return false;
	}

	int x = start.x;
	int y = start.y;
	const int distanceX = std::abs(end.x - start.x);
	const int distanceY = std::abs(end.y - start.y);
	const int stepX = start.x < end.x ? 1 : -1;
	const int stepY = start.y < end.y ? 1 : -1;
	int error = distanceX - distanceY;

	while (x != end.x || y != end.y)
	{
		const Vector2 previous(x, y);
		const int doubledError = error * 2;

		if (doubledError > -distanceY)
		{
			error -= distanceY;
			x += stepX;
		}

		if (doubledError < distanceX)
		{
			error += distanceX;
			y += stepY;
		}

		const Vector2 currentPosition(x, y);

		if (previous.x != x && previous.y != y &&
			(!raidMap->IsWalkable(Vector2(previous.x, y)) || !raidMap->IsWalkable(Vector2(x, previous.y))))
		{
			return false;
		}

		if (currentPosition != end && !raidMap->IsWalkable(currentPosition))
		{
			return false;
		}
	}

	return true;
}

bool RaidLevel::ContainsPosition(const std::vector<Vector2>& positions, const Vector2& position) const
{
	return std::find(positions.begin(), positions.end(), position) != positions.end();
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
		previewPath = BuildPlayerMovePath(*selectedPlayer, targetPosition);
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
	if (!player || !player->IsActive())
	{
		return;
	}

	if (selectedPlayer)
	{
		selectedPlayer->SetSelected(false);
	}

	selectedPlayer = player;
	selectedPlayer->SetSelected(true);
	ResetCardTargeting();

	const std::vector<Card>& cards = cardHand.GetCards();

	for (int index = 0; index < static_cast<int>(cards.size()); ++index)
	{
		if (cards[index].id == player->GetReservedCardId())
		{
			selectedCardIndex = index;
			break;
		}
	}

	targetPosition = selectedPlayer->GetPosition();

	previewStartPosition = targetPosition;
	hasPreviewTarget = false;
	previewPath.clear();
}

std::shared_ptr<Player> RaidLevel::FindPlayerAt(const Vector2& position) const
{
	for (const std::shared_ptr<Player>& player : players)
	{
		if (!player || !player->IsActive())
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

bool RaidLevel::IsOccupiedByActor(const Vector2& position, const Actor* ignoreActor) const
{
	for (const auto& object : deployables)
	{
		if (object && object->IsActive() && object.get() != ignoreActor && object->GetPosition() == position)
		{
			return true;
		}
	}

	if (boss && boss->IsActive() && boss.get() != ignoreActor && boss->GetPosition() == position)
	{
		return true;
	}

	for (const std::shared_ptr<Player>& player : players)
	{
		if (!player || !player->IsActive())
		{
			continue;
		}

		if (player.get() == ignoreActor)
		{
			continue;
		}

		if (player->GetPosition() == position)
		{
			return true;
		}
	}

	for (const std::shared_ptr<Minion>& minion : minions)
	{
		if (minion && minion->IsActive() && minion.get() != ignoreActor && minion->GetPosition() == position)
		{
			return true;
		}
	}

	return false;
}

std::vector<Vector2> RaidLevel::BuildBlockedPositions(const Actor* ignoreActor) const
{
	std::vector<Vector2> blockedPositions;
	const bool playerMovement = dynamic_cast<const Player*>(ignoreActor) != nullptr;

	for (const auto& object : deployables)
	{
		if (object && object->IsActive() && object.get() != ignoreActor)
		{
			blockedPositions.emplace_back(object->GetPosition());
		}
	}

	if (boss && boss->IsActive() && boss.get() != ignoreActor)
	{
		blockedPositions.emplace_back(boss->GetPosition());

		if (boss->HasReservedPath())
		{
			blockedPositions.emplace_back(boss->GetReservedPath().back());
		}
		else if (boss->GetPlannedDestination() != boss->GetPosition())
		{
			blockedPositions.emplace_back(boss->GetPlannedDestination());
		}

		if (boss->GetPlannedActionType() == Boss::ActionType::SummonMinions)
		{
			for (const Vector2& spawnPosition : boss->GetAttackWarningPositions())
			{
				blockedPositions.emplace_back(spawnPosition);
			}
		}
	}

	for (const std::shared_ptr<Player>& player : players)
	{
		if (!player || !player->IsActive())
		{
			continue;
		}

		if (player.get() == ignoreActor)
		{
			continue;
		}

		if (player->GetReservedCardPosition())
		{
			blockedPositions.emplace_back(*player->GetReservedCardPosition());
		}

		if (playerMovement)
		{
			continue;
		}

		blockedPositions.emplace_back(player->GetPosition());

		for (const Vector2& reservedPosition : player->GetReservedPath())
		{
			blockedPositions.emplace_back(reservedPosition);
		}
	}

	for (const std::shared_ptr<Minion>& minion : minions)
	{
		if (!minion || !minion->IsActive() || minion.get() == ignoreActor)
		{
			continue;
		}

		blockedPositions.emplace_back(minion->GetPosition());

		for (const Vector2& reservedPosition : minion->GetReservedPath())
		{
			blockedPositions.emplace_back(reservedPosition);
		}
	}

	return blockedPositions;
}

std::vector<Vector2> RaidLevel::BuildPlayerMovePath(const Player& player, const Vector2& destination) const
{
	if (!raidMap || !player.IsActive() || !raidMap->IsWalkable(destination) ||
		player.GetPosition() == destination || IsOccupiedByActor(destination, &player))
	{
		return {};
	}

	for (const auto& other : players)
	{
		if (other && other->IsActive() && other.get() != &player &&
			other->HasReservedPath() && other->GetReservedPath().back() == destination)
		{
			return {};
		}
	}

	auto path = AStar::FindPath(*raidMap, player.GetPosition(), destination, BuildBlockedPositions(&player));
	return static_cast<int>(path.size()) <= Player::MaxMoveDistance ? path : std::vector<Vector2>{};
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

bool RaidLevel::IsAnyMinionMoving() const
{
	for (const std::shared_ptr<Minion>& minion : minions)
	{
		if (minion && minion->IsActive() && minion->HasPath())
		{
			return true;
		}
	}

	return false;
}

int RaidLevel::GetActiveMinionCount() const
{
	int count = 0;

	for (const std::shared_ptr<Minion>& minion : minions)
	{
		if (minion && minion->IsActive())
		{
			++count;
		}
	}

	return count;
}

void RaidLevel::DrawBossPlannedPath() const
{
	if (!boss || !boss->IsActive() || !boss->HasReservedPath())
	{
		return;
	}

	const std::vector<Vector2>& plannedPath = boss->GetReservedPath();

	for (const Vector2& pathPosition : plannedPath)
	{
		Renderer::Get().Submit(L"·", pathPosition, Color::Red, 16);
	}

	Renderer::Get().Submit(L"X", plannedPath.back(), Color::Red, 17);
}

void RaidLevel::DrawBossAttackWarning() const
{
	if (!boss || !boss->IsActive() || boss->GetPlannedActionType() == Boss::ActionType::None)
	{
		return;
	}

	const bool isSummonAction = boss->GetPlannedActionType() == Boss::ActionType::SummonMinions;

	if (turnState == TurnState::Boss && !isSummonAction && !boss->HasPath() &&
		!hasBossWarningBlinked && bossWarningBlinkTimer < BossWarningBlinkHiddenDuration)
	{
		return;
	}

	const wchar_t* warningImage = isSummonAction ? L"S" : L"!";
	const Color warningColor = isSummonAction ? Color::Purple : Color::Yellow;

	for (const Vector2& attackPosition : boss->GetAttackWarningPositions())
	{
		Renderer::Get().Submit(warningImage, attackPosition, warningColor, 15);
	}
}

void RaidLevel::DrawMinionWarnings() const
{
	for (const std::shared_ptr<Minion>& minion : minions)
	{
		if (!minion || !minion->IsActive())
		{
			continue;
		}

		for (const Vector2& warningPosition : minion->GetWarningPositions())
		{
			Renderer::Get().Submit(L">", warningPosition, Color::Purple, 16);
		}
	}
}

void RaidLevel::DrawReservedPaths() const
{
	for (const std::shared_ptr<Player>& player : players)
	{
		if (!player || !player->IsActive() || !player->HasReservedPath())
		{
			continue;
		}

		const std::vector<Vector2>& reservedPath = player->GetReservedPath();

		for (const Vector2& pathPosition : reservedPath)
		{
			Renderer::Get().Submit(L"·", pathPosition, Color::Cyan, 17);
		}

		Renderer::Get().Submit(L"X", reservedPath.back(), Color::Cyan, 18);
	}
}

void RaidLevel::DrawPathPreview() const
{
	for (const Vector2& pathPosition : previewPath)
	{
		Renderer::Get().Submit(L"·", pathPosition, Color::Green, 18);
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
	else if (turnState == TurnState::PlayerCards)
	{
		turnText = L"Turn : Cards";
		turnColor = Color::Yellow;
	}
	else if (turnState == TurnState::Victory)
	{
		turnText = L"VICTORY";
		turnColor = Color::Green;
	}
	else if (turnState == TurnState::Defeat)
	{
		turnText = L"DEFEAT";
		turnColor = Color::Red;
	}

	Renderer::Get().Submit(turnText, Vector2(interfaceX + 8, interfaceY + 3), turnColor, 10);

	const Color bossBorderColor = boss && boss->IsActive() ? Color::Red : Color::White;
	DrawBox(Vector2(interfaceX + 3, interfaceY + 5), interfaceWidth - 6, 4, bossBorderColor);

	std::wstring bossText = L"Boss HP";

	if (boss)
	{
		bossText += L" : " + std::to_wstring(boss->GetHealth()) + L" / " + std::to_wstring(Boss::MaxHealth);
	}

	Renderer::Get().Submit(bossText, Vector2(interfaceX + 5, interfaceY + 6), Color::White, 10);

	std::wstring bossActionText = L"Action : None";

	if (boss && boss->GetPlannedActionType() == Boss::ActionType::LaserAttack)
	{
		bossActionText = L"Action : Laser";
	}
	else if (boss && boss->GetPlannedActionType() == Boss::ActionType::NearbyAttack)
	{
		bossActionText = L"Action : Nearby";
	}
	else if (boss && boss->GetPlannedActionType() == Boss::ActionType::ConeAttack)
	{
		bossActionText = L"Action : Cone";
	}
	else if (boss && boss->GetPlannedActionType() == Boss::ActionType::TeleportAttack)
	{
		bossActionText = L"Action : Teleport";
	}
	else if (boss && boss->GetPlannedActionType() == Boss::ActionType::SummonMinions)
	{
		bossActionText = L"Action : Summon";
	}
	else if (boss && boss->GetPlannedActionType() == Boss::ActionType::SpecialAttack)
	{
		bossActionText = L"Action : Special";
	}

	Renderer::Get().Submit(bossActionText, Vector2(interfaceX + 5, interfaceY + 7), Color::Yellow, 10);

	for (int ix = 0; ix < 4; ++ix)
	{
		int boxY = interfaceY + 10 + (ix * 4);
		std::shared_ptr<Player> interfacePlayer;

		if (ix < static_cast<int>(players.size()))
		{
			interfacePlayer = players[ix];
		}

		const int playerIndex = interfacePlayer ? interfacePlayer->GetPlayerIndex() : ix + 1;
		const Color borderColor = interfacePlayer && interfacePlayer->IsSelected() ? Color::Blue :
			interfacePlayer && pendingCardTarget.lock() == interfacePlayer ? Color::Green : Color::White;

		DrawBox(Vector2(interfaceX + 3, boxY), interfaceWidth - 6, 4, borderColor);

		std::wstring text = L"Player" + std::to_wstring(playerIndex) + L" HP";

		if (interfacePlayer)
		{
			text += L" : " + std::to_wstring(interfacePlayer->GetHealth());
			if (interfacePlayer->HasBarrier())
			{
				text += L" [B]";
			}
		}

		Renderer::Get().Submit(text, Vector2(interfaceX + 5, boxY + 1), Color::White, 10);

		std::wstring actionText = L"Wait";

		if (interfacePlayer && !interfacePlayer->IsActive())
		{
			actionText = L"Dead - Revive target";
		}
		else if (interfacePlayer && interfacePlayer->HasReservedCard())
		{
			if (const Card* card = cardHand.FindCard(interfacePlayer->GetReservedCardId()))
			{
				actionText = std::wstring(card->GetDefinition().name) + L">" + GetCardTargetLabel(*interfacePlayer);
			}
		}
		else if (interfacePlayer && (interfacePlayer->HasReservedPath() || interfacePlayer->HasPath()))
		{
			actionText = L"Move";
		}

		Renderer::Get().Submit(actionText.substr(0, 20), Vector2(interfaceX + 5, boxY + 2), Color::Cyan, 10);
	}

	const std::wstring minionText = L"Turret:" + std::to_wstring(GetTurretCount()) + L"/3";
	Renderer::Get().Submit(minionText, Vector2(interfaceX + 5, interfaceY + 26), Color::Purple, 10);

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

	const int cardX = raidMap->GetPosition().x + 5;
	const int cardY = raidMap->GetPosition().y + raidMap->GetHeight() + 2;
	const int cardWidth = raidMap->GetWidth() + 28;
	const std::vector<Card>& cards = cardHand.GetCards();
	DrawBox(Vector2(cardX, cardY), cardWidth, 12);

	const std::wstring title = L"Cards: " + std::to_wstring(cards.size()) + L" / 8  |  +3 per turn, oldest discarded if full";
	Renderer::Get().Submit(title, Vector2(cardX + 3, cardY + 1), Color::White, 10);

	for (int index = 0; index < CardHand::MaxCards; ++index)
	{
		const Vector2 slot = GetCardSlotPosition(index);

		if (index >= static_cast<int>(cards.size()))
		{
			DrawBox(slot, CardSlotWidth, CardSlotHeight);
			Renderer::Get().Submit(L"Empty", slot + Vector2(2, 2), Color::White, 10);
			continue;
		}

		const Card& card = cards[index];
		const std::shared_ptr<Player> owner = FindCardOwner(card.id);
		Color borderColor = owner ? Color::Cyan : Color::White;

		if (index == selectedCardIndex && turnState == TurnState::PlayerPlanning)
		{
			borderColor = isSelectingCardTarget ? Color::Green : Color::Blue;
		}

		DrawBox(slot, CardSlotWidth, CardSlotHeight, borderColor);
		const std::wstring cardText = std::to_wstring(index + 1) + L" " + card.GetDefinition().name;
		const Color rarityColor = card.GetDefinition().rarity == CardRarity::Epic ? Color::Purple :
			card.GetDefinition().rarity == CardRarity::Rare ? Color::Cyan : Color::White;
		Renderer::Get().Submit(cardText.substr(0, 14), slot + Vector2(1, 1), rarityColor, 10);
		Renderer::Get().Submit(std::wstring(card.GetDefinition().description).substr(0, 14), slot + Vector2(1, 2), Color::Yellow, 10);

		std::wstring reservation = std::to_wstring(card.GetDefinition().drawWeight) + L"% Available";

		if (owner)
		{
			reservation = L"P" + std::to_wstring(owner->GetPlayerIndex());
			reservation += L">" + GetCardTargetLabel(*owner);
		}

		Renderer::Get().Submit(reservation.substr(0, 14), slot + Vector2(1, 3), owner ? Color::Cyan : Color::White, 10);
	}

	std::wstring itemHint = L"";
	for (const auto& object : deployables)
	{
		if (object && object->IsActive() && object->GetPosition() == Input::Get().GetMousePosition())
		{
			itemHint = std::wstring(object->GetKind() == Deployable::Kind::Turret ? L"Turret" : L"Barricade") +
				L" - " + std::to_wstring(object->GetRemainingTurns()) + L" turn(s) remaining, including the current turn.";
			break;
		}
	}
	Renderer::Get().Submit(itemHint.substr(0, cardWidth - 6), Vector2(cardX + 3, cardY + 9), Color::White, 10);
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
