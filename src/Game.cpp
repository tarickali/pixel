#include "Game.h"

#include "Components.h"
#include "Systems.h"

#include <SDL2/SDL.h>
#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

Game::Game() {
    running = false;
    debugging = false;

    coordinator = std::make_unique<Coordinator>();

    spdlog::info("Game constructor called!");
}

Game::~Game() {
    spdlog::info("Game destructor called!");
}

void Game::initialize() {
    // Initialize SDL
    int error = SDL_Init(SDL_INIT_EVERYTHING);
    if (error != 0) {
        spdlog::error("Could not initialize SDL.");
        return;
    }

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    windowWidth = displayMode.w;
    windowHeight = displayMode.h;

    window = SDL_CreateWindow(
        "pixel",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        windowWidth,
        windowHeight,
        SDL_WINDOW_BORDERLESS
    );
    if (!window) {
        spdlog::error("Could not create SDL window.");
        return;
    }

    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        spdlog::error("Could not create SDL renderer.");
        return;
    }

    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    running = true;
}

void Game::setup() {
    // Add systems
    coordinator->addSystem<PhysicsSystem>();

    Entity player = coordinator->create();

    coordinator->addComponent<TransformComponent>(
        player,
        glm::vec2(100, 100),
        glm::vec2(1, 1),
        0.0
    );
    coordinator->addComponent<RigidBodyComponent>(
        player,
        glm::vec2(30, 0),
        glm::vec2(0, 0),
        0.0
    );

    // SDL_Rect player;
    // player = {100, 100, 32, 32};
}

void Game::run() {
    setup();

    double previous = SDL_GetTicks();
    double lag = 0.0;

    while (running) {
        double current = SDL_GetTicks();
        double elapsed = current - previous;
        previous = current;
        lag += elapsed;

        processInput();

        // Each game update is called every MS_PER_FRAME
        while (lag >= MS_PER_FRAME) {
            update(1.0 / FPS);
            lag -= MS_PER_FRAME;
        }

        // TODO: Add (lag/MS_PER_FRAME) for render updates to fix midway renders
        render();
    }
}

void Game::processInput() {
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT:
                running = false;
                break;
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym) {
                    case SDLK_ESCAPE:    
                        running = false;
                        break;
                    // case SDLK_LEFT:
                    //     player.x -= 5;
                    //     break;
                    // case SDLK_RIGHT:
                    //     player.x += 5;
                    //     break;
                    // case SDLK_UP:
                    //     player.y -= 5;
                    //     break;
                    // case SDLK_DOWN:
                    //     player.y += 5;
                    //     break;
                }
                break;
        }
    }
}

void Game::update(double deltaTime) {
    // Update the coordinator to create and destroy entities from last update
    coordinator->update();
    
    // Update all systems
    coordinator->getSystem<PhysicsSystem>().update(coordinator, deltaTime);
}

void Game::render() {
    SDL_SetRenderDrawColor(renderer, 21, 21, 21, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    const auto &transform = coordinator->getComponent<TransformComponent>(Entity(0));
    SDL_Rect player = { static_cast<int>(transform.position.x), static_cast<int>(transform.position.y), 32, 32};
    SDL_RenderFillRect(renderer, &player);

    SDL_RenderPresent(renderer);
}

void Game::destroy() {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}