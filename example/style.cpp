#include <print>
#include <vector>
#include "parf/parf.hpp"

struct[[= parf::CamelCase]] Player {
  int _id;
  int Health;
  std::string m_name;
  std::vector<std::string> __inventory;
  std::vector<std::string> quick_slots_;
};

struct[[= parf::SnakeCase]] Enemy {
  [[= parf::PascalCase]] int _id;
  [[ = parf::RenameGetter{"Health"},
     = parf::RenameSetter{"set_h"} ]] int health;
  [[= parf::NoNormalize]] std::string m_name;
  std::vector<std::string> __lootItems;
};

int main() {
  Player player{1, 100, "Hero", {"Sword", "Shield"}, {"Potion", "Bow"}};
  Enemy enemy{2, 50, "Goblin", {"Gold Coin", "Dagger"}};
  auto playerAccessor = parf::make_accessor(player);
  auto enemy_accessor = parf::make_accessor(enemy);

  std::println("Player ID: {}", playerAccessor.getId());
  std::println("Player Health: {}", playerAccessor.getHealth());
  std::println("Player Name: {}", playerAccessor.getName());
  std::println("Player Inventory: {}", playerAccessor.getInventory());
  std::println("Player Quick Slots: {}", playerAccessor.getQuickSlots());

  std::println("Enemy ID: {}", enemy_accessor.GetId());
  std::println("Enemy Health: {}", enemy_accessor.Health());
  std::println("Enemy Name: {}", enemy_accessor.get_m_name());
  std::println("Enemy Loot Items: {}", enemy_accessor.get_loot_items());
  return 0;
}