#ifndef SRC_PACKET_P_SNAKE_H_
#define SRC_PACKET_P_SNAKE_H_

#include "game/snake.h"
#include "packet/p_base.h"

// Sent when another snake enters range.
struct packet_add_snake : public PacketBase {
  packet_add_snake() : PacketBase(packet_t_snake), s(nullptr) {}

  // Added is_modern flag to constructor
  packet_add_snake(const Snake* input, bool modern)
      : PacketBase(packet_t_snake), s(input), is_modern(modern) {}

  const Snake* s;
  bool is_modern = false;

  size_t get_size() const noexcept {
    return 25 + s->name.length() + 2 * 3 + (s->parts.size() - 1 /* head */) * 2;
  }
};

// Sent when another snake leaves range or dies.
struct packet_remove_snake : public PacketBase {
  packet_remove_snake() : PacketBase(packet_t_snake) {}
  packet_remove_snake(uint16_t id, uint8_t st)
      : PacketBase(packet_t_snake), snakeId(id), status(st) {}

  uint16_t snakeId = 0;  // 3-4, int16, Snake id
  uint8_t status = status_snake_left;  // 5, int8, 0 (left) or 1 (died)

  size_t get_size() const noexcept { return 6; }

  static const uint8_t status_snake_left = 0;
  static const uint8_t status_snake_died = 1;
};

std::ostream& operator<<(std::ostream& out, const packet_add_snake& p);
std::ostream& operator<<(std::ostream& out, const packet_remove_snake& p);

#endif  // SRC_PACKET_P_SNAKE_H_