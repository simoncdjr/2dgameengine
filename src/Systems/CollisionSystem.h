#ifndef COLLISIONSYSTEM_H
#define COLLISIONSYSTEM_H

#include "../ECS/ECS.h"
#include "../EventBus/EventBus.h"
#include "../Events/CollisionEvent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/TransformComponent.h"


class CollisionSystem: public System {
	public:
		CollisionSystem() {
			RequireComponent<BoxColliderComponent>();
			RequireComponent<TransformComponent>();
		}

		void Update(std::unique_ptr<EventBus>& eventBus) {
			auto entities = GetSystemEntities();

			// Loop all the entities that the system is interested in
			for (auto i = entities.begin(); i != entities.end(); i++) {
				Entity a = *i;
				auto aTransform = a.GetComponent<TransformComponent>();
				auto aCollider = a.GetComponent<BoxColliderComponent>();
				// Loop all the entities that still need to be checked to the right of i
				for (auto j = i; j != entities.end(); j++) {
					Entity b = *j;

					// Bypass if we are trying to test the same entity
					if (a == b) continue;

					auto bTransform = b.GetComponent<TransformComponent>();
					auto bCollider = b.GetComponent<BoxColliderComponent>();

					// Perform the AABB collision check between entities a and b
					bool collisionHappened = CheckAABBCollision(
						aTransform.position.x + aCollider.offset.x,
						aTransform.position.y + aCollider.offset.y,
						(aCollider.width * aTransform.scale.x),
						(aCollider.height * aTransform.scale.y),
						bTransform.position.x + bCollider.offset.x,
						bTransform.position.y + bCollider.offset.y,
						(bCollider.width * bTransform.scale.x),
						(bCollider.height * bTransform.scale.y)
						);

					if (collisionHappened) {
						//Logger::Log("Entity " + std::to_string(a.GetId()) +
						//		" is colliding with entity " + std::to_string(b.GetId()));
						eventBus->EmitEvent<CollisionEvent>(a, b);
					}
				}
			}
			// This was my own implementation
			/*
			// Load in the first entity with its components
			for(auto entity1: GetSystemEntities()) {
				auto box1 = entity1.GetComponent<BoxColliderComponent>();
				auto transform1 = entity1.GetComponent<TransformComponent>();

				// Load in the second entity with its components
				for(auto entity2: GetSystemEntities()) {
					// Skip if both entities loaded have the same ID
					if (entity1.GetId() == entity2.GetId()) continue;

					auto box2 = entity2.GetComponent<BoxColliderComponent>();
					auto transform2 = entity2.GetComponent<TransformComponent>();

					// Now do the check
					if (transform1.position.x < transform2.position.x + (box2.width) &&
						transform1.position.x + (box1.width) > transform2.position.x &&
						transform1.position.y < transform2.position.y + (box2.height) &&
						transform1.position.y + (box1.height) > transform2.position.y)
					{
						Logger::Log("Collision detected!");
					}
					else {
						Logger::Log("No collision detected!");
					}
				}
			}
			*/
		}

		bool CheckAABBCollision(double aX, double aY, double aW, double aH,
				double bX, double bY, double bW, double bH) {
			return (
				aX < bX + bW &&
				aX + aW > bX &&
				aY < bY + bH &&
				aY + aH > bY
			);
		}
};
#endif
