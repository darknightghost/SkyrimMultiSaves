/*
Copyright 2015,暗夜幽灵 <darknightghost.cn@gmail.com>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DEBUG_H_INCLUDE
#define	DEBUG_H_INCLUDE

#include "common.h"

#ifndef _AMD64

	#define	RAW_BREAKPOINT	_asm int 3

#endif // !_AMD64

#ifdef _AMD64
	void		break_point();

	#define		RAW_BREAKPOINT	break_point()

#endif // _AMD64

#ifdef _DEBUG

bool	is_debugger_present();

#define	BREAKPOINT		if(is_debugger_present())	RAW_BREAKPOINT;
#define	PRINTF(fmt,...)	printf(fmt,##__VA_ARGS__)
#define	PRINT_ERR(str)		{\
		MessageBox(NULL, (str), L"Error", MB_OK | MB_ICONERROR);\
		RAW_BREAKPOINT;\
	}
#define	PAUSE	{\
		printf("Press any key to continue...\n");\
		fflush(stdin);\
		getc(stdin);\
	}

#define	PRTNT_ERRCODE	PRINTF("Error code:%.8X.\n",GetLastError())
#endif // _DEBUG

#ifndef _DEBUG
	#ifdef _AMD64
		void		break_point();
	#endif	// _AMD64
	#define	BREAKPOINT
	#define	PRINTF(fmt,...)
	#define	PAUSE
	#define	PRTNT_ERRCODE
	#define	PRINT_ERR(str)
#endif // !_DEBUG


#endif // !DEBUG_H_INCLUDE
