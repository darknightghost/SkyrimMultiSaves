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

static	BOOLEAN			run_flag;
static	VOID			call_gate_dispatch_thread(PVOID context);
static	KEVENT			thread_exit_event;

NTSTATUS start_call_gate(PDRIVER_OBJECT p_driver_object)
{
	NTSTATUS status;
	HANDLE interpreter_thread_handle;

	run_flag = TRUE;

	KeInitializeEvent(&thread_exit_event, NotificationEvent, TRUE);
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
	run_flag = FALSE;
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
	return;
}

VOID call_gate_dispatch_thread(PVOID context)
{
	char b[32];
	SIZE_T l;
	NTSTATUS status;

	KdPrint(("sms:call_gate_dispatch thread started.\n"));

	while(run_flag) {
		status = read_pipe(&write_buf, b, 32, &l);

		if(NT_SUCCESS(status)) {
			write_pipe(&read_buf, b, l);
		}
	}

	KeSetEvent(&thread_exit_event, 0, FALSE);
	KdPrint(("sms:call_gate_dispatch thread is exiting.\n"));
	PsTerminateSystemThread(STATUS_SUCCESS);
	UNREFERENCED_PARAMETER(context);
	return;
}