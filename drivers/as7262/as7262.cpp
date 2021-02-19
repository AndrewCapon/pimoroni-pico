#include <cstdlib>
#include <math.h>
#include <map>
#include <vector>

#include "as7262.hpp"

namespace pimoroni {

  /***** Device registers and masks here *****/

  enum reg {
    DEVICE      = 0x00,
    HW_VERSION  = 0x01,
    FW_VERSION  = 0x02, // + 0x03
    CONTROL     = 0x04,
    INT_T       = 0x05,
    TEMP        = 0x06,
    LED_CONTROL = 0x07,
    V_HIGH      = 0x08, // Violet
    V_LOW       = 0x09,
    B_HIGH      = 0x0A, // Blue
    B_LOW       = 0x0B,
    G_HIGH      = 0x0C, // Green
    G_LOW       = 0x0D,
    Y_HIGH      = 0x0E, // Yellow
    Y_LOW       = 0x0F,
    O_HIGH      = 0x10, // Orange
    O_LOW       = 0x11,
    R_HIGH      = 0x12, // Red
    R_LOW       = 0x13,
    V_CAL_F     = 0x14, // -> 0x17 Float (Violet)
    B_CAL_F     = 0x18, // -> 0x1B Float (Blue)
    G_CAL_F     = 0x1C, // -> 0x1F Float (Green)
    Y_CAL_F     = 0x20, // -> 0x23 Float (Yellow)
    O_CAL_F     = 0x24, // -> 0x27 Float (Orange)
    R_CAL_F     = 0x28, // -> 0x27 Float (Red)
  };


  bool AS7262::init() {
    bool succeeded = false;

    i2c_init(i2c, 400000);

    gpio_set_function(sda, GPIO_FUNC_I2C);
    gpio_pull_up(sda);
    gpio_set_function(scl, GPIO_FUNC_I2C);
    gpio_pull_up(scl);

    if(interrupt != PIN_UNUSED) {
      gpio_set_function(interrupt, GPIO_FUNC_SIO);
      gpio_set_dir(interrupt, GPIO_IN);
      gpio_pull_up(interrupt);
    }

    reset();

    /***** Replace if(true) with any operations needed to initialise the device *****/
    if(true) {
      succeeded = true;
    }

    return succeeded;
  }

  void AS7262::reset() {
    i2c_reg_write_uint8(reg::CONTROL, 0b10000000);
    sleep_ms(1000);
  }

  void AS7262::set_gain(gain gain) {
    uint8_t temp = i2c_reg_read_uint8(reg::CONTROL) & ~0b00110000;
    temp |= (uint8_t)gain << 4;
    i2c_reg_write_uint8(reg::CONTROL, temp);
  }

  void AS7262::set_measurement_mode(measurement_mode mode) {
    uint8_t temp = i2c_reg_read_uint8(reg::CONTROL) & ~0b00001100;
    temp |= (uint8_t)mode << 2;
    i2c_reg_write_uint8(reg::CONTROL, temp);
  }

  void AS7262::set_indicator_current(indicator_current current) {
    uint8_t temp = i2c_reg_read_uint8(reg::LED_CONTROL) & ~0b00000110;
    temp |= (uint8_t)current << 1;
    i2c_reg_write_uint8(reg::LED_CONTROL, temp);
  }

  void AS7262::set_illumination_current(illumination_current current) {
    uint8_t temp = i2c_reg_read_uint8(reg::LED_CONTROL) & ~0b00110000;
    temp |= (uint8_t)current << 4;
    i2c_reg_write_uint8(reg::LED_CONTROL, temp);
  }

  void AS7262::set_leds(bool illumination, bool indicator) {
    uint8_t temp = i2c_reg_read_uint8(reg::LED_CONTROL) & ~0b00001001;
    temp |= indicator ? 1 : 0;
    temp |= (illumination ? 1 : 0) << 3;
    i2c_reg_write_uint8(reg::LED_CONTROL, temp);
  }

  std::string AS7262::firmware_version() {
    std::string buf;
    uint16_t fw_version = i2c_reg_read_uint16(reg::FW_VERSION);
    buf += std::to_string(fw_version);
    return buf;
  }

  bool AS7262::data_ready() {
    return i2c_reg_read_uint8(reg::CONTROL) & 0b00000010;
  }

  AS7262::reading AS7262::read() {
    while(!data_ready()) {}
    return AS7262::reading {
      i2c_reg_read_float(reg::R_CAL_F),
      i2c_reg_read_float(reg::O_CAL_F),
      i2c_reg_read_float(reg::Y_CAL_F),
      i2c_reg_read_float(reg::G_CAL_F),
      i2c_reg_read_float(reg::B_CAL_F),
      i2c_reg_read_float(reg::V_CAL_F)
    };
  }

  uint8_t AS7262::temperature() {
    return i2c_reg_read_uint8(reg::TEMP);
  }

  // i2c IO wrappers around the weird virtual i2c nonsense

  void AS7262::i2c_reg_write_uint8(uint8_t reg, uint8_t value) {
    _i2c_write(reg, &value, 1);
  }

  float AS7262::i2c_reg_read_float(uint8_t reg) {
    float value;
    _i2c_read(reg, (uint8_t *)&value, 4);
    return __builtin_bswap32(value);
  }

  uint8_t AS7262::i2c_reg_read_uint8(uint8_t reg) {
    uint8_t value;
    _i2c_read(reg, &value, 1);
    return value;
  }

  uint16_t AS7262::i2c_reg_read_uint16(uint8_t reg) {
    uint16_t value;
    _i2c_read(reg, (uint8_t *)&value, 2);
    return value;
  }

  // Plumbing for virtual i2c
  void AS7262::_i2c_reg_write_uint8(uint8_t reg, uint8_t value) {
    uint8_t buffer[2] = {reg, value};
    i2c_write_blocking(i2c, address, buffer, 2, false);
  }

  uint8_t AS7262::_i2c_reg_read_uint8(uint8_t reg) {
    uint8_t value;
    i2c_write_blocking(i2c, address, &reg, 1, false);
    i2c_read_blocking(i2c, address, (uint8_t *)&value, 1, false);
    return value;
  }

  uint8_t AS7262::_i2c_status() {
    return _i2c_reg_read_uint8(0x00);
  }

  int AS7262::_i2c_read(uint8_t reg, uint8_t *values, int len) {
    for (auto i = 0u; i < len; i++){
      while((_i2c_status() & 0b10) != 0) {};   // Wait for write-ready
      _i2c_reg_write_uint8(0x01, reg + i);     // Set address pointer
      while((_i2c_status() & 0b01) != 1) {};   // Wait for read-ready
      values[i] = _i2c_reg_read_uint8(0x02);   // Read *one* byte :|
    }
    return 0;
  }

  int AS7262::_i2c_write(uint8_t reg, uint8_t *values, int len) {
    for (auto i = 0u; i < len; i++){
      while((_i2c_status() & 0b10) != 0) {};   // Wait for write-ready
      _i2c_reg_write_uint8(0x01, reg | 0x80);  // Set address pointer
      while ((_i2c_status() & 0b10) != 0) {};  // Wait for write-ready
      _i2c_reg_write_uint8(0x01, values[i]);   // Write *one* byte :|
    }
    return 0;
  }
}