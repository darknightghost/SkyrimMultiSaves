/*
Copyright 2015,��ҹ���� <darknightghost.cn@gmail.com>

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

#include "../common.h"
#include "../../common/driver/gate_func_defines.h"
#include "control_device.h"

NTSTATUS		start_call_gate(PDRIVER_OBJECT p_driver_object);
VOID			stop_call_gate();

#endif // !COMMAND_H_INCLUDE