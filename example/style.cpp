#include <print>
#include <string>
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

template <typename T>
concept HasGetIdCamel = requires(T& t) { t.getId(); };

template <typename T>
concept HasSetIdCamel = requires(T& t, int value) { t.setId(value); };

template <typename T>
concept HasGetHealthCamel = requires(T& t) { t.getHealth(); };

template <typename T>
concept HasSetHealthCamel = requires(T& t, int value) { t.setHealth(value); };

template <typename T>
concept HasGetNameCamel = requires(T& t) { t.getName(); };

template <typename T>
concept HasSetNameCamel =
    requires(T& t, std::string value) { t.setName(value); };

template <typename T>
concept HasGetInventoryCamel = requires(T& t) { t.getInventory(); };

template <typename T>
concept HasSetInventoryCamel =
    requires(T& t, std::vector<std::string> value) { t.setInventory(value); };

template <typename T>
concept HasGetQuickSlotsCamel = requires(T& t) { t.getQuickSlots(); };

template <typename T>
concept HasSetQuickSlotsCamel =
    requires(T& t, std::vector<std::string> value) { t.setQuickSlots(value); };

// Enemy, [[= parf::SnakeCase]] -> snake_case by default ...
template <typename T>
concept HasGetIdSnake = requires(T& t) { t.get_id(); };

template <typename T>
concept HasSetIdSnake = requires(T& t, int value) { t.set_id(value); };

template <typename T>
concept HasGetHealthSnake = requires(T& t) { t.get_health(); };

template <typename T>
concept HasSetHealthSnake = requires(T& t, int value) { t.set_health(value); };

template <typename T>
concept HasGetNameSnake = requires(T& t) { t.get_name(); };

template <typename T>
concept HasSetNameSnake =
    requires(T& t, std::string value) { t.set_name(value); };

template <typename T>
concept HasGetLootItemsSnake = requires(T& t) { t.get_loot_items(); };

template <typename T>
concept HasSetLootItemsSnake =
    requires(T& t, std::vector<std::string> value) { t.set_loot_items(value); };

// ... overridden per member by [[= parf::PascalCase]], Rename*, NoNormalize.
template <typename T>
concept HasGetIdPascal = requires(T& t) { t.GetId(); };

template <typename T>
concept HasSetIdPascal = requires(T& t, int value) { t.SetId(value); };

template <typename T>
concept HasGetHealthRenamed = requires(T& t) { t.Health(); };

template <typename T>
concept HasSetHealthRenamed = requires(T& t, int value) { t.set_h(value); };

template <typename T>
concept HasGetNameRaw = requires(T& t) { t.get_m_name(); };

template <typename T>
concept HasSetNameRaw =
    requires(T& t, std::string value) { t.set_m_name(value); };

int main() {
  Player player{1, 100, "Hero", {"Sword", "Shield"}, {"Potion", "Bow"}};
  Enemy enemy{2, 50, "Goblin", {"Gold Coin", "Dagger"}};
  auto playerAccessor = parf::make_accessor(player);
  auto enemy_accessor = parf::make_accessor(enemy);

  using PlayerAccessor = decltype(playerAccessor);
  using EnemyAccessor = decltype(enemy_accessor);

  // [[= parf::CamelCase]] produces camelCase names.
  static_assert(HasGetIdCamel<PlayerAccessor>);
  static_assert(HasSetIdCamel<PlayerAccessor>);
  static_assert(HasGetHealthCamel<PlayerAccessor>);
  static_assert(HasSetHealthCamel<PlayerAccessor>);
  static_assert(HasGetNameCamel<PlayerAccessor>);
  static_assert(HasSetNameCamel<PlayerAccessor>);
  static_assert(HasGetInventoryCamel<PlayerAccessor>);
  static_assert(HasSetInventoryCamel<PlayerAccessor>);
  static_assert(HasGetQuickSlotsCamel<PlayerAccessor>);
  static_assert(HasSetQuickSlotsCamel<PlayerAccessor>);

  // ... and not snake_case or PascalCase.
  static_assert(!HasGetIdSnake<PlayerAccessor>);
  static_assert(!HasSetIdSnake<PlayerAccessor>);
  static_assert(!HasGetNameSnake<PlayerAccessor>);
  static_assert(!HasGetIdPascal<PlayerAccessor>);

  // Enemy defaults to [[= parf::SnakeCase]] with per-member overrides.
  static_assert(HasGetIdPascal<EnemyAccessor>);
  static_assert(HasSetIdPascal<EnemyAccessor>);
  static_assert(HasGetHealthRenamed<EnemyAccessor>);
  static_assert(HasSetHealthRenamed<EnemyAccessor>);
  static_assert(HasGetNameRaw<EnemyAccessor>);
  static_assert(HasSetNameRaw<EnemyAccessor>);
  static_assert(HasGetLootItemsSnake<EnemyAccessor>);
  static_assert(HasSetLootItemsSnake<EnemyAccessor>);

  // The overrides replace the default spellings.
  static_assert(!HasGetIdCamel<EnemyAccessor>);
  static_assert(!HasGetIdSnake<EnemyAccessor>);
  static_assert(!HasGetHealthCamel<EnemyAccessor>);
  static_assert(!HasSetHealthCamel<EnemyAccessor>);
  static_assert(!HasGetHealthSnake<EnemyAccessor>);
  static_assert(!HasSetHealthSnake<EnemyAccessor>);
  static_assert(!HasGetNameCamel<EnemyAccessor>);
  static_assert(!HasSetNameSnake<EnemyAccessor>);

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
