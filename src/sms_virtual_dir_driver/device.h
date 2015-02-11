/*
Copyright 2015,∞µ“π”ƒ¡È <darknightghost.cn@gmail.com>

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

#ifndef DEVICE_H_INCLUDE
#define	DEVICE_H_INCLUDE

#include "common.h"

typedef	NTSTATUS(*pdispatch_func)(PDEVICE_OBJECT, PIRP);

typedef	struct dispatch_table_ {
	PDEVICE_OBJECT			p_device;				//Address of device object
	pdispatch_func			dispatch_func;			//Address of dispatch func
	PFAST_IO_DISPATCH		p_fast_io_dispatch;		//Address of FAST_IO_DISPATCH.If not useing Fast I/O function,It is NULL
} dispatch_table, *pdispatch_table;

BOOLEAN			init_dispatch_func(PDRIVER_OBJECT p_driver_object);
VOID			clean_dispatch_func(PDRIVER_OBJECT p_driver_object);
BOOLEAN			add_device(PDEVICE_OBJECT p_device, pdispatch_func p_dispatch_func, PFAST_IO_DISPATCH p_fast_io_dispatch);
VOID			remove_device(PDEVICE_OBJECT p_device);

#endif // !DEVICE_H_INCLUDE
