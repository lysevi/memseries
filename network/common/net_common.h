#pragma once

#include <cstdint>
#include <ostream>
#include <string>
#include <common/net_cmn_exports.h>

namespace dariadb {
namespace net {

const uint32_t PROTOCOL_VERSION = 1;

enum class DATA_KINDS : uint8_t {
  OK=0,
  ERR,
  HELLO,
  DISCONNECT,
  PING,
  PONG,
  APPEND,
  READ_INTERVAL,
  READ_TIMEPOINT,
  CURRENT_VALUE,
  SUBSCRIBE,
  COMPACT
};

enum class CLIENT_STATE {
  CONNECT, // connection is beginning but a while not ended.
  WORK,    // normal client.
  DISCONNETION_START,
  DISCONNECTED
};

enum class ERRORS : uint16_t {
  WRONG_PROTOCOL_VERSION,
  WRONG_QUERY_PARAM_FROM_GE_TO, // if in readInterval from>=to
};

CM_EXPORT std::ostream &operator<<(std::ostream &stream, const CLIENT_STATE &state);
CM_EXPORT std::ostream &operator<<(std::ostream &stream, const ERRORS &state);

typedef uint32_t QueryNumber;
}
}
