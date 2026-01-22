#ifndef SRC_SERVER_GAME_H_
#define SRC_SERVER_GAME_H_

#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <sstream>
#include <iomanip>

#include "server/server.h"

#include "game/world.h"

#include "packet/d_all.h"
#include "packet/p_all.h"

// ANSI color codes
#define COLOR_RESET   "\033[0m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_BOLD    "\033[1m"

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

struct Session {
  snake_id_t snake_id = 0;
  long last_packet_time = 0;

  // New: Track when the player died to delay the 'v' packet
  long death_timestamp = 0; 

  std::string name;
  std::string message;
  std::string custom_skin_data;

  uint8_t protocol_version = 0;  
  uint8_t skin = 0;              

  bool is_modern_protocol() const { 
      return protocol_version >= 25; // C Client is usually 31
  }

  Session() = default;
  Session(snake_id_t id, long now) : snake_id(id), last_packet_time(now) {}
};

class GameServer {
 public:
  GameServer();
  int Run(IncomingConfig in_config);
  PacketInit BuildInitPacket();

  typedef std::unordered_map<snake_id_t, connection_hdl> ConnectionMap;
  typedef std::map<connection_hdl, Session, std::owner_less<connection_hdl>> SessionMap;
  typedef SessionMap::iterator SessionIter;

 private:
  void on_socket_init(connection_hdl, boost::asio::ip::tcp::socket &s);  // NOLINT(runtime/references)
  void on_open(connection_hdl hdl);
  void on_message(connection_hdl hdl, message_ptr ptr);
  void on_close(connection_hdl hdl);
  void on_timer(error_code const &ec);

  void SendPOVUpdateTo(SessionIter ses_i, Snake *ptr);
  void SendFoodUpdate(Snake *ptr);
  void BroadcastDebug();
  void BroadcastUpdates();

  void BroadcastLeaderboard();
  void BroadcastMinimap();

  long last_leaderboard_time = 0;
  long last_minimap_time = 0;

  SessionIter LoadSessionIter(snake_id_t id);

  void DoSnake(snake_id_t id, std::function<void(Snake *)> f);
  void RemoveSnake(snake_id_t id);
  void RemoveDeadSnakes();

  long GetCurrentTime();
  void NextTick(long last);

  void PrintWorldInfo();

 private:
  // Helper for hex dump in templates
  static std::string packet_to_hex(const std::string& data, size_t max_bytes = 32) {
    std::stringstream ss;
    size_t len = std::min(data.size(), max_bytes);
    for (size_t i = 0; i < len; ++i) {
      ss << std::hex << std::setfill('0') << std::setw(2) 
         << (int)(unsigned char)data[i] << " ";
    }
    if (data.size() > max_bytes) ss << "...";
    return ss.str();
  }

  template <typename T>
  void send_binary(SessionMap::iterator s, T packet) {
    const long now = GetCurrentTime();
    const uint16_t interval =
        static_cast<uint16_t>(now - s->second.last_packet_time);
    s->second.last_packet_time = now;
    packet.client_time = interval;
    
    // Serialize packet to get data
    std::stringstream pkt_stream;
    pkt_stream << packet;
    std::string pkt_data = pkt_stream.str();
    
    // Log outgoing packet  
    // Packet structure: [time_delta:2 bytes][type:1 byte][data...]
    unsigned char pkt_type = pkt_data.size() >= 3 ? (unsigned char)pkt_data[2] : 0;
    std::stringstream log_out;
    log_out << COLOR_GREEN << COLOR_BOLD << ">>> SEND" << COLOR_RESET
            << COLOR_GREEN << " [" << pkt_data.size() << " bytes] "
            << "type='" << (char)pkt_type << "' (0x" 
            << std::hex << (int)pkt_type << std::dec << ")" 
            << COLOR_RESET << "\n"
            << "                " << packet_to_hex(pkt_data);
    endpoint.get_alog().write(alevel::app, log_out.str());
    
    endpoint.send_binary(s->first, packet);
  }

  template <typename T>
  void broadcast_binary(T packet) {
    const long now = GetCurrentTime();
    
    // Serialize once for logging
    std::stringstream pkt_stream;
    pkt_stream << packet;
    std::string pkt_data = pkt_stream.str();
    
    // Only log if we have human players (non-empty sessions means someone connected)
    // and skip frequent movement packets (0x67 'g') to reduce noise
    bool should_log = !sessions.empty();
    // Packet structure: [time_delta:2 bytes][type:1 byte][data...]
    unsigned char pkt_type = pkt_data.size() >= 3 ? (unsigned char)pkt_data[2] : 0;
    
    // Skip logging movement packets (0x67 = 'g') as they flood the console
    if (should_log && pkt_type != 0x67) {
      std::stringstream log_out;
      log_out << COLOR_MAGENTA << COLOR_BOLD << ">>> BROADCAST" << COLOR_RESET
              << COLOR_MAGENTA << " [" << pkt_data.size() << " bytes] "
              << "type='" << (char)pkt_type << "' (0x" 
              << std::hex << (int)pkt_type << std::dec << ")" 
              << COLOR_RESET << "\n"
              << "                " << packet_to_hex(pkt_data);
      endpoint.get_alog().write(alevel::app, log_out.str());
    }
    
    for (auto &s : sessions) {
      // FIX: CRITICAL CHECK
      // If snake_id is 0, the client is still in the menu/handshake phase.
      // Sending them game data ('g', '3', 'c', etc.) violates the protocol and crashes them.
      if (s.second.snake_id == 0) {
        continue;
      }

      const uint16_t interval =
          static_cast<uint16_t>(now - s.second.last_packet_time);
      s.second.last_packet_time = now;
      packet.client_time = interval;
      endpoint.send_binary(s.first, packet);
    }
  }

  template <typename T>
  void broadcast_debug(T packet) {
    for (auto &s : sessions) {
      endpoint.send_binary(s.first, packet);
    }
  }

  WSPPServer endpoint;

  WSPPServer::timer_ptr timer;
  long last_time_point;
  static const long timer_interval_ms = 10;

  World world;
  PacketInit init;
  IncomingConfig config;

  // TODO(john.koepi): reserve to collections
  SessionMap sessions;
  ConnectionMap connections;
};

#endif  // SRC_SERVER_GAME_H_
