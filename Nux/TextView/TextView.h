/*
 * Copyright 2010 Inalogic Inc.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 3, as
 * published by the  Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranties of
 * MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the applicable version of the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of both the GNU Lesser General Public
 * License version 3 along with this program.  If not, see
 * <http://www.gnu.org/licenses/>
 *
 * Authored by: Jay Taoko <jaytaoko@inalogic.com>
 *
 */


//
//	TextView public interface - include this file
//	when you want a TextView edit control
//
#ifndef TEXTVIEW_INCLUDED
#define TEXTVIEW_INCLUDED

namespace nux
{

//
//	TextView API declared here
//
  COLORREF RealizeColour (COLORREF col);


//
// currently supported Neatpad Codepages
//
#define NCP_ASCII		0
#define NCP_UTF8		1
#define NCP_UTF16		2
#define NCP_UTF16BE		3
#define NCP_UTF32		4
#define NCP_UTF32BE		5

//
//	TextView Window Messages defined here
//
#define TXM_BASE				 (WM_USER)
#define TXM_OPENFILE			 (TXM_BASE + 0)
#define TXM_CLEAR				 (TXM_BASE + 1)
#define TXM_SETLINESPACING		 (TXM_BASE + 2)
#define TXM_ADDFONT				 (TXM_BASE + 3)
#define TXM_SETCOLOR			 (TXM_BASE + 4)
#define TXM_GETCOLOR			 (TXM_BASE + 5)
#define TXM_SETSTYLE			 (TXM_BASE + 6)
#define TXM_GETSTYLE			 (TXM_BASE + 7)
#define TXM_SETCARETWIDTH		 (TXM_BASE + 8)
#define TXM_SETIMAGELIST		 (TXM_BASE + 9)
#define TXM_SETLONGLINE			 (TXM_BASE + 10)
#define TXM_SETLINEIMAGE		 (TXM_BASE + 11)
#define TXM_GETFORMAT			 (TXM_BASE + 12)

//
//	TextView Window Styles defined here
//	(set using TXM_SETSTYLE)
//
#define TXS_SELMARGIN			1
#define TXS_LINENUMBERS			2
#define TXS_TREECTRL			4
#define TXS_LONGLINES			8
#define TXS_HIGHLIGHTCURLINE	16

//
//	End-of-line mode
//
#define TXL_LF				1		// line-feed
#define TXL_CR				2		// carriage-return
#define TXL_CRLF			4		// carriage-return, line-feed (default)
#define TXL_ALL				7		// allow all forms regardless


//
//	TextView Message Macros defined here
//
#define TextView_OpenFile(hwndTV, szFile)	\
	SendMessage((hwndTV), TXM_OPENFILE, 0, (LPARAM)(szFile))

#define TextView_Clear(hwndTV)	\
	SendMessage((hwndTV), TXM_CLEAR, 0, 0)

#define TextView_SetLineSpacing(hwndTV, nAbove, nBelow) \
	SendMessage((hwndTV), TXM_SETLINESPACING, (int)(nAbove), (int)(nBelow))

#define TextView_AddFont(hwndTV, hFont) \
	SendMessage((hwndTV), TXM_ADDFONT, (WPARAM)(HFONT)(hFont), 0)

#define TextView_SetColor(hwndTV, nIdx, rgbColor) \
	SendMessage((hwndTV), TXM_SETCOLOR, (WPARAM)(nIdx), (LPARAM)(rgbColor))

#define TextView_SetStyle(hwndTV, uMask, uStyles) \
	SendMessage((hwndTV), TXM_SETSTYLE, (WPARAM)(uMask), (LPARAM)(uStyles))

#define TextView_SetStyleBool(hwndTV, uStyle, fBoolean) \
	SendMessage((hwndTV), TXM_SETSTYLE, (WPARAM)(uStyle), (LPARAM)(fBoolean ? uStyle : 0))

#define TextView_SetCaretWidth(hwndTV, nWidth) \
	SendMessage((hwndTV), TXM_SETCARETWIDTH, (WPARAM)(nWidth), 0)

#define TextView_SetImageList(hwndTV, hImgList) \
	SendMessage((hwndTV), TXM_SETIMAGELIST, (WPARAM)(HIMAGELIST)(hImgList), 0)

#define TextView_SetLongLine(hwndTV, nLength) \
	SendMessage((hwndTV), TXM_SETLONGLINE, (WPARAM)(0), (LPARAM)(nLength))

#define TextView_SetLineImage(hwndTV, nLineNo, nImageIdx) \
	SendMessage((hwndTV), TXM_SETLINEIMAGE, (WPARAM)(t_u32)(nLineNo), (LPARAM)(t_u32)nImageIdx)

#define TextView_GetFormat(hwndTV) \
	SendMessage((hwndTV), TXM_GETFORMAT, 0, 0)


//
//	TextView Macros defined here
//
#define TEXTVIEW_CLASS _T("TextView32")

//
//	TextView colours
//
#define TXC_FOREGROUND			0			// normal foreground colour
#define TXC_BACKGROUND			1			// normal background colour
#define TXC_HIGHLIGHTTEXT		2			// normal text highlight colour
#define TXC_HIGHLIGHT			3			// normal background highlight colour
#define TXC_HIGHLIGHTTEXT2		4			// inactive text highlight colour
#define TXC_HIGHLIGHT2			5			// inactive background highlight colour
#define TXC_SELMARGIN1			6			// selection margin colour#1
#define TXC_SELMARGIN2			7			// selection margin colour#2
#define TXC_LINENUMBERTEXT		8			// line number text
#define TXC_LINENUMBER			9			// line number background
#define TXC_LONGLINETEXT		10			// long-line text
#define TXC_LONGLINE			11			// long-line background
#define TXC_CURRENTLINETEXT		12			// active line text
#define TXC_CURRENTLINE			13			// active line background


#define TXC_MAX_COLOURS			14			// keep this updated!

#define SYSCOL(COLOR_IDX)					   ( 0x80000000 |                     COLOR_IDX  )
#define MIXED_SYSCOL(COLOR_IDX1, COLOR_IDX2)   ( 0xC0000000 | (COLOR_IDX2 << 8) | COLOR_IDX1 )
#define MIXED_SYSCOL2(COLOR_IDX1, COLOR_IDX2)  ( 0xE0000000 | (COLOR_IDX2 << 8) | COLOR_IDX1 )

#define SYSCOLIDX(COLREF)   ( 0x00FFFFFF & COLREF )

#define REALIZE_SYSCOL(col) (RealizeColour(col))

}

#endif // TEXTVIEW_INCLUDED
