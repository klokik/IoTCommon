
#include <cstddef>
#include <cstdint>


void serial_read(void *_data, size_t _length) {
}

bool serial_write(void *_data, size_t _length) {
  return true;
}

namespace l2 {
struct Message {
  uint32_t sync {0xffffffff};
  uint8_t data_length;
  uint8_t id;
  union {
    uint8_t data[16];
    uint8_t data8;
    uint16_t data16;
    uint32_t data32;
  };
  uint16_t CRC;
};

bool send(Message &msg) {
  ::serial_write(&msg, 6);
  ::serial_write(&msg.data[0], msg.data_length);
  ::serial_write(&msg.CRC, sizeof(msg.CRC));
  return true;
}

bool sendValue(uint8_t id, uint8_t length, void *data) {
  Message msg;
  msg.data_length = length;
  msg.id = id;
  msg.CRC = 0;

  uint8_t *ptr = static_cast<uint8_t*>(data);
  for (int i = 0; i < length; ++i) {
    msg.data[i] = *ptr++;
    msg.CRC += msg.data[i];
  }

  return send(msg);
}

Message read() {
  size_t counter {0};
  // wait for sync
  while (counter < 4) {
    uint8_t byte;
    ::serial_read(&byte, 1);
    if (byte == 0xff)
      counter += 1;
    else
      counter = 1;
  }

  Message msg;
  ::serial_read(&msg.data_length, 1);
  ::serial_read(&msg.id, 1);
  ::serial_read(&msg.data[0], msg.data_length);
  ::serial_read(&msg.CRC, 2);

  return msg;
}

size_t available() {
  // pass
  return 0;
}

}

struct VRM {
  void setDutyCycle(uint16_t cycle) {}
  bool voltageReady() {return false;}
  bool currentReady() {return false;}

  uint16_t voltage;
  uint16_t current;
  uint16_t power;
};

void updateControlLoop() {
  // pass
}

int main() {
  VRM vrm;

  while (true) {
    if (l2::available() >= 4) {
      l2::Message msg = l2::read();

      switch (msg.id) {
        case 0: // ping
          l2::sendValue(1, 0, nullptr);
          break;

        case 1: // ping_response
          break;

        case 2: // duty_cycle
          vrm.setDutyCycle(msg.data16);
          break;

        case 3: // current
        case 4: // voltage
        case 5: // power
          // pass
          break;

        default:
          // kek
          break;
      }
    }

    if (vrm.voltageReady()) {
      l2::sendValue(4, 2, &vrm.voltage);
    }

    if (vrm.currentReady()) {
      l2::sendValue(4, 2, &vrm.current);
      l2::sendValue(5, 2, &vrm.power);
    }

    updateControlLoop();
  }

  return 0;
}
