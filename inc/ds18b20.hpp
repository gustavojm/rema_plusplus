/*
 * Copyright (c) 2010-2011, Fabian Greif
 * Copyright (c) 2012-2014, Niklas Hauser
 * Copyright (c) 2022, Christopher Durand
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------
#ifndef DS18B20_HPP
#define DS18B20_HPP

#include <cstring>
#include "one-wire_bitbang_master.hpp"

/**
 * \brief Measurement resolution setting
 *
 * The maximum conversion time doubles for every additional bit:
 * 9 bits:	93.75 ms
 * 10 bits:	187.5 ms
 * 11 bits:	375 ms
 * 12 bits:	750 ms
 */
struct ds18b20 {
  enum class Resolution : uint8_t {
    Bits9 = 0b0'00'11111,
    Bits10 = 0b0'01'11111,
    Bits11 = 0b0'10'11111,
    Bits12 = 0b0'11'11111
  };
};

/**
 * \author	Fabian Greif
 */
template <typename OneWire> class Ds18b20 : public ds18b20 {
public:
  /**
   * \brief	Constructor
   *
   * Default resolution is 12-bit.
   *
   * \param 	rom		8-byte unique ROM number of the device
   */
  Ds18b20(const uint8_t *rom) { std::memcpy(this->identifier, rom, 8); }

  /**
   * \brief	Configure measurement resolution
   *
   * Set the measurement resolution. Every additional bit doubles the
   * measurement time.
   *
   * \return	\c true if a device is found, \c false if no
   * 			device is available on the bus.
   */
  bool configure(Resolution resolution) {
    if (!selectDevice()) {
      return false;
    }

    ow.writeByte(this->WRITE_SCRATCHPAD);
    ow.writeByte(0); // alert high value, not supported
    ow.writeByte(0); // alert low value, not supported
    ow.writeByte(static_cast<uint8_t>(resolution));
    return true;
  }

  /**
   * \brief	Check if the device is present
   *
   * \return	\c true if the device is found, \c false if the
   * 			ROM number is not available on the bus.
   */
  bool isAvailable() { return ow.verifyDevice(this->identifier); }

  /**
   * \brief	Start the conversion of this device
   */
  void startConversion() {
    selectDevice();

    ow.writeByte(CONVERT_T);
  }

  /**
   * \brief	Start the conversion for all connected devices
   *
   * Uses the SKIP_ROM command to start the conversion on all
   * connected DS18B20 devices.
   *
   * \warning	Use this function with caution. If you have devices other
   * 			than the DS18B20 connected to your 1-wire bus check if
   * 			they tolerate the SKIP_ROM + CONVERT_T command.
   */
  void startConversions() {
    // Reset the bus / Initialization
    if (!ow.touchReset()) {
      // no devices detected
      return;
    }

    // Send this to everybody
    ow.writeByte(one_wire::SKIP_ROM);

    // Issue Convert Temperature command
    ow.writeByte(this->CONVERT_T);
  }

  /**
   * \brief	Check if the last conversion is complete
   *
   * \return	\c true conversion complete, \n
   * 			\c false conversion in progress.
   */
  bool isConversionDone() { return ow.readBit(); }

  /**
   * \brief	Read the current temperature in centi-degree
   *
   * \todo	Needs a better output format
   * \return	temperature in centi-degree
   */
  int16_t readTemperature() {
    selectDevice();
    ow.writeByte(this->READ_SCRATCHPAD);

    // Read the first bytes of the scratchpad memory
    // and then send a reset because we do not want the other bytes

    int16_t temp = ow.readByte();
    temp |= (ow.readByte() << 8);

    ow.touchReset();

    int32_t convertedTemperature = INT32_C(625) * temp;

    // round to centi-degree
    convertedTemperature = (convertedTemperature + 50) / 100;

    return (static_cast<int16_t>(convertedTemperature));
  }

protected:
  /**
   * \brief	Select the current device
   */
  bool selectDevice() {
    // Reset the bus / Initialization
    if (!ow.touchReset()) {
      // no devices detected
      return false;
    }

    ow.writeByte(one_wire::MATCH_ROM);

    for (uint8_t i = 0; i < 8; ++i) {
      ow.writeByte(this->identifier[i]);
    }

    return true;
  }

  uint8_t identifier[8];

  static const uint8_t CONVERT_T = 0x44;
  static const uint8_t WRITE_SCRATCHPAD = 0x4e;
  static const uint8_t READ_SCRATCHPAD = 0xbe;
  static const uint8_t COPY_SCRATCHPAD = 0x48;
  static const uint8_t RECALL_E2 = 0xb8;
  static const uint8_t READ_POWER_SUPPLY = 0xb4;

  static OneWire ow;
};

#endif // DS18B20_HPP
