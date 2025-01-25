#ifndef MOVEMENTSYSTEM_H
#define MOVEMENTSYSTEM_H

#include "../ECS/ECS.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Events/CollisionEvent.h"
#include "../EventBus/EventBus.h"

class MovementSystem: public System {
	public:
		MovementSystem() {
			RequireComponent<TransformComponent>();
			RequireComponent<RigidBodyComponent>();
		}

		void SubscribeToEvents(const std::unique_ptr<EventBus>& eventBus) {
			eventBus->SubscribeToEvent<CollisionEvent>(this, &MovementSystem::OnCollision);
		}

		void OnCollision(CollisionEvent& event) {
			Entity a = event.a;
			Entity b = event.b;

			if (a.BelongsToGroup("enemies") && b.BelongsToGroup("obstacles")) {
				OnEnemyHitsObstacle(a, b); // "a" is the enemy, "b" is the obstacle
			}
			
			if (a.BelongsToGroup("obstacles") && b.BelongsToGroup("enemies")) {
				OnEnemyHitsObstacle(b, a); // "b" is the enemy, "a" is the obstacle
			}
		}

		void OnEnemyHitsObstacle(Entity enemy, Entity obstacle) {
			if (enemy.HasComponent<RigidBodyComponent>() && enemy.HasComponent<SpriteComponent>()) {
				auto& rigidBody = enemy.GetComponent<RigidBodyComponent>();
				auto& sprite = enemy.GetComponent<SpriteComponent>();

				if (rigidBody.velocity.x != 0) {
					rigidBody.velocity.x *= -1;
					sprite.flip = (sprite.flip == SDL_FLIP_NONE) ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE;
				}
				if (rigidBody.velocity.y != 0) {
					rigidBody.velocity.y *= -1;
					sprite.flip = (sprite.flip == SDL_FLIP_NONE) ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE;
				}
			}
		}

		void Update(double deltaTime) {
			// Loop all entities that th system is interested in
			for (auto entity: GetSystemEntities()) {
				// Update entity position on its velocity
				auto& transform = entity.GetComponent<TransformComponent>();
				auto& rigidbody = entity.GetComponent<RigidBodyComponent>();

				transform.position.x += rigidbody.velocity.x * deltaTime;
				transform.position.y += rigidbody.velocity.y * deltaTime;

				// TODO: Prevent the main player from outside the map limits

				if (entity.HasTag("player")) {
					int mapPaddingTop = 10;
					int mapPaddingLeft = 10;
					int mapPaddingRight = (Game::mapWidth - 65);
					int mapPaddingBottom = (Game::mapHeight - 65);

					if (transform.position.x <= mapPaddingTop || (transform.position.x >= mapPaddingRight))
						transform.position.x = SDL_clamp(transform.position.x, mapPaddingTop, mapPaddingRight);
					if (transform.position.y <= mapPaddingLeft || (transform.position.y >= mapPaddingBottom))
						transform.position.y = SDL_clamp(transform.position.y, mapPaddingLeft, mapPaddingBottom);

				}

				int margin = 100;
				
				bool isEntityOutsideMap = (
						transform.position.x < -margin ||
						transform.position.x > Game::mapWidth + margin ||
						transform.position.y < -margin ||
						transform.position.y > Game::mapHeight + margin
						);

				// Kill all entities that move outside the map boundaries
				if (isEntityOutsideMap && !entity.HasTag("player")) {
					entity.Kill();
				}
				//Logger::Log("Entity id = " + std::to_string(entity.GetId())
				//		+ " position is now (" + std::to_string(transform.position.x)
				//		+ ", " + std::to_string(transform.position.y) + " )");
			}

		}
};

#endif
