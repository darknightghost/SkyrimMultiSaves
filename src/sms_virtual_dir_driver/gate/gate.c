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

#pragma warning(disable:4054)

static	KSPIN_LOCK		flag_lock;
static	KMUTEX			usermode_call_lock;
static	BOOLEAN			run_flag;
static	KEVENT			ret_event;
static	ret_ptr			ret_value;
static	VOID			call_gate_dispatch_thread(PVOID context);
static	ULONG			thread_num;
static	KEVENT			thread_exit_event;
PVOID					driver_caller(PVOID buf);
void*					kernel_mode_func_table[] = {
	(void*)k_enable_filter,
	(void*)k_disable_filter,
	(void*)k_set_base_dir,
	(void*)k_add_virtual_path,
	(void*)k_change_virtual_path,
	(void*)k_remove_virtual_path,
	(void*)k_clean_all_virtual_path
};

NTSTATUS start_call_gate(PDRIVER_OBJECT p_driver_object)
{
	NTSTATUS status;
	HANDLE interpreter_thread_handle;

	run_flag = TRUE;

	KeInitializeEvent(&thread_exit_event, NotificationEvent, TRUE);
	KeInitializeEvent(&ret_event, NotificationEvent, TRUE);
	KeInitializeSpinLock(&flag_lock);
	KeInitializeMutex(&usermode_call_lock, 0);
	thread_num = 0;
	KeResetEvent(&thread_exit_event);
	//Start interpreter thread
	status = PsCreateSystemThread(
	             &interpreter_thread_handle,
	             0,
	             NULL,
	             NULL,
	             NULL,
	             call_gate_dispatch_thread,
	             NULL);

	if(!NT_SUCCESS(status)) {
		return status;
	}

	ASSERT(NT_SUCCESS(status));

	ZwClose(interpreter_thread_handle);

	//Initialize control device
	status = init_control_device(p_driver_object);
	ASSERT(NT_SUCCESS(status));
	return status;
}

VOID stop_call_gate()
{
	KIRQL irql;
	KeAcquireSpinLock(&flag_lock, &irql);
	run_flag = FALSE;
	KeReleaseSpinLock(&flag_lock, irql);
	destroy_control_device();
	KdPrint(("sms:Waiting for call_gate_dispatch thread to exit.\n"));
	KeWaitForSingleObject(
	    &thread_exit_event,
	    Executive,
	    KernelMode,
	    FALSE,
	    NULL
	);
	KdPrint(("sms:call_gate_dispatch thread has been exited.\n"));

	while(thread_num != 0) {
		sleep(1000);
	}

	return;
}

VOID call_gate_dispatch_thread(PVOID context)
{
	char head_buf[64];
	SIZE_T length_read;
	SIZE_T len;
	pcall_pkg p_call_head;
	char* call_buf;
	parg_head p_arg_head;
	char* p;
	ret_pkg	call_ret;
	UINT32 count;
	NTSTATUS status;

	KdPrint(("sms:call_gate_dispatch thread started.\n"));
	p_call_head = (pcall_pkg)head_buf;

	while(run_flag) {

		//Get call number
		length_read = 0;

		while(length_read < sizeof(UINT16)) {
			status = read_pipe(&write_buf, head_buf + length_read, sizeof(UINT16) - length_read, &len);

			if(NT_SUCCESS(status)) {
				length_read += len;
			}

			if(!run_flag) {
				goto _END_THREAD;
			}
		}

		if(!IS_KERNERL_MODE_CALL_NUMBER(p_call_head->call_number)) {
			//It's a return value
			//Get return value
			length_read = 0;

			while(length_read < sizeof(UINT64)) {
				status = read_pipe(&write_buf, head_buf + length_read, sizeof(UINT64) - length_read, &len);

				if(NT_SUCCESS(status)) {
					length_read += len;
				}

				if(!run_flag) {
					goto _END_THREAD;
				}
			}

			ret_value = *(pret_ptr)head_buf;
			KeSetEvent(&ret_event, 0, FALSE);
			continue;

		} else {
			//It's a call from kernel
			//Get call_pkg
			length_read = 0;

			while(length_read < sizeof(call_pkg) - sizeof(UINT16)) {
				status = read_pipe(&write_buf,
				                   head_buf + sizeof(UINT16) + length_read,
				                   sizeof(call_pkg) - sizeof(UINT16) - length_read,
				                   &len);

				if(NT_SUCCESS(status)) {
					length_read += len;
				}

				if(!run_flag) {
					goto _END_THREAD;
				}
			}

			call_buf = get_memory(p_call_head->buf_len, *(PULONG)"sms", NonPagedPool);
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
				KeWaitForSingleObject(&usermode_call_lock, Executive, KernelMode, FALSE, NULL);
				write_pipe(&read_buf, &call_ret, sizeof(call_ret));
				KeReleaseMutex(&usermode_call_lock, FALSE);
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
					status = read_pipe(&write_buf,
					                   p,
					                   sizeof(parg_head) - length_read,
					                   &len);

					if(NT_SUCCESS(status)) {
						length_read += len;
						p += len;
					}

					if(!run_flag) {
						free_memory(call_buf);
						goto _END_THREAD;
					}
				}

				//Get arg value
				length_read = 0;

				while(length_read < p_arg_head->size) {
					status = read_pipe(&write_buf,
					                   p,
					                   p_arg_head->size - length_read,
					                   &len);

					if(NT_SUCCESS(status)) {
						length_read += len;
						p += len;
					}

					if(!run_flag) {
						free_memory(call_buf);
						goto _END_THREAD;
					}
				}
			}

			MEM_CHK(call_buf);
			//Call function
			call_ret.ret.value = driver_caller(call_buf);
			//Write return value
			KeWaitForSingleObject(&usermode_call_lock, Executive, KernelMode, FALSE, NULL);
			write_pipe(&read_buf, &call_ret, sizeof(call_ret));
			KeReleaseMutex(&usermode_call_lock, FALSE);
			free_memory(call_buf);

		}
	}

_END_THREAD:
	KeSetEvent(&thread_exit_event, 0, FALSE);
	KdPrint(("sms:call_gate_dispatch thread is exiting.\n"));
	PsTerminateSystemThread(STATUS_SUCCESS);
	UNREFERENCED_PARAMETER(context);
	return;
}

VOID k_enable_filter()
{
	KIRQL irql;

	PASSIVE_LEVEL_ASSERT;
	KeAcquireSpinLock(&flag_lock, &irql);

	if(!run_flag) {
		return;
	}

	thread_num++;

	KeReleaseSpinLock(&flag_lock, irql);
	KdPrint(("sms:k_enable_filter called.\n"));
	thread_num--;
	return;
}

VOID k_disable_filter()
{
	KIRQL irql;

	PASSIVE_LEVEL_ASSERT;
	KeAcquireSpinLock(&flag_lock, &irql);

	if(!run_flag) {
		return;
	}

	thread_num++;

	KeReleaseSpinLock(&flag_lock, irql);
	KdPrint(("sms:k_disable_filter called.\n"));
	thread_num--;
	return;
}

BOOLEAN k_set_base_dir(PWCHAR path)
{
	KIRQL irql;

	PASSIVE_LEVEL_ASSERT;
	KeAcquireSpinLock(&flag_lock, &irql);

	if(!run_flag) {
		return FALSE;
	}

	thread_num++;

	KeReleaseSpinLock(&flag_lock, irql);
	KdPrint(("sms:k_set_base_dir called.path=\"%ws\"\n", path));
	thread_num--;
	return TRUE;
}

hvdir k_add_virtual_path(PWCHAR src, PWCHAR dest, UINT32 flag)
{
	KIRQL irql;

	PASSIVE_LEVEL_ASSERT;
	KeAcquireSpinLock(&flag_lock, &irql);

	if(!run_flag) {
		return (hvdir)NULL;
	}

	thread_num++;

	KeReleaseSpinLock(&flag_lock, irql);
	KdPrint(("sms:k_add_virtual_path called.src=\"%ws\",dest=\"%ws\",flag=0x%.8X\n", src, dest, flag));
	thread_num--;
	return 0;
}

NTSTATUS k_change_virtual_path(hvdir vdir_hnd, PWCHAR new_src, UINT32 flag)
{
	KIRQL irql;

	PASSIVE_LEVEL_ASSERT;
	KeAcquireSpinLock(&flag_lock, &irql);

	if(!run_flag) {
		return STATUS_DEVICE_REMOVED;
	}

	thread_num++;

	KeReleaseSpinLock(&flag_lock, irql);
	KdPrint(("sms:k_change_virtual_path called.vdir_hnd=0x%.8X,new_src=\"%ws\",flag=0x%.8X\n", vdir_hnd, new_src, flag));
	thread_num--;
	return STATUS_SUCCESS;
}

BOOLEAN k_remove_virtual_path(hvdir vdir_hnd)
{
	KIRQL irql;

	PASSIVE_LEVEL_ASSERT;
	KeAcquireSpinLock(&flag_lock, &irql);

	if(!run_flag) {
		return FALSE;
	}

	thread_num++;

	KeReleaseSpinLock(&flag_lock, irql);
	KdPrint(("sms:k_remove_virtual_path called.vdir_hnd=0x%.8X\n", vdir_hnd));
	thread_num--;
	return TRUE;
}

VOID k_clean_all_virtual_path()
{
	KIRQL irql;

	PASSIVE_LEVEL_ASSERT;
	KeAcquireSpinLock(&flag_lock, &irql);

	if(!run_flag) {
		return;
	}

	thread_num++;

	KeReleaseSpinLock(&flag_lock, irql);
	KdPrint(("sms:k_clean_all_virtual_path called.\n"));
	thread_num--;
	return;
}

BOOLEAN u_create_virtual_path(hvdir new_vdir_hnd, PWCHAR dest, UINT32 flag)
{
	KIRQL irql;

	PASSIVE_LEVEL_ASSERT;
	KeAcquireSpinLock(&flag_lock, &irql);

	if(!run_flag) {
		return FALSE;
	}

	thread_num++;

	KeReleaseSpinLock(&flag_lock, irql);
	thread_num--;
	UNREFERENCED_PARAMETER(new_vdir_hnd);
	UNREFERENCED_PARAMETER(dest);
	UNREFERENCED_PARAMETER(flag);
	return TRUE;
}

BOOLEAN u_change_virtual_path(hvdir vdir_hnd)
{
	KIRQL irql;

	PASSIVE_LEVEL_ASSERT;
	KeAcquireSpinLock(&flag_lock, &irql);

	if(!run_flag) {
		return FALSE;
	}

	thread_num++;

	KeReleaseSpinLock(&flag_lock, irql);
	thread_num--;
	UNREFERENCED_PARAMETER(vdir_hnd);
	return TRUE;
}