#include "Game/Game.h"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    Game game;

    game.Initialize();
    game.Run();
    game.Destroy();

    return 0;
}
