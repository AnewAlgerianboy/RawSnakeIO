#include "packet/p_snake.h"

std::ostream& operator<<(std::ostream& out, const packet_add_snake& p) {
  out << static_cast<PacketBase>(p);

  const Snake* s = p.s;

  // Protocol 11 Header Layout
  out << write_uint16(s->id);
  out << write_ang24(s->angle); // ehang (3 bytes)
  out << write_uint8(0);        // Unused 8th byte (always 0, original trace had 0x30 but 0 is safe)
  out << write_ang24(s->wangle); // wang (3 bytes)
  out << write_uint16(static_cast<uint16_t>(s->speed * 1000.0f / 32.0f)); // Speed
  out << write_fp24(s->fullness / 100.0f);
  out << write_uint8(s->skin);
  out << write_uint24(s->get_head_x() * 5.0f);
  out << write_uint24(s->get_head_y() * 5.0f);
  
  // Name
  // MUST use write_string to ensure the [Length][String] format.
  // Original trace: 03 31 32 33 (Len: 3, "123")
  out << write_string(s->name); 

  // Custom Skin Data
  // Standard format: [Length][Data]
  // Note: write_string automatically writes the length byte first.
  if (s->custom_skin_data.empty()) {
      out << write_uint8(0);
  } else {
      // Safety clamp for skin data length (max 255)
      if (s->custom_skin_data.size() > 255) {
          out << write_uint8(255); 
          out.write(s->custom_skin_data.data(), 255);
      } else {
          out << write_string(s->custom_skin_data);
      }
  }

  // Padding / Accessory ID
  // This byte is CRITICAL. The client reads [SkinLen], [SkinData], then [Accessory/Padding].
  // Without this, the client parser is misaligned by 1 byte for the body parts.
  out << write_uint8(0);

  // Body Parts
  if (!s->parts.empty()) {
    // Protocol 11 explicitly sends the TAIL position in absolute coords first
    const Body& tail = s->parts.back();
    float tx = tail.x;
    float ty = tail.y;
    
    out << write_uint24(static_cast<uint24_t>(tx * 5.0f))
        << write_uint24(static_cast<uint24_t>(ty * 5.0f));

    // Then relative coords from Tail -> Head
    for (size_t i = s->parts.size() - 1; i > 0; --i) {
        const Body& curr = s->parts[i];
        const Body& next = s->parts[i-1]; // Moving towards head

        float dx = next.x - curr.x;
        float dy = next.y - curr.y;

        out << write_uint8(static_cast<uint8_t>(dx * 2.0f + 127.0f))
            << write_uint8(static_cast<uint8_t>(dy * 2.0f + 127.0f));
    }
  }

  return out;
}

std::ostream& operator<<(std::ostream& out, const packet_remove_snake& p) {
  out << static_cast<PacketBase>(p);
  out << write_uint16(p.snakeId);
  out << write_uint8(p.status);
  return out;
}