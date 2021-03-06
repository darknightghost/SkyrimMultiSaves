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

#include "sleep.h"

#define	ONE_MICROSECOND		(-10)
#define	ONE_MILLISECOND		(ONE_MICROSECOND * 1000)

VOID sleep(ULONG milliseconds)
{
	LARGE_INTEGER interval;
	interval.QuadPart = ((LONGLONG)ONE_MILLISECOND) * ((LONGLONG)milliseconds);
	KeDelayExecutionThread(KernelMode, 0, &interval);
	return;
}