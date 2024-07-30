/*
 * Copyright (c) 2010-2011, Fabian Greif
 * Copyright (c) 2012-2014, 2017, Niklas Hauser
 * Copyright (c) 2012, 2014, Sascha Schade
 *
 * This file is part of the modm project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */
// ----------------------------------------------------------------------------
#pragma once

#include "wait.h"
#include <chrono>
#include <gpio.h>

namespace one_wire {

/**
 * ROM Commands
 *
 * After the bus master has detected a presence pulse, it can issue a
 * ROM command. These commands operate on the unique 64-bit ROM codes
 * of each slave device and allow the master to single out a specific
 * device if many are present on the 1-Wire bus.
 *
 * These commands also allow the master to determine how many and what
 * types of devices are present on the bus or if any device has
 * experienced an alarm condition.
 *
 * There are five ROM commands, and each command is 8 bits long.
 *
 * @ingroup modm_architecture_1_wire
 */
enum RomCommand {
  SEARCH_ROM = 0xf0,
  READ_ROM = SEARCH_ROM + 1,
  MATCH_ROM = 0x55,
  SKIP_ROM = 0xcc,
  ALARM_SEARCH = 0xec
};

/**
 * Software emulation of a 1-wire master
 *
 * 1-Wire is extremely timing critical. This implementation relies
 * on simple delay loops to achieve this timing. Any interrupt during
 * the operation can disturb the timing.
 *
 * You should make sure that no interrupt occurs during the 1-Wire
 * transmissions, for example by disabling interrupts.
 *
 * Based on the Maxim 1-Wire AppNote at
 * http://www.maxim-ic.com/appnotes.cfm/appnote_number/126
 *
 * 1-Wire Search Algorithm based on AppNote 187 at
 * http://www.maxim-ic.com/appnotes.cfm/appnote_number/187
 *
 * \ingroup	modm_platform_1_wire_bitbang
 */
template <class Pin> class BitBangOneWireMaster {

public:
  static void connect() { Pin::init_output(); }

  static void initialize() { Pin::init_input(); }

  /**
   * Generate a 1-wire reset
   *
   * \return	\c true devices detected, \n
   * 			\c false failed to detect devices
   */
  static bool touchReset() {
    taskENTER_CRITICAL();
    delay(G);
    Pin::init_output();
    Pin::reset(); // drives the bus low
    delay(H);
    Pin::init_input(); // releases the bus
    delay(I);

    // sample for presence pulse from slave
    bool result = !Pin::read();

    delay(J); // complete the reset sequence recovery
    taskEXIT_CRITICAL();
    return result;
  }

  /**
   * Send a 1-wire write bit.
   *
   * Provides 10us recovery time.
   */
  static void writeBit(bool bit) {
    taskENTER_CRITICAL();
    if (bit) {
      Pin::init_output();
      Pin::reset(); // Drives bus low
      delay(A);
      Pin::init_input(); // Releases the bus
      delay(B);          // Complete the time slot and 10us recovery
    } else {
      Pin::init_output();
      Pin::reset();
      delay(C);
      Pin::init_input();
      delay(D);
    }
    taskEXIT_CRITICAL();
  }

  /**
   * Read a bit from the 1-wire bus and return it.
   *
   * Provides 10us recovery time.
   */
  static bool readBit() {
    taskENTER_CRITICAL();
    Pin::init_output();
    Pin::reset(); // drives the bus low
    delay(A);
    Pin::init_input(); // releases the bus
    delay(E);

    // Sample the bit value from the slave
    bool result = Pin::read();

    delay(F); // complete the reset sequence recovery
    taskEXIT_CRITICAL();
    return result;
  }

  /// Write 1-Wire data byte
  static void writeByte(uint8_t data) {
    // Loop to write each bit in the byte, LS-bit first
    for (uint8_t i = 0; i < 8; ++i) {
      writeBit(data & 0x01);
      data >>= 1;
    }
  }

  /// Read 1-Wire data byte and return it
  static uint8_t readByte() {
    uint8_t result = 0;
    for (uint8_t i = 0; i < 8; ++i) {
      result >>= 1;
      if (readBit()) {
        result |= 0x80;
      }
    }

    return result;
  }

  /// Write a 1-Wire data byte and return the sampled result.
  static uint8_t touchByte(uint8_t data) {
    uint8_t result = 0;
    for (uint8_t i = 0; i < 8; ++i) {
      result >>= 1;

      // If sending a '1' then read a bit else write a '0'
      if (data & 0x01) {
        if (readBit()) {
          result |= 0x80;
        }
      } else {
        writeBit(0);
      }

      data >>= 1;
    }

    return result;
  }

  /**
   * Verify that the with the given ROM number is present
   *
   * \param 	rom		8-byte ROM number
   * \return	\c true device presens verified, \n
   * 			\c false device not present
   */
  static bool verifyDevice(const uint8_t *rom) {
    uint8_t romBufferBackup[8];
    bool result;

    // keep a backup copy of the current state
    for (uint8_t i = 0; i < 8; i++) {
      romBufferBackup[i] = romBuffer[i];
    }
    uint16_t ld_backup = lastDiscrepancy;
    bool ldf_backup = lastDeviceFlag;
    uint16_t lfd_backup = lastFamilyDiscrepancy;

    // set search to find the same device
    lastDiscrepancy = 64;
    lastDeviceFlag = false;
    if (performSearch()) {
      // check if same device found
      result = true;
      for (uint8_t i = 0; i < 8; i++) {
        if (rom[i] != romBuffer[i]) {
          result = false;
          break;
        }
      }
    } else {
      result = false;
    }

    // restore the search state
    for (uint8_t i = 0; i < 8; i++) {
      romBuffer[i] = romBufferBackup[i];
    }
    lastDiscrepancy = ld_backup;
    lastDeviceFlag = ldf_backup;
    lastFamilyDiscrepancy = lfd_backup;

    // return the result of the verify
    return result;
  }

  /**
   * Reset search state
   * \see		searchNext()
   */
  static void resetSearch() {
    // reset the search state
    lastDiscrepancy = 0;
    lastFamilyDiscrepancy = 0;
    lastDeviceFlag = false;
  }

  /**
   * Reset search state and setup it to find the device type
   * 			'familyCode' on the next call to searchNext().
   *
   * This will accelerate the search because only devices of the givenreadBit
   * type will be considered.
   */
  static void resetSearch(uint8_t familyCode) {
    // set the search state to find family type devices
    romBuffer[0] = familyCode;
    for (uint8_t i = 1; i < 8; ++i) {
      romBuffer[i] = 0;
    }

    lastDiscrepancy = 64;
    lastFamilyDiscrepancy = 0;
    lastDeviceFlag = false;
  }

  /**
   * Perform the 1-Wire search algorithm on the 1-Wire bus
   * 			using the existing search state.
   *
   * \param[out]	rom		8 byte array which will be filled with
   * ROM number of the device found. \return	\c true is a device is found. \p
   * rom will contain the ROM number. \c false if no device found. This also
   * 			marks the end of the search.
   *
   * \see		resetSearch()
   */
  static bool searchNext(uint8_t *rom) {
    if (performSearch()) {
      for (uint8_t i = 0; i < 8; ++i) {
        rom[i] = romBuffer[i];
      }
      return true;
    }
    return false;
  }

  /**
   * Setup the search to skip the current device type on the
   * 			next call to searchNext()
   */
  static void searchSkipCurrentFamily() {
    // set the Last discrepancy to last family discrepancy
    lastDiscrepancy = lastFamilyDiscrepancy;
    lastFamilyDiscrepancy = 0;

    // check for end of list
    if (lastDiscrepancy == 0) {
      lastDeviceFlag = true;
    }
  }

  static void delay(std::chrono::microseconds us) { wait_us(us.count()); }

protected:
  static uint8_t crcUpdate(uint8_t crc, uint8_t data) {
    crc = crc ^ data;
    for (uint_fast8_t i = 0; i < 8; ++i) {
      if (crc & 0x01) {
        crc = (crc >> 1) ^ 0x8C;
      } else {
        crc >>= 1;
      }
    }
    return crc;
  }

  /// Perform the actual search algorithm
  static bool performSearch() {
    bool searchResult = false;

    // if the last call was not the last one
    if (!lastDeviceFlag) {
      // 1-Wire reset
      if (!touchReset()) {
        // reset the search
        lastDiscrepancy = 0;
        lastDeviceFlag = false;
        lastFamilyDiscrepancy = 0;
        return false;
      }

      // issue the search command
      writeByte(0xF0);

      // initialize for search
      uint8_t idBitNumber = 1;
      uint8_t lastZeroBit = 0;
      uint8_t romByteNumber = 0;
      uint8_t romByteMask = 1;
      bool searchDirection;

      crc8 = 0;

      // loop to do the search
      do {
        // read a bit and its complement
        bool idBit = readBit();
        bool complementIdBit = readBit();

        // check for no devices on 1-wire
        if ((idBit == true) && (complementIdBit == true)) {
          break;
        } else {
          // all devices coupled have 0 or 1
          if (idBit != complementIdBit) {
            searchDirection = idBit; // bit write value for search
          } else {
            // if this discrepancy if before the Last Discrepancy
            // on a previous next then pick the same as last time
            if (idBitNumber < lastDiscrepancy) {
              searchDirection = ((romBuffer[romByteNumber] & romByteMask) > 0);
            } else {
              // if equal to last pick 1, if not then pick 0
              searchDirection = (idBitNumber == lastDiscrepancy);
            }

            // if 0 was picked then record its position in LastZero
            if (searchDirection == false) {
              lastZeroBit = idBitNumber;
              // check for Last discrepancy in family
              if (lastZeroBit < 9)
                lastFamilyDiscrepancy = lastZeroBit;
            }
          }

          // set or clear the bit in the ROM byte rom_byte_number
          // with mask rom_byte_mask
          if (searchDirection == true) {
            romBuffer[romByteNumber] |= romByteMask;
          } else {
            romBuffer[romByteNumber] &= ~romByteMask;
          }

          // serial number search direction write bit
          writeBit(searchDirection);

          // increment the byte counter id_bit_number
          // and shift the mask rom_byte_mask
          idBitNumber++;
          romByteMask <<= 1;

          // if the mask is 0 then go to new SerialNum byte rom_byte_number and
          // reset mask
          if (romByteMask == 0) {
            crc8 =
                crcUpdate(crc8, romBuffer[romByteNumber]); // accumulate the CRC
            romByteNumber++;
            romByteMask = 1;
          }
        }
      } while (romByteNumber < 8); // loop until through all ROM bytes 0-7

      // if the search was successful then
      if (!((idBitNumber < 65) || (crc8 != 0))) {
        // search successful
        lastDiscrepancy = lastZeroBit;
        // check for last device
        if (lastDiscrepancy == 0) {
          lastDeviceFlag = true;
        }
        searchResult = true;
      }
    }

    // if no device found then reset counters so next 'search' will be like a
    // first
    if (!searchResult || !romBuffer[0]) {
      lastDiscrepancy = 0;
      lastDeviceFlag = false;
      lastFamilyDiscrepancy = 0;
      searchResult = false;
    }

    return searchResult;
  }

public:
  // standard delay times in microseconds
  static constexpr std::chrono::microseconds A{6};
  static constexpr std::chrono::microseconds B{64};
  static constexpr std::chrono::microseconds C{60};
  static constexpr std::chrono::microseconds D{10};
  static constexpr std::chrono::microseconds E{9};
  static constexpr std::chrono::microseconds F{55};
  static constexpr std::chrono::microseconds G{0};
  static constexpr std::chrono::microseconds H{480};
  static constexpr std::chrono::microseconds I{70};
  static constexpr std::chrono::microseconds J{410};

  // state of the search
  static uint8_t lastDiscrepancy;
  static uint8_t lastFamilyDiscrepancy;
  static bool lastDeviceFlag;
  static uint8_t crc8;
  static uint8_t romBuffer[8];
};

template <typename Pin>
uint8_t one_wire::BitBangOneWireMaster<Pin>::lastDiscrepancy;
template <typename Pin>
uint8_t one_wire::BitBangOneWireMaster<Pin>::lastFamilyDiscrepancy;
template <typename Pin>
bool one_wire::BitBangOneWireMaster<Pin>::lastDeviceFlag;
template <typename Pin> uint8_t one_wire::BitBangOneWireMaster<Pin>::crc8;
template <typename Pin>
uint8_t one_wire::BitBangOneWireMaster<Pin>::romBuffer[8];

template <typename Pin>
constexpr std::chrono::microseconds one_wire::BitBangOneWireMaster<Pin>::A;
template <typename Pin>
constexpr std::chrono::microseconds one_wire::BitBangOneWireMaster<Pin>::B;
template <typename Pin>
constexpr std::chrono::microseconds one_wire::BitBangOneWireMaster<Pin>::C;
template <typename Pin>
constexpr std::chrono::microseconds one_wire::BitBangOneWireMaster<Pin>::D;
template <typename Pin>
constexpr std::chrono::microseconds one_wire::BitBangOneWireMaster<Pin>::E;
template <typename Pin>
constexpr std::chrono::microseconds one_wire::BitBangOneWireMaster<Pin>::F;
template <typename Pin>
constexpr std::chrono::microseconds one_wire::BitBangOneWireMaster<Pin>::G;
template <typename Pin>
constexpr std::chrono::microseconds one_wire::BitBangOneWireMaster<Pin>::H;
template <typename Pin>
constexpr std::chrono::microseconds one_wire::BitBangOneWireMaster<Pin>::I;
template <typename Pin>
constexpr std::chrono::microseconds one_wire::BitBangOneWireMaster<Pin>::J;

} // namespace one_wire

