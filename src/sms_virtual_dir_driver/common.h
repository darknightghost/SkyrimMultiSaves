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

#ifndef COMMON_H_INCLUDE
#define	COMMON_H_INCLUDE

#include "debug.h"
#include <ntifs.h>
#include "mem.h"
#include "sleep.h"
#include "data_structure/pipe.h"

VOID			get_object_name(
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name);

#endif // !COMMON_H_INCLUDE
