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

#include "gate.h"
#include "../data_structure/pipe.h"

static	CRITICAL_SECTION	call_kernel_lock;
static	CRITICAL_SECTION	kernel_write_lock;
static	CRITICAL_SECTION	flag_lock;
static	bool				init_flag = false;
static	bool				run_flag = false;
static	ULONG				calling_count = 0;
static	HANDLE				device_read_thread_hnd;
static	ret_ptr				kernel_ret;
static	HANDLE				kernel_ret_event;
static	HANDLE				dev_hnd;
static	int					thread_run_status = 0;
static	HANDLE				device_handle;

void*						user_mode_func_table[] = {
	u_create_virtual_path,
	u_change_virtual_path,
	u_remove_virtual_path
};

void*						daemon_caller(void* buf);
static	DWORD	WINAPI	device_read_func(LPVOID p_null);

bool init_call_gate()
{
	if(init_flag) {
		return true;
	}

	InitializeCriticalSection(&call_kernel_lock);
	InitializeCriticalSection(&flag_lock);
	InitializeCriticalSection(&kernel_write_lock);
	kernel_ret_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	init_flag = true;
	return true;
}

void destroy_call_gate()
{
	if(!init_flag || run_flag) {
		return;
	}

	DeleteCriticalSection(&call_kernel_lock);
	DeleteCriticalSection(&flag_lock);
	DeleteCriticalSection(&kernel_write_lock);
	CloseHandle(kernel_ret_event);
	init_flag = false;
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

	if(!load_driver()) {
		LeaveCriticalSection(&flag_lock);
		MessageBox(NULL, L"Cannot load driver!", L"Error", MB_OK | MB_ICONWARNING);
		return false;
	}

	dev_hnd = open_device();

	if(dev_hnd == NULL) {
		LeaveCriticalSection(&flag_lock);
		return false;
	}

	//Start device reading thread.
	calling_count = 0;
	run_flag = true;
	thread_run_status = 0;
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

	while(thread_run_status == 0) {
		Sleep(1000);
	}

	if(thread_run_status < 0) {
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
	k_close_call_gate();
	close_device(device_handle);
	close_device(dev_hnd);
	unload_driver();
	PRINTF("Waiting for device reading thread to exit..\n");
	//Waiting for threads to exit
	WaitForSingleObject(device_read_thread_hnd, INFINITE);
	PRINTF("Device reading thread exited..\n");
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
	char* call_buf;
	parg_head p_arg_head;
	char* p;
	ret_pkg	call_ret;
	ret_pkg tmp_ret;
	DWORD length_wrritten;
	UINT32 count;

	device_handle = open_device();

	if(device_handle == NULL) {
		thread_run_status = -1;
		return 0;

	} else {
		thread_run_status = 1;
	}

	p_call_head = (pcall_pkg)read_buf;

	while(run_flag) {
		//Get call number
		length_read = 0;

		while(length_read < sizeof(UINT16)) {
			read_device(device_handle, read_buf + length_read, sizeof(UINT16) - length_read, &len);

			if(!run_flag) {
				close_device(device_handle);
				return 0;
			}

			length_read += len;
		}

		if(IS_KERNERL_MODE_CALL_NUMBER(p_call_head->call_number)) {
			//It's a return value
			//Get return value
			length_read = 0;

			while(length_read < sizeof(ret_pkg) - sizeof(UINT16)) {
				read_device(device_handle, (char*)&tmp_ret + sizeof(UINT16) + length_read,
				            sizeof(ret_pkg) - sizeof(UINT16) - length_read, &len);

				if(!run_flag) {
					close_device(device_handle);
					return 0;
				}

				length_read += len;
			}

			kernel_ret = tmp_ret.ret;
			SetEvent(kernel_ret_event);
			continue;

		} else {
			//It's a call from kernel
			//Get call_pkg
			length_read = 0;

			while(length_read < sizeof(call_pkg) - sizeof(UINT16)) {
				read_device(device_handle, read_buf + sizeof(UINT16) + length_read, sizeof(call_pkg) - sizeof(UINT16) - length_read, &len);

				if(!run_flag) {
					close_device(device_handle);
					return 0;
				}

				length_read += len;
			}

			call_buf = get_memory(p_call_head->buf_len);
			call_ret.call_number = p_call_head->call_number;

			if(call_buf == NULL) {
				//Return failure
				call_ret.ret.ret_64 = 0;

				switch(p_call_head->return_type) {
				case TYPE_VOID:
					call_ret.ret.ret_64 = 0;
					break;

				case TYPE_BOOLEAN:
					call_ret.ret.ret_8 = FALSE;
					break;

				case TYPE_HVDIR:
					call_ret.ret.ret_32 = (UINT32)NULL;
					break;

				case TYPE_UINT32:
					call_ret.ret.ret_32 = 0;
					break;
				}

				//Write return value
				EnterCriticalSection(&kernel_write_lock);
				write_device(device_handle, &call_ret, sizeof(call_ret), &length_wrritten);
				LeaveCriticalSection(&kernel_write_lock);
				continue;
			}

			//Copy call head
			memcpy(call_buf, p_call_head, sizeof(pcall_pkg));
			p = call_buf + sizeof(pcall_pkg);

			//Get args
			for(count = 0; count < p_call_head->arg_num; count++) {
				p_arg_head = (parg_head)p;
				//Get arg head
				length_read = 0;

				while(length_read < sizeof(parg_head)) {
					read_device(device_handle, p, sizeof(parg_head) - length_read, &len);

					if(!run_flag) {
						free_memory(call_buf);
						close_device(device_handle);
						return 0;
					}

					length_read += len;
					p += len;
				}

				//Get arg value
				length_read = 0;

				while(length_read < p_arg_head->size) {
					read_device(device_handle, p, p_arg_head->size - length_read, &len);

					if(!run_flag) {
						free_memory(call_buf);
						close_device(device_handle);
						return 0;
					}

					length_read += len;
					p += len;
				}

			}

			MEM_CHK(call_buf);
			//Call function
			call_ret.ret.value = daemon_caller(call_buf);
			//Write return value
			EnterCriticalSection(&kernel_write_lock);
			write_device(device_handle, &call_ret, sizeof(call_ret), &length_wrritten);
			LeaveCriticalSection(&kernel_write_lock);
			free_memory(call_buf);
		}
	}

	close_device(device_handle);
	return 0;
}

VOID k_enable_filter()
{
	call_pkg call_head;
	DWORD length_wrritten;

	if(!run_flag) {
		return;
	}

	EnterCriticalSection(&flag_lock);

	if(!run_flag) {
		LeaveCriticalSection(&flag_lock);
		return;
	}

	calling_count++;
	LeaveCriticalSection(&flag_lock);

	call_head.call_number = K_ENABLE_FILTER;
	call_head.return_type = TYPE_VOID;
	call_head.arg_num = 0;
	call_head.buf_len = sizeof(call_pkg);

	//Call function
	EnterCriticalSection(&call_kernel_lock);
	EnterCriticalSection(&kernel_write_lock);
	PRINTF(("Calling kernel...\n"));
	write_device(dev_hnd, &call_head, sizeof(call_pkg), &length_wrritten);
	LeaveCriticalSection(&kernel_write_lock);

	//Wait for return
	ResetEvent(kernel_ret_event);
	WaitForSingleObject(kernel_ret_event, INFINITE);
	LeaveCriticalSection(&call_kernel_lock);
	calling_count--;
	return;
}

VOID k_disable_filter()
{
	call_pkg call_head;
	DWORD length_wrritten;

	if(!run_flag) {
		return;
	}

	EnterCriticalSection(&flag_lock);

	if(!run_flag) {
		LeaveCriticalSection(&flag_lock);
		return;
	}

	calling_count++;
	LeaveCriticalSection(&flag_lock);

	call_head.call_number = K_DISABLE_FILTER;
	call_head.return_type = TYPE_VOID;
	call_head.arg_num = 0;
	call_head.buf_len = sizeof(call_pkg);

	//Call function
	EnterCriticalSection(&call_kernel_lock);
	EnterCriticalSection(&kernel_write_lock);
	write_device(dev_hnd, &call_head, sizeof(call_pkg), &length_wrritten);
	LeaveCriticalSection(&kernel_write_lock);

	//Wait for return
	ResetEvent(kernel_ret_event);
	WaitForSingleObject(kernel_ret_event, INFINITE);
	LeaveCriticalSection(&call_kernel_lock);
	calling_count--;
	return;
}

BOOLEAN k_set_base_dir(PWCHAR path)
{
	pcall_pkg p_call_head;
	parg_head p_arg;
	char* p_arg_value;
	char* buf;
	UINT32 buf_len;
	DWORD length_wrritten;
	BOOLEAN ret;

	if(!run_flag) {
		return FALSE;
	}

	EnterCriticalSection(&flag_lock);

	if(!run_flag) {
		LeaveCriticalSection(&flag_lock);
		return FALSE;
	}

	calling_count++;
	LeaveCriticalSection(&flag_lock);

	//Compute the size of new buf.
	buf_len = (UINT32)(
	              sizeof(call_pkg) + sizeof(arg_head) + wcslen(path) * sizeof(WCHAR) + sizeof(WCHAR)
	          );
	buf = get_memory(buf_len);

	if(buf == NULL) {
		calling_count--;
		return FALSE;
	}

	//Fill call head
	p_call_head = (pcall_pkg)buf;
	p_call_head->call_number = K_SET_BASE_DIR;
	p_call_head->buf_len = buf_len;
	p_call_head->return_type = TYPE_BOOLEAN;
	p_call_head->arg_num = 1;

	//Fill args
	p_arg = (parg_head)(p_call_head + 1);
	p_arg->size = (UINT32)(wcslen(path) * sizeof(WCHAR) + sizeof(WCHAR));
	p_arg->type = TYPE_WCHAR_STRING;

	p_arg_value = (char*)(p_arg + 1);
	wcscpy((PWCHAR)p_arg_value, path);

	//Call function
	EnterCriticalSection(&call_kernel_lock);
	EnterCriticalSection(&kernel_write_lock);
	write_device(dev_hnd, buf, buf_len, &length_wrritten);
	LeaveCriticalSection(&kernel_write_lock);

	//Wait for return
	ResetEvent(kernel_ret_event);
	WaitForSingleObject(kernel_ret_event, INFINITE);
	ret = kernel_ret.ret_8;
	LeaveCriticalSection(&call_kernel_lock);
	calling_count--;
	return ret;
}

hvdir k_add_virtual_path(PWCHAR src, PWCHAR dest, UINT32 flag)
{
	pcall_pkg p_call_head;
	parg_head p_arg;
	char* p_arg_value;
	char* buf;
	UINT32 buf_len;
	DWORD length_wrritten;
	hvdir ret;

	if(!run_flag) {
		return (hvdir)NULL;
	}

	EnterCriticalSection(&flag_lock);

	if(!run_flag) {
		LeaveCriticalSection(&flag_lock);
		return (hvdir)NULL;
	}

	calling_count++;
	LeaveCriticalSection(&flag_lock);

	//Compute the size of new buf.
	buf_len = (UINT32)(
	              sizeof(call_pkg) + sizeof(arg_head) * 3 + wcslen(src) * sizeof(WCHAR)
	              + wcslen(dest) * sizeof(WCHAR) + sizeof(WCHAR) * 2 + sizeof(UINT32)
	          );
	buf = get_memory(buf_len);

	if(buf == NULL) {
		calling_count--;
		return (hvdir)NULL;
	}

	//Fill call head
	p_call_head = (pcall_pkg)buf;
	p_call_head->call_number = K_ADD_VIRTUAL_PATH;
	p_call_head->buf_len = buf_len;
	p_call_head->return_type = TYPE_HVDIR;
	p_call_head->arg_num = 3;

	//First arg
	p_arg = (parg_head)(p_call_head + 1);
	p_arg->size = (UINT32)(wcslen(src) * sizeof(WCHAR) + sizeof(WCHAR));
	p_arg->type = TYPE_WCHAR_STRING;

	p_arg_value = (char*)(p_arg + 1);
	wcscpy((PWCHAR)p_arg_value, src);

	//Second arg
	p_arg = (parg_head)(p_arg_value + wcslen(src) * sizeof(WCHAR) + sizeof(WCHAR));
	p_arg->size = (UINT32)(wcslen(dest) * sizeof(WCHAR) + sizeof(WCHAR));
	p_arg->type = TYPE_WCHAR_STRING;

	p_arg_value = (char*)(p_arg + 1);
	wcscpy((PWCHAR)p_arg_value, dest);

	//Third arg
	p_arg = (parg_head)(p_arg_value + wcslen(dest) * sizeof(WCHAR) + sizeof(WCHAR));
	p_arg->size = sizeof(UINT32);
	p_arg->type = TYPE_UINT32;

	p_arg_value = (char*)(p_arg + 1);
	*(UINT32*)p_arg_value = flag;

	//Call function
	EnterCriticalSection(&call_kernel_lock);
	EnterCriticalSection(&kernel_write_lock);
	write_device(dev_hnd, buf, buf_len, &length_wrritten);
	LeaveCriticalSection(&kernel_write_lock);

	//Wait for return
	ResetEvent(kernel_ret_event);
	WaitForSingleObject(kernel_ret_event, INFINITE);
	ret = kernel_ret.ret_32;
	LeaveCriticalSection(&call_kernel_lock);
	calling_count--;
	free_memory(buf);
	return ret;
}

NTSTATUS k_change_virtual_path(hvdir vdir_hnd, PWCHAR new_src, UINT32 flag)
{
	pcall_pkg p_call_head;
	parg_head p_arg;
	char* p_arg_value;
	char* buf;
	UINT32 buf_len;
	DWORD length_wrritten;
	hvdir ret;

	if(!run_flag) {
		return STATUS_UNSUCCESSFUL;
	}

	EnterCriticalSection(&flag_lock);

	if(!run_flag) {
		LeaveCriticalSection(&flag_lock);
		return STATUS_UNSUCCESSFUL;
	}

	calling_count++;
	LeaveCriticalSection(&flag_lock);

	//Compute the size of new buf.
	buf_len = (UINT32)(
	              sizeof(call_pkg) + sizeof(arg_head) * 3 + sizeof(hvdir)
	              + wcslen(new_src) * sizeof(WCHAR) + sizeof(WCHAR) + sizeof(UINT32)
	          );
	buf = get_memory(buf_len);

	if(buf == NULL) {
		calling_count--;
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	//Fill call head
	p_call_head = (pcall_pkg)buf;
	p_call_head->call_number = K_CHANGE_VIRTUAL_PATH;
	p_call_head->buf_len = buf_len;
	p_call_head->return_type = TYPE_NTSTATUS;
	p_call_head->arg_num = 3;

	//First arg
	p_arg = (parg_head)(p_call_head + 1);
	p_arg->size = (UINT32)sizeof(hvdir);
	p_arg->type = TYPE_HVDIR;

	p_arg_value = (char*)(p_arg + 1);
	*(phvdir)p_arg_value = vdir_hnd;

	//Second arg
	p_arg = (parg_head)(p_arg_value + wcslen(new_src) * sizeof(WCHAR) + sizeof(WCHAR));
	p_arg->size = (UINT32)(wcslen(new_src) * sizeof(WCHAR) + sizeof(WCHAR));
	p_arg->type = TYPE_WCHAR_STRING;

	p_arg_value = (char*)(p_arg + 1);
	wcscpy((PWCHAR)p_arg_value, new_src);

	//Third arg
	p_arg = (parg_head)(p_arg_value + wcslen(new_src) * sizeof(WCHAR) + sizeof(WCHAR));
	p_arg->size = sizeof(UINT32);
	p_arg->type = TYPE_UINT32;

	p_arg_value = (char*)(p_arg + 1);
	*(UINT32*)p_arg_value = flag;

	//Call function
	EnterCriticalSection(&call_kernel_lock);
	EnterCriticalSection(&kernel_write_lock);
	write_device(dev_hnd, buf, buf_len, &length_wrritten);
	LeaveCriticalSection(&kernel_write_lock);

	//Wait for return
	ResetEvent(kernel_ret_event);
	WaitForSingleObject(kernel_ret_event, INFINITE);
	ret = kernel_ret.ret_32;
	LeaveCriticalSection(&call_kernel_lock);
	calling_count--;
	return ret;
}

BOOLEAN k_remove_virtual_path(hvdir vdir_hnd)
{
	pcall_pkg p_call_head;
	parg_head p_arg;
	char* p_arg_value;
	char* buf;
	UINT32 buf_len;
	DWORD length_wrritten;
	hvdir ret;

	if(!run_flag) {
		return FALSE;
	}

	EnterCriticalSection(&flag_lock);

	if(!run_flag) {
		LeaveCriticalSection(&flag_lock);
		return FALSE;
	}

	calling_count++;
	LeaveCriticalSection(&flag_lock);
	//Compute the size of new buf.
	buf_len = (UINT32)(
	              sizeof(call_pkg) + sizeof(arg_head) + sizeof(hvdir)
	          );
	buf = get_memory(buf_len);

	if(buf == NULL) {
		calling_count--;
		return FALSE;
	}

	//Fill call head
	p_call_head = (pcall_pkg)buf;
	p_call_head->call_number = K_CHANGE_VIRTUAL_PATH;
	p_call_head->buf_len = buf_len;
	p_call_head->return_type = TYPE_BOOLEAN;
	p_call_head->arg_num = 1;

	//First arg
	p_arg = (parg_head)(p_call_head + 1);
	p_arg->size = (UINT32)sizeof(hvdir);
	p_arg->type = TYPE_HVDIR;

	p_arg_value = (char*)(p_arg + 1);
	*(phvdir)p_arg_value = vdir_hnd;

	//Call function
	EnterCriticalSection(&call_kernel_lock);
	EnterCriticalSection(&kernel_write_lock);
	write_device(dev_hnd, buf, buf_len, &length_wrritten);
	LeaveCriticalSection(&kernel_write_lock);

	//Wait for return
	ResetEvent(kernel_ret_event);
	WaitForSingleObject(kernel_ret_event, INFINITE);
	ret = kernel_ret.ret_8;
	LeaveCriticalSection(&call_kernel_lock);
	calling_count--;
	return ret;
}

VOID k_clean_all_virtual_path()
{
	call_pkg call_head;
	DWORD length_wrritten;

	if(!run_flag) {
		return;
	}

	EnterCriticalSection(&flag_lock);

	if(!run_flag) {
		LeaveCriticalSection(&flag_lock);
		return;
	}

	calling_count++;
	LeaveCriticalSection(&flag_lock);
	call_head.call_number = K_CLEAN_ALL_VIRTUAL_PATH;
	call_head.return_type = TYPE_VOID;
	call_head.arg_num = 0;
	call_head.buf_len = sizeof(call_pkg);

	//Call function
	EnterCriticalSection(&call_kernel_lock);
	EnterCriticalSection(&kernel_write_lock);
	write_device(dev_hnd, &call_head, sizeof(call_pkg), &length_wrritten);
	LeaveCriticalSection(&kernel_write_lock);

	//Wait for return
	ResetEvent(kernel_ret_event);
	WaitForSingleObject(kernel_ret_event, INFINITE);
	LeaveCriticalSection(&call_kernel_lock);
	calling_count--;
	return;
}

VOID k_close_call_gate()
{
	call_pkg call_head;
	DWORD length_wrritten;

	EnterCriticalSection(&flag_lock);

	if(run_flag) {
		LeaveCriticalSection(&flag_lock);
		return;
	}

	calling_count++;
	LeaveCriticalSection(&flag_lock);

	call_head.call_number = K_CLOSE_CALL_GATE;
	call_head.return_type = TYPE_VOID;
	call_head.arg_num = 0;
	call_head.buf_len = sizeof(call_pkg);

	//Call function
	EnterCriticalSection(&call_kernel_lock);
	EnterCriticalSection(&kernel_write_lock);
	write_device(dev_hnd, &call_head, sizeof(call_pkg), &length_wrritten);
	LeaveCriticalSection(&kernel_write_lock);

	LeaveCriticalSection(&call_kernel_lock);
	calling_count--;
	return;
}

BOOLEAN u_create_virtual_path(PWCHAR p_path, UINT32 flag)
{
	if(!run_flag) {
		return FALSE;
	}

	EnterCriticalSection(&flag_lock);

	if(!run_flag) {
		LeaveCriticalSection(&flag_lock);
		return FALSE;
	}

	calling_count++;
	LeaveCriticalSection(&flag_lock);
	PRINTF("sms:u_create_virtual_path called.dest=\"%ws\",flag=0x%.8X\n", p_path, flag);
	calling_count--;
	return TRUE;
}

BOOLEAN u_change_virtual_path(hvdir vdir_hnd)
{
	if(!run_flag) {
		return FALSE;
	}

	EnterCriticalSection(&flag_lock);

	if(!run_flag) {
		LeaveCriticalSection(&flag_lock);
		return FALSE;
	}

	calling_count++;
	LeaveCriticalSection(&flag_lock);
	PRINTF("sms:u_change_virtual_path called.vdir_hnd=0x%.8X.\n", vdir_hnd);
	calling_count--;
	return TRUE;
}

BOOLEAN u_remove_virtual_path(hvdir vdir_hnd)
{
	if(!run_flag) {
		return FALSE;
	}

	EnterCriticalSection(&flag_lock);

	if(!run_flag) {
		LeaveCriticalSection(&flag_lock);
		return FALSE;
	}

	calling_count++;
	LeaveCriticalSection(&flag_lock);
	PRINTF("sms:u_remove_virtual_path called.vdir_hnd=0x%.8X.\n", vdir_hnd);
	calling_count--;
	return TRUE;
}