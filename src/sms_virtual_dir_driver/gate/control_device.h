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

#ifndef CONTROL_DEVICE_H_INCLUDE
#define	CONTROL_DEVICE_H_INCLUDE
#include "common.h"

extern	PDEVICE_OBJECT		g_p_control_device;
extern	pipe				read_buf;
extern	pipe				write_buf;

NTSTATUS		init_control_device(PDRIVER_OBJECT p_driver_object);
VOID			destroy_control_device();

#endif // !CONTROL_DEVICE_H_INCLUDE
