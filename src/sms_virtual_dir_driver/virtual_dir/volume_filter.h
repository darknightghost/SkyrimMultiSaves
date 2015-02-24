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

#ifndef VOLUME_FILTER_H_INCLUDE
#define	VOLUME_FILTER_H_INCLUDE

#include "../common.h"
#include "filter.h"

VOID			init_volume_filter_module();

NTSTATUS		attach_volumes(
    IN PDEVICE_OBJECT p_fs_dev_obj,
    IN PUNICODE_STRING fs_name);

VOID			clean_volume_filter_devices();

#endif // !VOLUME_FILTER_H_INCLUDE
