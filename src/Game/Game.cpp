#include "Game.h"
#include "../ECS/ECS.h"
#include "../Logger/Logger.h"
#include "../Components.h"
#include "../Systems.h"

#include <fstream>
#include <sstream>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <imgui/imgui.h>
#include <imgui/imgui_sdl.h>
#include <imgui/imgui_impl_sdl.h>
#include <glm/glm.hpp>

int Game::windowWidth = 0;
int Game::windowHeight = 0;
int Game::mapWidth = 0;
int Game::mapHeight = 0;

Game::Game() {
    isRunning = false;
    isDebug = false;

    world = std::make_unique<World>();
    assetStore = std::make_unique<AssetStore>();
    eventBus = std::make_unique<EventBus>();

    Logger::Log("Game constructor called.");
}

Game::~Game() {
    Logger::Log("Game destructor called.");
}

void Game::Initialize() {
    int err = SDL_Init(SDL_INIT_EVERYTHING);
    if (err != 0) {
        Logger::Error("Could not initialize SDL.");
        return;
    }

    if (TTF_Init() != 0) {
        Logger::Error("Could not initialize SDL TTF.");
        return;
    }

    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    windowWidth = displayMode.w;
    windowHeight = displayMode.h;

    window = SDL_CreateWindow(
        "Pixel",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        windowWidth,
        windowHeight,
        SDL_WINDOW_BORDERLESS
    );
    if (!window) {
        Logger::Error("Could not create SDL window.");
        return;
    }

    renderer = SDL_CreateRenderer(
        window,
        -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
    );
    if (!renderer) {
        Logger::Error("Could not create SDL renderer.");
        return;
    }

    ImGui::CreateContext();
    ImGuiSDL::Initialize(renderer, windowWidth, windowHeight);

    camera.x = 0;
    camera.y = 0;
    camera.w = windowWidth;
    camera.h = windowHeight;

    SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

    isRunning = true;
}

void Game::Run() {
    Setup();

    while (isRunning) {
        ProcessInput();
        Update();
        Render();
    }
}

void Game::ProcessInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);

        int mouseX, mouseY;
        const int buttons = SDL_GetMouseState(&mouseX, &mouseY);
        ImGuiIO &io = ImGui::GetIO();
        io.MousePos = ImVec2(static_cast<float>(mouseX), static_cast<float>(mouseY));
        io.MouseDown[0] = (buttons & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
        io.MouseDown[1] = (buttons & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;

        switch (event.type) {
            case SDL_QUIT:
                isRunning = false;
                break;
            case SDL_KEYDOWN:
                if (event.key.keysym.sym == SDLK_ESCAPE) {
                    isRunning = false;
                }
                if (event.key.keysym.sym == SDLK_d) {
                    isDebug = !isDebug;
                }
                eventBus->EmitEvent<KeyPressedEvent>(event.key.keysym.sym);
                break;
            default:
                break;
        }
    }
}

void Game::LoadLevel(int level) {
    (void)level;

    world->AddSystem<MovementSystem>();
    world->AddSystem<RenderSystem>();
    world->AddSystem<AnimationSystem>();
    world->AddSystem<CollisionSystem>();
    world->AddSystem<RenderCollisionSystem>();
    world->AddSystem<DamageSystem>();
    world->AddSystem<KeyboardMovementSystem>();
    world->AddSystem<CameraMovementSystem>();
    world->AddSystem<ProjectileEmitSystem>();
    world->AddSystem<ProjectileLifecycleSystem>();
    world->AddSystem<RenderTextSystem>();
    world->AddSystem<RenderHealthSystem>();
    world->AddSystem<RenderGUISystem>();

    assetStore->AddTexture(renderer, "tank-image", "./assets/images/tank-panther-right.png");
    assetStore->AddTexture(renderer, "truck-image", "./assets/images/truck-ford-right.png");
    assetStore->AddTexture(renderer, "chopper-image", "./assets/images/chopper-spritesheet.png");
    assetStore->AddTexture(renderer, "radar-image", "./assets/images/radar.png");
    assetStore->AddTexture(renderer, "tilemap-image", "./assets/tilemaps/jungle.png");
    assetStore->AddTexture(renderer, "bullet-image", "./assets/images/bullet.png");
    assetStore->AddTexture(renderer, "tree-image", "./assets/images/tree.png");

    assetStore->AddFont("title-font", "./assets/fonts/charriot.ttf", 20);
    assetStore->AddFont("health-font", "./assets/fonts/charriot.ttf", 12);

    int tileSize = 32;
    double tileScale = 2.0;

    std::fstream mapFile("./assets/tilemaps/jungle.map", std::ios::in);
    if (!mapFile) {
        Logger::Error("Could not open tilemap file: ./assets/tilemaps/jungle.map");
        return;
    }

    std::string line;
    int y = 0;
    int x = 0;
    while (std::getline(mapFile, line)) {
        std::stringstream ss(line);
        std::string value;
        x = 0;
        while (std::getline(ss, value, ',')) {
            if (value.size() < 2) continue;
            int srcRectY = (value[0] - '0') * tileSize;
            int srcRectX = (value[1] - '0') * tileSize;

            Entity tile = world->CreateEntity();
            tile.Group("tiles");
            tile.AddComponent<TransformComponent>(
                glm::vec2(x * (tileSize * tileScale), y * (tileSize * tileScale)),
                glm::vec2(tileScale, tileScale),
                0.0
            );
            tile.AddComponent<SpriteComponent>("tilemap-image", tileSize, tileSize, 0, false, srcRectX, srcRectY);
            x++;
        }
        y++;
    }
    mapFile.close();

    mapWidth = x * tileSize * static_cast<int>(tileScale);
    mapHeight = y * tileSize * static_cast<int>(tileScale);

    Entity chopper = world->CreateEntity();
    chopper.Tag("player");
    chopper.AddComponent<TransformComponent>(glm::vec2(10.0f, 10.0f), glm::vec2(1.0f, 1.0f), 0.0);
    chopper.AddComponent<RigidBodyComponent>(glm::vec2(0.0f, 0.0f));
    chopper.AddComponent<SpriteComponent>("chopper-image", 32, 32, 1, false);
    chopper.AddComponent<AnimationComponent>(2, 10, true);
    chopper.AddComponent<BoxColliderComponent>(32, 32, glm::vec2(0));
    chopper.AddComponent<KeyboardControlledComponent>(
        glm::vec2(0, -80),
        glm::vec2(80, 0),
        glm::vec2(0, 80),
        glm::vec2(-80, 0)
    );
    chopper.AddComponent<CameraFollowComponent>();
    chopper.AddComponent<ProjectileEmitterComponent>(glm::vec2(150.0f, 150.0f), 0, 10000, 10, true);
    chopper.AddComponent<HealthComponent>(100);
    chopper.AddComponent<HealthBarComponent>("health-font", 30, 5, true);

    Entity radar = world->CreateEntity();
    radar.AddComponent<TransformComponent>(
        glm::vec2(windowWidth - 74.0f, 10.0f),
        glm::vec2(1.0f, 1.0f),
        0.0
    );
    radar.AddComponent<SpriteComponent>("radar-image", 64, 64, 2, true);
    radar.AddComponent<AnimationComponent>(8, 5, true);

    Entity tank = world->CreateEntity();
    tank.Group("enemies");
    tank.AddComponent<TransformComponent>(glm::vec2(500.0f, 500.0f), glm::vec2(1.0f, 1.0f), 0.0);
    tank.AddComponent<RigidBodyComponent>(glm::vec2(20.0f, 0.0f));
    tank.AddComponent<SpriteComponent>("tank-image", 32, 32, 1, false);
    tank.AddComponent<BoxColliderComponent>(32, 32, glm::vec2(0));
    tank.AddComponent<ProjectileEmitterComponent>(glm::vec2(100.0f, 0.0f), 200, 3000, 10, false);
    tank.AddComponent<HealthComponent>(100);
    tank.AddComponent<HealthBarComponent>("health-font", 30, 5, true);

    Entity truck = world->CreateEntity();
    truck.Group("enemies");
    truck.AddComponent<TransformComponent>(glm::vec2(120.0f, 500.0f), glm::vec2(1.0f, 1.0f), 0.0);
    truck.AddComponent<RigidBodyComponent>(glm::vec2(0.0f, 0.0f));
    truck.AddComponent<SpriteComponent>("truck-image", 32, 32, 1, false);
    truck.AddComponent<BoxColliderComponent>(32, 32, glm::vec2(0));
    truck.AddComponent<ProjectileEmitterComponent>(glm::vec2(0.0f, 100.0f), 200, 5000, 10, false);
    truck.AddComponent<HealthComponent>(100);
    truck.AddComponent<HealthBarComponent>("health-font", 30, 5, true);

    Entity treeA = world->CreateEntity();
    treeA.Group("obstacles");
    treeA.AddComponent<TransformComponent>(glm::vec2(600.0f, 495.0f), glm::vec2(1.0f, 1.0f), 0.0);
    treeA.AddComponent<SpriteComponent>("tree-image", 16, 32, 2);
    treeA.AddComponent<BoxColliderComponent>(16, 32);

    Entity treeB = world->CreateEntity();
    treeB.Group("obstacles");
    treeB.AddComponent<TransformComponent>(glm::vec2(400.0f, 495.0f), glm::vec2(1.0f, 1.0f), 0.0);
    treeB.AddComponent<SpriteComponent>("tree-image", 16, 32, 2);
    treeB.AddComponent<BoxColliderComponent>(16, 32);

    Entity label = world->CreateEntity();
    SDL_Color green = {0, 255, 0, 255};
    label.AddComponent<TextLabelComponent>(glm::vec2(windowWidth / 2.0f - 40, 40), "PIXEL 1.0", "title-font", green);
}

void Game::Setup() {
    LoadLevel(1);
}

void Game::Update() {
    int timeToWait = MILLISECS_PER_FRAME - (SDL_GetTicks() - millisecsPreviousFrame);
    if (timeToWait > 0 && timeToWait <= MILLISECS_PER_FRAME) {
        SDL_Delay(static_cast<unsigned int>(timeToWait));
    }

    double deltaTime = (SDL_GetTicks() - millisecsPreviousFrame) / 1000.0;
    millisecsPreviousFrame = SDL_GetTicks();

    eventBus->Reset();

    world->GetSystem<MovementSystem>().SubscribeToEvents(eventBus);
    world->GetSystem<DamageSystem>().SubscribeToEvents(eventBus);
    world->GetSystem<KeyboardMovementSystem>().SubscribeToEvents(eventBus);
    world->GetSystem<ProjectileEmitSystem>().SubscribeToEvents(eventBus);

    world->Update();

    world->GetSystem<MovementSystem>().Update(deltaTime);
    world->GetSystem<AnimationSystem>().Update();
    world->GetSystem<CollisionSystem>().Update(eventBus);
    world->GetSystem<ProjectileEmitSystem>().Update(world);
    world->GetSystem<CameraMovementSystem>().Update(camera);
    world->GetSystem<ProjectileLifecycleSystem>().Update();
}

void Game::Render() {
    SDL_SetRenderDrawColor(renderer, 21, 21, 21, 255);
    SDL_RenderClear(renderer);

    world->GetSystem<RenderSystem>().Update(renderer, camera, assetStore);
    world->GetSystem<RenderTextSystem>().Update(renderer, camera, assetStore);
    world->GetSystem<RenderHealthSystem>().Update(renderer, camera, assetStore);

    if (isDebug) {
        world->GetSystem<RenderCollisionSystem>().Update(renderer, camera);
        world->GetSystem<RenderGUISystem>().Update(world, camera);
    }

    SDL_RenderPresent(renderer);
}

void Game::Destroy() {
    ImGuiSDL::Deinitialize();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}
