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

#ifndef OPEN_CLOSE_H_INCLUDE
#define	OPEN_CLOSE_H_INCLUDE

#include "../filter.h"

NTSTATUS		dispatch_func_create(PDEVICE_OBJECT p_device,
                                     PIRP p_irp, PIO_STACK_LOCATION p_irpsp);

#endif // !OPEN_CLOSE_H_INCLUDE
