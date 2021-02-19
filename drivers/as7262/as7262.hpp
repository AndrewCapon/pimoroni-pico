#pragma once

#include <string>

#include "hardware/i2c.h"
#include "hardware/gpio.h"

namespace pimoroni {

  class AS7262 {
    //--------------------------------------------------
    // Constants
    //--------------------------------------------------
  public:
    static const uint8_t DEFAULT_I2C_ADDRESS  = 0x49;
    static const uint8_t DEFAULT_SDA_PIN      = 20;
    static const uint8_t DEFAULT_SCL_PIN      = 21;
    static const uint8_t DEFAULT_INT_PIN      = 22;
    static const uint8_t PIN_UNUSED           = UINT8_MAX;

    /***** More public constants here *****/

  private:
    /***** Private constants here *****/


    //--------------------------------------------------
    // Enums
    //--------------------------------------------------
  public:
    enum class gain : uint8_t {
      X1   = 0b00,
      X3_7 = 0b01,
      X16  = 0b10,
      X64  = 0b11
    };

    enum class illumination_current : uint8_t {
      ma12  = 0b00,
      ma25  = 0b01,
      ma50  = 0b10,
      ma100 = 0b11
    };

    enum class indicator_current : uint8_t {
      ma1 = 0b00,
      ma2 = 0b01,
      ma4 = 0b10,
      ma8 = 0b11,
    };

    enum class measurement_mode : uint8_t {
      cont_ygnv   = 0b00, // yellow, green, blue, violet - continuous
      cont_royg   = 0b01, // red, orange, yellow, green - continuous
      cont_roygbr = 0b10, // red, orange, yellow, green, violet - continuous
      oneshot     = 0b11  // everything - one-shot
    };


    //--------------------------------------------------
    // Substructures
    //--------------------------------------------------
  public:
    struct reading {
        float red;
        float orange;
        float yellow;
        float green;
        float blue;
        float violet;
    };


    //--------------------------------------------------
    // Variables
    //--------------------------------------------------
  private:
    i2c_inst_t *i2c = i2c0;

    // interface pins with our standard defaults where appropriate
    int8_t address    = DEFAULT_I2C_ADDRESS;
    int8_t sda        = DEFAULT_SDA_PIN;
    int8_t scl        = DEFAULT_SCL_PIN;
    int8_t interrupt  = DEFAULT_INT_PIN;


    //--------------------------------------------------
    // Constructors/Destructor
    //--------------------------------------------------
  public:
    AS7262() {}

    AS7262(i2c_inst_t *i2c, uint8_t sda, uint8_t scl, uint8_t interrupt = PIN_UNUSED) :
      i2c(i2c), sda(sda), scl(scl), interrupt(interrupt) {}


    //--------------------------------------------------
    // Methods
    //--------------------------------------------------
  public:
    bool init();
    void reset();

    uint8_t device_type();
    uint8_t hardware_version();
    std::string firmware_version();
    reading read();
    uint8_t temperature();

    void set_gain(gain gain);
    void set_measurement_mode(measurement_mode mode);
    void set_indicator_current(indicator_current current);
    void set_illumination_current(illumination_current current);
    void set_leds(bool illumination, bool indicator);

    // Virtual i2c transfers, routed through read/write/status regs
    uint8_t i2c_reg_read_uint8(uint8_t reg);
    void i2c_reg_write_uint8(uint8_t reg, uint8_t value);
    uint16_t i2c_reg_read_uint16(uint8_t reg);
    float i2c_reg_read_float(uint8_t reg);

  private:
    bool data_ready();
    uint8_t _i2c_status(); 
    int _i2c_read(uint8_t reg, uint8_t *values, int len);
    int _i2c_write(uint8_t reg, uint8_t *values, int len);

    // *Real* single-byte i2c transfers
    uint8_t _i2c_reg_read_uint8(uint8_t reg);
    void _i2c_reg_write_uint8(uint8_t reg, uint8_t value);
  };

}
