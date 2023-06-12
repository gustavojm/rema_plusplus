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
#ifndef ONE_WIRE_BITBANG_HPP
#define ONE_WIRE_BITBANG_HPP

#include <gpio.h>
#include <chrono>

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
enum RomCommand
{
    SEARCH_ROM = 0xf0,
    READ_ROM = 0x33,
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
template<typename Pin>
class BitBangOneWireMaster {
    using PIN = Pin;

public:
    template<class ... Signals>
    static void connect() {
        PIN::init_output();
    }

    template<class SystemClock>
    static void initialize() {
        PIN::set();
    }

    /**
     * Generate a 1-wire reset
     *
     * \return	\c true devices detected, \n
     * 			\c false failed to detect devices
     */
    static bool
    touchReset();

    /**
     * Send a 1-wire write bit.direction_pin
     *
     * Provides 10us recovery time.
     */
    static void
    writeBit(bool bit);

    /**
     * Read a bit from the 1-wire bus and return it.
     *
     * Provides 10us recovery time.
     */
    static bool
    readBit();

    /// Write 1-Wire data byte
    static void
    writeByte(uint8_t data);

    /// Read 1-Wire data byte and return it
    static uint8_t
    readByte();

    /// Write a 1-Wire data byte and return the sampled result.
    static uint8_t
    touchByte(uint8_t data);

    /**
     * Verify that the with the given ROM number is present
     *
     * \param 	rom		8-byte ROM number
     * \return	\c true device presens verified, \n
     * 			\c false device not present
     */
    static bool
    verifyDevice(const uint8_t *rom);

    /**
     * Reset search state
     * \see		searchNext()
     */
    static void
    resetSearch();

    /**
     * Reset search state and setup it to find the device type
     * 			'familyCode' on the next call to searchNext().
     *
     * This will accelerate the search because only devices of the given
     * type will be considered.
     */
    static void
    resetSearch(uint8_t familyCode);

    /**
     * Perform the 1-Wire search algorithm on the 1-Wire bus
     * 			using the existing search state.
     *
     * \param[out]	rom		8 byte array which will be filled with ROM
     * 						number of the device found.
     * \return	\c true is a device is found. \p rom will contain the
     * 			ROM number. \c false if no device found. This also
     * 			marks the end of the search.
     *
     * \see		resetSearch()
     */
    static bool
    searchNext(uint8_t *rom);

    /**
     * Setup the search to skip the current device type on the
     * 			next call to searchNext()
     */
    static void
    searchSkipCurrentFamily();

protected:
    static uint8_t
    crcUpdate(uint8_t crc, uint8_t data);

    /// Perform the actual search algorithm
    static bool
    performSearch();

    // standard delay times in microseconds
    static constexpr std::chrono::microseconds A { 6 };
    static constexpr std::chrono::microseconds B { 64 };
    static constexpr std::chrono::microseconds C { 60 };
    static constexpr std::chrono::microseconds D { 10 };
    static constexpr std::chrono::microseconds E { 9 };
    static constexpr std::chrono::microseconds F { 55 };
    static constexpr std::chrono::microseconds G { 0 };
    static constexpr std::chrono::microseconds H { 480 };
    static constexpr std::chrono::microseconds I { 70 };
    static constexpr std::chrono::microseconds J { 410 };

    // state of the search
    static uint8_t lastDiscrepancy;
    static uint8_t lastFamilyDiscrepancy;
    static bool lastDeviceFlag;
    static uint8_t crc8;
    static uint8_t romBuffer[8];
};

}
#endif // ONE-WIRE_BITBANG_HPP
