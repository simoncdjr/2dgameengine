#ifndef RENDERGUISYSTEM_H
#define RENDERGUISYSTEM_H

#include "../ECS/ECS.h"
#include "../Components/TransformComponent.h"
#include "../Components/RigidBodyComponent.h"
#include "../Components/SpriteComponent.h"
#include "../Components/BoxColliderComponent.h"
#include "../Components/ProjectileEmitterComponent.h"
#include "../Components/HealthComponent.h"
#include "../Components/TextLabelComponent.h"
#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl2.h>
#include <imgui/imgui_impl_sdlrenderer2.h>
#include <string>

class RenderGUISystem: public System {
	public:
		RenderGUISystem() = default;

		void Update(SDL_Renderer* renderer, const std::unique_ptr<Registry>& registry, const std::unique_ptr<AssetStore>& assetStore) {
			ImGui_ImplSDLRenderer2_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			// TODO:
			//
			// Exercise:
			// Your task is to create GUI inputs to customize the new enemy
			//
			// For example, you should have different inputs for
			// - Enemy X & Y position, enemy scale, and enemy rotation
			// - Enemy X and Y velocity,
			// - A dropdown list to select the sprite texture id
			// - The angle and the speed of the projectiles being emitted
			// - The repeat frequency and duration of projectiles (in seconds)
			// - The initial health percentage

			std::vector<const char*> texturesList = assetStore->GetTexturesKeys();
			const char* items[texturesList.size()];
			int index = 0;

			for (auto& texture : texturesList) {
				items[index++] = texture;
			}

			static int item_current = 0;
			static ImGuiComboFlags flags = 0;
			const char* combo_preview_value = items[item_current];

			if (ImGui::Begin("Spawn enemies")) {
				static std::string chosen_sprite;
				if (ImGui::BeginCombo("Sprite", combo_preview_value, flags)) {
					for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
						const bool is_selected = (item_current == n);
						if (ImGui::Selectable(items[n], is_selected))
							item_current = n;

						if (is_selected)
							ImGui::SetItemDefaultFocus();
						chosen_sprite = items[item_current];
					}
					ImGui::EndCombo();
				}

				// Input for position X and Y
				static int xPos = 0;
				static int yPos = 0;
				static int enemyScale = 0;
				static int xVel = 0;
				static int yVel = 0;
				static int projectileVelX = 0;
				static int projectileVelY = 0;
				static int projectileFrequency = 0;
				static int projectileDuration = 0;
				static int projectileDamage = 0;
				static int enemyHealth = 0;

				if (ImGui::CollapsingHeader("Position, Velocity, Scale", ImGuiTreeNodeFlags_None)) {
					ImGui::InputInt("X Position", &xPos);
					ImGui::InputInt("Y Position", &yPos);
					ImGui::InputInt("X Velocity", &xVel);
					ImGui::InputInt("Y Velocity", &yVel);
					ImGui::InputInt("Scale", &enemyScale);
				}
				ImGui::Spacing();
				if (ImGui::CollapsingHeader("Projectile", ImGuiTreeNodeFlags_None)) {
					ImGui::InputInt("Projectile X Velocity", &projectileVelX);
					ImGui::InputInt("Projectile Y Velocity", &projectileVelY);
					ImGui::InputInt("Projectile Frequency (Seconds)", &projectileFrequency);
					ImGui::InputInt("Projectile Duration (Seconds)", &projectileDuration);
					ImGui::InputInt("Projectile Damage", &projectileDamage);
				}
				if (ImGui::CollapsingHeader("Enemy Health", ImGuiTreeNodeFlags_None)) {
					ImGui::InputInt("Health", &enemyHealth);
				}

				// Converting Projectile Frequency and Duration from ms to s
				//static int projectileFrequencySecs = projectileFrequency / 1000;
				//static int projectileDurationSecs = projectileDuration / 1000;
				if (ImGui::Button("Create new enemy")) {
					Entity enemy = registry ->CreateEntity();
					enemy.Group("enemies");
					enemy.AddComponent<TransformComponent>(glm::vec2(xPos, yPos), glm::vec2(enemyScale, enemyScale), 0.0);
					enemy.AddComponent<RigidBodyComponent>(glm::vec2(xVel, yVel));
					enemy.AddComponent<SpriteComponent>(chosen_sprite, 32, 32, 1);
					enemy.AddComponent<BoxColliderComponent>(32, 32);
					enemy.AddComponent<ProjectileEmitterComponent>(glm::vec2(projectileVelX, projectileVelY),
						   (projectileFrequency*1000),
						   (projectileDuration*1000),
						   projectileDamage,
						   false);
					enemy.AddComponent<HealthComponent>(enemyHealth);
					enemy.AddComponent<TextLabelComponent>(glm::vec2(0), " ", "charriot-font"); 
				}
			}
			ImGui::End();

			ImGui::Render();
			ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), renderer);
	}
};

#endif
