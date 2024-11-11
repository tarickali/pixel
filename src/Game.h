#ifndef GAME_H
#define GAME_H

#include "ECS.h"

#include <SDL2/SDL.h>
#include <memory>

const int FPS = 60;
const int MS_PER_FRAME = 1000 / FPS;

class Game {
    private:
        bool running;
        bool debugging;

        SDL_Window *window;
        SDL_Renderer *renderer;

        std::unique_ptr<Coordinator> coordinator;

    public:
        Game();
        ~Game();

        void initialize();
        void setup();
        void run();
        void processInput();
        void update(double deltaTime);
        void render();
        void destroy();

        int windowWidth;
        int windowHeight;
};

#endif