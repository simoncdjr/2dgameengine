#ifndef RENDERHEALTHSYSTEM_H
#define RENDERHEALTHSYSTEM_H

#include "../ECS/ECS.h"
#include "../AssetStore/AssetStore.h"
#include "../Logger/Logger.h"
#include "../Components/HealthComponent.h"
#include "../Components/TransformComponent.h"
#include "../Components/SpriteComponent.h"

#include <SDL2/SDL.h>

class RenderHealthSystem: public System {
	public:
		RenderHealthSystem() {
			RequireComponent<HealthComponent>();
			RequireComponent<TransformComponent>();
			RequireComponent<SpriteComponent>();
		}

		void Update(SDL_Renderer* renderer, std::unique_ptr<AssetStore>& assetStore, const SDL_Rect& camera) {
			for (auto entity: GetSystemEntities()) {
				auto health = entity.GetComponent<HealthComponent>();
				auto transform = entity.GetComponent<TransformComponent>();
				auto sprite = entity.GetComponent<SpriteComponent>();

				bool isOutsideCameraView = (
						transform.position.x + (transform.scale.x * sprite.width) < camera.x ||
						transform.position.x > camera.x + camera.w ||
						transform.position.y + (transform.scale.y * sprite.height) < camera.y ||
						transform.position.y > camera.y + camera.h
						);

				if (isOutsideCameraView) continue;

				// Render the health bar
				int healthBarHeight = 5;
				int healthBarWidth = 30;

				healthBarWidth = sprite.width * transform.scale.y;

				SDL_Rect healthBarRect = {
					static_cast<int>((transform.position.x - camera.x)),
					static_cast<int>((transform.position.y - camera.y) - 30),
					static_cast<int>(healthBarWidth * (health.healthPercentage / 100.0)),
					healthBarHeight
				};

				if (health.healthPercentage > 30) {
					SDL_SetRenderDrawColor(renderer, 0, 255, 0, 0);
				} else if (health.healthPercentage <= 30) {
					SDL_SetRenderDrawColor(renderer, 255, 0, 0, 0);
				} else {
					Logger::Err("Could not execute SDL_SetRenderDrawColor");
				}

				SDL_RenderFillRect(renderer, &healthBarRect);

				// Render the health percentage
				
				SDL_Color color = {255, 255, 255};
				std::string healthPercentage = std::to_string(health.healthPercentage) + "%";

				if (health.healthPercentage > 30) {
					color = {0, 255, 0};
				}
				else if (health.healthPercentage <= 30) {
					color = {255, 0, 0};
				} else {
					Logger::Err("Could not create SDL_Surface for RenderHealthSystem!");
				}
				SDL_Surface* surface = TTF_RenderText_Blended(assetStore->GetFont("charriot"),
						healthPercentage.c_str(), color);

				SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
				SDL_FreeSurface(surface);

				int labelWidth = 0;
				int labelHeight = 0;

				SDL_QueryTexture(texture, NULL, NULL, &labelWidth, &labelHeight);

				SDL_Rect dstRect {
					healthBarRect.x, 
					healthBarRect.y - 20,
					labelWidth,
					labelHeight
				};

				SDL_RenderCopy(renderer, texture, NULL, &dstRect);

				SDL_DestroyTexture(texture);
			}
	}
};

#endif
