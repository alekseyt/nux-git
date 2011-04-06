/*
 * Copyright 2010 Inalogic® Inc.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License, as
 * published by the  Free Software Foundation; either version 2.1 or 3.0
 * of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License along with this program.  If not, see <http://www.gnu.org/licenses/>
 *
 * Authored by: Jay Taoko <jay.taoko_AT_gmail_DOT_com>
 *
 */


//
//	Right-arrow cursor raw bytes
//

#pragma once

/* Generated by HexEdit */
static BYTE XORMask[128] =
{
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf3, 0xff, 0xff, 0xff, 0xe3, 0xff, 0xff, 0xff, 0xc3, 0xff,
  0xff, 0xff, 0x83, 0xff, 0xff, 0xff, 0x03, 0xff, 0xff, 0xfe, 0x03, 0xff, 0xff, 0xfc, 0x03, 0xff,
  0xff, 0xf8, 0x03, 0xff, 0xff, 0xf0, 0x03, 0xff, 0xff, 0xe0, 0x03, 0xff, 0xff, 0xc0, 0x03, 0xff,
  0xff, 0xfc, 0x03, 0xff, 0xff, 0xfc, 0x03, 0xff, 0xff, 0xf8, 0x63, 0xff, 0xff, 0xf8, 0x73, 0xff,
  0xff, 0xf0, 0xfb, 0xff, 0xff, 0xf0, 0xff, 0xff, 0xff, 0xe1, 0xff, 0xff, 0xff, 0xe1, 0xff, 0xff,
  0xff, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

/* Generated by HexEdit */
static BYTE ANDMask[128] =
{
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x18, 0x00,
  0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x78, 0x00, 0x00, 0x00, 0xf8, 0x00, 0x00, 0x01, 0xf8, 0x00,
  0x00, 0x03, 0xf8, 0x00, 0x00, 0x07, 0xf8, 0x00, 0x00, 0x0f, 0xf8, 0x00, 0x00, 0x01, 0xf8, 0x00,
  0x00, 0x01, 0xb8, 0x00, 0x00, 0x01, 0x98, 0x00, 0x00, 0x03, 0x08, 0x00, 0x00, 0x03, 0x00, 0x00,
  0x00, 0x06, 0x00, 0x00, 0x00, 0x06, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
