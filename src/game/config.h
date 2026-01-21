#ifndef SRC_GAME_CONFIG_H_
#define SRC_GAME_CONFIG_H_

#include <cstdint>

typedef uint16_t snake_id_t;

struct WorldConfig {
  // generate bots on start count
  uint16_t bots = 0;

  // average snake length on init
  uint16_t snake_average_length = 2;
  uint16_t snake_min_length = 2;

  static const uint16_t game_radius = 21600;
  static const uint16_t max_snake_parts = 411;

  // Official Sector Size is 480 (45 x 480 = 21600 radius * 2 width)
  // Note: Offline mode sometimes uses 450, but 480 is standard grid.
  static const uint16_t sector_size = 480; 

  // 21600 * 2 / 480 = 90 sectors wide
  static const uint16_t sector_count_along_edge = 90; 
  
  static const uint16_t death_radius = game_radius - sector_size;
  static const uint16_t sector_diag_size = 680; // sqrt(480^2 + 480^2)
  static const uint16_t move_step_distance = 42; 
  
  static const long frame_time_ms = 8;
  static const uint8_t protocol_version = 14;
};

#endif  // SRC_GAME_CONFIG_H_
