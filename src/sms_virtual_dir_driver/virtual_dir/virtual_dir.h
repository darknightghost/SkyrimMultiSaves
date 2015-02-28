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
#include "../data_structure/list.h"
#include "../../common/driver/gate_func_defines.h"

typedef	struct _vpath_info {
	hvdir					node_hnd;
	struct _vpath_info*		p_parent;
	PWCHAR					virtual_path;
	PWCHAR					src;
	UINT32					flag;
	UINT32					reference;
	UINT32					open_status;
	BOOLEAN					destroy_flag;
	BOOLEAN					initialized_flag;
	KMUTEX					lock;
	UINT32					child_list_ref;
	list					child_list;
	KMUTEX					child_list_lock;
} vpath_info, *pvpath_info;

VOID			init_virtual_path();
BOOLEAN			set_root_path(PWCHAR root_path);
BOOLEAN			is_virtual_path(PWCHAR path);
hvdir			add_virtual_path(PWCHAR path, PWCHAR src, UINT32 flag);
pvpath_info		get_virtual_path_by_path(PWCHAR virtual_path);
pvpath_info		get_virtual_path_by_hvdir(hvdir hnd);
VOID			decrease_virtual_path_reference(pvpath_info p_info);
BOOLEAN			increase_virtual_path_child_reference(pvpath_info p_info);
VOID			decrease_virtual_path_child_reference(pvpath_info p_info);
BOOLEAN			remove_virtual_path();
VOID			clear_virtual_path();

#endif // !VIRTUAL_DIR_H_INCLUDE
