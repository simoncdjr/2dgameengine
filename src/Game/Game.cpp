#include "Game.h"
#include "LevelLoader.h"
#include "../Logger/Logger.h"
#include "../ECS/ECS.h"
#include "../Systems/MovementSystem.h"
#include "../Systems/CameraMovementSystem.h"
#include "../Systems/RenderSystem.h"
#include "../Systems/AnimationSystem.h"
#include "../Systems/CollisionSystem.h"
#include "../Systems/RenderColliderSystem.h"
#include "../Systems/DamageSystem.h"
#include "../Systems/KeyboardControlSystem.h"
#include "../Systems/ProjectileEmitSystem.h"
#include "../Systems/ProjectileLifecycleSystem.h"
#include "../Systems/RenderTextSystem.h"
#include "../Systems/RenderHealthSystem.h"
#include "../Systems/RenderGUISystem.h"
#include "../Systems/ScriptSystem.h"
#include "../Events/KeyPressedEvent.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <glm/glm.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_sdlrenderer2.h>
#include <iostream>

int Game::windowWidth;
int Game::windowHeight;
int Game::mapWidth;
int Game::mapHeight;

Game::Game() {
	isRunning = false;
	isDebug = false;
	registry = std::make_unique<Registry>();
	assetStore = std::make_unique<AssetStore>();
	eventBus = std::make_unique<EventBus>();
	Logger::Log("Game constructor called!"); 
}

Game::~Game() {
	Logger::Log("Game destructor called!"); 
}

void Game::Initialize() {
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		Logger::Err("Error initializing SDL."); 
		return;
	}

	if (TTF_Init() != 0) {
		Logger::Err("Error initializing SDL TTF.");
		return;
	}

	SDL_DisplayMode displayMode;
	SDL_GetCurrentDisplayMode(0, &displayMode);
	windowHeight = displayMode.h;
	windowWidth = displayMode.w;
	window = SDL_CreateWindow(
			NULL,
		   	SDL_WINDOWPOS_CENTERED, 
			SDL_WINDOWPOS_CENTERED,
			windowWidth,
			windowHeight,
			SDL_WINDOW_BORDERLESS
	);
	if (!window) {
		Logger::Err("Error creating SDL window."); 
		return;
	}
	renderer = SDL_CreateRenderer(window, -1, 0);
	if (!renderer) {
		Logger::Err("Error creating SDL renderer."); 
		return;
	}
	SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN);

	// Initialize the imGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplSDL2_InitForSDLRenderer(window, renderer);
	ImGui_ImplSDLRenderer2_Init(renderer);

	// Initialize the camera view with the entire screen area
	camera.x = 0;
	camera.y = 0;
	camera.w = windowWidth;
	camera.h = windowHeight;

	isRunning = true;
}

void Game::ProcessInput() {
	SDL_Event sdlEvent;
	while (SDL_PollEvent(&sdlEvent)) {
		// ImGui SDL input
		ImGui_ImplSDL2_ProcessEvent(&sdlEvent);
		ImGuiIO& io = ImGui::GetIO(); 
		int mouseX, mouseY;
		const int buttons = SDL_GetMouseState(&mouseX, &mouseY);
		io.MousePos = ImVec2(mouseX, mouseY);
		io.MouseDown[0] = buttons & SDL_BUTTON(SDL_BUTTON_LEFT);
		io.MouseDown[1] = buttons & SDL_BUTTON(SDL_BUTTON_RIGHT);

		// Handle core SDL events (close window, key pressed, etc.)
		switch (sdlEvent.type) {
			case SDL_QUIT:
				isRunning = false;
				break;
			case SDL_KEYDOWN:
				if (sdlEvent.key.keysym.sym == SDLK_ESCAPE) {
					isRunning = false;
				}
				if (sdlEvent.key.keysym.sym == SDLK_d) {
					isDebug = !isDebug;
				}
				eventBus->EmitEvent<KeyPressedEvent>(sdlEvent.key.keysym.sym);
				break;
		}
	}
}

void Game::Setup() {
	// Add the systems that need to be processed in our game
	registry->AddSystem<MovementSystem>();
	registry->AddSystem<CameraMovementSystem>();
	registry->AddSystem<RenderSystem>();
	registry->AddSystem<AnimationSystem>();
	registry->AddSystem<CollisionSystem>();
	registry->AddSystem<RenderColliderSystem>();
	registry->AddSystem<DamageSystem>();
	registry->AddSystem<KeyboardControlSystem>();
	registry->AddSystem<ProjectileEmitSystem>();
	registry->AddSystem<ProjectileLifecycleSystem>();
	registry->AddSystem<RenderTextSystem>();
	registry->AddSystem<RenderHealthSystem>();
	registry->AddSystem<RenderGUISystem>();
	registry->AddSystem<ScriptSystem>();

	// Create the bindings between C++ and Lua
	registry->GetSystem<ScriptSystem>().CreateLuaBindings(lua);

	// Load the first level
	LevelLoader loader;
	lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::os);
	loader.LoadLevel(lua, registry, assetStore, renderer, 1);
}

void Game::Update() {
	// If we are too fast, waste some time until we reach the MILLISECS_PER_FRAME
	int timeToWait = MILLISECS_PER_FRAME - (SDL_GetTicks() - millisecsPreviousFrame);
	if (timeToWait > 0 && timeToWait <= MILLISECS_PER_FRAME) {
		SDL_Delay(timeToWait);	
	}

	// The difference in ticks since the last frame, converted to seconds
	double deltaTime = (SDL_GetTicks() - millisecsPreviousFrame) / 1000.0;

	// Store the "previous" frame time
	millisecsPreviousFrame = SDL_GetTicks();

	// Reset all event handlers for the current frame
	eventBus->Reset();

	// Perform the subscription of the events for all systems
	registry->GetSystem<MovementSystem>().SubscribeToEvents(eventBus);
	registry->GetSystem<DamageSystem>().SubscribeToEvents(eventBus);
	registry->GetSystem<KeyboardControlSystem>().SubscribeToEvents(eventBus);
	registry->GetSystem<ProjectileEmitSystem>().SubscribeToEvents(eventBus);
	
	// Update the registry to process the entities that are waiting to be created/deleted
	registry->Update();

	// Invoke all the systems that need to update
	registry->GetSystem<MovementSystem>().Update(deltaTime);
	registry->GetSystem<AnimationSystem>().Update();
	registry->GetSystem<CollisionSystem>().Update(eventBus);
	registry->GetSystem<ProjectileEmitSystem>().Update(registry);
	registry->GetSystem<CameraMovementSystem>().Update(camera);
	registry->GetSystem<ProjectileLifecycleSystem>().Update();
	registry->GetSystem<ScriptSystem>().Update(deltaTime, SDL_GetTicks());
}

void Game::Render() {
	SDL_SetRenderDrawColor(renderer, 21, 21, 21, 255);
	SDL_RenderClear(renderer);

	// Invoke all the systems that need to render
	registry->GetSystem<RenderSystem>().Update(renderer, assetStore, camera);
	registry->GetSystem<RenderTextSystem>().Update(renderer, assetStore, camera);
	registry->GetSystem<RenderHealthSystem>().Update(renderer, assetStore, camera);

	if (isDebug) {
		registry->GetSystem<RenderColliderSystem>().Update(renderer, camera);

		registry->GetSystem<RenderGUISystem>().Update(renderer, registry, assetStore);
	}
		SDL_RenderPresent(renderer);
}

void Game::Run() {
	Setup();
	while (isRunning) {
		ProcessInput();
		Update();
		Render();
	}

}

void Game::Destroy() {
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

}
