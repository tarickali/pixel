#ifndef GAME_H
#define GAME_H

#include "../ECS/ECS.h"
#include "../AssetStore/AssetStore.h"
#include "../Events/EventBus.h"

#include <SDL2/SDL.h>

const int FPS = 120;
const int MILLISECS_PER_FRAME = 1000 / FPS;

class Game {
    private:
        bool isRunning;
        bool isDebug;
        int millisecsPreviousFrame;

        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_Rect camera;

        std::unique_ptr<World> world;
        std::unique_ptr<AssetStore> assetStore;
        std::unique_ptr<EventBus> eventBus;

    public:
        Game();
        ~Game();

        void Initialize();
        void LoadLevel(int level);
        void Setup();
        void Run();
        void ProcessInput();
        void Update();
        void Render();
        void Destroy();

        static int windowWidth;
        static int windowHeight;
        static int mapWidth;
        static int mapHeight;
};

#endif
