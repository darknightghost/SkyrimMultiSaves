﻿/*
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

#ifndef GATE_H_INCLUDE
#define	GATE_H_INCLUDE

#define		UMDF_USING_NTSTATUS

#include "../common.h"
#include "../../common/driver/gate_func_defines.h"
#include "driver.h"
#include <ntstatus.h>

void		init_call_gate();
void		destroy_call_gate();
bool		start_call_gate();
void		stop_call_gate();

#endif // !GATE_H_INCLUDE
