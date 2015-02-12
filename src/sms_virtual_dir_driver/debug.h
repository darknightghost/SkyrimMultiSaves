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

//Break points

#ifdef _DEBUG

	#ifdef AMD64

		void	break_point();

		#define	RAW_BREAKPOINT	break_point()

	#endif // AMD64

	#ifndef AMD64

		#define	RAW_BREAKPOINT	_asm int 3

	#endif // !AMD64

	//Make sure the operating system will not show blue screen with death without kernel debuggers
	#define	BREAKPOINT				if(!KdRefreshDebuggerNotPresent()) RAW_BREAKPOINT
	#define	BREAKIFNOT(condition)	if(!(condition)) if(!KdRefreshDebuggerNotPresent())	RAW_BREAKPOINT

#endif //_DEBUG
#ifndef _DEBUG

	#define	BREAKPOINT
	#define	BREAKIFNOT(condition)
	#define	RAW_BREAKPOINT

#endif // !_DEBUG

#endif // !DEBUG_H_INCLUDE
