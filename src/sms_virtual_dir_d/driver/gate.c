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

#include "gate.h"
#include "../data_structure/pipe.h"

static	CRITICAL_SECTION	call_kernel_lock;
static	CRITICAL_SECTION	flag_lock;
static	bool				init_flag = false;
static	bool				run_flag = false;
static	ULONG				calling_count = 0;
static	HANDLE				device_read_thread_hnd;
static	UINT64				kernel_ret;
static	HANDLE				kernel_ret_event;

void*						user_mode_func_table[] = {
	u_create_virtual_path,
	u_change_virtual_path
};

void*						daemon_caller(void* buf);
static	DWORD	WINAPI	device_read_func(LPVOID p_null);

void init_call_gate()
{
	if(init_flag) {
		return;
	}

	InitializeCriticalSection(&call_kernel_lock);
	InitializeCriticalSection(&flag_lock);
	kernel_ret_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	return;
}

void destroy_call_gate()
{
	if(!init_flag || run_flag) {
		return;
	}

	DeleteCriticalSection(&call_kernel_lock);
	DeleteCriticalSection(&flag_lock);
	CloseHandle(kernel_ret_event);
	return;
}

bool start_call_gate()
{
	//Check if the module is running.
	EnterCriticalSection(&flag_lock);

	if(!init_flag) {
		LeaveCriticalSection(&flag_lock);
		return false;
	}

	if(run_flag) {
		LeaveCriticalSection(&flag_lock);
		return true;
	}

	//Start device reading thread.
	calling_count = 0;
	run_flag = true;
	device_read_thread_hnd = CreateThread(
	                             NULL,
	                             0,
	                             device_read_func,
	                             NULL,
	                             0,
	                             NULL);

	if(device_read_thread_hnd == NULL) {
		run_flag = false;
		LeaveCriticalSection(&flag_lock);
		return false;
	}

	LeaveCriticalSection(&flag_lock);

	return true;
}

void stop_call_gate()
{
	//Check if the module is not running.
	EnterCriticalSection(&flag_lock);

	if(!run_flag || !init_flag) {
		LeaveCriticalSection(&flag_lock);
		return;
	}

	run_flag = false;
	LeaveCriticalSection(&flag_lock);

	//Waiting for threads to exit
	WaitForSingleObject(device_read_thread_hnd, INFINITE);
	CloseHandle(device_read_thread_hnd);

	while(calling_count > 0) {
		Sleep(1000);
	}

	return;
}

DWORD WINAPI device_read_func(LPVOID p_null)
{
	char read_buf[64];
	DWORD length_read;
	DWORD len;
	pcall_pkg p_call_head;
	pret_pkg p_ret_head;
	char* call_buf;
	parg_head p_arg_head;
	char* p;
	ret_pkg	call_ret;
	DWORD length_wrritten;
	UINT32 count;

	p_call_head = (pcall_pkg)read_buf;
	p_ret_head = (pret_pkg)read_buf;

	while(run_flag) {
		//Get call number
		length_read = 0;

		while(length_read < sizeof(UINT16)) {
			read_device(read_buf + length_read, sizeof(UINT16), &len);

			if(!run_flag) {
				return 0;
			}

			length_read += len;
		}

		if(IS_KERNERL_MODE_CALL_NUMBER(p_call_head->call_number)) {
			//It's a return value
			//Get return value
			length_read = 0;

			while(length_read < sizeof(UINT64)) {
				read_device(read_buf + length_read, sizeof(UINT64) - length_read, &len);

				if(!run_flag) {
					return 0;
				}

				length_read += len;
			}

			kernel_ret = *(UINT64*)read_buf;
			SetEvent(kernel_ret_event);
			continue;

		} else {
			//It's a call from kernel
			//Get call_pkg
			length_read = 0;

			while(length_read < sizeof(call_pkg) - sizeof(UINT16)) {
				read_device(read_buf + sizeof(UINT16) + length_read, sizeof(call_pkg) - sizeof(UINT16) - length_read, &len);

				if(!run_flag) {
					return 0;
				}

				length_read += len;
			}

			call_buf = get_memory(p_call_head->buf_len);
			call_ret.call_number = p_call_head->call_number;

			if(call_buf == NULL) {
				//Return failure
				switch(p_call_head->return_type) {
				case TYPE_BOOLEAN:
					call_ret.ret = (LONGLONG)(BOOLEAN)FALSE;
					break;

				case TYPE_HVDIR:
					call_ret.ret = (LONGLONG)(hvdir)NULL;
					break;

				case TYPE_UINT32:
					call_ret.ret = (LONGLONG)(UINT32)0;
					break;
				}

				EnterCriticalSection(&call_kernel_lock);
				write_device(&call_ret, sizeof(call_ret), &length_wrritten);
				LeaveCriticalSection(&call_kernel_lock);
				continue;
			}

			//Copy call head
			memcpy(call_buf, p_call_head, sizeof(pcall_pkg));
			p = call_buf + sizeof(pcall_pkg);

			//Get args
			for(count = 0; count < p_call_head->arg_num; count++) {
				p_arg_head = p;
				//Get arg head
				length_read = 0;

				while(length_read < sizeof(parg_head)) {
					read_device(p, sizeof(parg_head) - length_read, &len);

					if(!run_flag) {
						free_memory(call_buf);
						return 0;
					}

					length_read += len;
				}

				p += length_read;
				//Get arg value
				length_read = 0;

				while(length_read < p_arg_head->size) {
					read_device(p, p_arg_head->size - length_read, &len);

					if(!run_flag) {
						free_memory(call_buf);
						return 0;
					}

					length_read += len;
				}

				p += length_read;
			}

			//Call function
			call_ret.ret = daemon_caller(call_buf);
			EnterCriticalSection(&call_kernel_lock);
			write_device(&call_ret, sizeof(call_ret), &length_wrritten);
			LeaveCriticalSection(&call_kernel_lock);
			free_memory(call_buf);
		}
	}

	return 0;
}

VOID			k_enable_filter();
VOID			k_disable_filter();
BOOLEAN			k_set_base_dir(PWCHAR path);
hvdir			k_add_virtual_path(PWCHAR src, PWCHAR dest, UINT32 flag);
NTSTATUS		k_change_virtual_path(hvdir vdir_hnd, PWCHAR new_src, UINT32 flag);
BOOLEAN			k_remove_virtual_path(hvdir vdir_hnd);
VOID			k_clean_all_virtual_path();

BOOLEAN	 u_create_virtual_path(hvdir new_vdir_hnd, PWCHAR dest, UINT32 flag)
{
	return TRUE;
}

BOOLEAN u_change_virtual_path(hvdir vdir_hnd)
{
	return TRUE;
}

