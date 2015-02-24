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

#ifndef GATE_FUNC_DEFINES_H_INCLUDE
#define	GATE_FUNC_DEFINES_H_INCLUDE

//Types
typedef	unsigned __int32		hvdir, *phvdir;
typedef	struct _call_pkg_head {
	UINT16		call_number;
	UCHAR		arg_num;
	UINT32		buf_len;
	UINT32		return_type;
} call_pkg, *pcall_pkg;

typedef	struct _arg_head {
	UINT32	type;
	UINT32	size;
} arg_head, *parg_head;

typedef	union _ret_ptr {
	PVOID		value;
	UINT64		ret_64;
	UINT32		ret_32;
	UINT16		ret_16;
	UINT8		ret_8;
} ret_ptr, *pret_ptr;

typedef	struct _ret_pkg_head {
	UINT16		call_number;
	ret_ptr		ret;
} ret_pkg, *pret_pkg;

//Functions
/*
The name of kernel mode functions start with "k_" and user mode functions start
with "u".The number of arguments of each function is at most 4.
*/
VOID			k_enable_filter();
VOID			k_disable_filter();
BOOLEAN			k_set_base_dir(PWCHAR path);
hvdir			k_add_virtual_path(PWCHAR src, PWCHAR dest, UINT32 flag);
NTSTATUS		k_change_virtual_path(hvdir vdir_hnd, PWCHAR new_src, UINT32 flag);
BOOLEAN			k_remove_virtual_path(hvdir vdir_hnd);
VOID			k_clean_all_virtual_path();
VOID			k_close_call_gate();

BOOLEAN			u_create_virtual_path(hvdir new_vdir_hnd, PWCHAR dest, UINT32 flag);
BOOLEAN			u_change_virtual_path(hvdir vdir_hnd);
BOOLEAN			u_remove_virtual_path(hvdir vdir_hnd);

//Call number
#define	K_ENABLE_FILTER				0x0000
#define	K_DISABLE_FILTER			0x0001
#define	K_SET_BASE_DIR				0x0002
#define	K_ADD_VIRTUAL_PATH			0x0003
#define	K_CHANGE_VIRTUAL_PATH		0x0004
#define	K_REMOVE_VIRTUAL_PATH		0x0005
#define	K_CLEAN_ALL_VIRTUAL_PATH	0x0006
#define	K_CLOSE_CALL_GATE			0x0007

#define	U_CREATE_VIRTUAL_PATH		0x8000
#define	U_CHANGE_VIRTUAL_PATH		0x8001
#define	U_REMOVE_VIRTUAL_PATH		0x8002

//Flags
#define	FLAG_DIRECTORY				0x00000001
#define	FLAG_COPYONWRITE			0x00000002
#define	FLAG_CREATE					0x00000004
#define	FLAG_CHANGESRC				0x00000008
#define	FLAG_CHANGEFLAGS			0x00000010

#define	TYPE_VOID					0x0000
#define	TYPE_BOOLEAN				0x0001
#define	TYPE_HVDIR					0x0002
#define	TYPE_UINT32					0x0003
#define	TYPE_WCHAR_STRING			0x0004
#define	TYPE_NTSTATUS				0x0005

//Marcos
#define	IS_KERNERL_MODE_CALL_NUMBER(call_num)	((call_num) < 0x8000)

#endif // !GATE_FUNC_DEFINES_H_INCLUDE
