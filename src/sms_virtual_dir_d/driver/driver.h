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

#ifndef DRIVER_H_INCLUDE
#define	DRIVER_H_INCLUDE

#include "../common.h"

bool		load_driver();
void		unload_driver();
HANDLE		open_device();
void		close_device(HANDLE dev_hnd);
bool		read_device(HANDLE dev_hnd, void* buf, DWORD len, LPDWORD p_length_read);
bool		write_device(HANDLE dev_hnd, void* buf, DWORD size, LPDWORD p_length_written);

//Kernel functions

#endif // !DRIVER_H_INCLUDE
