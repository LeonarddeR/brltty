/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 1995-2020 by The BRLTTY Developers.
 *
 * BRLTTY comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU Lesser General Public License, as published by the Free Software
 * Foundation; either version 2.1 of the License, or (at your option) any
 * later version. Please see the file LICENSE-LGPL for details.
 *
 * Web Page: http://brltty.app/
 *
 * This software is maintained by Dave Mielke <dave@mielke.cc>.
 */

#ifndef BRLTTY_INCLUDED_CN_BRLDEFS
#define BRLTTY_INCLUDED_CN_BRLDEFS

typedef enum {
  CN_CMD_COLUMN_COUNT     = 0X00,
  CN_CMD_ROW_COUNT        = 0X01,
  CN_CMD_PROTOCOL_VERSION = 0X03,
  CN_CMD_SEND_ROW         = 0X06,
  CN_CMD_RESET_CELLS      = 0X07,
  CN_CMD_LOWER_ROWS       = 0X09,
  CN_CMD_KEYS_STATE       = 0X0A,
  CN_CMD_FIRMWARE_VERSION = 0X0B,
  CN_CMD_DEVICE_STATE     = 0X0D,
  CN_CMD_SET_ROW          = 0X0E,
} CN_Command;

typedef enum {
  CN_KEY_Help    =  0,
  CN_KEY_Line1   =  1,
  CN_KEY_Line2   =  2,
  CN_KEY_Line3   =  3,
  CN_KEY_Line4   =  4,
  CN_KEY_Line5   =  5,
  CN_KEY_Line6   =  6,
  CN_KEY_Line7   =  7,
  CN_KEY_Line8   =  8,
  CN_KEY_Line9   =  9,
  CN_KEY_Refresh = 10,
  CN_KEY_Back    = 11,
  CN_KEY_Menu    = 12,
  CN_KEY_Forward = 13,
} CN_NavigationKey;

typedef enum {
  CN_GRP_NavigationKeys = 0,
} CN_KeyGroup;

typedef enum {
  CN_DEV_MOTORS_ACTIVE = 0X01,
} CN_DeviceStates;

#define CN_PACKET_CHECKSUM_ALGORITHM "CRC-16/ISO-HDLC"
#define CN_PACKET_FRAMING_BYTE 0X7E
#define CN_PACKET_ESCAPE_BYTE 0X7D
#define CN_PACKET_ESCAPE_BIT 0X20

typedef uint16_t CN_PacketInteger;

static inline CN_PacketInteger
CN_getResponseInteger (const unsigned char *response, unsigned int offset) {
  return response[offset] | (response[offset+1] << 8);
}

static inline CN_PacketInteger
CN_getResponseResult (const unsigned char *response) {
  return CN_getResponseInteger(response, 1);
}

#endif /* BRLTTY_INCLUDED_CN_BRLDEFS */ 