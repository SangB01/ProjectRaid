#pragma once
#include <array>

enum class BattleOutcome { Victory, Defeat };

struct BattleResult
{
    BattleOutcome outcome = BattleOutcome::Defeat;
    int bossHealth = 0;
    std::array<int, 4> playerHealth{};

    float elapsedTimeSeconds = 0.0f;
};
