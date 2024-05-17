/*
  An Arduino Library which allows you to communicate seamlessly with u-blox GNSS modules using the Configuration Interface

  SparkFun sells these at its website: www.sparkfun.com
  Do you like this library? Help support SparkFun. Buy a board!
  https://www.sparkfun.com/products/15136
  https://www.sparkfun.com/products/16481
  https://www.sparkfun.com/products/16344
  https://www.sparkfun.com/products/18037
  https://www.sparkfun.com/products/18719
  https://www.sparkfun.com/products/18774
  https://www.sparkfun.com/products/19663
  https://www.sparkfun.com/products/17722

  Original version by Nathan Seidle @ SparkFun Electronics, September 6th, 2018
  v2.0 rework by Paul Clark @ SparkFun Electronics, December 31st, 2020
  v3.0 rework by Paul Clark @ SparkFun Electronics, December 8th, 2022

  https://github.com/sparkfun/SparkFun_u-blox_GNSS_v3

  This library is an updated version of the popular SparkFun u-blox GNSS Arduino Library.
  v3 uses the u-blox Configuration Interface (VALSET and VALGET) to:
  detect the module (during begin)Lu configure message intervalsLu configure the base locationLu etc..

  This version of the library will not work with older GNSS modules.
  It is specifically written for newer modules like the ZED-F9P, ZED-F9R and MAX-M10S.
  For older modules, please use v2 of the library: https://github.com/sparkfun/SparkFun_u-blox_GNSS_Arduino_Library

  Development environment specifics:
  Arduino IDE 1.8.19

  SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).
  The MIT License (MIT)
  Copyright (c) 2018 SparkFun Electronics
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
  associated documentation files (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the Software is furnished to
  do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all copies or substantial
  portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
  NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

// Define the maximum length of a multi-CfgValset construct
// "This message is limited to containing a maximum of 64 key-value pairs"
#define  CFG_VALSET_MAX_KEYS  64

// These are the Bitfield layers definitions for the UBX-CFG-VALSET message (not to be confused with Bitfield deviceMask in UBX-CFG-CFG)
#define  VAL_LAYER_DEFAULT  0x7 // ONLY valid with getVal()
#define  VAL_LAYER_RAM  (1 << 0)
#define  VAL_LAYER_BBR  (1 << 1)
#define  VAL_LAYER_FLASH  (1 << 2)
#define  VAL_LAYER_RAM_BBR  VAL_LAYER_RAM | VAL_LAYER_BBR               // Not valid with getVal()
#define  VAL_LAYER_ALL  VAL_LAYER_RAM | VAL_LAYER_BBR | VAL_LAYER_FLASH // Not valid with getVal()

// Below are various Groups, IDs, and sizes for various settings
// These can be used to call getVal/setVal/delVal
//--------------------------------------------------------
#define  VAL_SIZE_1  0x01  // One bit
#define  VAL_SIZE_8  0x02  // One byte
#define  VAL_SIZE_16  0x03 // Two bytes
#define  VAL_SIZE_32  0x04 // Four bytes
#define  VAL_SIZE_64  0x05 // Eight bytes

#define  VAL_ID_PROT_UBX  0x01
#define  VAL_ID_PROT_NMEA  0x02
#define  VAL_ID_PROT_RTCM3  0x04

#define  VAL_GROUP_I2C  0x51
#define  VAL_GROUP_I2COUTPROT  0x72
#define  VAL_GROUP_UART1INPROT  0x73
#define  VAL_GROUP_UART1OUTPROT  0x74
#define  VAL_GROUP_UART2INPROT  0x75
#define  VAL_GROUP_UART2OUTPROT  0x76
#define  VAL_GROUP_USBINPROT  0x77
#define  VAL_GROUP_USBOUTPROT  0x78

#define  VAL_GROUP_UART_SIZE  VAL_SIZE_1 // All fields in UART group are currently 1 bit
#define  VAL_GROUP_I2C_SIZE  VAL_SIZE_8  // All fields in I2C group are currently 1 byte

#define  VAL_ID_I2C_ADDRESS  0x01

// The following enum allows automatic identification of the Configuration Item data type.
// These are OR'd into the reserved bits in each Config Key ID.
// Based on an idea by Michael Ammann. Thank you @mazgch
#define UBX_CFG_L  0x01001000Lu        // bool
#define UBX_CFG_U1  0x01002000Lu       // uint8_t
#define UBX_CFG_U2  0x01003000Lu       // uint16_t
#define UBX_CFG_U4  0x01004000Lu       // uint32_t
#define UBX_CFG_U8  0x01005000Lu       // uint64_t
#define UBX_CFG_I1  0x01006000Lu       // int8_t
#define UBX_CFG_I2  0x01007000Lu       // int16_t
#define UBX_CFG_I4  0x01008000Lu       // int32_t
#define UBX_CFG_I8  0x01009000Lu       // int64_t
#define UBX_CFG_R4  0x0100A000Lu       // float (32-bit)
#define UBX_CFG_R8  0x0100B000Lu       // double (64-bit)
#define UBX_CFG_E1  0x0100C000Lu       // 8-bit enum  uint8_t
#define UBX_CFG_E2  0x0100D000Lu       // 16-bit enum  uint16_t
#define UBX_CFG_E4  0x0100E000Lu       // 32-bit enum  uint32_t
#define UBX_CFG_X1  0x0100F000Lu       // 8-bit bitfield / string  uint8_t
#define UBX_CFG_X2  0x02001000Lu       // 16-bit bitfield / string  uint16_t
#define UBX_CFG_X4  0x02002000Lu       // 32-bit bitfield / string  uint32_t
#define UBX_CFG_X8  0x02003000Lu       // 64-bit bitfield / string  uint64_t
#define UBX_CFG_SIZE_MASK  0x0F00F000Lu // Bit mask

// Below are the key values for a given configuration setting

// CFG-ANA: AssistNow Autonomous and Offline configuration
//--------------------------------------------------------
#define UBLOX_CFG_ANA_USE_ANA  UBX_CFG_L | 0x10230001Lu    // Use AssistNow Autonomous
#define UBLOX_CFG_ANA_ORBMAXERR  UBX_CFG_U2 | 0x30230002Lu // Maximum acceptable (modeled) orbit error in m. Range is from 5 to 1000.

// CFG-BDS: BeiDou system configuration
//--------------------------------------------------------
#define UBLOX_CFG_BDS_USE_PRN_1_TO_5  UBX_CFG_L | 0x10340014Lu // Use BeiDou geostationary satellites (PRN 1-5)

// CFG-GEOFENCE: Geofencing configuration
//--------------------------------------------------------
#define UBLOX_CFG_GEOFENCE_CONFLVL  UBX_CFG_E1 | 0x20240011Lu    // Required confidence level for state evaluation
#define UBLOX_CFG_GEOFENCE_USE_PIO  UBX_CFG_L | 0x10240012Lu     // Use PIO combined fence state output
#define UBLOX_CFG_GEOFENCE_PINPOL  UBX_CFG_E1 | 0x20240013Lu     // PIO pin polarity
#define UBLOX_CFG_GEOFENCE_PIN  UBX_CFG_U1 | 0x20240014Lu        // PIO pin number
#define UBLOX_CFG_GEOFENCE_USE_FENCE1  UBX_CFG_L | 0x10240020Lu  // Use frst geofence
#define UBLOX_CFG_GEOFENCE_FENCE1_LAT  UBX_CFG_I4 | 0x40240021Lu // Latitude of the first geofence circle center
#define UBLOX_CFG_GEOFENCE_FENCE1_LON  UBX_CFG_I4 | 0x40240022Lu // Longitude of the first geofence circle center
#define UBLOX_CFG_GEOFENCE_FENCE1_RAD  UBX_CFG_U4 | 0x40240023Lu // Radius of the first geofence circle
#define UBLOX_CFG_GEOFENCE_USE_FENCE2  UBX_CFG_L | 0x10240030Lu  // Use second geofence
#define UBLOX_CFG_GEOFENCE_FENCE2_LAT  UBX_CFG_I4 | 0x40240031Lu // Latitude of the second geofence circle center
#define UBLOX_CFG_GEOFENCE_FENCE2_LON  UBX_CFG_I4 | 0x40240032Lu // Longitude of the second geofence circle center
#define UBLOX_CFG_GEOFENCE_FENCE2_RAD  UBX_CFG_U4 | 0x40240033Lu // Radius of the second geofence circle
#define UBLOX_CFG_GEOFENCE_USE_FENCE3  UBX_CFG_L | 0x10240040Lu  // Use third geofence
#define UBLOX_CFG_GEOFENCE_FENCE3_LAT  UBX_CFG_I4 | 0x40240041Lu // Latitude of the third geofence circle center
#define UBLOX_CFG_GEOFENCE_FENCE3_LON  UBX_CFG_I4 | 0x40240042Lu // Longitude of the third geofence circle center
#define UBLOX_CFG_GEOFENCE_FENCE3_RAD  UBX_CFG_U4 | 0x40240043Lu // Radius of the third geofence circle
#define UBLOX_CFG_GEOFENCE_USE_FENCE4  UBX_CFG_L | 0x10240050Lu  // Use fourth geofence
#define UBLOX_CFG_GEOFENCE_FENCE4_LAT  UBX_CFG_I4 | 0x40240051Lu // Latitude of the fourth geofence circle center
#define UBLOX_CFG_GEOFENCE_FENCE4_LON  UBX_CFG_I4 | 0x40240052Lu // Longitude of the fourth geofence circle center
#define UBLOX_CFG_GEOFENCE_FENCE4_RAD  UBX_CFG_U4 | 0x40240053Lu // Radius of the fourth geofence circle

// CFG-HW: Hardware configuration
//--------------------------------------------------------
#define UBLOX_CFG_HW_ANT_CFG_VOLTCTRL  UBX_CFG_L | 0x10a3002eLu     // Active antenna voltage control flag
#define UBLOX_CFG_HW_ANT_CFG_SHORTDET  UBX_CFG_L | 0x10a3002fLu     // Short antenna detection flag
#define UBLOX_CFG_HW_ANT_CFG_SHORTDET_POL  UBX_CFG_L | 0x10a30030Lu // Short antenna detection polarity
#define UBLOX_CFG_HW_ANT_CFG_OPENDET  UBX_CFG_L | 0x10a30031Lu      // Open antenna detection flag
#define UBLOX_CFG_HW_ANT_CFG_OPENDET_POL  UBX_CFG_L | 0x10a30032Lu  // Open antenna detection polarity
#define UBLOX_CFG_HW_ANT_CFG_PWRDOWN  UBX_CFG_L | 0x10a30033Lu      // Power down antenna flag
#define UBLOX_CFG_HW_ANT_CFG_PWRDOWN_POL  UBX_CFG_L | 0x10a30034Lu  // Power down antenna logic polarity
#define UBLOX_CFG_HW_ANT_CFG_RECOVER  UBX_CFG_L | 0x10a30035Lu      // Automatic recovery from short state flag
#define UBLOX_CFG_HW_ANT_SUP_SWITCH_PIN  UBX_CFG_U1 | 0x20a30036Lu  // ANT1 PIO number
#define UBLOX_CFG_HW_ANT_SUP_SHORT_PIN  UBX_CFG_U1 | 0x20a30037Lu   // ANT0 PIO number
#define UBLOX_CFG_HW_ANT_SUP_OPEN_PIN  UBX_CFG_U1 | 0x20a30038Lu    // ANT2 PIO number
#define UBLOX_CFG_HW_ANT_SUP_ENGINE  UBX_CFG_E1 | 0x20a30054Lu      // Antenna supervisor engine selection
#define UBLOX_CFG_HW_ANT_SUP_SHORT_THR  UBX_CFG_U1 | 0x20a30055Lu   // Antenna supervisor MADC engine short detection threshold
#define UBLOX_CFG_HW_ANT_SUP_OPEN_THR  UBX_CFG_U1 | 0x20a30056Lu    // Antenna supervisor MADC engine open detection threshold

// CFG-I2C: Configuration of the I2C interface
//--------------------------------------------------------
#define UBLOX_CFG_I2C_ADDRESS  UBX_CFG_U1 | 0x20510001Lu        // I2C slave address of the receiver (7 bits)
#define UBLOX_CFG_I2C_EXTENDEDTIMEOUT  UBX_CFG_L | 0x10510002Lu // Flag to disable timeouting the interface after 1.5 s
#define UBLOX_CFG_I2C_ENABLED  UBX_CFG_L | 0x10510003Lu         // Flag to indicate if the I2C interface should be enabled

// CFG-I2CINPROT: Input protocol configuration of the I2C interface
//--------------------------------------------------------
#define UBLOX_CFG_I2CINPROT_UBX  UBX_CFG_L | 0x10710001Lu    // Flag to indicate if UBX should be an input protocol on I2C
#define UBLOX_CFG_I2CINPROT_NMEA  UBX_CFG_L | 0x10710002Lu   // Flag to indicate if NMEA should be an input protocol on I2C
#define UBLOX_CFG_I2CINPROT_RTCM3X  UBX_CFG_L | 0x10710004Lu // Flag to indicate if RTCM3X should be an input protocol on I2C
#define UBLOX_CFG_I2CINPROT_SPARTN  UBX_CFG_L | 0x10710005Lu // Flag to indicate if SPARTN should be an input protocol on I2C

// CFG-I2COUTPROT: Output protocol configuration of the I2C interface
//--------------------------------------------------------
#define UBLOX_CFG_I2COUTPROT_UBX  UBX_CFG_L | 0x10720001Lu    // Flag to indicate if UBX should be an output protocol on I2C
#define UBLOX_CFG_I2COUTPROT_NMEA  UBX_CFG_L | 0x10720002Lu   // Flag to indicate if NMEA should be an output protocol on I2C
#define UBLOX_CFG_I2COUTPROT_RTCM3X  UBX_CFG_L | 0x10720004Lu // Flag to indicate if RTCM3X should be an output protocol on I2C

// CFG-INFMSG: Information message configuration
//--------------------------------------------------------
#define UBLOX_CFG_INFMSG_UBX_I2C  UBX_CFG_X1 | 0x20920001Lu    // Information message enable flags for the UBX protocol on the I2C interface
#define UBLOX_CFG_INFMSG_UBX_UART1  UBX_CFG_X1 | 0x20920002Lu  // Information message enable flags for the UBX protocol on the UART1 interface
#define UBLOX_CFG_INFMSG_UBX_UART2  UBX_CFG_X1 | 0x20920003Lu  // Information message enable flags for the UBX protocol on the UART2 interface
#define UBLOX_CFG_INFMSG_UBX_USB  UBX_CFG_X1 | 0x20920004Lu    // Information message enable flags for the UBX protocol on the USB interface
#define UBLOX_CFG_INFMSG_UBX_SPI  UBX_CFG_X1 | 0x20920005Lu    // Information message enable flags for the UBX protocol on the SPI interface
#define UBLOX_CFG_INFMSG_NMEA_I2C  UBX_CFG_X1 | 0x20920006Lu   // Information message enable flags for the NMEA protocol on the I2C interface
#define UBLOX_CFG_INFMSG_NMEA_UART1  UBX_CFG_X1 | 0x20920007Lu // Information message enable flags for the NMEA protocol on the UART1 interface
#define UBLOX_CFG_INFMSG_NMEA_UART2  UBX_CFG_X1 | 0x20920008Lu // Information message enable flags for the NMEA protocol on the UART2 interface
#define UBLOX_CFG_INFMSG_NMEA_USB  UBX_CFG_X1 | 0x20920009Lu   // Information message enable flags for the NMEA protocol on the USB interface
#define UBLOX_CFG_INFMSG_NMEA_SPI  UBX_CFG_X1 | 0x2092000aLu   // Information message enable flags for the NMEA protocol on the SPI interface

// CFG-ITFM: Jamming and interference monitor configuration
//--------------------------------------------------------
#define UBLOX_CFG_ITFM_BBTHRESHOLD  UBX_CFG_U1 | 0x20410001Lu // Broadband jamming detection threshold
#define UBLOX_CFG_ITFM_CWTHRESHOLD  UBX_CFG_U1 | 0x20410002Lu // CW jamming detection threshold
#define UBLOX_CFG_ITFM_ENABLE  UBX_CFG_L | 0x1041000dLu       // Enable interference detection
#define UBLOX_CFG_ITFM_ANTSETTING  UBX_CFG_E1 | 0x20410010Lu  // Antenna setting
#define UBLOX_CFG_ITFM_ENABLE_AUX  UBX_CFG_L | 0x10410013Lu   // Scan auxiliary bands

// CFG-LOGFILTER: Data logger configuration
//--------------------------------------------------------
#define UBLOX_CFG_LOGFILTER_RECORD_ENA  UBX_CFG_L | 0x10de0002Lu           // Recording enabled
#define UBLOX_CFG_LOGFILTER_ONCE_PER_WAKE_UP_ENA  UBX_CFG_L | 0x10de0003Lu // Once per wake up
#define UBLOX_CFG_LOGFILTER_APPLY_ALL_FILTERS  UBX_CFG_U2 | 0x10de0004Lu   // Apply all filter settings
#define UBLOX_CFG_LOGFILTER_MIN_INTERVAL  UBX_CFG_U2 | 0x30de0005Lu        // Minimum time interval between loggedpositions
#define UBLOX_CFG_LOGFILTER_TIME_THRS  UBX_CFG_U2 | 0x30de0006Lu           // Time threshold
#define UBLOX_CFG_LOGFILTER_SPEED_THRS  UBX_CFG_U2 | 0x30de0007Lu          // Speed threshold
#define UBLOX_CFG_LOGFILTER_POSITION_THRS  UBX_CFG_U2 | 0x40de0008Lu       // Position threshold

// CFG-MOT: Motion detector configuration
//--------------------------------------------------------
#define UBLOX_CFG_MOT_GNSSSPEED_THRS  UBX_CFG_U1 | 0x20250038Lu // GNSS speed threshold below which platform is considered as stationary (a.k.a. static hold threshold)
#define UBLOX_CFG_MOT_GNSSDIST_THRS  UBX_CFG_U2 | 0x3025003bLu  // Distance above which GNSS-based stationary motion is exit (a.k.a. static hold distance threshold)

// CFG-MSGOUT: Message output configuration
//--------------------------------------------------------
// For each message and port a separate output rate (per second, per epoch) can be configured.
#define UBLOX_CFG_MSGOUT_NMEA_ID_DTM_I2C  UBX_CFG_U1 | 0x209100a6Lu          // Output rate of the NMEA-GX-DTM message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_DTM_SPI  UBX_CFG_U1 | 0x209100aaLu          // Output rate of the NMEA-GX-DTM message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_DTM_UART1  UBX_CFG_U1 | 0x209100a7Lu        // Output rate of the NMEA-GX-DTM message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_DTM_UART2  UBX_CFG_U1 | 0x209100a8Lu        // Output rate of the NMEA-GX-DTM message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_DTM_USB  UBX_CFG_U1 | 0x209100a9Lu          // Output rate of the NMEA-GX-DTM message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_GBS_I2C  UBX_CFG_U1 | 0x209100ddLu          // Output rate of the NMEA-GX-GBS message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_GBS_SPI  UBX_CFG_U1 | 0x209100e1Lu          // Output rate of the NMEA-GX-GBS message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_GBS_UART1  UBX_CFG_U1 | 0x209100deLu        // Output rate of the NMEA-GX-GBS message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_GBS_UART2  UBX_CFG_U1 | 0x209100dfLu        // Output rate of the NMEA-GX-GBS message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_GBS_USB  UBX_CFG_U1 | 0x209100e0Lu          // Output rate of the NMEA-GX-GBS message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_GGA_I2C  UBX_CFG_U1 | 0x209100baLu          // Output rate of the NMEA-GX-GGA message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_GGA_SPI  UBX_CFG_U1 | 0x209100beLu          // Output rate of the NMEA-GX-GGA message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_GGA_UART1  UBX_CFG_U1 | 0x209100bbLu        // Output rate of the NMEA-GX-GGA message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_GGA_UART2  UBX_CFG_U1 | 0x209100bcLu        // Output rate of the NMEA-GX-GGA message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_GGA_USB  UBX_CFG_U1 | 0x209100bdLu          // Output rate of the NMEA-GX-GGA message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_GLL_I2C  UBX_CFG_U1 | 0x209100c9Lu          // Output rate of the NMEA-GX-GLL message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_GLL_SPI  UBX_CFG_U1 | 0x209100cdLu          // Output rate of the NMEA-GX-GLL message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_GLL_UART1  UBX_CFG_U1 | 0x209100caLu        // Output rate of the NMEA-GX-GLL message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_GLL_UART2  UBX_CFG_U1 | 0x209100cbLu        // Output rate of the NMEA-GX-GLL message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_GLL_USB  UBX_CFG_U1 | 0x209100ccLu          // Output rate of the NMEA-GX-GLL message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_GNS_I2C  UBX_CFG_U1 | 0x209100b5Lu          // Output rate of the NMEA-GX-GNS message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_GNS_SPI  UBX_CFG_U1 | 0x209100b9Lu          // Output rate of the NMEA-GX-GNS message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_GNS_UART1  UBX_CFG_U1 | 0x209100b6Lu        // Output rate of the NMEA-GX-GNS message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_GNS_UART2  UBX_CFG_U1 | 0x209100b7Lu        // Output rate of the NMEA-GX-GNS message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_GNS_USB  UBX_CFG_U1 | 0x209100b8Lu          // Output rate of the NMEA-GX-GNS message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_GRS_I2C  UBX_CFG_U1 | 0x209100ceLu          // Output rate of the NMEA-GX-GRS message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_GRS_SPI  UBX_CFG_U1 | 0x209100d2Lu          // Output rate of the NMEA-GX-GRS message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_GRS_UART1  UBX_CFG_U1 | 0x209100cfLu        // Output rate of the NMEA-GX-GRS message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_GRS_UART2  UBX_CFG_U1 | 0x209100d0Lu        // Output rate of the NMEA-GX-GRS message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_GRS_USB  UBX_CFG_U1 | 0x209100d1Lu          // Output rate of the NMEA-GX-GRS message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_GSA_I2C  UBX_CFG_U1 | 0x209100bfLu          // Output rate of the NMEA-GX-GSA message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_GSA_SPI  UBX_CFG_U1 | 0x209100c3Lu          // Output rate of the NMEA-GX-GSA message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_GSA_UART1  UBX_CFG_U1 | 0x209100c0Lu        // Output rate of the NMEA-GX-GSA message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_GSA_UART2  UBX_CFG_U1 | 0x209100c1Lu        // Output rate of the NMEA-GX-GSA message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_GSA_USB  UBX_CFG_U1 | 0x209100c2Lu          // Output rate of the NMEA-GX-GSA message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_GST_I2C  UBX_CFG_U1 | 0x209100d3Lu          // Output rate of the NMEA-GX-GST message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_GST_SPI  UBX_CFG_U1 | 0x209100d7Lu          // Output rate of the NMEA-GX-GST message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_GST_UART1  UBX_CFG_U1 | 0x209100d4Lu        // Output rate of the NMEA-GX-GST message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_GST_UART2  UBX_CFG_U1 | 0x209100d5Lu        // Output rate of the NMEA-GX-GST message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_GST_USB  UBX_CFG_U1 | 0x209100d6Lu          // Output rate of the NMEA-GX-GST message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_GSV_I2C  UBX_CFG_U1 | 0x209100c4Lu          // Output rate of the NMEA-GX-GSV message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_GSV_SPI  UBX_CFG_U1 | 0x209100c8Lu          // Output rate of the NMEA-GX-GSV message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_GSV_UART1  UBX_CFG_U1 | 0x209100c5Lu        // Output rate of the NMEA-GX-GSV message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_GSV_UART2  UBX_CFG_U1 | 0x209100c6Lu        // Output rate of the NMEA-GX-GSV message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_GSV_USB  UBX_CFG_U1 | 0x209100c7Lu          // Output rate of the NMEA-GX-GSV message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_RLM_I2C  UBX_CFG_U1 | 0x20910400Lu          // Output rate of the NMEA-GX-RLM message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_RLM_SPI  UBX_CFG_U1 | 0x20910404Lu          // Output rate of the NMEA-GX-RLM message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_RLM_UART1  UBX_CFG_U1 | 0x20910401Lu        // Output rate of the NMEA-GX-RLM message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_RLM_UART2  UBX_CFG_U1 | 0x20910402Lu        // Output rate of the NMEA-GX-RLM message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_RLM_USB  UBX_CFG_U1 | 0x20910403Lu          // Output rate of the NMEA-GX-RLM message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_RMC_I2C  UBX_CFG_U1 | 0x209100abLu          // Output rate of the NMEA-GX-RMC message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_RMC_SPI  UBX_CFG_U1 | 0x209100afLu          // Output rate of the NMEA-GX-RMC message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_RMC_UART1  UBX_CFG_U1 | 0x209100acLu        // Output rate of the NMEA-GX-RMC message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_RMC_UART2  UBX_CFG_U1 | 0x209100adLu        // Output rate of the NMEA-GX-RMC message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_RMC_USB  UBX_CFG_U1 | 0x209100aeLu          // Output rate of the NMEA-GX-RMC message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_VLW_I2C  UBX_CFG_U1 | 0x209100e7Lu          // Output rate of the NMEA-GX-VLW message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_VLW_SPI  UBX_CFG_U1 | 0x209100ebLu          // Output rate of the NMEA-GX-VLW message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_VLW_UART1  UBX_CFG_U1 | 0x209100e8Lu        // Output rate of the NMEA-GX-VLW message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_VLW_UART2  UBX_CFG_U1 | 0x209100e9Lu        // Output rate of the NMEA-GX-VLW message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_VLW_USB  UBX_CFG_U1 | 0x209100eaLu          // Output rate of the NMEA-GX-VLW message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_VTG_I2C  UBX_CFG_U1 | 0x209100b0Lu          // Output rate of the NMEA-GX-VTG message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_VTG_SPI  UBX_CFG_U1 | 0x209100b4Lu          // Output rate of the NMEA-GX-VTG message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_VTG_UART1  UBX_CFG_U1 | 0x209100b1Lu        // Output rate of the NMEA-GX-VTG message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_VTG_UART2  UBX_CFG_U1 | 0x209100b2Lu        // Output rate of the NMEA-GX-VTG message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_VTG_USB  UBX_CFG_U1 | 0x209100b3Lu          // Output rate of the NMEA-GX-VTG message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_ZDA_I2C  UBX_CFG_U1 | 0x209100d8Lu          // Output rate of the NMEA-GX-ZDA message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_ZDA_SPI  UBX_CFG_U1 | 0x209100dcLu          // Output rate of the NMEA-GX-ZDA message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_ZDA_UART1  UBX_CFG_U1 | 0x209100d9Lu        // Output rate of the NMEA-GX-ZDA message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_ZDA_UART2  UBX_CFG_U1 | 0x209100daLu        // Output rate of the NMEA-GX-ZDA message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_ZDA_USB  UBX_CFG_U1 | 0x209100dbLu          // Output rate of the NMEA-GX-ZDA message on port USB
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYP_I2C  UBX_CFG_U1 | 0x209100ecLu        // Output rate of the NMEA-GX-PUBX00 message on port I2C
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYP_SPI  UBX_CFG_U1 | 0x209100f0Lu        // Output rate of the NMEA-GX-PUBX00 message on port SPI
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYP_UART1  UBX_CFG_U1 | 0x209100edLu      // Output rate of the NMEA-GX-PUBX00 message on port UART1
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYP_UART2  UBX_CFG_U1 | 0x209100eeLu      // Output rate of the NMEA-GX-PUBX00 message on port UART2
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYP_USB  UBX_CFG_U1 | 0x209100efLu        // Output rate of the NMEA-GX-PUBX00 message on port USB
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYS_I2C  UBX_CFG_U1 | 0x209100f1Lu        // Output rate of the NMEA-GX-PUBX03 message on port I2C
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYS_SPI  UBX_CFG_U1 | 0x209100f5Lu        // Output rate of the NMEA-GX-PUBX03 message on port SPI
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYS_UART1  UBX_CFG_U1 | 0x209100f2Lu      // Output rate of the NMEA-GX-PUBX03 message on port UART1
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYS_UART2  UBX_CFG_U1 | 0x209100f3Lu      // Output rate of the NMEA-GX-PUBX03 message on port UART2
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYS_USB  UBX_CFG_U1 | 0x209100f4Lu        // Output rate of the NMEA-GX-PUBX03 message on port USB
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYT_I2C  UBX_CFG_U1 | 0x209100f6Lu        // Output rate of the NMEA-GX-PUBX04 message on port I2C
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYT_SPI  UBX_CFG_U1 | 0x209100faLu        // Output rate of the NMEA-GX-PUBX04 message on port SPI
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYT_UART1  UBX_CFG_U1 | 0x209100f7Lu      // Output rate of the NMEA-GX-PUBX04 message on port UART1
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYT_UART2  UBX_CFG_U1 | 0x209100f8Lu      // Output rate of the NMEA-GX-PUBX04 message on port UART2
#define UBLOX_CFG_MSGOUT_PUBX_ID_POLYT_USB  UBX_CFG_U1 | 0x209100f9Lu        // Output rate of the NMEA-GX-PUBX04 message on port USB
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_I2C  UBX_CFG_U1 | 0x209102bdLu     // Output rate of the RTCM-3X-TYPE1005 message on port I2C
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_SPI  UBX_CFG_U1 | 0x209102c1Lu     // Output rate of the RTCM-3X-TYPE1005 message on port SPI
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_UART1  UBX_CFG_U1 | 0x209102beLu   // Output rate of the RTCM-3X-TYPE1005 message on port UART1
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_UART2  UBX_CFG_U1 | 0x209102bfLu   // Output rate of the RTCM-3X-TYPE1005 message on port UART2
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1005_USB  UBX_CFG_U1 | 0x209102c0Lu     // Output rate of the RTCM-3X-TYPE1005 message on port USB
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1074_I2C  UBX_CFG_U1 | 0x2091035eLu     // Output rate of the RTCM-3X-TYPE1074 message on port I2C
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1074_SPI  UBX_CFG_U1 | 0x20910362Lu     // Output rate of the RTCM-3X-TYPE1074 message on port SPI
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1074_UART1  UBX_CFG_U1 | 0x2091035fLu   // Output rate of the RTCM-3X-TYPE1074 message on port UART1
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1074_UART2  UBX_CFG_U1 | 0x20910360Lu   // Output rate of the RTCM-3X-TYPE1074 message on port UART2
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1074_USB  UBX_CFG_U1 | 0x20910361Lu     // Output rate of the RTCM-3X-TYPE1074 message on port USB
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1077_I2C  UBX_CFG_U1 | 0x209102ccLu     // Output rate of the RTCM-3X-TYPE1077 message on port I2C
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1077_SPI  UBX_CFG_U1 | 0x209102d0Lu     // Output rate of the RTCM-3X-TYPE1077 message on port SPI
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1077_UART1  UBX_CFG_U1 | 0x209102cdLu   // Output rate of the RTCM-3X-TYPE1077 message on port UART1
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1077_UART2  UBX_CFG_U1 | 0x209102ceLu   // Output rate of the RTCM-3X-TYPE1077 message on port UART2
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1077_USB  UBX_CFG_U1 | 0x209102cfLu     // Output rate of the RTCM-3X-TYPE1077 message on port USB
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1084_I2C  UBX_CFG_U1 | 0x20910363Lu     // Output rate of the RTCM-3X-TYPE1084 message on port I2C
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1084_SPI  UBX_CFG_U1 | 0x20910367Lu     // Output rate of the RTCM-3X-TYPE1084 message on port SPI
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1084_UART1  UBX_CFG_U1 | 0x20910364Lu   // Output rate of the RTCM-3X-TYPE1084 message on port UART1
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1084_UART2  UBX_CFG_U1 | 0x20910365Lu   // Output rate of the RTCM-3X-TYPE1084 message on port UART2
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1084_USB  UBX_CFG_U1 | 0x20910366Lu     // Output rate of the RTCM-3X-TYPE1084 message on port USB
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1087_I2C  UBX_CFG_U1 | 0x209102d1Lu     // Output rate of the RTCM-3X-TYPE1087 message on port I2C
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1087_SPI  UBX_CFG_U1 | 0x209102d5Lu     // Output rate of the RTCM-3X-TYPE1087 message on port SPI
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1087_UART1  UBX_CFG_U1 | 0x209102d2Lu   // Output rate of the RTCM-3X-TYPE1087 message on port UART1
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1087_UART2  UBX_CFG_U1 | 0x209102d3Lu   // Output rate of the RTCM-3X-TYPE1087 message on port UART2
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1087_USB  UBX_CFG_U1 | 0x209102d4Lu     // Output rate of the RTCM-3X-TYPE1087 message on port USB
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1094_I2C  UBX_CFG_U1 | 0x20910368Lu     // Output rate of the RTCM-3X-TYPE1094 message on port I2C
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1094_SPI  UBX_CFG_U1 | 0x2091036cLu     // Output rate of the RTCM-3X-TYPE1094 message on port SPI
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1094_UART1  UBX_CFG_U1 | 0x20910369Lu   // Output rate of the RTCM-3X-TYPE1094 message on port UART1
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1094_UART2  UBX_CFG_U1 | 0x2091036aLu   // Output rate of the RTCM-3X-TYPE1094 message on port UART2
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1094_USB  UBX_CFG_U1 | 0x2091036bLu     // Output rate of the RTCM-3X-TYPE1094 message on port USB
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1097_I2C  UBX_CFG_U1 | 0x20910318Lu     // Output rate of the RTCM-3X-TYPE1097 message on port I2C
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1097_SPI  UBX_CFG_U1 | 0x2091031cLu     // Output rate of the RTCM-3X-TYPE1097 message on port SPI
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1097_UART1  UBX_CFG_U1 | 0x20910319Lu   // Output rate of the RTCM-3X-TYPE1097 message on port UART1
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1097_UART2  UBX_CFG_U1 | 0x2091031aLu   // Output rate of the RTCM-3X-TYPE1097 message on port UART2
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1097_USB  UBX_CFG_U1 | 0x2091031bLu     // Output rate of the RTCM-3X-TYPE1097 message on port USB
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1124_I2C  UBX_CFG_U1 | 0x2091036dLu     // Output rate of the RTCM-3X-TYPE1124 message on port I2C
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1124_SPI  UBX_CFG_U1 | 0x20910371Lu     // Output rate of the RTCM-3X-TYPE1124 message on port SPI
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1124_UART1  UBX_CFG_U1 | 0x2091036eLu   // Output rate of the RTCM-3X-TYPE1124 message on port UART1
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1124_UART2  UBX_CFG_U1 | 0x2091036fLu   // Output rate of the RTCM-3X-TYPE1124 message on port UART2
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1124_USB  UBX_CFG_U1 | 0x20910370Lu     // Output rate of the RTCM-3X-TYPE1124 message on port USB
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1127_I2C  UBX_CFG_U1 | 0x209102d6Lu     // Output rate of the RTCM-3X-TYPE1127 message on port I2C
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1127_SPI  UBX_CFG_U1 | 0x209102daLu     // Output rate of the RTCM-3X-TYPE1127 message on port SPI
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1127_UART1  UBX_CFG_U1 | 0x209102d7Lu   // Output rate of the RTCM-3X-TYPE1127 message on port UART1
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1127_UART2  UBX_CFG_U1 | 0x209102d8Lu   // Output rate of the RTCM-3X-TYPE1127 message on port UART2
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1127_USB  UBX_CFG_U1 | 0x209102d9Lu     // Output rate of the RTCM-3X-TYPE1127 message on port USB
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_I2C  UBX_CFG_U1 | 0x20910303Lu     // Output rate of the RTCM-3X-TYPE1230 message on port I2C
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_SPI  UBX_CFG_U1 | 0x20910307Lu     // Output rate of the RTCM-3X-TYPE1230 message on port SPI
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_UART1  UBX_CFG_U1 | 0x20910304Lu   // Output rate of the RTCM-3X-TYPE1230 message on port UART1
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_UART2  UBX_CFG_U1 | 0x20910305Lu   // Output rate of the RTCM-3X-TYPE1230 message on port UART2
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE1230_USB  UBX_CFG_U1 | 0x20910306Lu     // Output rate of the RTCM-3X-TYPE1230 message on port USB
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_0_I2C  UBX_CFG_U1 | 0x209102feLu   // Output rate of the RTCM-3X-TYPE4072_0 message on port I2C
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_0_SPI  UBX_CFG_U1 | 0x20910302Lu   // Output rate of the RTCM-3X-TYPE4072_0 message on port SPI
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_0_UART1  UBX_CFG_U1 | 0x209102ffLu // Output rate of the RTCM-3X-TYPE4072_0 message on port UART1
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_0_UART2  UBX_CFG_U1 | 0x20910300Lu // Output rate of the RTCM-3X-TYPE4072_0 message on port UART2
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_0_USB  UBX_CFG_U1 | 0x20910301Lu   // Output rate of the RTCM-3X-TYPE4072_0 message on port USB
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_1_I2C  UBX_CFG_U1 | 0x20910381Lu   // Output rate of the RTCM-3X-TYPE4072_1 message on port I2C
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_1_SPI  UBX_CFG_U1 | 0x20910385Lu   // Output rate of the RTCM-3X-TYPE4072_1 message on port SPI
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_1_UART1  UBX_CFG_U1 | 0x20910382Lu // Output rate of the RTCM-3X-TYPE4072_1 message on port UART1
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_1_UART2  UBX_CFG_U1 | 0x20910383Lu // Output rate of the RTCM-3X-TYPE4072_1 message on port UART2
#define UBLOX_CFG_MSGOUT_RTCM_3X_TYPE4072_1_USB  UBX_CFG_U1 | 0x20910384Lu   // Output rate of the RTCM-3X-TYPE4072_1 message on port USB
#define UBLOX_CFG_MSGOUT_UBX_LOG_INFO_I2C  UBX_CFG_U1 | 0x20910259Lu         // Output rate of the UBX-LOG-INFO message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_LOG_INFO_SPI  UBX_CFG_U1 | 0x2091025dLu         // Output rate of the UBX-LOG-INFO message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_LOG_INFO_UART1  UBX_CFG_U1 | 0x2091025aLu       // Output rate of the UBX-LOG-INFO message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_LOG_INFO_UART2  UBX_CFG_U1 | 0x2091025bLu       // Output rate of the UBX-LOG-INFO message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_LOG_INFO_USB  UBX_CFG_U1 | 0x2091025cLu         // Output rate of the UBX-LOG-INFO message on port USB
#define UBLOX_CFG_MSGOUT_UBX_MON_COMMS_I2C  UBX_CFG_U1 | 0x2091034fLu        // Output rate of the UBX-MON-COMMS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_MON_COMMS_SPI  UBX_CFG_U1 | 0x20910353Lu        // Output rate of the UBX-MON-COMMS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_MON_COMMS_UART1  UBX_CFG_U1 | 0x20910350Lu      // Output rate of the UBX-MON-COMMS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_MON_COMMS_UART2  UBX_CFG_U1 | 0x20910351Lu      // Output rate of the UBX-MON-COMMS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_MON_COMMS_USB  UBX_CFG_U1 | 0x20910352Lu        // Output rate of the UBX-MON-COMMS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_MON_HW2_I2C  UBX_CFG_U1 | 0x209101b9Lu          // Output rate of the UBX-MON-HW2 message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_MON_HW2_SPI  UBX_CFG_U1 | 0x209101bdLu          // Output rate of the UBX-MON-HW2 message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_MON_HW2_UART1  UBX_CFG_U1 | 0x209101baLu        // Output rate of the UBX-MON-HW2 message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_MON_HW2_UART2  UBX_CFG_U1 | 0x209101bbLu        // Output rate of the UBX-MON-HW2 message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_MON_HW2_USB  UBX_CFG_U1 | 0x209101bcLu          // Output rate of the UBX-MON-HW2 message on port USB
#define UBLOX_CFG_MSGOUT_UBX_MON_HW3_I2C  UBX_CFG_U1 | 0x20910354Lu          // Output rate of the UBX-MON-HW3 message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_MON_HW3_SPI  UBX_CFG_U1 | 0x20910358Lu          // Output rate of the UBX-MON-HW3 message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_MON_HW3_UART1  UBX_CFG_U1 | 0x20910355Lu        // Output rate of the UBX-MON-HW3 message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_MON_HW3_UART2  UBX_CFG_U1 | 0x20910356Lu        // Output rate of the UBX-MON-HW3 message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_MON_HW3_USB  UBX_CFG_U1 | 0x20910357Lu          // Output rate of the UBX-MON-HW3 message on port USB
#define UBLOX_CFG_MSGOUT_UBX_MON_HW_I2C  UBX_CFG_U1 | 0x209101b4Lu           // Output rate of the UBX-MON-HW message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_MON_HW_SPI  UBX_CFG_U1 | 0x209101b8Lu           // Output rate of the UBX-MON-HW message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_MON_HW_UART1  UBX_CFG_U1 | 0x209101b5Lu         // Output rate of the UBX-MON-HW message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_MON_HW_UART2  UBX_CFG_U1 | 0x209101b6Lu         // Output rate of the UBX-MON-HW message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_MON_HW_USB  UBX_CFG_U1 | 0x209101b7Lu           // Output rate of the UBX-MON-HW message on port USB
#define UBLOX_CFG_MSGOUT_UBX_MON_IO_I2C  UBX_CFG_U1 | 0x209101a5Lu           // Output rate of the UBX-MON-IO message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_MON_IO_SPI  UBX_CFG_U1 | 0x209101a9Lu           // Output rate of the UBX-MON-IO message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_MON_IO_UART1  UBX_CFG_U1 | 0x209101a6Lu         // Output rate of the UBX-MON-IO message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_MON_IO_UART2  UBX_CFG_U1 | 0x209101a7Lu         // Output rate of the UBX-MON-IO message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_MON_IO_USB  UBX_CFG_U1 | 0x209101a8Lu           // Output rate of the UBX-MON-IO message on port USB
#define UBLOX_CFG_MSGOUT_UBX_MON_MSGPP_I2C  UBX_CFG_U1 | 0x20910196Lu        // Output rate of the UBX-MON-MSGPP message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_MON_MSGPP_SPI  UBX_CFG_U1 | 0x2091019aLu        // Output rate of the UBX-MON-MSGPP message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_MON_MSGPP_UART1  UBX_CFG_U1 | 0x20910197Lu      // Output rate of the UBX-MON-MSGPP message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_MON_MSGPP_UART2  UBX_CFG_U1 | 0x20910198Lu      // Output rate of the UBX-MON-MSGPP message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_MON_MSGPP_USB  UBX_CFG_U1 | 0x20910199Lu        // Output rate of the UBX-MON-MSGPP message on port USB
#define UBLOX_CFG_MSGOUT_UBX_MON_RF_I2C  UBX_CFG_U1 | 0x20910359Lu           // Output rate of the UBX-MON-RF message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_MON_RF_SPI  UBX_CFG_U1 | 0x2091035dLu           // Output rate of the UBX-MON-RF message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_MON_RF_UART1  UBX_CFG_U1 | 0x2091035aLu         // Output rate of the UBX-MON-RF message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_MON_RF_UART2  UBX_CFG_U1 | 0x2091035bLu         // Output rate of the UBX-MON-RF message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_MON_RF_USB  UBX_CFG_U1 | 0x2091035cLu           // Output rate of the UBX-MON-RF message on port USB
#define UBLOX_CFG_MSGOUT_UBX_MON_RXBUF_I2C  UBX_CFG_U1 | 0x209101a0Lu        // Output rate of the UBX-MON-RXBUF message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_MON_RXBUF_SPI  UBX_CFG_U1 | 0x209101a4Lu        // Output rate of the UBX-MON-RXBUF message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_MON_RXBUF_UART1  UBX_CFG_U1 | 0x209101a1Lu      // Output rate of the UBX-MON-RXBUF message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_MON_RXBUF_UART2  UBX_CFG_U1 | 0x209101a2Lu      // Output rate of the UBX-MON-RXBUF message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_MON_RXBUF_USB  UBX_CFG_U1 | 0x209101a3Lu        // Output rate of the UBX-MON-RXBUF message on port USB
#define UBLOX_CFG_MSGOUT_UBX_MON_RXR_I2C  UBX_CFG_U1 | 0x20910187Lu          // Output rate of the UBX-MON-RXR message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_MON_RXR_SPI  UBX_CFG_U1 | 0x2091018bLu          // Output rate of the UBX-MON-RXR message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_MON_RXR_UART1  UBX_CFG_U1 | 0x20910188Lu        // Output rate of the UBX-MON-RXR message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_MON_RXR_UART2  UBX_CFG_U1 | 0x20910189Lu        // Output rate of the UBX-MON-RXR message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_MON_RXR_USB  UBX_CFG_U1 | 0x2091018aLu          // Output rate of the UBX-MON-RXR message on port USB
#define UBLOX_CFG_MSGOUT_UBX_MON_SPAN_I2C  UBX_CFG_U1 | 0x2091038bLu         // Output rate of the UBX-MON-SPAN message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_MON_SPAN_SPI  UBX_CFG_U1 | 0x2091038fLu         // Output rate of the UBX-MON-SPAN message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_MON_SPAN_UART1  UBX_CFG_U1 | 0x2091038cLu       // Output rate of the UBX-MON-SPAN message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_MON_SPAN_UART2  UBX_CFG_U1 | 0x2091038dLu       // Output rate of the UBX-MON-SPAN message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_MON_SPAN_USB  UBX_CFG_U1 | 0x2091038eLu         // Output rate of the UBX-MON-SPAN message on port USB
#define UBLOX_CFG_MSGOUT_UBX_MON_SYS_I2C  UBX_CFG_U1 | 0x2091069dLu          // Output rate of the UBX-MON-SYS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_MON_SYS_SPI  UBX_CFG_U1 | 0x209106a1Lu          // Output rate of the UBX-MON-SYS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_MON_SYS_UART1  UBX_CFG_U1 | 0x2091069eLu        // Output rate of the UBX-MON-SYS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_MON_SYS_UART2  UBX_CFG_U1 | 0x2091069fLu        // Output rate of the UBX-MON-SYS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_MON_SYS_USB  UBX_CFG_U1 | 0x209106a0Lu          // Output rate of the UBX-MON-SYS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_MON_TXBUF_I2C  UBX_CFG_U1 | 0x2091019bLu        // Output rate of the UBX-MON-TXBUF message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_MON_TXBUF_SPI  UBX_CFG_U1 | 0x2091019fLu        // Output rate of the UBX-MON-TXBUF message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_MON_TXBUF_UART1  UBX_CFG_U1 | 0x2091019cLu      // Output rate of the UBX-MON-TXBUF message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_MON_TXBUF_UART2  UBX_CFG_U1 | 0x2091019dLu      // Output rate of the UBX-MON-TXBUF message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_MON_TXBUF_USB  UBX_CFG_U1 | 0x2091019eLu        // Output rate of the UBX-MON-TXBUF message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_ATT_I2C  UBX_CFG_U1 | 0x2091001fLu          // Output rate of the UBX_NAV_ATT message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_ATT_SPI  UBX_CFG_U1 | 0x20910023Lu          // Output rate of the UBX_NAV_ATT message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_ATT_UART1  UBX_CFG_U1 | 0x20910020Lu        // Output rate of the UBX_NAV_ATT message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_ATT_UART2  UBX_CFG_U1 | 0x20910021Lu        // Output rate of the UBX_NAV_ATT message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_ATT_USB  UBX_CFG_U1 | 0x20910022Lu          // Output rate of the UBX_NAV_ATT message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_CLOCK_I2C  UBX_CFG_U1 | 0x20910065Lu        // Output rate of the UBX-NAV-CLOCK message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_CLOCK_SPI  UBX_CFG_U1 | 0x20910069Lu        // Output rate of the UBX-NAV-CLOCK message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_CLOCK_UART1  UBX_CFG_U1 | 0x20910066Lu      // Output rate of the UBX-NAV-CLOCK message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_CLOCK_UART2  UBX_CFG_U1 | 0x20910067Lu      // Output rate of the UBX-NAV-CLOCK message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_CLOCK_USB  UBX_CFG_U1 | 0x20910068Lu        // Output rate of the UBX-NAV-CLOCK message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_DOP_I2C  UBX_CFG_U1 | 0x20910038Lu          // Output rate of the UBX-NAV-DOP message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_DOP_SPI  UBX_CFG_U1 | 0x2091003cLu          // Output rate of the UBX-NAV-DOP message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_DOP_UART1  UBX_CFG_U1 | 0x20910039Lu        // Output rate of the UBX-NAV-DOP message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_DOP_UART2  UBX_CFG_U1 | 0x2091003aLu        // Output rate of the UBX-NAV-DOP message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_DOP_USB  UBX_CFG_U1 | 0x2091003bLu          // Output rate of the UBX-NAV-DOP message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_EOE_I2C  UBX_CFG_U1 | 0x2091015fLu          // Output rate of the UBX-NAV-EOE message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_EOE_SPI  UBX_CFG_U1 | 0x20910163Lu          // Output rate of the UBX-NAV-EOE message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_EOE_UART1  UBX_CFG_U1 | 0x20910160Lu        // Output rate of the UBX-NAV-EOE message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_EOE_UART2  UBX_CFG_U1 | 0x20910161Lu        // Output rate of the UBX-NAV-EOE message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_EOE_USB  UBX_CFG_U1 | 0x20910162Lu          // Output rate of the UBX-NAV-EOE message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_GEOFENCE_I2C  UBX_CFG_U1 | 0x209100a1Lu     // Output rate of the UBX-NAV-GEOFENCE message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_GEOFENCE_SPI  UBX_CFG_U1 | 0x209100a5Lu     // Output rate of the UBX-NAV-GEOFENCE message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_GEOFENCE_UART1  UBX_CFG_U1 | 0x209100a2Lu   // Output rate of the UBX-NAV-GEOFENCE message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_GEOFENCE_UART2  UBX_CFG_U1 | 0x209100a3Lu   // Output rate of the UBX-NAV-GEOFENCE message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_GEOFENCE_USB  UBX_CFG_U1 | 0x209100a4Lu     // Output rate of the UBX-NAV-GEOFENCE message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSECEF_I2C  UBX_CFG_U1 | 0x2091002eLu    // Output rate of the UBX-NAV-HPPOSECEF message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSECEF_SPI  UBX_CFG_U1 | 0x20910032Lu    // Output rate of the UBX-NAV-HPPOSECEF message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSECEF_UART1  UBX_CFG_U1 | 0x2091002fLu  // Output rate of the UBX-NAV-HPPOSECEF message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSECEF_UART2  UBX_CFG_U1 | 0x20910030Lu  // Output rate of the UBX-NAV-HPPOSECEF message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSECEF_USB  UBX_CFG_U1 | 0x20910031Lu    // Output rate of the UBX-NAV-HPPOSECEF message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSLLH_I2C  UBX_CFG_U1 | 0x20910033Lu     // Output rate of the UBX-NAV-HPPOSLLH message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSLLH_SPI  UBX_CFG_U1 | 0x20910037Lu     // Output rate of the UBX-NAV-HPPOSLLH message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSLLH_UART1  UBX_CFG_U1 | 0x20910034Lu   // Output rate of the UBX-NAV-HPPOSLLH message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSLLH_UART2  UBX_CFG_U1 | 0x20910035Lu   // Output rate of the UBX-NAV-HPPOSLLH message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_HPPOSLLH_USB  UBX_CFG_U1 | 0x20910036Lu     // Output rate of the UBX-NAV-HPPOSLLH message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_ODO_I2C  UBX_CFG_U1 | 0x2091007eLu          // Output rate of the UBX-NAV-ODO message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_ODO_SPI  UBX_CFG_U1 | 0x20910082Lu          // Output rate of the UBX-NAV-ODO message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_ODO_UART1  UBX_CFG_U1 | 0x2091007fLu        // Output rate of the UBX-NAV-ODO message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_ODO_UART2  UBX_CFG_U1 | 0x20910080Lu        // Output rate of the UBX-NAV-ODO message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_ODO_USB  UBX_CFG_U1 | 0x20910081Lu          // Output rate of the UBX-NAV-ODO message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_ORB_I2C  UBX_CFG_U1 | 0x20910010Lu          // Output rate of the UBX-NAV-ORB message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_ORB_SPI  UBX_CFG_U1 | 0x20910014Lu          // Output rate of the UBX-NAV-ORB message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_ORB_UART1  UBX_CFG_U1 | 0x20910011Lu        // Output rate of the UBX-NAV-ORB message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_ORB_UART2  UBX_CFG_U1 | 0x20910012Lu        // Output rate of the UBX-NAV-ORB message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_ORB_USB  UBX_CFG_U1 | 0x20910013Lu          // Output rate of the UBX-NAV-ORB message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_PL_I2C  UBX_CFG_U1 | 0x20910415Lu           // Output rate of the UBX-NAV-PL message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_PL_SPI  UBX_CFG_U1 | 0x20910419Lu           // Output rate of the UBX-NAV-PL message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_PL_UART1  UBX_CFG_U1 | 0x20910416Lu         // Output rate of the UBX-NAV-PL message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_PL_UART2  UBX_CFG_U1 | 0x20910417Lu         // Output rate of the UBX-NAV-PL message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_PL_USB  UBX_CFG_U1 | 0x20910418Lu           // Output rate of the UBX-NAV-PL message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_POSECEF_I2C  UBX_CFG_U1 | 0x20910024Lu      // Output rate of the UBX-NAV-POSECEF message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_POSECEF_SPI  UBX_CFG_U1 | 0x20910028Lu      // Output rate of the UBX-NAV-POSECEF message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_POSECEF_UART1  UBX_CFG_U1 | 0x20910025Lu    // Output rate of the UBX-NAV-POSECEF message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_POSECEF_UART2  UBX_CFG_U1 | 0x20910026Lu    // Output rate of the UBX-NAV-POSECEF message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_POSECEF_USB  UBX_CFG_U1 | 0x20910027Lu      // Output rate of the UBX-NAV-POSECEF message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_POSLLH_I2C  UBX_CFG_U1 | 0x20910029Lu       // Output rate of the UBX-NAV-POSLLH message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_POSLLH_SPI  UBX_CFG_U1 | 0x2091002dLu       // Output rate of the UBX-NAV-POSLLH message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_POSLLH_UART1  UBX_CFG_U1 | 0x2091002aLu     // Output rate of the UBX-NAV-POSLLH message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_POSLLH_UART2  UBX_CFG_U1 | 0x2091002bLu     // Output rate of the UBX-NAV-POSLLH message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_POSLLH_USB  UBX_CFG_U1 | 0x2091002cLu       // Output rate of the UBX-NAV-POSLLH message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_PVT_I2C  UBX_CFG_U1 | 0x20910006Lu          // Output rate of the UBX-NAV-PVT message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_PVT_SPI  UBX_CFG_U1 | 0x2091000aLu          // Output rate of the UBX-NAV-PVT message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_PVT_UART1  UBX_CFG_U1 | 0x20910007Lu        // Output rate of the UBX-NAV-PVT message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_PVT_UART2  UBX_CFG_U1 | 0x20910008Lu        // Output rate of the UBX-NAV-PVT message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_PVT_USB  UBX_CFG_U1 | 0x20910009Lu          // Output rate of the UBX-NAV-PVT message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_RELPOSNED_I2C  UBX_CFG_U1 | 0x2091008dLu    // Output rate of the UBX-NAV-RELPOSNED message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_RELPOSNED_SPI  UBX_CFG_U1 | 0x20910091Lu    // Output rate of the UBX-NAV-RELPOSNED message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_RELPOSNED_UART1  UBX_CFG_U1 | 0x2091008eLu  // Output rate of the UBX-NAV-RELPOSNED message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_RELPOSNED_UART2  UBX_CFG_U1 | 0x2091008fLu  // Output rate of the UBX-NAV-RELPOSNED message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_RELPOSNED_USB  UBX_CFG_U1 | 0x20910090Lu    // Output rate of the UBX-NAV-RELPOSNED message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_SAT_I2C  UBX_CFG_U1 | 0x20910015Lu          // Output rate of the UBX-NAV-SAT message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_SAT_SPI  UBX_CFG_U1 | 0x20910019Lu          // Output rate of the UBX-NAV-SAT message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_SAT_UART1  UBX_CFG_U1 | 0x20910016Lu        // Output rate of the UBX-NAV-SAT message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_SAT_UART2  UBX_CFG_U1 | 0x20910017Lu        // Output rate of the UBX-NAV-SAT message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_SAT_USB  UBX_CFG_U1 | 0x20910018Lu          // Output rate of the UBX-NAV-SAT message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_SBAS_I2C  UBX_CFG_U1 | 0x2091006aLu         // Output rate of the UBX-NAV-SBAS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_SBAS_SPI  UBX_CFG_U1 | 0x2091006eLu         // Output rate of the UBX-NAV-SBAS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_SBAS_UART1  UBX_CFG_U1 | 0x2091006bLu       // Output rate of the UBX-NAV-SBAS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_SBAS_UART2  UBX_CFG_U1 | 0x2091006cLu       // Output rate of the UBX-NAV-SBAS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_SBAS_USB  UBX_CFG_U1 | 0x2091006dLu         // Output rate of the UBX-NAV-SBAS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_SIG_I2C  UBX_CFG_U1 | 0x20910345Lu          // Output rate of the UBX-NAV-SIG message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_SIG_SPI  UBX_CFG_U1 | 0x20910349Lu          // Output rate of the UBX-NAV-SIG message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_SIG_UART1  UBX_CFG_U1 | 0x20910346Lu        // Output rate of the UBX-NAV-SIG message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_SIG_UART2  UBX_CFG_U1 | 0x20910347Lu        // Output rate of the UBX-NAV-SIG message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_SIG_USB  UBX_CFG_U1 | 0x20910348Lu          // Output rate of the UBX-NAV-SIG message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_SLAS_I2C  UBX_CFG_U1 | 0x20910336Lu         // Output rate of the UBX-NAV-SLAS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_SLAS_SPI  UBX_CFG_U1 | 0x2091033aLu         // Output rate of the UBX-NAV-SLAS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_SLAS_UART1  UBX_CFG_U1 | 0x20910337Lu       // Output rate of the UBX-NAV-SLAS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_SLAS_UART2  UBX_CFG_U1 | 0x20910338Lu       // Output rate of the UBX-NAV-SLAS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_SLAS_USB  UBX_CFG_U1 | 0x20910339Lu         // Output rate of the UBX-NAV-SLAS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_STATUS_I2C  UBX_CFG_U1 | 0x2091001aLu       // Output rate of the UBX-NAV-STATUS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_STATUS_SPI  UBX_CFG_U1 | 0x2091001eLu       // Output rate of the UBX-NAV-STATUS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_STATUS_UART1  UBX_CFG_U1 | 0x2091001bLu     // Output rate of the UBX-NAV-STATUS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_STATUS_UART2  UBX_CFG_U1 | 0x2091001cLu     // Output rate of the UBX-NAV-STATUS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_STATUS_USB  UBX_CFG_U1 | 0x2091001dLu       // Output rate of the UBX-NAV-STATUS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_SVIN_I2C  UBX_CFG_U1 | 0x20910088Lu         // Output rate of the UBX-NAV-SVIN message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_SVIN_SPI  UBX_CFG_U1 | 0x2091008cLu         // Output rate of the UBX-NAV-SVIN message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_SVIN_UART1  UBX_CFG_U1 | 0x20910089Lu       // Output rate of the UBX-NAV-SVIN message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_SVIN_UART2  UBX_CFG_U1 | 0x2091008aLu       // Output rate of the UBX-NAV-SVIN message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_SVIN_USB  UBX_CFG_U1 | 0x2091008bLu         // Output rate of the UBX-NAV-SVIN message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEBDS_I2C  UBX_CFG_U1 | 0x20910051Lu      // Output rate of the UBX-NAV-TIMEBDS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEBDS_SPI  UBX_CFG_U1 | 0x20910055Lu      // Output rate of the UBX-NAV-TIMEBDS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEBDS_UART1  UBX_CFG_U1 | 0x20910052Lu    // Output rate of the UBX-NAV-TIMEBDS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEBDS_UART2  UBX_CFG_U1 | 0x20910053Lu    // Output rate of the UBX-NAV-TIMEBDS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEBDS_USB  UBX_CFG_U1 | 0x20910054Lu      // Output rate of the UBX-NAV-TIMEBDS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGAL_I2C  UBX_CFG_U1 | 0x20910056Lu      // Output rate of the UBX-NAV-TIMEGAL message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGAL_SPI  UBX_CFG_U1 | 0x2091005aLu      // Output rate of the UBX-NAV-TIMEGAL message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGAL_UART1  UBX_CFG_U1 | 0x20910057Lu    // Output rate of the UBX-NAV-TIMEGAL message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGAL_UART2  UBX_CFG_U1 | 0x20910058Lu    // Output rate of the UBX-NAV-TIMEGAL message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGAL_USB  UBX_CFG_U1 | 0x20910059Lu      // Output rate of the UBX-NAV-TIMEGAL message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGLO_I2C  UBX_CFG_U1 | 0x2091004cLu      // Output rate of the UBX-NAV-TIMEGLO message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGLO_SPI  UBX_CFG_U1 | 0x20910050Lu      // Output rate of the UBX-NAV-TIMEGLO message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGLO_UART1  UBX_CFG_U1 | 0x2091004dLu    // Output rate of the UBX-NAV-TIMEGLO message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGLO_UART2  UBX_CFG_U1 | 0x2091004eLu    // Output rate of the UBX-NAV-TIMEGLO message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGLO_USB  UBX_CFG_U1 | 0x2091004fLu      // Output rate of the UBX-NAV-TIMEGLO message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGPS_I2C  UBX_CFG_U1 | 0x20910047Lu      // Output rate of the UBX-NAV-TIMEGPS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGPS_SPI  UBX_CFG_U1 | 0x2091004bLu      // Output rate of the UBX-NAV-TIMEGPS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGPS_UART1  UBX_CFG_U1 | 0x20910048Lu    // Output rate of the UBX-NAV-TIMEGPS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGPS_UART2  UBX_CFG_U1 | 0x20910049Lu    // Output rate of the UBX-NAV-TIMEGPS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEGPS_USB  UBX_CFG_U1 | 0x2091004aLu      // Output rate of the UBX-NAV-TIMEGPS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMELS_I2C  UBX_CFG_U1 | 0x20910060Lu       // Output rate of the UBX-NAV-TIMELS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMELS_SPI  UBX_CFG_U1 | 0x20910064Lu       // Output rate of the UBX-NAV-TIMELS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMELS_UART1  UBX_CFG_U1 | 0x20910061Lu     // Output rate of the UBX-NAV-TIMELS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMELS_UART2  UBX_CFG_U1 | 0x20910062Lu     // Output rate of the UBX-NAV-TIMELS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMELS_USB  UBX_CFG_U1 | 0x20910063Lu       // Output rate of the UBX-NAV-TIMELS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEQZSS_I2C  UBX_CFG_U1 | 0x20910386Lu     // Output rate of the UBX-NAV-TIMEQZSSmessage on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEQZSS_SPI  UBX_CFG_U1 | 0x2091038aLu     // Output rate of the UBX-NAV-TIMEQZSSmessage on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEQZSS_UART1  UBX_CFG_U1 | 0x20910387Lu   // Output rate of the UBX-NAV-TIMEQZSSmessage on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEQZSS_UART2  UBX_CFG_U1 | 0x20910388Lu   // Output rate of the UBX-NAV-TIMEQZSSmessage on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEQZSS_USB  UBX_CFG_U1 | 0x20910389Lu     // Output rate of the UBX-NAV-TIMEQZSSmessage on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEUTC_I2C  UBX_CFG_U1 | 0x2091005bLu      // Output rate of the UBX-NAV-TIMEUTC message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEUTC_SPI  UBX_CFG_U1 | 0x2091005fLu      // Output rate of the UBX-NAV-TIMEUTC message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEUTC_UART1  UBX_CFG_U1 | 0x2091005cLu    // Output rate of the UBX-NAV-TIMEUTC message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEUTC_UART2  UBX_CFG_U1 | 0x2091005dLu    // Output rate of the UBX-NAV-TIMEUTC message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_TIMEUTC_USB  UBX_CFG_U1 | 0x2091005eLu      // Output rate of the UBX-NAV-TIMEUTC message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_VELECEF_I2C  UBX_CFG_U1 | 0x2091003dLu      // Output rate of the UBX-NAV-VELECEF message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_VELECEF_SPI  UBX_CFG_U1 | 0x20910041Lu      // Output rate of the UBX-NAV-VELECEF message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_VELECEF_UART1  UBX_CFG_U1 | 0x2091003eLu    // Output rate of the UBX-NAV-VELECEF message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_VELECEF_UART2  UBX_CFG_U1 | 0x2091003fLu    // Output rate of the UBX-NAV-VELECEF message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_VELECEF_USB  UBX_CFG_U1 | 0x20910040Lu      // Output rate of the UBX-NAV-VELECEF message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_VELNED_I2C  UBX_CFG_U1 | 0x20910042Lu       // Output rate of the UBX-NAV-VELNED message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_VELNED_SPI  UBX_CFG_U1 | 0x20910046Lu       // Output rate of the UBX-NAV-VELNED message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_VELNED_UART1  UBX_CFG_U1 | 0x20910043Lu     // Output rate of the UBX-NAV-VELNED message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_VELNED_UART2  UBX_CFG_U1 | 0x20910044Lu     // Output rate of the UBX-NAV-VELNED message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_VELNED_USB  UBX_CFG_U1 | 0x20910045Lu       // Output rate of the UBX-NAV-VELNED message on port USB
#define UBLOX_CFG_MSGOUT_UBX_RXM_COR_I2C  UBX_CFG_U1 | 0x209106b6Lu          // Output rate of the UBX-RXM-COR message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_RXM_COR_SPI  UBX_CFG_U1 | 0x209106baLu          // Output rate of the UBX-RXM-COR message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_RXM_COR_UART1  UBX_CFG_U1 | 0x209106b7Lu        // Output rate of the UBX-RXM-COR message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_RXM_COR_UART2  UBX_CFG_U1 | 0x209106b8Lu        // Output rate of the UBX-RXM-COR message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_RXM_COR_USB  UBX_CFG_U1 | 0x209106b9Lu          // Output rate of the UBX-RXM-COR message on port USB
#define UBLOX_CFG_MSGOUT_UBX_RXM_MEASX_I2C  UBX_CFG_U1 | 0x20910204Lu        // Output rate of the UBX-RXM-MEASX message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_RXM_MEASX_SPI  UBX_CFG_U1 | 0x20910208Lu        // Output rate of the UBX-RXM-MEASX message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_RXM_MEASX_UART1  UBX_CFG_U1 | 0x20910205Lu      // Output rate of the UBX-RXM-MEASX message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_RXM_MEASX_UART2  UBX_CFG_U1 | 0x20910206Lu      // Output rate of the UBX-RXM-MEASX message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_RXM_MEASX_USB  UBX_CFG_U1 | 0x20910207Lu        // Output rate of the UBX-RXM-MEASX message on port USB
#define UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_I2C  UBX_CFG_U1 | 0x209102a4Lu         // Output rate of the UBX-RXM-RAWX message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_SPI  UBX_CFG_U1 | 0x209102a8Lu         // Output rate of the UBX-RXM-RAWX message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_UART1  UBX_CFG_U1 | 0x209102a5Lu       // Output rate of the UBX-RXM-RAWX message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_UART2  UBX_CFG_U1 | 0x209102a6Lu       // Output rate of the UBX-RXM-RAWX message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_RXM_RAWX_USB  UBX_CFG_U1 | 0x209102a7Lu         // Output rate of the UBX-RXM-RAWX message on port USB
#define UBLOX_CFG_MSGOUT_UBX_RXM_RLM_I2C  UBX_CFG_U1 | 0x2091025eLu          // Output rate of the UBX-RXM-RLM message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_RXM_RLM_SPI  UBX_CFG_U1 | 0x20910262Lu          // Output rate of the UBX-RXM-RLM message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_RXM_RLM_UART1  UBX_CFG_U1 | 0x2091025fLu        // Output rate of the UBX-RXM-RLM message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_RXM_RLM_UART2  UBX_CFG_U1 | 0x20910260Lu        // Output rate of the UBX-RXM-RLM message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_RXM_RLM_USB  UBX_CFG_U1 | 0x20910261Lu          // Output rate of the UBX-RXM-RLM message on port USB
#define UBLOX_CFG_MSGOUT_UBX_RXM_RTCM_I2C  UBX_CFG_U1 | 0x20910268Lu         // Output rate of the UBX-RXM-RTCM message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_RXM_RTCM_SPI  UBX_CFG_U1 | 0x2091026cLu         // Output rate of the UBX-RXM-RTCM message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_RXM_RTCM_UART1  UBX_CFG_U1 | 0x20910269Lu       // Output rate of the UBX-RXM-RTCM message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_RXM_RTCM_UART2  UBX_CFG_U1 | 0x2091026aLu       // Output rate of the UBX-RXM-RTCM message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_RXM_RTCM_USB  UBX_CFG_U1 | 0x2091026bLu         // Output rate of the UBX-RXM-RTCM message on port USB
#define UBLOX_CFG_MSGOUT_UBX_RXM_SFRBX_I2C  UBX_CFG_U1 | 0x20910231Lu        // Output rate of the UBX-RXM-SFRBX message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_RXM_SFRBX_SPI  UBX_CFG_U1 | 0x20910235Lu        // Output rate of the UBX-RXM-SFRBX message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_RXM_SFRBX_UART1  UBX_CFG_U1 | 0x20910232Lu      // Output rate of the UBX-RXM-SFRBX message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_RXM_SFRBX_UART2  UBX_CFG_U1 | 0x20910233Lu      // Output rate of the UBX-RXM-SFRBX message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_RXM_SFRBX_USB  UBX_CFG_U1 | 0x20910234Lu        // Output rate of the UBX-RXM-SFRBX message on port USB
#define UBLOX_CFG_MSGOUT_UBX_RXM_SPARTN_I2C  UBX_CFG_U1 | 0x20910605Lu       // Output rate of the UBX-RXM-SPARTN message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_RXM_SPARTN_UART1  UBX_CFG_U1 | 0x20910606Lu     // Output rate of the UBX-RXM-SPARTN message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_RXM_SPARTN_UART2  UBX_CFG_U1 | 0x20910607Lu     // Output rate of the UBX-RXM-SPARTN message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_RXM_SPARTN_USB  UBX_CFG_U1 | 0x20910608Lu       // Output rate of the UBX-RXM-SPARTN message on port USB
#define UBLOX_CFG_MSGOUT_UBX_RXM_SPARTN_SPI  UBX_CFG_U1 | 0x20910609Lu       // Output rate of the UBX-RXM-SPARTN message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_SEC_SIG_I2C  UBX_CFG_U1 | 0x20910634Lu          // Output rate of the UBX-SEC-SIG message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_SEC_SIG_SPI  UBX_CFG_U1 | 0x20910638Lu          // Output rate of the UBX-SEC-SIG message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_SEC_SIG_UART1  UBX_CFG_U1 | 0x20910635Lu        // Output rate of the UBX-SEC-SIG message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_SEC_SIG_UART2  UBX_CFG_U1 | 0x20910636Lu        // Output rate of the UBX-SEC-SIG message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_SEC_SIG_USB  UBX_CFG_U1 | 0x20910637Lu          // Output rate of the UBX-SEC-SIG message on port USB
#define UBLOX_CFG_MSGOUT_UBX_TIM_TM2_I2C  UBX_CFG_U1 | 0x20910178Lu          // Output rate of the UBX-TIM-TM2 message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_TIM_TM2_SPI  UBX_CFG_U1 | 0x2091017cLu          // Output rate of the UBX-TIM-TM2 message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_TIM_TM2_UART1  UBX_CFG_U1 | 0x20910179Lu        // Output rate of the UBX-TIM-TM2 message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_TIM_TM2_UART2  UBX_CFG_U1 | 0x2091017aLu        // Output rate of the UBX-TIM-TM2 message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_TIM_TM2_USB  UBX_CFG_U1 | 0x2091017bLu          // Output rate of the UBX-TIM-TM2 message on port USB
#define UBLOX_CFG_MSGOUT_UBX_TIM_TP_I2C  UBX_CFG_U1 | 0x2091017dLu           // Output rate of the UBX-TIM-TP message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_TIM_TP_SPI  UBX_CFG_U1 | 0x20910181Lu           // Output rate of the UBX-TIM-TP message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_TIM_TP_UART1  UBX_CFG_U1 | 0x2091017eLu         // Output rate of the UBX-TIM-TP message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_TIM_TP_UART2  UBX_CFG_U1 | 0x2091017fLu         // Output rate of the UBX-TIM-TP message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_TIM_TP_USB  UBX_CFG_U1 | 0x20910180Lu           // Output rate of the UBX-TIM-TP message on port USB
#define UBLOX_CFG_MSGOUT_UBX_TIM_VRFY_I2C  UBX_CFG_U1 | 0x20910092Lu         // Output rate of the UBX-TIM-VRFY message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_TIM_VRFY_SPI  UBX_CFG_U1 | 0x20910096Lu         // Output rate of the UBX-TIM-VRFY message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_TIM_VRFY_UART1  UBX_CFG_U1 | 0x20910093Lu       // Output rate of the UBX-TIM-VRFY message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_TIM_VRFY_UART2  UBX_CFG_U1 | 0x20910094Lu       // Output rate of the UBX-TIM-VRFY message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_TIM_VRFY_USB  UBX_CFG_U1 | 0x20910095Lu         // Output rate of the UBX-TIM-VRFY message on port USB

// Additional CFG_MSGOUT keys for the ZED-F9R HPS121
//--------------------------------------------------------
#define UBLOX_CFG_MSGOUT_UBX_NAV_COV_I2C  UBX_CFG_U1 | 0x20910083Lu      // Output rate of the UBX-NAV-COV message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_COV_UART1  UBX_CFG_U1 | 0x20910084Lu    // Output rate of the UBX-NAV-COV message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_COV_UART2  UBX_CFG_U1 | 0x20910085Lu    // Output rate of the UBX-NAV-COV message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_COV_USB  UBX_CFG_U1 | 0x20910086Lu      // Output rate of the UBX-NAV-COV message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_COV_SPI  UBX_CFG_U1 | 0x20910087Lu      // Output rate of the UBX-NAV-COV message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_ID_THS_I2C  UBX_CFG_U1 | 0x209100e2Lu      // Output rate of the NMEA-GX-THS message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_ID_THS_UART1  UBX_CFG_U1 | 0x209100e3Lu    // Output rate of the NMEA-GX-THS message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_ID_THS_UART2  UBX_CFG_U1 | 0x209100e4Lu    // Output rate of the NMEA-GX-THS message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_ID_THS_USB  UBX_CFG_U1 | 0x209100e5Lu      // Output rate of the NMEA-GX-THS message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_ID_THS_SPI  UBX_CFG_U1 | 0x209100e6Lu      // Output rate of the NMEA-GX-THS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_ESF_STATUS_I2C  UBX_CFG_U1 | 0x20910105Lu   // Output rate of the UBX-ESF-STATUS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_ESF_STATUS_UART1  UBX_CFG_U1 | 0x20910106Lu // Output rate of the UBX-ESF-STATUS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_ESF_STATUS_UART2  UBX_CFG_U1 | 0x20910107Lu // Output rate of the UBX-ESF-STATUS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_ESF_STATUS_USB  UBX_CFG_U1 | 0x20910108Lu   // Output rate of the UBX-ESF-STATUS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_ESF_STATUS_SPI  UBX_CFG_U1 | 0x20910109Lu   // Output rate of the UBX-ESF-STATUS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_ESF_ALG_I2C  UBX_CFG_U1 | 0x2091010fLu      // Output rate of the UBX-ESF-ALG message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_ESF_ALG_UART1  UBX_CFG_U1 | 0x20910110Lu    // Output rate of the UBX-ESF-ALG message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_ESF_ALG_UART2  UBX_CFG_U1 | 0x20910111Lu    // Output rate of the UBX-ESF-ALG message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_ESF_ALG_USB  UBX_CFG_U1 | 0x20910112Lu      // Output rate of the UBX-ESF-ALG message on port USB
#define UBLOX_CFG_MSGOUT_UBX_ESF_ALG_SPI  UBX_CFG_U1 | 0x20910113Lu      // Output rate of the UBX-ESF-ALG message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_ESF_INS_I2C  UBX_CFG_U1 | 0x20910114Lu      // Output rate of the UBX-ESF-INS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_ESF_INS_UART1  UBX_CFG_U1 | 0x20910115Lu    // Output rate of the UBX-ESF-INS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_ESF_INS_UART2  UBX_CFG_U1 | 0x20910116Lu    // Output rate of the UBX-ESF-INS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_ESF_INS_USB  UBX_CFG_U1 | 0x20910117Lu      // Output rate of the UBX-ESF-INS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_ESF_INS_SPI  UBX_CFG_U1 | 0x20910118Lu      // Output rate of the UBX-ESF-INS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_ESF_MEAS_I2C  UBX_CFG_U1 | 0x20910277Lu     // Output rate of the UBX-ESF-MEAS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_ESF_MEAS_UART1  UBX_CFG_U1 | 0x20910278Lu   // Output rate of the UBX-ESF-MEAS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_ESF_MEAS_UART2  UBX_CFG_U1 | 0x20910279Lu   // Output rate of the UBX-ESF-MEAS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_ESF_MEAS_USB  UBX_CFG_U1 | 0x2091027aLu     // Output rate of the UBX-ESF-MEAS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_ESF_MEAS_SPI  UBX_CFG_U1 | 0x2091027bLu     // Output rate of the UBX-ESF-MEAS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_ESF_RAW_I2C  UBX_CFG_U1 | 0x2091029fLu      // Output rate of the UBX-ESF-RAW message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_ESF_RAW_UART1  UBX_CFG_U1 | 0x209102a0Lu    // Output rate of the UBX-ESF-RAW message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_ESF_RAW_UART2  UBX_CFG_U1 | 0x209102a1Lu    // Output rate of the UBX-ESF-RAW message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_ESF_RAW_USB  UBX_CFG_U1 | 0x209102a2Lu      // Output rate of the UBX-ESF-RAW message on port USB
#define UBLOX_CFG_MSGOUT_UBX_ESF_RAW_SPI  UBX_CFG_U1 | 0x209102a3Lu      // Output rate of the UBX-ESF-RAW message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_EELL_I2C  UBX_CFG_U1 | 0x20910313Lu     // Output rate of the UBX-NAV-EELL message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_EELL_UART1  UBX_CFG_U1 | 0x20910314Lu   // Output rate of the UBX-NAV-EELL message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_EELL_UART2  UBX_CFG_U1 | 0x20910315Lu   // Output rate of the UBX-NAV-EELL message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_EELL_USB  UBX_CFG_U1 | 0x20910316Lu     // Output rate of the UBX-NAV-EELL message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_EELL_SPI  UBX_CFG_U1 | 0x20910317Lu     // Output rate of the UBX-NAV-EELL message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_PVAT_I2C  UBX_CFG_U1 | 0x2091062aLu     // Output rate of the UBX-NAV-PVAT message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_PVAT_UART1  UBX_CFG_U1 | 0x2091062bLu   // Output rate of the UBX-NAV-PVAT message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_PVAT_UART2  UBX_CFG_U1 | 0x2091062cLu   // Output rate of the UBX-NAV-PVAT message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_PVAT_USB  UBX_CFG_U1 | 0x2091062dLu     // Output rate of the UBX-NAV-PVAT message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_PVAT_SPI  UBX_CFG_U1 | 0x2091062eLu     // Output rate of the UBX-NAV-PVAT message on port SPI

// Additional CFG_MSGOUT keys for the M10
//--------------------------------------------------------
#define UBLOX_CFG_MSGOUT_UBX_NAV_AOPSTATUS_I2C  UBX_CFG_U1 | 0x20910079Lu
#define UBLOX_CFG_MSGOUT_UBX_NAV_AOPSTATUS_SPI  UBX_CFG_U1 | 0x2091007dLu
#define UBLOX_CFG_MSGOUT_UBX_NAV_AOPSTATUS_UART1  UBX_CFG_U1 | 0x2091007aLu

// Additional CFG_MSGOUT keys for the ZED-F9T
//--------------------------------------------------------
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GGA_I2C  UBX_CFG_U1 | 0x20910661Lu    // Output rate of the NMEA-NAV2-GX-GGA message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GGA_SPI  UBX_CFG_U1 | 0x20910665Lu    // Output rate of the NMEA-NAV2-GX-GGA message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GGA_UART1  UBX_CFG_U1 | 0x20910662Lu  // Output rate of the NMEA-NAV2-GX-GGA message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GGA_UART2  UBX_CFG_U1 | 0x20910663Lu  // Output rate of the NMEA-NAV2-GX-GGA message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GGA_USB  UBX_CFG_U1 | 0x20910664Lu    // Output rate of the NMEA-NAV2-GX-GGA message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GLL_I2C  UBX_CFG_U1 | 0x20910670Lu    // Output rate of the NMEA-NAV2-GX-GLL message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GLL_SPI  UBX_CFG_U1 | 0x20910674Lu    // Output rate of the NMEA-NAV2-GX-GLL message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GLL_UART1  UBX_CFG_U1 | 0x20910671Lu  // Output rate of the NMEA-NAV2-GX-GLL message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GLL_UART2  UBX_CFG_U1 | 0x20910672Lu  // Output rate of the NMEA-NAV2-GX-GLL message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GLL_USB  UBX_CFG_U1 | 0x20910673Lu    // Output rate of the NMEA-NAV2-GX-GLL message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GNS_I2C  UBX_CFG_U1 | 0x2091065cLu    // Output rate of the NMEA-NAV2-GX-GNS message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GNS_SPI  UBX_CFG_U1 | 0x20910660Lu    // Output rate of the NMEA-NAV2-GX-GNS message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GNS_UART1  UBX_CFG_U1 | 0x2091065dLu  // Output rate of the NMEA-NAV2-GX-GNS message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GNS_UART2  UBX_CFG_U1 | 0x2091065eLu  // Output rate of the NMEA-NAV2-GX-GNS message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GNS_USB  UBX_CFG_U1 | 0x2091065fLu    // Output rate of the NMEA-NAV2-GX-GNS message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GSA_I2C  UBX_CFG_U1 | 0x20910666Lu    // Output rate of the NMEA-NAV2-GX-GSA message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GSA_SPI  UBX_CFG_U1 | 0x2091066aLu    // Output rate of the NMEA-NAV2-GX-GSA message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GSA_UART1  UBX_CFG_U1 | 0x20910667Lu  // Output rate of the NMEA-NAV2-GX-GSA message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GSA_UART2  UBX_CFG_U1 | 0x20910668Lu  // Output rate of the NMEA-NAV2-GX-GSA message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_GSA_USB  UBX_CFG_U1 | 0x20910669Lu    // Output rate of the NMEA-NAV2-GX-GSA message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_RMC_I2C  UBX_CFG_U1 | 0x20910652Lu    // Output rate of the NMEA-NAV2-GX-RMC message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_RMC_SPI  UBX_CFG_U1 | 0x20910656Lu    // Output rate of the NMEA-NAV2-GX-RMC message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_RMC_UART1  UBX_CFG_U1 | 0x20910653Lu  // Output rate of the NMEA-NAV2-GX-RMC message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_RMC_UART2  UBX_CFG_U1 | 0x20910654Lu  // Output rate of the NMEA-NAV2-GX-RMC message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_RMC_USB  UBX_CFG_U1 | 0x20910655Lu    // Output rate of the NMEA-NAV2-GX-RMC message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_VTG_I2C  UBX_CFG_U1 | 0x20910657Lu    // Output rate of the NMEA-NAV2-GX-VTG message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_VTG_SPI  UBX_CFG_U1 | 0x2091065bLu    // Output rate of the NMEA-NAV2-GX-VTG message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_VTG_UART1  UBX_CFG_U1 | 0x20910658Lu  // Output rate of the NMEA-NAV2-GX-VTG message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_VTG_UART2  UBX_CFG_U1 | 0x20910659Lu  // Output rate of the NMEA-NAV2-GX-VTG message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_VTG_USB  UBX_CFG_U1 | 0x2091065aLu    // Output rate of the NMEA-NAV2-GX-VTG message on port USB
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_ZDA_I2C  UBX_CFG_U1 | 0x2091067fLu    // Output rate of the NMEA-NAV2-GX-ZDA message on port I2C
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_ZDA_SPI  UBX_CFG_U1 | 0x20910683Lu    // Output rate of the NMEA-NAV2-GX-ZDA message on port SPI
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_ZDA_UART1  UBX_CFG_U1 | 0x20910680Lu  // Output rate of the NMEA-NAV2-GX-ZDA message on port UART1
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_ZDA_UART2  UBX_CFG_U1 | 0x20910681Lu  // Output rate of the NMEA-NAV2-GX-ZDA message on port UART2
#define UBLOX_CFG_MSGOUT_NMEA_NAV2_ID_ZDA_USB  UBX_CFG_U1 | 0x20910682Lu    // Output rate of the NMEA-NAV2-GX-ZDA message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_CLOCK_I2C  UBX_CFG_U1 | 0x20910430Lu      // Output rate of the UBX-NAV2-CLOCK message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_CLOCK_SPI  UBX_CFG_U1 | 0x20910434Lu      // Output rate of the UBX-NAV2-CLOCK message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_CLOCK_UART1  UBX_CFG_U1 | 0x20910431Lu    // Output rate of the UBX-NAV2-CLOCK message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_CLOCK_UART2  UBX_CFG_U1 | 0x20910432Lu    // Output rate of the UBX-NAV2-CLOCK message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_CLOCK_USB  UBX_CFG_U1 | 0x20910433Lu      // Output rate of the UBX-NAV2-CLOCK message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_COV_I2C  UBX_CFG_U1 | 0x20910435Lu        // Output rate of the UBX-NAV2-COV message onport I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_COV_SPI  UBX_CFG_U1 | 0x20910439Lu        // Output rate of the UBX-NAV2-COV message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_COV_UART1  UBX_CFG_U1 | 0x20910436Lu      // Output rate of the UBX-NAV2-COV message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_COV_UART2  UBX_CFG_U1 | 0x20910437Lu      // Output rate of the UBX-NAV2-COV message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_COV_USB  UBX_CFG_U1 | 0x20910438Lu        // Output rate of the UBX-NAV2-COV message onport USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_DOP_I2C  UBX_CFG_U1 | 0x20910465Lu        // Output rate of the UBX-NAV2-DOP message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_DOP_SPI  UBX_CFG_U1 | 0x20910469Lu        // Output rate of the UBX-NAV2-DOP message onport SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_DOP_UART1  UBX_CFG_U1 | 0x20910466Lu      // Output rate of the UBX-NAV2-DOP message onport UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_DOP_UART2  UBX_CFG_U1 | 0x20910467Lu      // Output rate of the UBX-NAV2-DOP message onport UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_DOP_USB  UBX_CFG_U1 | 0x20910468Lu        // Output rate of the UBX-NAV2-DOP message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_EELL_I2C  UBX_CFG_U1 | 0x20910470Lu        // Output rate of the UBX-NAV2-EELL message onport I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_EELL_SPI  UBX_CFG_U1 | 0x20910474Lu        // Output rate of the UBX-NAV2-EELL message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_EELL_UART1  UBX_CFG_U1 | 0x20910471Lu      // Output rate of the UBX-NAV2-EELL message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_EELL_UART2  UBX_CFG_U1 | 0x20910472Lu      // Output rate of the UBX-NAV2-EELL message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_EELL_USB  UBX_CFG_U1 | 0x20910473Lu        // Output rate of the UBX-NAV2-EELL message onport USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_EOE_I2C  UBX_CFG_U1 | 0x20910565Lu        // Output rate of the UBX-NAV2-EOE message onport I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_EOE_SPI  UBX_CFG_U1 | 0x20910569Lu        // Output rate of the UBX-NAV2-EOE message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_EOE_UART1  UBX_CFG_U1 | 0x20910566Lu      // Output rate of the UBX-NAV2-EOE message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_EOE_UART2  UBX_CFG_U1 | 0x20910567Lu      // Output rate of the UBX-NAV2-EOE message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_EOE_USB  UBX_CFG_U1 | 0x20910568Lu        // Output rate of the UBX-NAV2-EOE message onport USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_ODO_I2C  UBX_CFG_U1 | 0x20910475Lu        // Output rate of the UBX-NAV2-ODO message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_ODO_SPI  UBX_CFG_U1 | 0x20910479Lu        // Output rate of the UBX-NAV2-ODO message onport SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_ODO_UART1  UBX_CFG_U1 | 0x20910476Lu      // Output rate of the UBX-NAV2-ODO message onport UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_ODO_UART2  UBX_CFG_U1 | 0x20910477Lu      // Output rate of the UBX-NAV2-ODO message onport UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_ODO_USB  UBX_CFG_U1 | 0x20910478Lu        // Output rate of the UBX-NAV2-ODO message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_POSECEF_I2C  UBX_CFG_U1 | 0x20910480Lu    // Output rate of the UBX-NAV2-POSECEF message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_POSECEF_SPI  UBX_CFG_U1 | 0x20910484Lu    // Output rate of the UBX-NAV2-POSECEF message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_POSECEF_UART1  UBX_CFG_U1 | 0x20910481Lu  // Output rate of the UBX-NAV2-POSECEF message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_POSECEF_UART2  UBX_CFG_U1 | 0x20910482Lu  // Output rate of the UBX-NAV2-POSECEF message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_POSECEF_USB  UBX_CFG_U1 | 0x20910483Lu    // Output rate of the UBX-NAV2-POSECEF message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_POSLLH_I2C  UBX_CFG_U1 | 0x20910485Lu     // Output rate of the UBX-NAV2-POSLLH message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_POSLLH_SPI  UBX_CFG_U1 | 0x20910489Lu     // Output rate of the UBX-NAV2-POSLLH message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_POSLLH_UART1  UBX_CFG_U1 | 0x20910486Lu   // Output rate of the UBX-NAV2-POSLLH message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_POSLLH_UART2  UBX_CFG_U1 | 0x20910487Lu   // Output rate of the UBX-NAV2-POSLLH message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_POSLLH_USB  UBX_CFG_U1 | 0x20910488Lu     // Output rate of the UBX-NAV2-POSLLH message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_PVAT_I2C  UBX_CFG_U1 | 0x2091062fLu        // Output rate of the UBX-NAV2-PVAT message onport I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_PVAT_SPI  UBX_CFG_U1 | 0x20910633Lu        // Output rate of the UBX-NAV2-PVAT message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_PVAT_UART1  UBX_CFG_U1 | 0x20910630Lu      // Output rate of the UBX-NAV2-PVAT message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_PVAT_UART2  UBX_CFG_U1 | 0x20910631Lu      // Output rate of the UBX-NAV2-PVAT message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_PVAT_USB  UBX_CFG_U1 | 0x20910632Lu        // Output rate of the UBX-NAV2-PVAT message onport USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_PVT_I2C  UBX_CFG_U1 | 0x20910490Lu        // Output rate of the UBX-NAV2-PVT message onport I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_PVT_SPI  UBX_CFG_U1 | 0x20910494Lu        // Output rate of the UBX-NAV2-PVT message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_PVT_UART1  UBX_CFG_U1 | 0x20910491Lu      // Output rate of the UBX-NAV2-PVT message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_PVT_UART2  UBX_CFG_U1 | 0x20910492Lu      // Output rate of the UBX-NAV2-PVT message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_PVT_USB  UBX_CFG_U1 | 0x20910493Lu        // Output rate of the UBX-NAV2-PVT message onport USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SAT_I2C  UBX_CFG_U1 | 0x20910495Lu        // Output rate of the UBX-NAV2-SAT message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SAT_SPI  UBX_CFG_U1 | 0x20910499Lu        // Output rate of the UBX-NAV2-SAT message onport SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SAT_UART1  UBX_CFG_U1 | 0x20910496Lu      // Output rate of the UBX-NAV2-SAT message onport UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SAT_UART2  UBX_CFG_U1 | 0x20910497Lu      // Output rate of the UBX-NAV2-SAT message onport UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SAT_USB  UBX_CFG_U1 | 0x20910498Lu        // Output rate of the UBX-NAV2-SAT message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SBAS_I2C  UBX_CFG_U1 | 0x20910500Lu       // Output rate of the UBX-NAV2-SBAS messageon port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SBAS_SPI  UBX_CFG_U1 | 0x20910504Lu       // Output rate of the UBX-NAV2-SBAS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SBAS_UART1  UBX_CFG_U1 | 0x20910501Lu     // Output rate of the UBX-NAV2-SBAS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SBAS_UART2  UBX_CFG_U1 | 0x20910502Lu     // Output rate of the UBX-NAV2-SBAS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SBAS_USB  UBX_CFG_U1 | 0x20910503Lu       // Output rate of the UBX-NAV2-SBAS messageon port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SIG_I2C  UBX_CFG_U1 | 0x20910505Lu        // Output rate of the UBX-NAV2-SIG message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SIG_SPI  UBX_CFG_U1 | 0x20910509Lu        // Output rate of the UBX-NAV2-SIG message onport SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SIG_UART1  UBX_CFG_U1 | 0x20910506Lu      // Output rate of the UBX-NAV2-SIG message onport UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SIG_UART2  UBX_CFG_U1 | 0x20910507Lu      // Output rate of the UBX-NAV2-SIG message onport UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SIG_USB  UBX_CFG_U1 | 0x20910508Lu        // Output rate of the UBX-NAV2-SIG message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SLAS_I2C  UBX_CFG_U1 | 0x20910510Lu       // Output rate of the UBX-NAV2-SLAS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SLAS_SPI  UBX_CFG_U1 | 0x20910514Lu       // Output rate of the UBX-NAV2-SLAS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SLAS_UART1  UBX_CFG_U1 | 0x20910511Lu     // Output rate of the UBX-NAV2-SLAS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SLAS_UART2  UBX_CFG_U1 | 0x20910512Lu     // Output rate of the UBX-NAV2-SLAS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SLAS_USB  UBX_CFG_U1 | 0x20910513Lu       // Output rate of the UBX-NAV2-SLAS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_STATUS_I2C  UBX_CFG_U1 | 0x20910515Lu     // Output rate of the UBX-NAV2-STATUS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_STATUS_SPI  UBX_CFG_U1 | 0x20910519Lu     // Output rate of the UBX-NAV2-STATUS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_STATUS_UART1  UBX_CFG_U1 | 0x20910516Lu   // Output rate of the UBX-NAV2-STATUS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_STATUS_UART2  UBX_CFG_U1 | 0x20910517Lu   // Output rate of the UBX-NAV2-STATUS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_STATUS_USB  UBX_CFG_U1 | 0x20910518Lu     // Output rate of the UBX-NAV2-STATUS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SVIN_I2C  UBX_CFG_U1 | 0x20910520Lu       // Output rate of the UBX-NAV2-SVIN message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SVIN_SPI  UBX_CFG_U1 | 0x20910524Lu       // Output rate of the UBX-NAV2-SVIN message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SVIN_UART1  UBX_CFG_U1 | 0x20910521Lu     // Output rate of the UBX-NAV2-SVIN message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SVIN_UART2  UBX_CFG_U1 | 0x20910522Lu     // Output rate of the UBX-NAV2-SVIN message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_SVIN_USB  UBX_CFG_U1 | 0x20910523Lu       // Output rate of the UBX-NAV2-SVIN message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEBDS_I2C  UBX_CFG_U1 | 0x20910525Lu    // Output rate of the UBX-NAV2-TIMEBDS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEBDS_SPI  UBX_CFG_U1 | 0x20910529Lu    // Output rate of the UBX-NAV2-TIMEBDS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEBDS_UART1  UBX_CFG_U1 | 0x20910526Lu  // Output rate of the UBX-NAV2-TIMEBDS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEBDS_UART2  UBX_CFG_U1 | 0x20910527Lu  // Output rate of the UBX-NAV2-TIMEBDS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEBDS_USB  UBX_CFG_U1 | 0x20910528Lu    // Output rate of the UBX-NAV2-TIMEBDS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGAL_I2C  UBX_CFG_U1 | 0x20910530Lu    // Output rate of the UBX-NAV2-TIMEGAL message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGAL_SPI  UBX_CFG_U1 | 0x20910534Lu    // Output rate of the UBX-NAV2-TIMEGAL message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGAL_UART1  UBX_CFG_U1 | 0x20910531Lu  // Output rate of the UBX-NAV2-TIMEGAL message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGAL_UART2  UBX_CFG_U1 | 0x20910532Lu  // Output rate of the UBX-NAV2-TIMEGAL message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGAL_USB  UBX_CFG_U1 | 0x20910533Lu    // Output rate of the UBX-NAV2-TIMEGAL message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGLO_I2C  UBX_CFG_U1 | 0x20910535Lu    // Output rate of the UBX-NAV2-TIMEGLO message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGLO_SPI  UBX_CFG_U1 | 0x20910539Lu    // Output rate of the UBX-NAV2-TIMEGLO message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGLO_UART1  UBX_CFG_U1 | 0x20910536Lu  // Output rate of the UBX-NAV2-TIMEGLO message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGLO_UART2  UBX_CFG_U1 | 0x20910537Lu  // Output rate of the UBX-NAV2-TIMEGLO message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGLO_USB  UBX_CFG_U1 | 0x20910538Lu    // Output rate of the UBX-NAV2-TIMEGLO message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGPS_I2C  UBX_CFG_U1 | 0x20910540Lu    // Output rate of the UBX-NAV2-TIMEGPS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGPS_SPI  UBX_CFG_U1 | 0x20910544Lu    // Output rate of the UBX-NAV2-TIMEGPS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGPS_UART1  UBX_CFG_U1 | 0x20910541Lu  // Output rate of the UBX-NAV2-TIMEGPS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGPS_UART2  UBX_CFG_U1 | 0x20910542Lu  // Output rate of the UBX-NAV2-TIMEGPS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEGPS_USB  UBX_CFG_U1 | 0x20910543Lu    // Output rate of the UBX-NAV2-TIMEGPS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMELS_I2C  UBX_CFG_U1 | 0x20910545Lu     // Output rate of the UBX-NAV2-TIMELS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMELS_SPI  UBX_CFG_U1 | 0x20910549Lu     // Output rate of the UBX-NAV2-TIMELS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMELS_UART1  UBX_CFG_U1 | 0x20910546Lu   // Output rate of the UBX-NAV2-TIMELS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMELS_UART2  UBX_CFG_U1 | 0x20910547Lu   // Output rate of the UBX-NAV2-TIMELS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMELS_USB  UBX_CFG_U1 | 0x20910548Lu     // Output rate of the UBX-NAV2-TIMELS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEQZSS_I2C  UBX_CFG_U1 | 0x20910575Lu   // Output rate of the UBX-NAV2-TIMEQZSS message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEQZSS_SPI  UBX_CFG_U1 | 0x20910579Lu   // Output rate of the UBX-NAV2-TIMEQZSS message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEQZSS_UART1  UBX_CFG_U1 | 0x20910576Lu // Output rate of the UBX-NAV2-TIMEQZSS message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEQZSS_UART2  UBX_CFG_U1 | 0x20910577Lu // Output rate of the UBX-NAV2-TIMEQZSS message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEQZSS_USB  UBX_CFG_U1 | 0x20910578Lu   // Output rate of the UBX-NAV2-TIMEQZSS message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEUTC_I2C  UBX_CFG_U1 | 0x20910550Lu    // Output rate of the UBX-NAV2-TIMEUTC message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEUTC_SPI  UBX_CFG_U1 | 0x20910554Lu    // Output rate of the UBX-NAV2-TIMEUTC message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEUTC_UART1  UBX_CFG_U1 | 0x20910551Lu  // Output rate of the UBX-NAV2-TIMEUTC message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEUTC_UART2  UBX_CFG_U1 | 0x20910552Lu  // Output rate of the UBX-NAV2-TIMEUTC message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_TIMEUTC_USB  UBX_CFG_U1 | 0x20910553Lu    // Output rate of the UBX-NAV2-TIMEUTC message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_VELECEF_I2C  UBX_CFG_U1 | 0x20910555Lu    // Output rate of the UBX-NAV2-VELECEF message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_VELECEF_SPI  UBX_CFG_U1 | 0x20910559Lu    // Output rate of the UBX-NAV2-VELECEF message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_VELECEF_UART1  UBX_CFG_U1 | 0x20910556Lu  // Output rate of the UBX-NAV2-VELECEF message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_VELECEF_UART2  UBX_CFG_U1 | 0x20910557Lu  // Output rate of the UBX-NAV2-VELECEF message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_VELECEF_USB  UBX_CFG_U1 | 0x20910558Lu    // Output rate of the UBX-NAV2-VELECEF message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV2_VELNED_I2C  UBX_CFG_U1 | 0x20910560Lu     // Output rate of the UBX-NAV2-VELNED message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV2_VELNED_SPI  UBX_CFG_U1 | 0x20910564Lu     // Output rate of the UBX-NAV2-VELNED message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV2_VELNED_UART1  UBX_CFG_U1 | 0x20910561Lu   // Output rate of the UBX-NAV2-VELNED message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV2_VELNED_UART2  UBX_CFG_U1 | 0x20910562Lu   // Output rate of the UBX-NAV2-VELNED message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV2_VELNED_USB  UBX_CFG_U1 | 0x20910563Lu     // Output rate of the UBX-NAV2-VELNED message on port USB
#define UBLOX_CFG_MSGOUT_UBX_NAV_NMI_I2C  UBX_CFG_U1 | 0x20910590Lu         // Output rate of the UBX-NAV-NMI message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_NAV_NMI_SPI  UBX_CFG_U1 | 0x20910594Lu         // Output rate of the UBX-NAV-NMI message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_NAV_NMI_UART1  UBX_CFG_U1 | 0x20910591Lu       // Output rate of the UBX-NAV-NMI message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_NAV_NMI_UART2  UBX_CFG_U1 | 0x20910592Lu       // Output rate of the UBX-NAV-NMI message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_NAV_NMI_USB  UBX_CFG_U1 | 0x20910593Lu         // Output rate of the UBX-NAV-NMI message on port USB
#define UBLOX_CFG_MSGOUT_UBX_RXM_TM_I2C  UBX_CFG_U1 | 0x20910610Lu          // Output rate of the UBX-RXM-TM message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_RXM_TM_SPI  UBX_CFG_U1 | 0x20910614Lu          // Output rate of the UBX-RXM-TM message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_RXM_TM_UART1  UBX_CFG_U1 | 0x20910611Lu        // Output rate of the UBX-RXM-TM message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_RXM_TM_UART2  UBX_CFG_U1 | 0x20910612Lu        // Output rate of the UBX-RXM-TM message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_RXM_TM_USB  UBX_CFG_U1 | 0x20910613Lu          // Output rate of the UBX-RXM-TM message on port USB
#define UBLOX_CFG_MSGOUT_UBX_SEC_SIGLOG_I2C  UBX_CFG_U1 | 0x20910689Lu      // Output rate of the UBX-SEC-SIGLOG message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_SEC_SIGLOG_SPI  UBX_CFG_U1 | 0x2091068dLu      // Output rate of the UBX-SEC-SIGLOG message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_SEC_SIGLOG_UART1  UBX_CFG_U1 | 0x2091068aLu    // Output rate of the UBX-SEC-SIGLOG message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_SEC_SIGLOG_UART2  UBX_CFG_U1 | 0x2091068bLu    // Output rate of the UBX-SEC-SIGLOG message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_SEC_SIGLOG_USB  UBX_CFG_U1 | 0x2091068cLu      // Output rate of the UBX-SEC-SIGLOG message on port USB
#define UBLOX_CFG_MSGOUT_UBX_TIM_SVIN_I2C  UBX_CFG_U1 | 0x20910097Lu        // Output rate of the UBX-TIM-SVIN message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_TIM_SVIN_SPI  UBX_CFG_U1 | 0x2091009bLu        // Output rate of the UBX-TIM-SVIN message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_TIM_SVIN_UART1  UBX_CFG_U1 | 0x20910098Lu      // Output rate of the UBX-TIM-SVIN message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_TIM_SVIN_UART2  UBX_CFG_U1 | 0x20910099Lu      // Output rate of the UBX-TIM-SVIN message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_TIM_SVIN_USB  UBX_CFG_U1 | 0x2091009aLu        // Output rate of the UBX-TIM-SVIN message on port USB

// Additional CFG_MSGOUT keys for the NEO-D9S
//--------------------------------------------------------
#define UBLOX_CFG_MSGOUT_UBX_RXM_PMP_I2C  UBX_CFG_U1 | 0x2091031dLu   // Output rate of the UBX_RXM_PMP message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_RXM_PMP_SPI  UBX_CFG_U1 | 0x20910321Lu   // Output rate of the UBX_RXM_PMP message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_RXM_PMP_UART1  UBX_CFG_U1 | 0x2091031eLu // Output rate of the UBX_RXM_PMP message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_RXM_PMP_UART2  UBX_CFG_U1 | 0x2091031fLu // Output rate of the UBX_RXM_PMP message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_RXM_PMP_USB  UBX_CFG_U1 | 0x20910320Lu   // Output rate of the UBX_RXM_PMP message on port USB
#define UBLOX_CFG_MSGOUT_UBX_MON_PMP_I2C  UBX_CFG_U1 | 0x20910322Lu   // Output rate of the UBX_MON_PMP message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_MON_PMP_SPI  UBX_CFG_U1 | 0x20910326Lu   // Output rate of the UBX_MON_PMP message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_MON_PMP_UART1  UBX_CFG_U1 | 0x20910323Lu // Output rate of the UBX_MON_PMP message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_MON_PMP_UART2  UBX_CFG_U1 | 0x20910324Lu // Output rate of the UBX_MON_PMP message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_MON_PMP_USB  UBX_CFG_U1 | 0x20910325Lu   // Output rate of the UBX_MON_PMP message on port USB

// Additional CFG_MSGOUT keys for the NEO-D9S
//--------------------------------------------------------
#define UBLOX_CFG_MSGOUT_UBX_RXM_QZSSL6_I2C  UBX_CFG_U1 | 0x2091033fLu   // Output rate of the UBX_RXM_QZSSL6 message on port I2C
#define UBLOX_CFG_MSGOUT_UBX_RXM_QZSSL6_SPI  UBX_CFG_U1 | 0x2091033eLu   // Output rate of the UBX_RXM_QZSSL6 message on port SPI
#define UBLOX_CFG_MSGOUT_UBX_RXM_QZSSL6_UART1  UBX_CFG_U1 | 0x2091033bLu // Output rate of the UBX_RXM_QZSSL6 message on port UART1
#define UBLOX_CFG_MSGOUT_UBX_RXM_QZSSL6_UART2  UBX_CFG_U1 | 0x2091033cLu // Output rate of the UBX_RXM_QZSSL6 message on port UART2
#define UBLOX_CFG_MSGOUT_UBX_RXM_QZSSL6_USB  UBX_CFG_U1 | 0x2091033dLu   // Output rate of the UBX_RXM_QZSSL6 message on port USB

// CFG-NAV2: Secondary output configuration
//--------------------------------------------------------
#define UBLOX_CFG_NAV2_OUT_ENABLED  UBX_CFG_L | 0x10170001Lu        // Enable secondary (NAV2) output
#define UBLOX_CFG_NAV2_SBAS_USE_INTEGRITY  UBX_CFG_L | 0x10170002Lu // Use SBAS integrity information in the secondary output

// CFG-NAVHPG: High precision navigation configuration
//--------------------------------------------------------
#define UBLOX_CFG_NAVHPG_DGNSSMODE  UBX_CFG_E1 | 0x20140011Lu // Diﬀerential corrections mode

// CFG-NAVSPG: Standard precision navigation configuration
//--------------------------------------------------------
#define UBLOX_CFG_NAVSPG_FIXMODE  UBX_CFG_E1 | 0x20110011Lu        // Position fix mode
#define UBLOX_CFG_NAVSPG_INIFIX3D  UBX_CFG_L | 0x10110013Lu        // Initial fix must be a 3D fix
#define UBLOX_CFG_NAVSPG_WKNROLLOVER  UBX_CFG_U2 | 0x30110017Lu    // GPS week rollover number
#define UBLOX_CFG_NAVSPG_USE_PPP  UBX_CFG_L | 0x10110019Lu         // Use precise point positioning (PPP)
#define UBLOX_CFG_NAVSPG_UTCSTANDARD  UBX_CFG_E1 | 0x2011001cLu    // UTC standard to be used
#define UBLOX_CFG_NAVSPG_DYNMODEL  UBX_CFG_E1 | 0x20110021Lu       // Dynamic platform model
#define UBLOX_CFG_NAVSPG_ACKAIDING  UBX_CFG_L | 0x10110025Lu       // Acknowledge assistance input messages
#define UBLOX_CFG_NAVSPG_USE_USRDAT  UBX_CFG_L | 0x10110061Lu      // Use user geodetic datum parameters
#define UBLOX_CFG_NAVSPG_USRDAT_MAJA  UBX_CFG_R8 | 0x50110062Lu    // Geodetic datum semi-major axis
#define UBLOX_CFG_NAVSPG_USRDAT_FLAT  UBX_CFG_R8 | 0x50110063Lu    // Geodetic datum 1.0 flattening
#define UBLOX_CFG_NAVSPG_USRDAT_DX  UBX_CFG_R4 | 0x40110064Lu      // Geodetic datum X axis shift at the origin
#define UBLOX_CFG_NAVSPG_USRDAT_DY  UBX_CFG_R4 | 0x40110065Lu      // Geodetic datum Y axis shift at the origin
#define UBLOX_CFG_NAVSPG_USRDAT_DZ  UBX_CFG_R4 | 0x40110066Lu      // Geodetic datum Z axis shift at the origin
#define UBLOX_CFG_NAVSPG_USRDAT_ROTX  UBX_CFG_R4 | 0x40110067Lu    // arcsec Geodetic datum rotation about the X axis
#define UBLOX_CFG_NAVSPG_USRDAT_ROTY  UBX_CFG_R4 | 0x40110068Lu    // arcsec Geodetic datum rotation about the Y axis
#define UBLOX_CFG_NAVSPG_USRDAT_ROTZ  UBX_CFG_R4 | 0x40110069Lu    // arcsec Geodetic datum rotation about the Z axis
#define UBLOX_CFG_NAVSPG_USRDAT_SCALE  UBX_CFG_R4 | 0x4011006aLu   // ppm Geodetic datum scale factor
#define UBLOX_CFG_NAVSPG_INFIL_MINSVS  UBX_CFG_U1 | 0x201100a1Lu   // Minimum number of satellites for navigation
#define UBLOX_CFG_NAVSPG_INFIL_MAXSVS  UBX_CFG_U1 | 0x201100a2Lu   // Maximum number of satellites for navigation
#define UBLOX_CFG_NAVSPG_INFIL_MINCNO  UBX_CFG_U1 | 0x201100a3Lu   // Minimum satellite signal level for navigation
#define UBLOX_CFG_NAVSPG_INFIL_MINELEV  UBX_CFG_U1 | 0x201100a4Lu  // Minimum elevation for a GNSS satellite to be used in navigation
#define UBLOX_CFG_NAVSPG_INFIL_NCNOTHRS  UBX_CFG_U1 | 0x201100aaLu // Number of satellites required to have C/N0 above #define UBLOX_CFG_NAVSPG-INFIL_CNOTHRS for a fix to be attempted
#define UBLOX_CFG_NAVSPG_INFIL_CNOTHRS  UBX_CFG_U1 | 0x201100abLu  // C/N0 threshold for deciding whether to attempt a fix
#define UBLOX_CFG_NAVSPG_OUTFIL_PDOP  UBX_CFG_U2 | 0x301100b1Lu    // Output filter position DOP mask (threshold)
#define UBLOX_CFG_NAVSPG_OUTFIL_TDOP  UBX_CFG_U2 | 0x301100b2Lu    // Output filter time DOP mask (threshold)
#define UBLOX_CFG_NAVSPG_OUTFIL_PACC  UBX_CFG_U2 | 0x301100b3Lu    // Output filter position accuracy mask (threshold)
#define UBLOX_CFG_NAVSPG_OUTFIL_TACC  UBX_CFG_U2 | 0x301100b4Lu    // Output filter time accuracy mask (threshold)
#define UBLOX_CFG_NAVSPG_OUTFIL_FACC  UBX_CFG_U2 | 0x301100b5Lu    // Output filter frequency accuracy mask (threshold)
#define UBLOX_CFG_NAVSPG_CONSTR_ALT  UBX_CFG_I4 | 0x401100c1Lu     // Fixed altitude (mean sea level) for 2D fix mode
#define UBLOX_CFG_NAVSPG_CONSTR_ALTVAR  UBX_CFG_U4 | 0x401100c2Lu  // Fixed altitude variance for 2D mode
#define UBLOX_CFG_NAVSPG_CONSTR_DGNSSTO  UBX_CFG_U1 | 0x201100c4Lu // DGNSS timeout
#define UBLOX_CFG_NAVSPG_SIGATTCOMP  UBX_CFG_E1 | 0x201100d6Lu     // Permanently attenuated signal compensation mode
#define UBLOX_CFG_NAVSPG_PL_ENA  UBX_CFG_L | 0x101100d7Lu          // Enable Protection level. If enabled, protection level computing will be on.

// CFG-NMEA: NMEA protocol configuration
//--------------------------------------------------------
#define UBLOX_CFG_NMEA_PROTVER  UBX_CFG_E1 | 0x20930001Lu      // NMEA protocol version
#define UBLOX_CFG_NMEA_MAXSVS  UBX_CFG_E1 | 0x20930002Lu       // Maximum number of SVs to report per Talker ID
#define UBLOX_CFG_NMEA_COMPAT  UBX_CFG_L | 0x10930003Lu        // Enable compatibility mode
#define UBLOX_CFG_NMEA_CONSIDER  UBX_CFG_L | 0x10930004Lu      // Enable considering mode
#define UBLOX_CFG_NMEA_LIMIT82  UBX_CFG_L | 0x10930005Lu       // Enable strict limit to 82 characters maximum NMEA message length
#define UBLOX_CFG_NMEA_HIGHPREC  UBX_CFG_L | 0x10930006Lu      // Enable high precision mode
#define UBLOX_CFG_NMEA_SVNUMBERING  UBX_CFG_E1 | 0x20930007Lu  // Display configuration for SVs that do not have value defined in NMEA
#define UBLOX_CFG_NMEA_FILT_GPS  UBX_CFG_L | 0x10930011Lu      // Disable reporting of GPS satellites
#define UBLOX_CFG_NMEA_FILT_SBAS  UBX_CFG_L | 0x10930012Lu     // Disable reporting of SBAS satellites
#define UBLOX_CFG_NMEA_FILT_GAL  UBX_CFG_L | 0x10930013Lu      // Disable reporting of Galileo satellites
#define UBLOX_CFG_NMEA_FILT_QZSS  UBX_CFG_L | 0x10930015Lu     // Disable reporting of QZSS satellites
#define UBLOX_CFG_NMEA_FILT_GLO  UBX_CFG_L | 0x10930016Lu      // Disable reporting of GLONASS satellites
#define UBLOX_CFG_NMEA_FILT_BDS  UBX_CFG_L | 0x10930017Lu      // Disable reporting of BeiDou satellites
#define UBLOX_CFG_NMEA_OUT_INVFIX  UBX_CFG_L | 0x10930021Lu    // Enable position output for failed or invalid fixes
#define UBLOX_CFG_NMEA_OUT_MSKFIX  UBX_CFG_L | 0x10930022Lu    // Enable position output for invalid fixes
#define UBLOX_CFG_NMEA_OUT_INVTIME  UBX_CFG_L | 0x10930023Lu   // Enable time output for invalid times
#define UBLOX_CFG_NMEA_OUT_INVDATE  UBX_CFG_L | 0x10930024Lu   // Enable date output for invalid dates
#define UBLOX_CFG_NMEA_OUT_ONLYGPS  UBX_CFG_L | 0x10930025Lu   // Restrict output to GPS satellites only
#define UBLOX_CFG_NMEA_OUT_FROZENCOG  UBX_CFG_L | 0x10930026Lu // Enable course over ground output even if it is frozen
#define UBLOX_CFG_NMEA_MAINTALKERID  UBX_CFG_E1 | 0x20930031Lu // Main Talker ID
#define UBLOX_CFG_NMEA_GSVTALKERID  UBX_CFG_E1 | 0x20930032Lu  // Talker ID for GSV NMEA messages
#define UBLOX_CFG_NMEA_BDSTALKERID  UBX_CFG_U2 | 0x30930033Lu  // BeiDou Talker ID

// CFG-ODO: Odometer and low-speed course over ground filter
//--------------------------------------------------------
#define UBLOX_CFG_ODO_USE_ODO  UBX_CFG_L | 0x10220001Lu       // Use odometer
#define UBLOX_CFG_ODO_USE_COG  UBX_CFG_L | 0x10220002Lu       // Use low-speed course over ground filter
#define UBLOX_CFG_ODO_OUTLPVEL  UBX_CFG_L | 0x10220003Lu      // Output low-pass filtered velocity
#define UBLOX_CFG_ODO_OUTLPCOG  UBX_CFG_L | 0x10220004Lu      // Output low-pass filtered course over ground (heading)
#define UBLOX_CFG_ODO_PROFILE  UBX_CFG_E1 | 0x20220005Lu      // Odometer profile configuration
#define UBLOX_CFG_ODO_COGMAXSPEED  UBX_CFG_U1 | 0x20220021Lu  // Upper speed limit for low-speed course over ground filter
#define UBLOX_CFG_ODO_COGMAXPOSACC  UBX_CFG_U1 | 0x20220022Lu // Maximum acceptable position accuracy for computing low-speed filtered course over ground
#define UBLOX_CFG_ODO_VELLPGAIN  UBX_CFG_U1 | 0x20220031Lu    // Velocity low-pass filter level
#define UBLOX_CFG_ODO_COGLPGAIN  UBX_CFG_U1 | 0x20220032Lu    // Course over ground low-pass filter level (at speed < 8 m/s)

// CFG-PM: Configuration for receiver power management (NEO-D9S and MAX-M10S)
//--------------------------------------------------------
#define UBLOX_CFG_PM_OPERATEMODE  UBX_CFG_E1 | 0x20d00001Lu      // General mode of operation
#define UBLOX_CFG_PM_POSUPDATEPERIOD  UBX_CFG_U4 | 0x40d00002Lu  // Position update period for PSMOO - s
#define UBLOX_CFG_PM_ACQPERIOD  UBX_CFG_U4 | 0x40d00003Lu        // Acquisition period used if the receiver previously failed to achieve a position fix - s
#define UBLOX_CFG_PM_GRIDOFFSET  UBX_CFG_U4 | 0x40d00004Lu       // Position update period grid offset relative to GPS start of week - s
#define UBLOX_CFG_PM_ONTIME  UBX_CFG_U2 | 0x30d00005Lu           // Time to stay in Tracking state - s
#define UBLOX_CFG_PM_MINACQTIME  UBX_CFG_U1 | 0x20d00006Lu       // Minimum time to spend in Acquisition state - s
#define UBLOX_CFG_PM_MAXACQTIME  UBX_CFG_U1 | 0x20d00007Lu       // Maximum time to spend in Acquisition state - s
#define UBLOX_CFG_PM_DONOTENTEROFF  UBX_CFG_L | 0x10d00008Lu     // Behavior of receiver in case it cannot achieve a position fix during a position update period.
#define UBLOX_CFG_PM_WAITTIMEFIX  UBX_CFG_L | 0x10d00009Lu       // Wait for time fix
#define UBLOX_CFG_PM_UPDATEEPH  UBX_CFG_L | 0x10d0000aLu         // Update ephemeris regularly.
#define UBLOX_CFG_PM_EXTINTSEL  UBX_CFG_E1 | 0x20d0000bLu        // EXTINT pin select
#define UBLOX_CFG_PM_EXTINTWAKE  UBX_CFG_L | 0x10d0000cLu        // EXTINT pin control (Wake). Enable to keep receiver awake as long as selected EXTINT pin is "high".
#define UBLOX_CFG_PM_EXTINTBACKUP  UBX_CFG_L | 0x10d0000dLu      // EXTINT pin control (Backup). Enable to force receiver into BACKUP mode when selected EXTINT pin is "low".
#define UBLOX_CFG_PM_EXTINTINACTIVE  UBX_CFG_L | 0x10d0000eLu    // EXTINT pin control (Inactive). Enable to force backup in case EXTINT Pin is inactive for time longer than CFG-PM-EXTINTINACTIVITY.
#define UBLOX_CFG_PM_EXTINTINACTIVITY  UBX_CFG_U4 | 0x40d0000fLu // Inactivity time out on EXTINT pin if enabled
#define UBLOX_CFG_PM_LIMITPEAKCURR  UBX_CFG_L | 0x10d00010Lu     // Limit peak current

// CFG-PMP: Point to multipoint (PMP) configuration (NEO-D9S)
//--------------------------------------------------------
#define UBLOX_CFG_PMP_CENTER_FREQUENCY  UBX_CFG_U4 | 0x40b10011Lu // Center frequency. The center frequency for the receiver can be set from 1525000000 to 1559000000 Hz.
#define UBLOX_CFG_PMP_SEARCH_WINDOW  UBX_CFG_U2 | 0x30b10012Lu    // Search window. Search window can be set from 0 to 65535 Hz. It is +/- this value from the center frequency set by CENTER_FREQUENCY.
#define UBLOX_CFG_PMP_USE_SERVICE_ID  UBX_CFG_L | 0x10b10016Lu    // Use service ID. Enable/disable service ID check to confirm the correct service is received.
#define UBLOX_CFG_PMP_SERVICE_ID  UBX_CFG_U2 | 0x30b10017Lu       // Service identifier. Defines the expected service ID.
#define UBLOX_CFG_PMP_DATA_RATE  UBX_CFG_E2 | 0x30b10013Lu        // bps Data rate. The data rate of the received data.
#define UBLOX_CFG_PMP_USE_DESCRAMBLER  UBX_CFG_L | 0x10b10014Lu   // Use descrambler. Enables/disables the descrambler.
#define UBLOX_CFG_PMP_DESCRAMBLER_INIT  UBX_CFG_U2 | 0x30b10015Lu // Descrambler initialization. Set the intialisation value for the descrambler.
#define UBLOX_CFG_PMP_USE_PRESCRAMBLING  UBX_CFG_L | 0x10b10019Lu // Use prescrambling. Enables/disables the prescrambling.
#define UBLOX_CFG_PMP_UNIQUE_WORD  UBX_CFG_U8 | 0x50b1001aLu      // Unique word. Defines value of unique word.

// CFG-QZSS-L6: QZSS system configuration configuration (NEO-D9C)
//--------------------------------------------------------
#define UBLOX_CFG_QZSSL6_SVIDA  UBX_CFG_I1 | 0x20370020Lu     // QZSS L6 SV Id to be decoded by channel A
#define UBLOX_CFG_QZSSL6_SVIDB  UBX_CFG_I1 | 0x20370030Lu     // QZSS L6 SV Id to be decoded by channel B
#define UBLOX_CFG_QZSSL6_MSGA  UBX_CFG_E1 | 0x20370050Lu      // QZSS L6 messages to be decoded by channel A
#define UBLOX_CFG_QZSSL6_MSGB  UBX_CFG_E1 | 0x20370060Lu      // QZSS L6 messages to be decoded by channel B
#define UBLOX_CFG_QZSSL6_RSDECODER  UBX_CFG_E1 | 0x20370080Lu // QZSS L6 message Reed-Solomon decoder mode

// CFG-QZSS: QZSS system configuration
//--------------------------------------------------------
#define UBLOX_CFG_QZSS_USE_SLAS_DGNSS  UBX_CFG_L | 0x10370005Lu       // Apply QZSS SLAS DGNSS corrections
#define UBLOX_CFG_QZSS_USE_SLAS_TESTMODE  UBX_CFG_L | 0x10370006Lu    // Use QZSS SLAS data when it is in test mode (SLAS msg 0)
#define UBLOX_CFG_QZSS_USE_SLAS_RAIM_UNCORR  UBX_CFG_L | 0x10370007Lu // Raim out measurements that are not corrected by QZSS SLAS, if at least 5 measurements are corrected
#define UBLOX_CFG_QZSS_SLAS_MAX_BASELINE  UBX_CFG_U2 | 0x30370008Lu   // Maximum baseline distance to closest Ground Monitoring Station: km

// CFG-RATE: Navigation and measurement rate configuration
//--------------------------------------------------------
#define UBLOX_CFG_RATE_MEAS  UBX_CFG_U2 | 0x30210001Lu     // Nominal time between GNSS measurements
#define UBLOX_CFG_RATE_NAV  UBX_CFG_U2 | 0x30210002Lu      // Ratio of number of measurements to number of navigation solutions
#define UBLOX_CFG_RATE_TIMEREF  UBX_CFG_E1 | 0x20210003Lu  // Time system to which measurements are aligned
#define UBLOX_CFG_RATE_NAV_PRIO  UBX_CFG_U1 | 0x20210004Lu // Output rate of priority navigation mode messages

// CFG-RINV: Remote inventory
//--------------------------------------------------------
#define UBLOX_CFG_RINV_DUMP  UBX_CFG_L | 0x10c70001Lu       // Dump data at startup
#define UBLOX_CFG_RINV_BINARY  UBX_CFG_L | 0x10c70002Lu     // Data is binary
#define UBLOX_CFG_RINV_DATA_SIZE  UBX_CFG_U1 | 0x20c70003Lu // Size of data
#define UBLOX_CFG_RINV_CHUNK0  UBX_CFG_X8 | 0x50c70004Lu    // Data bytes 1-8 (LSB)
#define UBLOX_CFG_RINV_CHUNK1  UBX_CFG_X8 | 0x50c70005Lu    // Data bytes 9-16
#define UBLOX_CFG_RINV_CHUNK2  UBX_CFG_X8 | 0x50c70006Lu    // Data bytes 17-24
#define UBLOX_CFG_RINV_CHUNK3  UBX_CFG_X8 | 0x50c70007Lu    // Data bytes 25-30 (MSB)

// CFG-RTCM: RTCM protocol configuration
//--------------------------------------------------------
#define UBLOX_CFG_RTCM_DF003_OUT  UBX_CFG_U2 | 0x30090001Lu       // RTCM DF003 (Reference station ID) output value
#define UBLOX_CFG_RTCM_DF003_IN  UBX_CFG_U2 | 0x30090008Lu        // RTCM DF003 (Reference station ID) input value
#define UBLOX_CFG_RTCM_DF003_IN_FILTER  UBX_CFG_E1 | 0x20090009Lu // RTCM input filter configuration based on RTCM DF003 (Reference station ID) value

// CFG-SBAS: SBAS configuration
//--------------------------------------------------------
#define UBLOX_CFG_SBAS_USE_TESTMODE  UBX_CFG_L | 0x10360002Lu  // Use SBAS data when it is in test mode (SBAS msg 0)
#define UBLOX_CFG_SBAS_USE_RANGING  UBX_CFG_L | 0x10360003Lu   // Use SBAS GEOs as a ranging source (for navigation)
#define UBLOX_CFG_SBAS_USE_DIFFCORR  UBX_CFG_L | 0x10360004Lu  // Use SBAS diﬀerential corrections
#define UBLOX_CFG_SBAS_USE_INTEGRITY  UBX_CFG_L | 0x10360005Lu // Use SBAS integrity information
#define UBLOX_CFG_SBAS_PRNSCANMASK  UBX_CFG_X8 | 0x50360006Lu  // SBAS PRN search configuration

// CFG-SEC: Security configuration (ZED-F9R)
//--------------------------------------------------------
#define UBLOX_CFG_SEC_CFG_LOCK  UBX_CFG_L | 0x10f60009Lu             // Configuration lockdown
#define UBLOX_CFG_SEC_CFG_LOCK_UNLOCKGRP1  UBX_CFG_U2 | 0x30f6000aLu // Configuration lockdown exempted group 1
#define UBLOX_CFG_SEC_CFG_LOCK_UNLOCKGRP2  UBX_CFG_U2 | 0x30f6000bLu // Configuration lockdown exempted group 2

// CFG-SFCORE: Sensor fusion (SF) core configuration (ZED-F9R)
//--------------------------------------------------------
#define UBLOX_CFG_SFCORE_USE_SF  UBX_CFG_L | 0x10080001Lu // Use ADR/UDR sensor fusion

// CFG-SFIMU: Sensor fusion (SF) inertial measurement unit (IMU) configuration (ZED-F9R)
//--------------------------------------------------------
#define UBLOX_CFG_SFIMU_GYRO_TC_UPDATE_PERIOD  UBX_CFG_U2 | 0x30060007Lu // Time period between each update for the saved temperature-dependent gyroscope bias table
#define UBLOX_CFG_SFIMU_GYRO_RMSTHDL  UBX_CFG_U1 | 0x20060008Lu          // Gyroscope sensor RMS threshold
#define UBLOX_CFG_SFIMU_GYRO_FREQUENCY  UBX_CFG_U1 | 0x20060009Lu        // Nominal gyroscope sensor data sampling frequency
#define UBLOX_CFG_SFIMU_GYRO_LATENCY  UBX_CFG_U2 | 0x3006000aLu          // Gyroscope sensor data latency due to e.g. CAN bus
#define UBLOX_CFG_SFIMU_GYRO_ACCURACY  UBX_CFG_U2 | 0x3006000bLu         // Gyroscope sensor data accuracy
#define UBLOX_CFG_SFIMU_ACCEL_RMSTHDL  UBX_CFG_U1 | 0x20060015Lu         // Accelerometer RMS threshold
#define UBLOX_CFG_SFIMU_ACCEL_FREQUENCY  UBX_CFG_U1 | 0x20060016Lu       // Nominal accelerometer sensor data sampling frequency
#define UBLOX_CFG_SFIMU_ACCEL_LATENCY  UBX_CFG_U2 | 0x30060017Lu         // Accelerometer sensor data latency due to e.g. CAN bus
#define UBLOX_CFG_SFIMU_ACCEL_ACCURACY  UBX_CFG_U2 | 0x30060018Lu        // Accelerometer sensor data accuracy
#define UBLOX_CFG_SFIMU_IMU_EN  UBX_CFG_L | 0x1006001dLu                 // IMU enabled
#define UBLOX_CFG_SFIMU_IMU_I2C_SCL_PIO  UBX_CFG_U1 | 0x2006001eLu       // SCL PIO of the IMU I2C
#define UBLOX_CFG_SFIMU_IMU_I2C_SDA_PIO  UBX_CFG_U1 | 0x2006001fLu       // SDA PIO of the IMU I2C
#define UBLOX_CFG_SFIMU_AUTO_MNTALG_ENA  UBX_CFG_L | 0x10060027Lu        // Enable automatic IMU-mount alignment
#define UBLOX_CFG_SFIMU_IMU_MNTALG_YAW  UBX_CFG_U4 | 0x4006002dLu        // User-defined IMU-mount yaw angle [0, 360]
#define UBLOX_CFG_SFIMU_IMU_MNTALG_PITCH  UBX_CFG_I2 | 0x3006002eLu      // User-defined IMU-mount pitch angle [-90, 90]
#define UBLOX_CFG_SFIMU_IMU_MNTALG_ROLL  UBX_CFG_I2 | 0x3006002fLu       // User-defined IMU-mount roll angle [-180, 180]

// CFG-SFODO: Sensor fusion (SF) odometer configuration (ZED-F9R)
//--------------------------------------------------------
#define UBLOX_CFG_SFODO_COMBINE_TICKS  UBX_CFG_L | 0x10070001Lu     // Use combined rear wheel ticks instead of the single tick
#define UBLOX_CFG_SFODO_USE_SPEED  UBX_CFG_L | 0x10070003Lu         // Use speed measurements
#define UBLOX_CFG_SFODO_DIS_AUTOCOUNTMAX  UBX_CFG_L | 0x10070004Lu  // Disable automatic estimation of maximum absolute wheel tick counter
#define UBLOX_CFG_SFODO_DIS_AUTODIRPINPOL  UBX_CFG_L | 0x10070005Lu // Disable automatic wheel tick direction pin polarity detection
#define UBLOX_CFG_SFODO_DIS_AUTOSPEED  UBX_CFG_L | 0x10070006Lu     // Disable automatic receiver reconfiguration for processing speed data
#define UBLOX_CFG_SFODO_FACTOR  UBX_CFG_U4 | 0x40070007Lu           // Wheel tick scale factor
#define UBLOX_CFG_SFODO_QUANT_ERROR  UBX_CFG_U4 | 0x40070008Lu      // Wheel tick quantization
#define UBLOX_CFG_SFODO_COUNT_MAX  UBX_CFG_U4 | 0x40070009Lu        // Wheel tick counter maximum value
#define UBLOX_CFG_SFODO_LATENCY  UBX_CFG_U2 | 0x3007000aLu          // Wheel tick data latency due to e.g. CAN bus
#define UBLOX_CFG_SFODO_FREQUENCY  UBX_CFG_U1 | 0x2007000bLu        // Nominal wheel tick data frequency (0  not set)
#define UBLOX_CFG_SFODO_CNT_BOTH_EDGES  UBX_CFG_L | 0x1007000dLu    // Count both rising and falling edges on wheel tick signal
#define UBLOX_CFG_SFODO_SPEED_BAND  UBX_CFG_U2 | 0x3007000eLu       // Speed sensor dead band (0  not set)
#define UBLOX_CFG_SFODO_USE_WT_PIN  UBX_CFG_L | 0x1007000fLu        // Wheel tick signal enabled
#define UBLOX_CFG_SFODO_DIR_PINPOL  UBX_CFG_L | 0x10070010Lu        // Wheel tick direction pin polarity
#define UBLOX_CFG_SFODO_DIS_AUTOSW  UBX_CFG_L | 0x10070011Lu        // Disable automatic use of wheel tick or speed data received over the software interface
#define UBLOX_CFG_SFODO_DIS_DIR_INFO  UBX_CFG_L | 0x1007001cLu      // Do not use directional information

// CFG-SIGNAL: Satellite systems (GNSS) signal configuration
//--------------------------------------------------------
#define UBLOX_CFG_SIGNAL_GPS_ENA  UBX_CFG_L | 0x1031001fLu       // GPS enable
#define UBLOX_CFG_SIGNAL_GPS_L1CA_ENA  UBX_CFG_L | 0x10310001Lu  // GPS L1C/A
#define UBLOX_CFG_SIGNAL_GPS_L5_ENA  UBX_CFG_L | 0x10310004Lu    // GPS L5
#define UBLOX_CFG_SIGNAL_GPS_L2C_ENA  UBX_CFG_L | 0x10310003Lu   // GPS L2C (only on u-blox F9 platform products)
#define UBLOX_CFG_SIGNAL_SBAS_ENA  UBX_CFG_L | 0x10310020Lu      // SBAS enable
#define UBLOX_CFG_SIGNAL_SBAS_L1CA_ENA  UBX_CFG_L | 0x10310005Lu // SBAS L1C/A
#define UBLOX_CFG_SIGNAL_GAL_ENA  UBX_CFG_L | 0x10310021Lu       // Galileo enable
#define UBLOX_CFG_SIGNAL_GAL_E1_ENA  UBX_CFG_L | 0x10310007Lu    // Galileo E1
#define UBLOX_CFG_SIGNAL_GAL_E5A_ENA  UBX_CFG_L | 0x10310009Lu   // Galileo E5a
#define UBLOX_CFG_SIGNAL_GAL_E5B_ENA  UBX_CFG_L | 0x1031000aLu   // Galileo E5b (only on u-blox F9 platform products)
#define UBLOX_CFG_SIGNAL_BDS_ENA  UBX_CFG_L | 0x10310022Lu       // BeiDou Enable
#define UBLOX_CFG_SIGNAL_BDS_B1_ENA  UBX_CFG_L | 0x1031000dLu    // BeiDou B1I
#define UBLOX_CFG_SIGNAL_BDS_B1C_ENA  UBX_CFG_L | 0x1031000fLu   // BeiDou B1C
#define UBLOX_CFG_SIGNAL_BDS_B2A_ENA  UBX_CFG_L | 0x10310028Lu   // BeiDou B2a
#define UBLOX_CFG_SIGNAL_BDS_B2_ENA  UBX_CFG_L | 0x1031000eLu    // BeiDou B2I (only on u-blox F9 platform products)
#define UBLOX_CFG_SIGNAL_QZSS_ENA  UBX_CFG_L | 0x10310024Lu      // QZSS enable
#define UBLOX_CFG_SIGNAL_QZSS_L1CA_ENA  UBX_CFG_L | 0x10310012Lu // QZSS L1C/A
#define UBLOX_CFG_SIGNAL_QZSS_L5_ENA  UBX_CFG_L | 0x10310017Lu   // QZSS L5
#define UBLOX_CFG_SIGNAL_QZSS_L1S_ENA  UBX_CFG_L | 0x10310014Lu  // QZSS L1S
#define UBLOX_CFG_SIGNAL_QZSS_L2C_ENA  UBX_CFG_L | 0x10310015Lu  // QZSS L2C (only on u-blox F9 platform products)
#define UBLOX_CFG_SIGNAL_GLO_ENA  UBX_CFG_L | 0x10310025Lu       // GLONASS enable
#define UBLOX_CFG_SIGNAL_GLO_L1_ENA  UBX_CFG_L | 0x10310018Lu    // GLONASS L1
#define UBLOX_CFG_SIGNAL_GLO_L2_ENA  UBX_CFG_L | 0x1031001aLu    // GLONASS L2 (only on u-blox F9 platform products)

// CFG-SPARTN: Configuration of the SPARTN interface
//--------------------------------------------------------
#define UBLOX_CFG_SPARTN_USE_SOURCE  UBX_CFG_E1 | 0x20a70001Lu

// CFG-SPI: Configuration of the SPI interface
//--------------------------------------------------------
#define UBLOX_CFG_SPI_MAXFF  UBX_CFG_U1 | 0x20640001Lu           // Number of bytes containing 0xFF to receive before switching oﬀ reception. Range: 0 (mechanism oﬀ) - 63
#define UBLOX_CFG_SPI_CPOLARITY  UBX_CFG_L | 0x10640002Lu       // Clock polarity select: 0: Active Hight Clock, SCLK idles low, 1: Active Low Clock, SCLK idles high
#define UBLOX_CFG_SPI_CPHASE  UBX_CFG_L | 0x10640003Lu          // Clock phase select: 0: Data captured on first edge of SCLK, 1: Data captured on second edge of SCLK
#define UBLOX_CFG_SPI_EXTENDEDTIMEOUT  UBX_CFG_L | 0x10640005Lu // Flag to disable timeouting the interface after 1.5s
#define UBLOX_CFG_SPI_ENABLED  UBX_CFG_L | 0x10640006Lu         // Flag to indicate if the SPI interface should be enabled

// CFG-SPIINPROT: Input protocol configuration of the SPI interface
//--------------------------------------------------------
#define UBLOX_CFG_SPIINPROT_UBX  UBX_CFG_L | 0x10790001Lu    // Flag to indicate if UBX should be an input protocol on SPI
#define UBLOX_CFG_SPIINPROT_NMEA  UBX_CFG_L | 0x10790002Lu   // Flag to indicate if NMEA should be an input protocol on SPI
#define UBLOX_CFG_SPIINPROT_RTCM3X  UBX_CFG_L | 0x10790004Lu // Flag to indicate if RTCM3X should be an input protocol on SPI
#define UBLOX_CFG_SPIINPROT_SPARTN  UBX_CFG_L | 0x10790005Lu // Flag to indicate if SPARTN should be an input protocol on SPI

// CFG-SPIOUTPROT: Output protocol configuration of the SPI interface
//--------------------------------------------------------
#define UBLOX_CFG_SPIOUTPROT_UBX  UBX_CFG_L | 0x107a0001Lu    // Flag to indicate if UBX should be an output protocol on SPI
#define UBLOX_CFG_SPIOUTPROT_NMEA  UBX_CFG_L | 0x107a0002Lu   // Flag to indicate if NMEA should be an output protocol on SPI
#define UBLOX_CFG_SPIOUTPROT_RTCM3X  UBX_CFG_L | 0x107a0004Lu // Flag to indicate if RTCM3X should be an output protocol on SPI

// CFG-TMODE: Time mode configuration
//--------------------------------------------------------
#define UBLOX_CFG_TMODE_MODE  UBX_CFG_E1 | 0x20030001Lu           // Receiver mode: 0-Disabled, 1-Survey-In, 2-Fixed
#define UBLOX_CFG_TMODE_POS_TYPE  UBX_CFG_E1 | 0x20030002Lu       // Determines whether the ARP position is given in ECEF or LAT/LON/HEIGHT?
#define UBLOX_CFG_TMODE_ECEF_X  UBX_CFG_I4 | 0x40030003Lu         // ECEF X coordinate of the ARP position.
#define UBLOX_CFG_TMODE_ECEF_Y  UBX_CFG_I4 | 0x40030004Lu         // ECEF Y coordinate of the ARP position.
#define UBLOX_CFG_TMODE_ECEF_Z  UBX_CFG_I4 | 0x40030005Lu         // ECEF Z coordinate of the ARP position.
#define UBLOX_CFG_TMODE_ECEF_X_HP  UBX_CFG_I1 | 0x20030006Lu      // High-precision ECEF X coordinate of the ARP position.
#define UBLOX_CFG_TMODE_ECEF_Y_HP  UBX_CFG_I1 | 0x20030007Lu      // High-precision ECEF Y coordinate of the ARP position.
#define UBLOX_CFG_TMODE_ECEF_Z_HP  UBX_CFG_I1 | 0x20030008Lu      // High-precision ECEF Z coordinate of the ARP position.
#define UBLOX_CFG_TMODE_LAT  UBX_CFG_I4 | 0x40030009Lu            // Latitude of the ARP position.
#define UBLOX_CFG_TMODE_LON  UBX_CFG_I4 | 0x4003000aLu            // Longitude of the ARP position.
#define UBLOX_CFG_TMODE_HEIGHT  UBX_CFG_I4 | 0x4003000bLu         // Height of the ARP position.
#define UBLOX_CFG_TMODE_LAT_HP  UBX_CFG_I1 | 0x2003000cLu         // High-precision latitude of the ARP position
#define UBLOX_CFG_TMODE_LON_HP  UBX_CFG_I1 | 0x2003000dLu         // High-precision longitude of the ARP position.
#define UBLOX_CFG_TMODE_HEIGHT_HP  UBX_CFG_I1 | 0x2003000eLu      // High-precision height of the ARP position.
#define UBLOX_CFG_TMODE_FIXED_POS_ACC  UBX_CFG_U4 | 0x4003000fLu  // Fixed position 3D accuracy
#define UBLOX_CFG_TMODE_SVIN_MIN_DUR  UBX_CFG_U4 | 0x40030010Lu   // Survey-in minimum duration
#define UBLOX_CFG_TMODE_SVIN_ACC_LIMIT  UBX_CFG_U4 | 0x40030011Lu // Survey-in position accuracy limit

// CFG-TP: Timepulse configuration
//--------------------------------------------------------
#define UBLOX_CFG_TP_PULSE_DEF  UBX_CFG_E1 | 0x20050023Lu        // Determines whether the time pulse is interpreted as frequency or period
#define UBLOX_CFG_TP_PULSE_LENGTH_DEF  UBX_CFG_E1 | 0x20050030Lu // Determines whether the time pulse length is interpreted as length[us] or pulse ratio[%]
#define UBLOX_CFG_TP_ANT_CABLEDELAY  UBX_CFG_I2 | 0x30050001Lu   // Antenna cable delay
#define UBLOX_CFG_TP_PERIOD_TP1  UBX_CFG_U4 | 0x40050002Lu       // Time pulse period (TP1)
#define UBLOX_CFG_TP_PERIOD_LOCK_TP1  UBX_CFG_U4 | 0x40050003Lu  // Time pulse period when locked to GNSS time (TP1)
#define UBLOX_CFG_TP_FREQ_TP1  UBX_CFG_U4 | 0x40050024Lu         // Time pulse frequency (TP1)
#define UBLOX_CFG_TP_FREQ_LOCK_TP1  UBX_CFG_U4 | 0x40050025Lu    // Time pulse frequency when locked to GNSS time (TP1)
#define UBLOX_CFG_TP_LEN_TP1  UBX_CFG_U4 | 0x40050004Lu          // Time pulse length (TP1)
#define UBLOX_CFG_TP_LEN_LOCK_TP1  UBX_CFG_U4 | 0x40050005Lu     // Time pulse length when locked to GNSS time (TP1)
#define UBLOX_CFG_TP_DUTY_TP1  UBX_CFG_R8 | 0x5005002aLu         // Time pulse duty cycle (TP1)
#define UBLOX_CFG_TP_DUTY_LOCK_TP1  UBX_CFG_R8 | 0x5005002bLu    // Time pulse duty cycle when locked to GNSS time (TP1)
#define UBLOX_CFG_TP_USER_DELAY_TP1  UBX_CFG_I4 | 0x40050006Lu   // User-configurable time pulse delay (TP1)
#define UBLOX_CFG_TP_TP1_ENA  UBX_CFG_L | 0x10050007Lu          // Enable the first timepulse
#define UBLOX_CFG_TP_SYNC_GNSS_TP1  UBX_CFG_L | 0x10050008Lu    // Sync time pulse to GNSS time or local clock (TP1)
#define UBLOX_CFG_TP_USE_LOCKED_TP1  UBX_CFG_L | 0x10050009Lu   // Use locked parameters when possible (TP1)
#define UBLOX_CFG_TP_ALIGN_TO_TOW_TP1  UBX_CFG_L | 0x1005000aLu // Align time pulse to top of second (TP1)
#define UBLOX_CFG_TP_POL_TP1  UBX_CFG_L | 0x1005000bLu          // Set time pulse polarity (TP1)
#define UBLOX_CFG_TP_TIMEGRID_TP1  UBX_CFG_E1 | 0x2005000cLu     // Time grid to use (TP1)
#define UBLOX_CFG_TP_PERIOD_TP2  UBX_CFG_U4 | 0x4005000dLu       // Time pulse period (TP2)
#define UBLOX_CFG_TP_PERIOD_LOCK_TP2  UBX_CFG_U4 | 0x4005000eLu  // Time pulse period when locked to GNSS time
#define UBLOX_CFG_TP_FREQ_TP2  UBX_CFG_U4 | 0x40050026Lu         // Time pulse frequency (TP2)
#define UBLOX_CFG_TP_FREQ_LOCK_TP2  UBX_CFG_U4 | 0x40050027Lu    // Time pulse frequency when locked to GNSS time
#define UBLOX_CFG_TP_LEN_TP2  UBX_CFG_U4 | 0x4005000fLu          // Time pulse length (TP2)
#define UBLOX_CFG_TP_LEN_LOCK_TP2  UBX_CFG_U4 | 0x40050010Lu     // Time pulse length when locked to GNSS time
#define UBLOX_CFG_TP_DUTY_TP2  UBX_CFG_R8 | 0x5005002cLu         // Time pulse duty cycle (TP2)
#define UBLOX_CFG_TP_DUTY_LOCK_TP2  UBX_CFG_R8 | 0x5005002dLu    // Time pulse duty cycle when locked to GNSS time
#define UBLOX_CFG_TP_USER_DELAY_TP2  UBX_CFG_I4 | 0x40050011Lu   // User-configurable time pulse delay (TP2)
#define UBLOX_CFG_TP_TP2_ENA  UBX_CFG_L | 0x10050012Lu          // Enable the second timepulse
#define UBLOX_CFG_TP_SYNC_GNSS_TP2  UBX_CFG_L | 0x10050013Lu    // Sync time pulse to GNSS time or local clock
#define UBLOX_CFG_TP_USE_LOCKED_TP2  UBX_CFG_L | 0x10050014Lu   // Use locked parameters when possible (TP2)
#define UBLOX_CFG_TP_ALIGN_TO_TOW_TP2  UBX_CFG_L | 0x10050015Lu // Align time pulse to top of second (TP2)
#define UBLOX_CFG_TP_POL_TP2  UBX_CFG_L | 0x10050016Lu          // Set time pulse polarity (TP2)
#define UBLOX_CFG_TP_TIMEGRID_TP2  UBX_CFG_E1 | 0x20050017Lu     // Time grid to use (TP2)
#define UBLOX_CFG_TP_DRSTR_TP1  UBX_CFG_E1 | 0x20050035Lu        // Set drive strength of TP1
#define UBLOX_CFG_TP_DRSTR_TP2  UBX_CFG_E1 | 0x20050036Lu        // Set drive strength of TP2

// CFG-TXREADY: TX ready configuration
//--------------------------------------------------------
#define UBLOX_CFG_TXREADY_ENABLED  UBX_CFG_L | 0x10a20001Lu   // Flag to indicate if TX ready pin mechanism should be enabled
#define UBLOX_CFG_TXREADY_POLARITY  UBX_CFG_L | 0x10a20002Lu  // The polarity of the TX ready pin: false:high- active, true:low-active
#define UBLOX_CFG_TXREADY_PIN  UBX_CFG_U1 | 0x20a20003Lu       // Pin number to use for the TX ready functionality
#define UBLOX_CFG_TXREADY_THRESHOLD  UBX_CFG_U2 | 0x30a20004Lu // Amount of data that should be ready on the interface before triggering the TX ready pin
#define UBLOX_CFG_TXREADY_INTERFACE  UBX_CFG_E1 | 0x20a20005Lu // Interface where the TX ready feature should be linked to

// CFG-UART1: Configuration of the UART1 interface
//--------------------------------------------------------
#define UBLOX_CFG_UART1_BAUDRATE  UBX_CFG_U4 | 0x40520001Lu // The baud rate that should be configured on the UART1
#define UBLOX_CFG_UART1_STOPBITS  UBX_CFG_E1 | 0x20520002Lu // Number of stopbits that should be used on UART1
#define UBLOX_CFG_UART1_DATABITS  UBX_CFG_E1 | 0x20520003Lu // Number of databits that should be used on UART1
#define UBLOX_CFG_UART1_PARITY  UBX_CFG_E1 | 0x20520004Lu   // Parity mode that should be used on UART1
#define UBLOX_CFG_UART1_ENABLED  UBX_CFG_L | 0x10520005Lu  // Flag to indicate if the UART1 should be enabled

// CFG-UART1INPROT: Input protocol configuration of the UART1 interface
//--------------------------------------------------------
#define UBLOX_CFG_UART1INPROT_UBX  UBX_CFG_L | 0x10730001Lu    // Flag to indicate if UBX should be an input protocol on UART1
#define UBLOX_CFG_UART1INPROT_NMEA  UBX_CFG_L | 0x10730002Lu   // Flag to indicate if NMEA should be an input protocol on UART1
#define UBLOX_CFG_UART1INPROT_RTCM3X  UBX_CFG_L | 0x10730004Lu // Flag to indicate if RTCM3X should be an input protocol on UART1
#define UBLOX_CFG_UART1INPROT_SPARTN  UBX_CFG_L | 0x10730005Lu // Flag to indicate if SPARTN should be an input protocol on UART1

// CFG-UART1OUTPROT: Output protocol configuration of the UART1 interface
//--------------------------------------------------------
#define UBLOX_CFG_UART1OUTPROT_UBX  UBX_CFG_L | 0x10740001Lu    // Flag to indicate if UBX should be an output protocol on UART1
#define UBLOX_CFG_UART1OUTPROT_NMEA  UBX_CFG_L | 0x10740002Lu   // Flag to indicate if NMEA should be an output protocol on UART1
#define UBLOX_CFG_UART1OUTPROT_RTCM3X  UBX_CFG_L | 0x10740004Lu // Flag to indicate if RTCM3X should be an output protocol on UART1

// CFG-UART2: Configuration of the UART2 interface
//--------------------------------------------------------
#define UBLOX_CFG_UART2_BAUDRATE  UBX_CFG_U4 | 0x40530001Lu // The baud rate that should be configured on the UART2
#define UBLOX_CFG_UART2_STOPBITS  UBX_CFG_E1 | 0x20530002Lu // Number of stopbits that should be used on UART2
#define UBLOX_CFG_UART2_DATABITS  UBX_CFG_E1 | 0x20530003Lu // Number of databits that should be used on UART2
#define UBLOX_CFG_UART2_PARITY  UBX_CFG_E1 | 0x20530004Lu   // Parity mode that should be used on UART2
#define UBLOX_CFG_UART2_ENABLED  UBX_CFG_L | 0x10530005Lu  // Flag to indicate if the UART2 should be enabled
#define UBLOX_CFG_UART2_REMAP  UBX_CFG_L | 0x10530006Lu    // UART2 Remapping

// CFG-UART2INPROT: Input protocol configuration of the UART2 interface
//--------------------------------------------------------
#define UBLOX_CFG_UART2INPROT_UBX  UBX_CFG_L | 0x10750001Lu    // Flag to indicate if UBX should be an input protocol on UART2
#define UBLOX_CFG_UART2INPROT_NMEA  UBX_CFG_L | 0x10750002Lu   // Flag to indicate if NMEA should be an input protocol on UART2
#define UBLOX_CFG_UART2INPROT_RTCM3X  UBX_CFG_L | 0x10750004Lu // Flag to indicate if RTCM3X should be an input protocol on UART2
#define UBLOX_CFG_UART2INPROT_SPARTN  UBX_CFG_L | 0x10750005Lu // Flag to indicate if SPARTN should be an input protocol on UART2

// CFG-UART2OUTPROT: Output protocol configuration of the UART2 interface
//--------------------------------------------------------
#define UBLOX_CFG_UART2OUTPROT_UBX  UBX_CFG_L | 0x10760001Lu    // Flag to indicate if UBX should be an output protocol on UART2
#define UBLOX_CFG_UART2OUTPROT_NMEA  UBX_CFG_L | 0x10760002Lu   // Flag to indicate if NMEA should be an output protocol on UART2
#define UBLOX_CFG_UART2OUTPROT_RTCM3X  UBX_CFG_L | 0x10760004Lu // Flag to indicate if RTCM3X should be an output protocol on UART2

// CFG-USB: Configuration of the USB interface
//--------------------------------------------------------
#define UBLOX_CFG_USB_ENABLED  UBX_CFG_L | 0x10650001Lu        // Flag to indicate if the USB interface should be enabled
#define UBLOX_CFG_USB_SELFPOW  UBX_CFG_L | 0x10650002Lu        // Self-powered device
#define UBLOX_CFG_USB_VENDOR_ID  UBX_CFG_U2 | 0x3065000aLu      // Vendor ID
#define UBLOX_CFG_USB_PRODUCT_ID  UBX_CFG_U2 | 0x3065000bLu     // Vendor ID
#define UBLOX_CFG_USB_POWER  UBX_CFG_U2 | 0x3065000cLu          // Power consumption
#define UBLOX_CFG_USB_VENDOR_STR0  UBX_CFG_X8 | 0x5065000dLu    // Vendor string characters 0-7
#define UBLOX_CFG_USB_VENDOR_STR1  UBX_CFG_X8 | 0x5065000eLu    // Vendor string characters 8-15
#define UBLOX_CFG_USB_VENDOR_STR2  UBX_CFG_X8 | 0x5065000fLu    // Vendor string characters 16-23
#define UBLOX_CFG_USB_VENDOR_STR3  UBX_CFG_X8 | 0x50650010Lu    // Vendor string characters 24-31
#define UBLOX_CFG_USB_PRODUCT_STR0  UBX_CFG_X8 | 0x50650011Lu   // Product string characters 0-7
#define UBLOX_CFG_USB_PRODUCT_STR1  UBX_CFG_X8 | 0x50650012Lu   // Product string characters 8-15
#define UBLOX_CFG_USB_PRODUCT_STR2  UBX_CFG_X8 | 0x50650013Lu   // Product string characters 16-23
#define UBLOX_CFG_USB_PRODUCT_STR3  UBX_CFG_X8 | 0x50650014Lu   // Product string characters 24-31
#define UBLOX_CFG_USB_SERIAL_NO_STR0  UBX_CFG_X8 | 0x50650015Lu // Serial number string characters 0-7
#define UBLOX_CFG_USB_SERIAL_NO_STR1  UBX_CFG_X8 | 0x50650016Lu // Serial number string characters 8-15
#define UBLOX_CFG_USB_SERIAL_NO_STR2  UBX_CFG_X8 | 0x50650017Lu // Serial number string characters 16-23
#define UBLOX_CFG_USB_SERIAL_NO_STR3  UBX_CFG_X8 | 0x50650018Lu // Serial number string characters 24-31

// CFG-USBINPROT: Input protocol configuration of the USB interface
//--------------------------------------------------------
#define UBLOX_CFG_USBINPROT_UBX  UBX_CFG_L | 0x10770001Lu    // Flag to indicate if UBX should be an input protocol on USB
#define UBLOX_CFG_USBINPROT_NMEA  UBX_CFG_L | 0x10770002Lu   // Flag to indicate if NMEA should be an input protocol on USB
#define UBLOX_CFG_USBINPROT_RTCM3X  UBX_CFG_L | 0x10770004Lu // Flag to indicate if RTCM3X should be an input protocol on USB
#define UBLOX_CFG_USBINPROT_SPARTN  UBX_CFG_L | 0x10770005Lu // Flag to indicate if SPARTN should be an input protocol on USB

// CFG-USBOUTPROT: Output protocol configuration of the USB interface
//--------------------------------------------------------
#define UBLOX_CFG_USBOUTPROT_UBX  UBX_CFG_L | 0x10780001Lu    // Flag to indicate if UBX should be an output protocol on USB
#define UBLOX_CFG_USBOUTPROT_NMEA  UBX_CFG_L | 0x10780002Lu   // Flag to indicate if NMEA should be an output protocol on USB
#define UBLOX_CFG_USBOUTPROT_RTCM3X  UBX_CFG_L | 0x10780004Lu // Flag to indicate if RTCM3X should be an output protocol on USB
