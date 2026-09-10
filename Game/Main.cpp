#include <Windows.h>
#include "Game/Game.h"

int main()
{
    SetConsoleTitleA("RAId");

    Game game;
    game.Run();

    return 0;
}