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

#ifndef VIRTUAL_DIR_H_INCLUDE
#define	VIRTUAL_DIR_H_INCLUDE
#include "common.h"
#include "../../common/driver/gate_func_defines.h"

typedef	struct _vpath_info {
	hvdir		node_hnd;
	PWCHAR		virtual_path;
	PWCHAR		src;
	UINT32		flag;
	UINT32		count;
	BOOLEAN		destroy_flag;
	KMUTEX		lock;
	list		child_list;
	KMUTEX		child_list_lock;
} vpath_info, *pvpath_info;

VOID			init_virtual_dir();
BOOLEAN			create_virtual_dir(PWCHAR path, UINT32 flag);
BOOLEAN			remove_virtual_dir();
VOID			clear_virtual_dir();

#endif // !VIRTUAL_DIR_H_INCLUDE
