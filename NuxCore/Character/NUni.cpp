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


/*
 * Copyright 2001-2004 Unicode, Inc.
 *
 * Disclaimer
 *
 * This source code is provided as is by Unicode, Inc. No claims are
 * made as to fitness for any particular purpose. No warranties of any
 * kind are expressed or implied. The recipient agrees to determine
 * applicability of information provided. If this file has been
 * purchased on magnetic or optical media from Unicode, Inc., the
 * sole remedy for any claim will be exchange of defective media
 * within 90 days of receipt.
 *
 * Limitations on Rights to Redistribute This Code
 *
 * Unicode, Inc. hereby grants the right to freely use the information
 * supplied in this file in the creation of products supporting the
 * Unicode Standard, and to make copies of this file in any form
 * for internal or external distribution as long as this notice
 * remains attached.
 */

/* ---------------------------------------------------------------------

    Conversions between UTF32, UTF-16, and UTF-8. Source code file.
    Author: Mark E. Davis, 1994.
    Rev History: Rick McGowan, fixes & updates May 2001.
    Sept 2001: fixed const & error conditions per
        mods suggested by S. Parent & A. Lillich.
    June 2002: Tim Dodd added detection and handling of incomplete
        source sequences, enhanced error detection, added casts
        to eliminate compiler warnings.
    July 2003: slight mods to back out aggressive FFFE detection.
    Jan 2004: updated switches in from-UTF8 conversions.
    Oct 2004: updated to use UNI_MAX_LEGAL_UTF32 in UTF-32 conversions.
    May 2006: updated isLegalUTF8Sequence.

    See the header file "ConvertUTF.h" for complete documentation.

------------------------------------------------------------------------ */

#include "NuxCore.h"
//#include "NUni.h"

namespace nux
{

  static const int halfShift  = 10; /* used for shifting by 10 bits */

  static const unsigned int halfBase = 0x0010000UL;
  static const unsigned int halfMask = 0x3FFUL;

#define UNI_SUR_HIGH_START  (unsigned int)0xD800
#define UNI_SUR_HIGH_END    (unsigned int)0xDBFF
#define UNI_SUR_LOW_START   (unsigned int)0xDC00
#define UNI_SUR_LOW_END     (unsigned int)0xDFFF


  ConversionResult ConvertUTF32toUTF16 (const unsigned int **sourceStart, const unsigned int *sourceEnd, wchar_t **targetStart, wchar_t *targetEnd, ConversionFlags flags)
  {
    ConversionResult result = conversionOK;
    const unsigned int *source = *sourceStart;
    wchar_t *target = *targetStart;

    while (source < sourceEnd)
    {
      unsigned int ch;

      if (target >= targetEnd)
      {
        result = targetExhausted;
        break;
      }

      ch = *source++;

      if (ch <= UNI_MAX_BMP)   /* Target is a character <= 0xFFFF */
      {
        /* UTF-16 surrogate values are illegal in UTF-32; 0xffff or 0xfffe are both reserved values */
        if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)
        {
          if (flags == strictConversion)
          {
            --source; /* return to the illegal value itself */
            result = sourceIllegal;
            break;
          }
          else
          {
            *target++ = UNI_REPLACEMENT_CHAR;
          }
        }
        else
        {
          *target++ = (wchar_t) ch; /* normal case */
        }
      }
      else if (ch > UNI_MAX_LEGAL_UTF32)
      {
        if (flags == strictConversion)
        {
          result = sourceIllegal;
        }
        else
        {
          *target++ = UNI_REPLACEMENT_CHAR;
        }
      }
      else
      {
        /* target is a character in range 0xFFFF - 0x10FFFF. */
        if (target + 1 >= targetEnd)
        {
          --source; /* Back up source pointer! */
          result = targetExhausted;
          break;
        }

        ch -= halfBase;
        *target++ = (wchar_t) ( (ch >> halfShift) + UNI_SUR_HIGH_START);
        *target++ = (wchar_t) ( (ch & halfMask) + UNI_SUR_LOW_START);
      }
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
  }

  /* --------------------------------------------------------------------- */

  ConversionResult ConvertUTF16toUTF32 (const wchar_t **sourceStart, const wchar_t *sourceEnd, unsigned int **targetStart, unsigned int *targetEnd, ConversionFlags flags)
  {
    ConversionResult result = conversionOK;
    const wchar_t *source = *sourceStart;
    unsigned int *target = *targetStart;
    unsigned int ch, ch2;

    while (source < sourceEnd)
    {
      const wchar_t *oldSource = source; /*  In case we have to back up because of target overflow. */
      ch = *source++;

      /* If we have a surrogate pair, convert to UTF32 first. */
      if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END)
      {
        /* If the 16 bits following the high surrogate are in the source buffer... */
        if (source < sourceEnd)
        {
          ch2 = *source;

          /* If it's a low surrogate, convert to UTF32. */
          if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END)
          {
            ch = ( (ch - UNI_SUR_HIGH_START) << halfShift)
                 + (ch2 - UNI_SUR_LOW_START) + halfBase;
            ++source;
          }
          else if (flags == strictConversion)     /* it's an unpaired high surrogate */
          {
            --source; /* return to the illegal value itself */
            result = sourceIllegal;
            break;
          }
        }
        else     /* We don't have the 16 bits following the high surrogate. */
        {
          --source; /* return to the high surrogate */
          result = sourceExhausted;
          break;
        }
      }
      else if (flags == strictConversion)
      {
        /* UTF-16 surrogate values are illegal in UTF-32 */
        if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END)
        {
          --source; /* return to the illegal value itself */
          result = sourceIllegal;
          break;
        }
      }

      if (target >= targetEnd)
      {
        source = oldSource; /* Back up source pointer! */
        result = targetExhausted;
        break;
      }

      *target++ = ch;
    }

    *sourceStart = source;
    *targetStart = target;
#ifdef CVTUTF_DEBUG

    if (result == sourceIllegal)
    {
      fprintf (stderr, "ConvertUTF16toUTF32 illegal seq 0x%04x,%04x\n", ch, ch2);
      fflush (stderr);
    }

#endif
    return result;
  }

  /* --------------------------------------------------------------------- */

  /*
  * Index into the table below with the first byte of a UTF-8 sequence to
  * get the number of trailing bytes that are supposed to follow it.
  * Note that *legal* UTF-8 values can't have 4 or 5-bytes. The table is
  * left as-is for anyone who may want to do such conversion, which was
  * allowed in earlier algorithms.
  */
  static const char trailingBytesForUTF8[256] =
  {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
  };

  /*
  * Magic values subtracted from a buffer value during UTF8 conversion.
  * This table contains as many values as there might be trailing bytes
  * in a UTF-8 sequence.
  */
  static const unsigned int offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL,
      0x03C82080UL, 0xFA082080UL, 0x82082080UL
                                            };

  /*
  * Once the bits are split out into bytes of UTF-8, this is a mask OR-ed
  * into the first byte, depending on how many bytes follow.  There are
  * as many entries in this table as there are UTF-8 sequence types.
  * (I.e., one byte sequence, two byte... etc.). Remember that sequencs
  * for *legal* UTF-8 will be 4 or fewer bytes total.
  */
  static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

  /* --------------------------------------------------------------------- */

  /* The interface converts a whole buffer to avoid function-call overhead.
  * Constants have been gathered. Loops & conditionals have been removed as
  * much as possible for efficiency, in favor of drop-through switches.
  * (See "Note A" at the bottom of the file for equivalent code.)
  * If your compiler supports it, the "isLegalUTF8" call can be turned
  * into an inline function.
  */

  /* --------------------------------------------------------------------- */

  ConversionResult ConvertUTF16toUTF8 (const wchar_t **sourceStart, const wchar_t *sourceEnd, unsigned char **targetStart, unsigned char *targetEnd, ConversionFlags flags)
  {
    ConversionResult result = conversionOK;
    const wchar_t *source = *sourceStart;
    unsigned char *target = *targetStart;

    while (source < sourceEnd)
    {
      unsigned int ch;
      unsigned short bytesToWrite = 0;
      const unsigned int byteMask = 0xBF;
      const unsigned int byteMark = 0x80;
      const wchar_t *oldSource = source; /* In case we have to back up because of target overflow. */
      ch = *source++;

      /* If we have a surrogate pair, convert to UTF32 first. */
      if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END)
      {
        /* If the 16 bits following the high surrogate are in the source buffer... */
        if (source < sourceEnd)
        {
          unsigned int ch2 = *source;

          /* If it's a low surrogate, convert to UTF32. */
          if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END)
          {
            ch = ( (ch - UNI_SUR_HIGH_START) << halfShift)
                 + (ch2 - UNI_SUR_LOW_START) + halfBase;
            ++source;
          }
          else if (flags == strictConversion)     /* it's an unpaired high surrogate */
          {
            --source; /* return to the illegal value itself */
            result = sourceIllegal;
            break;
          }
        }
        else     /* We don't have the 16 bits following the high surrogate. */
        {
          --source; /* return to the high surrogate */
          result = sourceExhausted;
          break;
        }
      }
      else if (flags == strictConversion)
      {
        /* UTF-16 surrogate values are illegal in UTF-32 */
        if (ch >= UNI_SUR_LOW_START && ch <= UNI_SUR_LOW_END)
        {
          --source; /* return to the illegal value itself */
          result = sourceIllegal;
          break;
        }
      }

      /* Figure out how many bytes the result will require */
      if (ch < (unsigned int) 0x80)
      {
        bytesToWrite = 1;
      }
      else if (ch < (unsigned int) 0x800)
      {
        bytesToWrite = 2;
      }
      else if (ch < (unsigned int) 0x10000)
      {
        bytesToWrite = 3;
      }
      else if (ch < (unsigned int) 0x110000)
      {
        bytesToWrite = 4;
      }
      else
      {
        bytesToWrite = 3;
        ch = UNI_REPLACEMENT_CHAR;
      }

      target += bytesToWrite;

      if (target > targetEnd)
      {
        source = oldSource; /* Back up source pointer! */
        target -= bytesToWrite;
        result = targetExhausted;
        break;
      }

      switch (bytesToWrite)   /* note: everything falls through. */
      {
        case 4:
          *--target = (unsigned char) ( (ch | byteMark) & byteMask);
          ch >>= 6;
          [[gnu::fallthrough]];
        case 3:
          *--target = (unsigned char) ( (ch | byteMark) & byteMask);
          ch >>= 6;
          [[gnu::fallthrough]];
        case 2:
          *--target = (unsigned char) ( (ch | byteMark) & byteMask);
          ch >>= 6;
          [[gnu::fallthrough]];
        case 1:
          *--target =  (unsigned char) (ch | firstByteMark[bytesToWrite]);
      }

      target += bytesToWrite;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
  }

  /* --------------------------------------------------------------------- */

  /*
  * Utility routine to tell whether a sequence of bytes is legal UTF-8.
  * This must be called with the length pre-determined by the first byte.
  * If not calling this from ConvertUTF8to*, then the length can be set by:
  *  length = trailingBytesForUTF8[*source]+1;
  * and the sequence is illegal right away if there aren't that many bytes
  * available.
  * If presented with a length > 4, this returns false.  The Unicode
  * definition of UTF-8 goes up to 4-byte sequences.
  */

  static bool isLegalUTF8 (const unsigned char *source, int length)
  {
    unsigned char a;
    const unsigned char *srcptr = source + length;

    switch (length)
    {
      default:
        return false;
        /* Everything else falls through when "true"... */
      case 4:
        if ( (a = (*--srcptr) ) < 0x80 || a > 0xBF)
          return false;

        [[gnu::fallthrough]];
      case 3:
        if ( (a = (*--srcptr) ) < 0x80 || a > 0xBF)
          return false;

        [[gnu::fallthrough]];
      case 2:

        if ( (a = (*--srcptr) ) > 0xBF)
          return false;

        switch (*source)
        {
            /* no fall-through in this inner switch */
          case 0xE0:

            if (a < 0xA0) return false;

            break;
          case 0xED:

            if ( (a < 0x80) || (a > 0x9F) ) return false;

            break;
          case 0xF0:

            if (a < 0x90) return false;

            break;
          case 0xF4:

            if (a > 0x8F) return false;

            break;
          default:

            if (a < 0x80) return false;
        }

        [[gnu::fallthrough]];

      case 1:
        if (*source >= 0x80 && *source < 0xC2)
          return false;

      [[gnu::fallthrough]];
    }

    if (*source > 0xF4) return false;

    return true;
  }

  /* --------------------------------------------------------------------- */

  /*
  * Exported function to return whether a UTF-8 sequence is legal or not.
  * This is not used here; it's just exported.
  */

  bool isLegalUTF8Sequence (const unsigned char *source, const unsigned char *sourceEnd)
  {
    int length;

    if (source == sourceEnd)
    {
      return true;
    }

    while (true)
    {
      length = trailingBytesForUTF8[*source] + 1;

      if (source + length > sourceEnd)
      {
        return false;
      }

      if (!isLegalUTF8 (source, length) )
      {
        return false;
      }

      source += length;

      if (source >= sourceEnd)
      {
        return true;
      }
    }
    return true;
  }

  /**
  * This is a variation of isLegalUTF8Sequence() that behaves like g_utf8_validate().
  * In addition to knowing if the sequence is legal, it also tells you the last good character.
  */
  bool
  tr_utf8_validate ( const char *str, int max_len, const char **end )
  {
    const unsigned char *source = (const unsigned char *) str;
    const unsigned char *sourceEnd;

    if ( max_len == 0 )
      return true;

    if ( str == NULL )
      return false;

    sourceEnd = source + ( (max_len < 0) ? strlen (str) : (size_t) max_len);

    if ( source == sourceEnd )
    {
      if ( end != NULL )
        *end = (const char *) source;

      return true;
    }

    for ( ;; )
    {
      const int length = trailingBytesForUTF8[*source] + 1;

      if (source + length > sourceEnd)
      {
        if ( end != NULL )
          *end = (const char *) source;

        return false;
      }

      if (!isLegalUTF8 (source, length) )
      {
        if ( end != NULL )
          *end = (const char *) source;

        return false;
      }

      source += length;

      if (source >= sourceEnd)
      {
        if ( end != NULL )
          *end = (const char *) source;

        return true;
      }
    }

    return true;
  }


  /* --------------------------------------------------------------------- */

  ConversionResult ConvertUTF8toUTF16 (const unsigned char **sourceStart, const unsigned char *sourceEnd, wchar_t **targetStart, wchar_t *targetEnd, ConversionFlags flags)
  {
    ConversionResult result = conversionOK;
    const unsigned char *source = *sourceStart;
    wchar_t *target = *targetStart;

    while (source < sourceEnd)
    {
      unsigned int ch = 0;
      unsigned short extraBytesToRead = trailingBytesForUTF8[*source];

      if (source + extraBytesToRead >= sourceEnd)
      {
        result = sourceExhausted;
        break;
      }

      /* Do this check whether lenient or strict */
      if (! isLegalUTF8 (source, extraBytesToRead + 1) )
      {
        result = sourceIllegal;
        break;
      }

      /*
       * The cases all fall through. See "Note A" below.
       */
      switch (extraBytesToRead)
      {
        case 5:
          ch += *source++;
          ch <<= 6; /* remember, illegal UTF-8 */
          [[gnu::fallthrough]];
        case 4:
          ch += *source++;
          ch <<= 6; /* remember, illegal UTF-8 */
          [[gnu::fallthrough]];
        case 3:
          ch += *source++;
          ch <<= 6;
          [[gnu::fallthrough]];
        case 2:
          ch += *source++;
          ch <<= 6;
          [[gnu::fallthrough]];
        case 1:
          ch += *source++;
          ch <<= 6;
          [[gnu::fallthrough]];
        case 0:
          ch += *source++;
      }

      ch -= offsetsFromUTF8[extraBytesToRead];

      if (target >= targetEnd)
      {
        source -= (extraBytesToRead + 1); /* Back up source pointer! */
        result = targetExhausted;
        break;
      }

      if (ch <= UNI_MAX_BMP)   /* Target is a character <= 0xFFFF */
      {
        /* UTF-16 surrogate values are illegal in UTF-32 */
        if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)
        {
          if (flags == strictConversion)
          {
            source -= (extraBytesToRead + 1); /* return to the illegal value itself */
            result = sourceIllegal;
            break;
          }
          else
          {
            *target++ = UNI_REPLACEMENT_CHAR;
          }
        }
        else
        {
          *target++ = (wchar_t) ch; /* normal case */
        }
      }
      else if (ch > UNI_MAX_UTF16)
      {
        if (flags == strictConversion)
        {
          result = sourceIllegal;
          source -= (extraBytesToRead + 1); /* return to the start */
          break; /* Bail out; shouldn't continue */
        }
        else
        {
          *target++ = UNI_REPLACEMENT_CHAR;
        }
      }
      else
      {
        /* target is a character in range 0xFFFF - 0x10FFFF. */
        if (target + 1 >= targetEnd)
        {
          source -= (extraBytesToRead + 1); /* Back up source pointer! */
          result = targetExhausted;
          break;
        }

        ch -= halfBase;
        *target++ = (wchar_t) ( (ch >> halfShift) + UNI_SUR_HIGH_START);
        *target++ = (wchar_t) ( (ch & halfMask) + UNI_SUR_LOW_START);
      }
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
  }

  /* --------------------------------------------------------------------- */

  ConversionResult ConvertUTF32toUTF8 (
    const unsigned int **sourceStart, const unsigned int *sourceEnd,
    unsigned char **targetStart, unsigned char *targetEnd, ConversionFlags flags)
  {
    ConversionResult result = conversionOK;
    const unsigned int *source = *sourceStart;
    unsigned char *target = *targetStart;

    while (source < sourceEnd)
    {
      unsigned int ch;
      unsigned short bytesToWrite = 0;
      const unsigned int byteMask = 0xBF;
      const unsigned int byteMark = 0x80;
      ch = *source++;

      if (flags == strictConversion )
      {
        /* UTF-16 surrogate values are illegal in UTF-32 */
        if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)
        {
          --source; /* return to the illegal value itself */
          result = sourceIllegal;
          break;
        }
      }

      /*
       * Figure out how many bytes the result will require. Turn any
       * illegally large UTF32 things (> Plane 17) into replacement chars.
       */
      if (ch < (unsigned int) 0x80)
      {
        bytesToWrite = 1;
      }
      else if (ch < (unsigned int) 0x800)
      {
        bytesToWrite = 2;
      }
      else if (ch < (unsigned int) 0x10000)
      {
        bytesToWrite = 3;
      }
      else if (ch <= UNI_MAX_LEGAL_UTF32)
      {
        bytesToWrite = 4;
      }
      else
      {
        bytesToWrite = 3;
        ch = UNI_REPLACEMENT_CHAR;
        result = sourceIllegal;
      }

      target += bytesToWrite;

      if (target > targetEnd)
      {
        --source; /* Back up source pointer! */
        target -= bytesToWrite;
        result = targetExhausted;
        break;
      }

      switch (bytesToWrite)   /* note: everything falls through. */
      {
        case 4:
          *--target = (unsigned char) ( (ch | byteMark) & byteMask);
          ch >>= 6;
          [[gnu::fallthrough]];
        case 3:
          *--target = (unsigned char) ( (ch | byteMark) & byteMask);
          ch >>= 6;
          [[gnu::fallthrough]];
        case 2:
          *--target = (unsigned char) ( (ch | byteMark) & byteMask);
          ch >>= 6;
          [[gnu::fallthrough]];
        case 1:
          *--target = (unsigned char) (ch | firstByteMark[bytesToWrite]);
      }

      target += bytesToWrite;
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
  }

  /* --------------------------------------------------------------------- */

  ConversionResult ConvertUTF8toUTF32 (
    const unsigned char **sourceStart, const unsigned char *sourceEnd,
    unsigned int **targetStart, unsigned int *targetEnd, ConversionFlags flags)
  {
    ConversionResult result = conversionOK;
    const unsigned char *source = *sourceStart;
    unsigned int *target = *targetStart;

    while (source < sourceEnd)
    {
      unsigned int ch = 0;
      unsigned short extraBytesToRead = trailingBytesForUTF8[*source];

      if (source + extraBytesToRead >= sourceEnd)
      {
        result = sourceExhausted;
        break;
      }

      /* Do this check whether lenient or strict */
      if (! isLegalUTF8 (source, extraBytesToRead + 1) )
      {
        result = sourceIllegal;
        break;
      }

      /*
       * The cases all fall through. See "Note A" below.
       */
      switch (extraBytesToRead)
      {
        case 5:
          ch += *source++;
          ch <<= 6;
          [[gnu::fallthrough]];
        case 4:
          ch += *source++;
          ch <<= 6;
          [[gnu::fallthrough]];
        case 3:
          ch += *source++;
          ch <<= 6;
          [[gnu::fallthrough]];
        case 2:
          ch += *source++;
          ch <<= 6;
          [[gnu::fallthrough]];
        case 1:
          ch += *source++;
          ch <<= 6;
          [[gnu::fallthrough]];
        case 0:
          ch += *source++;
      }

      ch -= offsetsFromUTF8[extraBytesToRead];

      if (target >= targetEnd)
      {
        source -= (extraBytesToRead + 1); /* Back up the source pointer! */
        result = targetExhausted;
        break;
      }

      if (ch <= UNI_MAX_LEGAL_UTF32)
      {
        /*
         * UTF-16 surrogate values are illegal in UTF-32, and anything
         * over Plane 17 (> 0x10FFFF) is illegal.
         */
        if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_LOW_END)
        {
          if (flags == strictConversion)
          {
            source -= (extraBytesToRead + 1); /* return to the illegal value itself */
            result = sourceIllegal;
            break;
          }
          else
          {
            *target++ = UNI_REPLACEMENT_CHAR;
          }
        }
        else
        {
          *target++ = ch;
        }
      }
      else     /* i.e., ch > UNI_MAX_LEGAL_UTF32 */
      {
        result = sourceIllegal;
        *target++ = UNI_REPLACEMENT_CHAR;
      }
    }

    *sourceStart = source;
    *targetStart = target;
    return result;
  }

  /* ---------------------------------------------------------------------

  Note A.
  The fall-through switches in UTF-8 reading code save a
  temp variable, some decrements & conditionals.  The switches
  are equivalent to the following loop:
      {
          int tmpBytesToRead = extraBytesToRead+1;
          do {
              ch += *source++;
              --tmpBytesToRead;
              if (tmpBytesToRead) ch <<= 6;
              [[gnu::fallthrough]];
          } while (tmpBytesToRead > 0);
      }
  In UTF-8 writing code, the switches on "bytesToWrite" are
  similarly unrolled loops.

  --------------------------------------------------------------------- */

}
