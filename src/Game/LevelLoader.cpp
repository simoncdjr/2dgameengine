#include "./LevelLoader.h"
#include "./Game.h"
#include "../Logger/Logger.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/AnimationComponent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/KeyboardControlledComponent.h"
#include "../Components/CameraFollowComponent.h"
#include "../Components/ProjectileEmitterComponent.h"
#include "../Components/HealthComponent.h"
#include "../Components/TextLabelComponent.h"
#include "../Components/ScriptComponent.h"
#include <fstream>

LevelLoader::LevelLoader() {
	Logger::Log("LevelLoader constructor called!");
}

LevelLoader::~LevelLoader() {
	Logger::Log("LevelLoader destructor called!");
}

void LevelLoader::LoadLevel(sol::state& lua, const std::unique_ptr<Registry>& registry, const std::unique_ptr<AssetStore>& assetStore,
				SDL_Renderer* renderer, int levelNumber) {
	// Checks the syntax of our script, but it does not execute the script
	sol::load_result script = lua.load_file("./assets/scripts/Level" + std::to_string(levelNumber) + ".lua");
	if (!script.valid()) {
		sol::error err = script;
		std::string errorMessage = err.what();
		Logger::Err("Error loading the lua script: " + errorMessage);
		return;
	}

	// Executes the script using the Sol state
	lua.script_file("./assets/scripts/Level" + std::to_string(levelNumber) + ".lua");

	// Read the big table for the current level
	sol::table level = lua["Level"];

	// Read the level assets
	sol::table assets = level["assets"];

	int i = 0;
	while (true) {
		sol::optional<sol::table> hasAsset = assets[i];
		if (hasAsset == sol::nullopt) {
			break;
		}
		sol::table asset = assets[i];
		std::string assetType = asset["type"];
		std::string assetId = asset["id"];
		if (assetType == "texture") {
			assetStore->AddTexture(renderer, assetId, asset["file"]);
			Logger::Log("New texture asset loaded to the asset store, id: " + assetId);
		}
		if (assetType == "font") {
			assetStore->AddFont(assetId, asset["file"], asset["font_size"]);
			Logger::Log("New font asset loaded to the asset store, id: " + assetId);
		}
		i++;
	}




	// Read the level tilemap information
	sol::table map = level["tilemap"];
	std::string mapFilePath = map["map_file"];
	std::string mapTextureAssetId = map["texture_asset_id"];
	int mapNumRows = map["num_rows"];
	int mapNumCols = map["num_cols"];
	int tileSize = map["tile_size"];
	double mapScale = map["scale"];
	std::fstream mapFile;
	mapFile.open(mapFilePath);
	for (int y = 0; y < mapNumRows; y++) {
		for (int x = 0; x < mapNumCols; x++) {
			char ch;
			mapFile.get(ch);
			int srcRectY = std::atoi(&ch) * tileSize;
			mapFile.get(ch);
			int srcRectX = std::atoi(&ch) * tileSize;
			mapFile.ignore();

			Entity tile = registry->CreateEntity();
			tile.AddComponent<TransformComponent>(
					glm::vec2(x * (mapScale * tileSize), y * (mapScale * tileSize)),
					glm::vec2(mapScale, mapScale),
					0.0);
			tile.AddComponent<SpriteComponent>(mapTextureAssetId, tileSize, tileSize, 0, false, srcRectX, srcRectY);
		}
	}
	mapFile.close();
	Game::mapWidth = mapNumCols * tileSize * mapScale;
	Game::mapHeight = mapNumRows * tileSize * mapScale;

	// Read the level entities and their components
	sol::table entities = level["entities"];
	i = 0;
	while (true) {
		sol::optional<sol::table> hasEntity = entities[i];
		if (hasEntity == sol::nullopt) {
			break;
		}

		sol::table entity = entities[i];

		Entity newEntity = registry->CreateEntity();

		// Tag
		sol::optional<std::string> tag = entity["tag"];
		if (tag != sol::nullopt) {
			newEntity.Tag(entity["tag"]);
		}

		// Group
		sol::optional<std::string> group = entity["group"];
		if (group != sol::nullopt) {
			newEntity.Group(entity["group"]);
		}

		// Components
		sol::optional<sol::table> hasComponents = entity["components"];
		if (hasComponents != sol::nullopt) {
			// Transform
			sol::optional<sol::table> transform = entity["components"]["transform"];
			if (transform != sol::nullopt) {
				newEntity.AddComponent<TransformComponent>(
					glm::vec2(
						entity["components"]["transform"]["position"]["x"],
						entity["components"]["transform"]["position"]["y"]
					),
					glm::vec2(
						entity["components"]["transform"]["scale"]["x"].get_or(1.0),
						entity["components"]["transform"]["scale"]["y"].get_or(1.0)
					),
					entity["components"]["transform"]["rotation"].get_or(0.0)
				);
			}

			// RigidBody
			sol::optional<sol::table> rigidbody = entity["components"]["rigidbody"];
			if (rigidbody != sol::nullopt) {
				newEntity.AddComponent<RigidBodyComponent>(
					glm::vec2(
						entity["components"]["rigidbody"]["velocity"]["x"].get_or(0.0),
						entity["components"]["rigidbody"]["velocity"]["y"].get_or(0.0)
					)
				);
			}

			// Sprite
			sol::optional<sol::table> sprite = entity["components"]["sprite"];
			if (sprite != sol::nullopt) {
				newEntity.AddComponent<SpriteComponent>(
						entity["components"]["sprite"]["texture_asset_id"],
						entity["components"]["sprite"]["width"],
						entity["components"]["sprite"]["height"],
						entity["components"]["sprite"]["z_index"].get_or(1),
						entity["components"]["sprite"]["fixed"].get_or(false),
						entity["components"]["sprite"]["src_rect_x"].get_or(0),
						entity["components"]["sprite"]["src_rect_y"].get_or(0)
					);
			}

			// Animation
			sol::optional<sol::table> animation = entity["components"]["animation"];
			if (animation != sol::nullopt) {
				newEntity.AddComponent<AnimationComponent>(
						entity["components"]["animation"]["num_frames"].get_or(1),
						entity["components"]["animation"]["speed_rate"].get_or(1)
				);
			}

			// BoxCollider
			sol::optional<sol::table> collider = entity["components"]["boxcollider"];
			if (collider != sol::nullopt) {
				newEntity.AddComponent<BoxColliderComponent>(
							entity["components"]["boxcollider"]["width"],
							entity["components"]["boxcollider"]["height"],
							glm::vec2(
								entity["components"]["boxcollider"]["offset"]["x"].get_or(0),
								entity["components"]["boxcollider"]["offset"]["y"].get_or(0)
						)
				);
			}

			// Health
			sol::optional<sol::table> health = entity["components"]["health"];
			if (health != sol::nullopt) {
				newEntity.AddComponent<HealthComponent>(
						static_cast<int>(entity["components"]["health"]["health_percentage"].get_or(100))
				);
			}

			// Projectile Emitter
			sol::optional<sol::table> projectileEmitter = entity["components"]["projectile_emitter"];
			if (projectileEmitter != sol::nullopt) {
				newEntity.AddComponent<ProjectileEmitterComponent>(
					glm::vec2(
						entity["components"]["projectile_emitter"]["projectile_velocity"]["x"],
						entity["components"]["projectile_emitter"]["projectile_velocity"]["y"]
						),
					static_cast<int>(entity["components"]["projectile_emitter"]["repeat_frequency"].get_or(1)) * 1000,
					static_cast<int>(entity["components"]["projectile_emitter"]["projectile_duration"].get_or(10)) * 1000,
					static_cast<int>(entity["components"]["projectile_emitter"]["hit_percentage_damage"].get_or(10)),
					entity["components"]["projectile_emitter"]["friendly"].get_or(false)
				);
			}

			// CameraFollow
			sol::optional<sol::table> cameraFollow = entity["components"]["camera_follow"];
			if (cameraFollow != sol::nullopt) {
				newEntity.AddComponent<CameraFollowComponent>();
			}

			// KeyboardControlled
			sol::optional<sol::table> keyboardControlled = entity["components"]["keyboard_controller"];
			if (keyboardControlled != sol::nullopt) {
				newEntity.AddComponent<KeyboardControlledComponent>(
					glm::vec2(
						entity["components"]["keyboard_controller"]["up_velocity"]["x"],
						entity["components"]["keyboard_controller"]["up_velocity"]["y"]
					),
					glm::vec2(
						entity["components"]["keyboard_controller"]["right_velocity"]["x"],
						entity["components"]["keyboard_controller"]["right_velocity"]["y"]
					),
					glm::vec2(
						entity["components"]["keyboard_controller"]["down_velocity"]["x"],
						entity["components"]["keyboard_controller"]["down_velocity"]["y"]
					),
					glm::vec2(
						entity["components"]["keyboard_controller"]["left_velocity"]["x"],
						entity["components"]["keyboard_controller"]["left_velocity"]["y"]
					)
				);
			}

			// Script
			sol::optional<sol::table> script = entity["components"]["on_update_script"];
			if (script != sol::nullopt) {
				sol::function func = entity["components"]["on_update_script"][0];
				newEntity.AddComponent<ScriptComponent>(func);
			}
		}
		i++;
	}
}
	// Adding assets to the asset store
	/*
	assetStore->AddTexture(renderer, "tree-image", "./assets/images/tree.png");
	assetStore->AddTexture(renderer, "tank-image", "./assets/images/tank-panther-right.png");
	assetStore->AddTexture(renderer, "truck-image", "./assets/images/truck-ford-right.png");
	assetStore->AddTexture(renderer, "chopper-image", "./assets/images/chopper-spritesheet.png");
	assetStore->AddTexture(renderer, "radar-image", "./assets/images/radar.png");
	assetStore->AddTexture(renderer, "tilemap-image", "./assets/tilemaps/jungle.png");
	assetStore->AddTexture(renderer, "bullet-image", "./assets/images/bullet.png");
	assetStore->AddFont("charriot-font", "./assets/fonts/charriot.ttf", 20);

	// Load the tilemap
	int tileSize = 32;
	double tileScale = 5.0;
	int mapNumCols = 25;
	int mapNumRows = 20;

	std::fstream mapFile;
	mapFile.open("./assets/tilemaps/jungle.map");

	for (int y = 0; y <mapNumRows; y++) {
		for (int x = 0; x < mapNumCols; x++) {
			char ch;
			mapFile.get(ch);
			int srcRectY = std::atoi(&ch) * tileSize;
			mapFile.get(ch);
			int srcRectX = std::atoi(&ch) * tileSize;
			mapFile.ignore();

			Entity tile = registry->CreateEntity();
			tile.Group("tiles");
			tile.AddComponent<TransformComponent>(
					glm::vec2(x * (tileScale * tileSize), y * (tileScale * tileSize)),
					glm::vec2(tileScale, tileScale),
					0.0);
			tile.AddComponent<SpriteComponent>("tilemap-image", tileSize, tileSize, 0, false, 
					srcRectX, srcRectY);
		}
	}
	mapFile.close();
	Game::mapWidth = mapNumCols * tileSize * tileScale;
	Game::mapHeight = mapNumRows * tileSize * tileScale;

	// Create some entities
	Entity chopper = registry->CreateEntity();	
	chopper.Tag("player");
	chopper.AddComponent<TransformComponent>(glm::vec2(10.0, 250.0), glm::vec2(2.0, 2.0), 0.0);
	chopper.AddComponent<RigidBodyComponent>(glm::vec2(0.0, 0.0));
	chopper.AddComponent<SpriteComponent>("chopper-image", 32, 32, 2);
	chopper.AddComponent<AnimationComponent>(2, 15, true);
	chopper.AddComponent<BoxColliderComponent>(32, 32);
	chopper.AddComponent<CameraFollowComponent>();
	chopper.AddComponent<KeyboardControlledComponent>(glm::vec2(0,-200), 
			glm::vec2(200, 0), glm::vec2(0, 200), glm::vec2(-200, 0));
	chopper.AddComponent<ProjectileEmitterComponent>(glm::vec2(350.0, 350.0), 0, 10000, 10, true);
	chopper.AddComponent<HealthComponent>(100);
	chopper.AddComponent<TextLabelComponent>(glm::vec2(0), " ", "charriot-font"); 

	Entity tank = registry->CreateEntity();	
	tank.Group("enemies");
	tank.AddComponent<TransformComponent>(glm::vec2(2000.0, 450.0), glm::vec2(2.0, 2.0), 0.0);
	tank.AddComponent<RigidBodyComponent>(glm::vec2(50.0, 0.0));
	tank.AddComponent<SpriteComponent>("tank-image", 32, 32, 1);
	tank.AddComponent<BoxColliderComponent>(32, 32);
	//tank.AddComponent<ProjectileEmitterComponent>(glm::vec2(100.0, 0.0), 5000, 3000, 10, false);
	tank.AddComponent<HealthComponent>(100);
	tank.AddComponent<TextLabelComponent>(glm::vec2(0), " ", "charriot-font"); 
	
	Entity truck = registry->CreateEntity();	
	truck.Group("enemies");
	truck.AddComponent<TransformComponent>(glm::vec2(300.0, 1250.0), glm::vec2(2.0, 2.0), 0.0);
	truck.AddComponent<RigidBodyComponent>(glm::vec2(0.0, 0.0));
	truck.AddComponent<SpriteComponent>("truck-image", 32, 32, 1);
	truck.AddComponent<BoxColliderComponent>(32, 32);
	truck.AddComponent<ProjectileEmitterComponent>(glm::vec2(100.0, 0.0), 500, 5000, 10, false);
	truck.AddComponent<HealthComponent>(100);
	truck.AddComponent<TextLabelComponent>(glm::vec2(0), " ", "charriot-font"); 

	Entity treeA = registry->CreateEntity();
	treeA.Group("obstacles");
	treeA.AddComponent<TransformComponent>(glm::vec2(2200.0, 450.0), glm::vec2(2.0, 2.0), 0.0);
	treeA.AddComponent<SpriteComponent>("tree-image", 16, 32, 1);
	treeA.AddComponent<BoxColliderComponent>(16, 32);

	Entity treeB = registry->CreateEntity();
	treeB.Group("obstacles");
	treeB.AddComponent<TransformComponent>(glm::vec2(1970.0, 450.0), glm::vec2(2.0, 2.0), 0.0);
	treeB.AddComponent<SpriteComponent>("tree-image", 16, 32, 1);
	treeB.AddComponent<BoxColliderComponent>(16, 32);

	Entity radar = registry->CreateEntity();	
	radar.AddComponent<TransformComponent>(glm::vec2(Game::windowWidth - 300, 10.0), glm::vec2(2.0, 2.0), 0.0);
	radar.AddComponent<SpriteComponent>("radar-image", 64, 64, 1, true);
	radar.AddComponent<AnimationComponent>(8, 5, true);

	Entity label = registry->CreateEntity();
	SDL_Color green = {0, 255, 0};
	label.AddComponent<TextLabelComponent>(glm::vec2(Game::windowWidth / 2 -40, 10), 
			"CHOPPER 1.0", "charriot-font", green, true);
	*/
