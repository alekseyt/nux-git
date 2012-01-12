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
 * License along with this program. If not, see <http://www.gnu.org/licenses/>
 *
 * Authored by: Jay Taoko <jaytaoko@inalogic.com>
 *
 */


#ifndef CRC32_H
#define CRC32_H

namespace nux
{
// http://www.networkdls.com/Software.Asp?Review=22

// This is the official polynomial used by CRC-32 in PKZip, WinZip and Ethernet.

// There are multiple 16-bit CRC polynomials in common use, but this is *the* standard CRC-32 polynomial,
// first popularized by Ethernet. It is used by PKZip, WinZip and Ethernet.
// x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x^1+x^0

// #define CRC32_POLYNOMIAL_LE 0xedb88320  // little Endian
#define CRC32_POLYNOMIAL 0x04c11db7     // big Endian

// How many bits at a time to use.  Requires a table of 4<<CRC_xx_BITS bytes. We use CRC_8_BITS
// CRC_xx_BITS must be a power of 2 between 1 and 8. We use CRC_8_BITS.
#define CRC32BUFSZ 1024 // 4 << 8 = 1024

  class CRC32
  {
  public:
    CRC32();
    unsigned int FileCRC (const char *sFileName);
    unsigned int FullCRC (const char *sData, unsigned int ulLength);
    void PartialCRC (unsigned int *ulInCRC, const char *sData, unsigned int ulLength);

  private:
    void Initialize (void);
    unsigned int Reflect (unsigned int ulReflect, char cChar);
    unsigned int CRCTable[256]; // CRC lookup table array.
  };

}
#endif // CRC32_H
