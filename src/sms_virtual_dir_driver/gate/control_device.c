/*
	Copyright 2015,∞µ“π”ƒ¡È <darknightghost.cn@gmail.com>

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

#include "control_device.h"
#include "../../common//driver/device_name.h"
#include "ntstrsafe.h."
#include "../data_structure/pipe.h"
#include "../device.h"

#define	DRIVER_SRV_REG_PATH		L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\"

pipe					read_buf;
pipe					write_buf;

static	PDEVICE_OBJECT	p_control_device = NULL;

static	BOOLEAN			open_fag = FALSE;
static	KMUTEX			flag_lock;
static	WCHAR			driver_full_name[(sizeof(DRIVER_SRV_REG_PATH) + sizeof(SERVICE_NAME)) / sizeof(WCHAR) + 1];

static	NTSTATUS		dispatch_func(PDEVICE_OBJECT p_device, PIRP p_irp);				//Dispatch func

static	NTSTATUS		dispatch_func_create(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp);		//IRP_MJ_CREATE

static	NTSTATUS		dispatch_func_close(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp);		//IRP_MJ_CLOSE

static	NTSTATUS		dispatch_func_read(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp);		//IRP_MJ_READ

static	NTSTATUS		dispatch_func_write(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp);		//IRP_MJ_WRITE

NTSTATUS init_control_device(PDRIVER_OBJECT p_driver_object)
{
	NTSTATUS status;
	UNICODE_STRING device_name =
	    RTL_CONSTANT_STRING(DEVICE_NAME);
	UNICODE_STRING symbol_link =
	    RTL_CONSTANT_STRING(SYMBOLLINK_NAME);

	//Initialize pipes
	status = initialize_pipe(&read_buf, 1024);

	if(!NT_SUCCESS(status)) {
		return status;
	}

	status = initialize_pipe(&write_buf, 1024);

	if(!NT_SUCCESS(status)) {
		destroy_pipe(&read_buf);
		return status;
	}

	//Create CDO
	status = IoCreateDevice(
	             p_driver_object,
	             0,
	             &device_name,
	             FILE_DEVICE_UNKNOWN,
	             0,
	             FALSE,
	             &p_control_device);

	if(!NT_SUCCESS(status)) {
		destroy_pipe(&read_buf);
		destroy_pipe(&write_buf);
		return status;
	}

	//Create symbolic link
	status = IoCreateSymbolicLink(&symbol_link, &device_name);

	if(!NT_SUCCESS(status)) {
		IoDeleteDevice(p_control_device);
		destroy_pipe(&read_buf);
		destroy_pipe(&write_buf);
		return status;
	}

	//Regist IRP dispatch functions
	if(!add_device(p_control_device, dispatch_func, NULL)) {
		IoDeleteSymbolicLink(&symbol_link);
		IoDeleteDevice(p_control_device);
		destroy_pipe(&read_buf);
		destroy_pipe(&write_buf);
	}

	//Initialize locks
	KeInitializeMutex(&flag_lock, 0);

	p_control_device->Flags &= ~DO_DEVICE_INITIALIZING;

	return status;
}

VOID destroy_control_device()
{
	UNICODE_STRING str_symbolic_link
	    = RTL_CONSTANT_STRING(SYMBOLLINK_NAME);

	//Destroy buffers
	destroy_pipe(&read_buf);
	destroy_pipe(&write_buf);

	//Destroy device
	IoDeleteSymbolicLink(&str_symbolic_link);
	IoDeleteDevice(p_control_device);
	KdPrint(("sms:Control device deleted.\n"));
	remove_device(p_control_device);
	KdPrint(("sms:Control device removed.\n"));
	return;
}

NTSTATUS dispatch_func(PDEVICE_OBJECT p_device, PIRP p_irp)
{
	PIO_STACK_LOCATION p_irpsp = IoGetCurrentIrpStackLocation(p_irp);

	switch(p_irpsp->MajorFunction) {
	case IRP_MJ_CREATE:
		return dispatch_func_create(p_device, p_irp, p_irpsp);

	case IRP_MJ_CLOSE:
		return dispatch_func_close(p_device, p_irp, p_irpsp);

	case IRP_MJ_READ:
		return dispatch_func_read(p_device, p_irp, p_irpsp);

	case IRP_MJ_WRITE:
		return dispatch_func_write(p_device, p_irp, p_irpsp);

	case IRP_MJ_CLEANUP:
		p_irp->IoStatus.Information = 0;
		p_irp->IoStatus.Status = STATUS_SUCCESS;
		IoCompleteRequest(p_irp, IO_NO_INCREMENT);
		return STATUS_SUCCESS;

	default:
		p_irp->IoStatus.Information = 0;
		p_irp->IoStatus.Status = STATUS_INVALID_DEVICE_REQUEST;
		IoCompleteRequest(p_irp, IO_NO_INCREMENT);
		return STATUS_INVALID_DEVICE_REQUEST;
	}
}

NTSTATUS dispatch_func_create(PDEVICE_OBJECT p_device, PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	//Test if the device has been opened
	if(!NT_SUCCESS(
	       KeWaitForSingleObject(
	           &flag_lock,
	           Executive,
	           KernelMode,
	           FALSE,
	           0
	       ))) {
		p_irp->IoStatus.Information = 0;
		p_irp->IoStatus.Status = STATUS_DEVICE_BUSY;
		IoCompleteRequest(p_irp, IO_NO_INCREMENT);
		return STATUS_DEVICE_BUSY;
	}

	if(open_fag) {
		p_irp->IoStatus.Information = 0;
		p_irp->IoStatus.Status = STATUS_DEVICE_BUSY;
		IoCompleteRequest(p_irp, IO_NO_INCREMENT);
		KeReleaseMutex(&flag_lock, FALSE);
		return STATUS_DEVICE_BUSY;
	}

	//Complete irp
	p_irp->IoStatus.Information = 0;
	p_irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(p_irp, IO_NO_INCREMENT);
	open_fag = TRUE;
	KeReleaseMutex(&flag_lock, FALSE);
	UNREFERENCED_PARAMETER(p_device);
	UNREFERENCED_PARAMETER(p_irpsp);
	return STATUS_SUCCESS;
}

NTSTATUS dispatch_func_close(PDEVICE_OBJECT p_device, PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	UNICODE_STRING str_srv_name =
	    RTL_CONSTANT_STRING(SERVICE_NAME);
	UNICODE_STRING str_srv_path =
	    RTL_CONSTANT_STRING(DRIVER_SRV_REG_PATH);
	LARGE_INTEGER daemon_exit_time;
	NTSTATUS status;
	UNICODE_STRING full_service_path;

	p_irp->IoStatus.Information = 0;
	p_irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(p_irp, IO_NO_INCREMENT);

	//Test if daemon crashed
	daemon_exit_time = PsGetProcessExitTime();

	if(daemon_exit_time.QuadPart != 0) {
		RtlInitEmptyUnicodeString(
		    &full_service_path,
		    driver_full_name,
		    sizeof(driver_full_name));
		RtlUnicodeStringCopy(&full_service_path, &str_srv_path);
		RtlUnicodeStringCat(&full_service_path, &str_srv_name);
		status = ZwUnloadDriver(&full_service_path);
		KdPrint(("sms: Daemon proess has been crashed!\n"));
		BREAKIFNOT(NT_SUCCESS(status));
	}

	UNREFERENCED_PARAMETER(p_device);
	UNREFERENCED_PARAMETER(p_irpsp);
	return STATUS_SUCCESS;
}

NTSTATUS dispatch_func_read(PDEVICE_OBJECT p_device, PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	NTSTATUS status;
	SIZE_T length_read;

	//Read data
	status = read_pipe(
	             &read_buf,
	             p_irp->UserBuffer,
	             p_irpsp->Parameters.Read.Length,
	             &length_read);

	if(!NT_SUCCESS(status)) {
		p_irp->IoStatus.Information = 0;
		p_irp->IoStatus.Status = status;
		IoCompleteRequest(p_irp, IO_NO_INCREMENT);
		return status;
	}

	//Complete irp
	p_irp->IoStatus.Information = length_read;
	p_irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(p_irp, IO_NO_INCREMENT);
	UNREFERENCED_PARAMETER(p_device);
	return STATUS_SUCCESS;
}

NTSTATUS dispatch_func_write(PDEVICE_OBJECT p_device, PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	NTSTATUS status;

	//Write data
	status = write_pipe(
	             &write_buf,
	             p_irp->UserBuffer,
	             p_irpsp->Parameters.Write.Length);

	if(!NT_SUCCESS(status)) {
		p_irp->IoStatus.Information = 0;
		p_irp->IoStatus.Status = status;
		IoCompleteRequest(p_irp, IO_NO_INCREMENT);
		return status;
	}

	//Complete irp
	p_irp->IoStatus.Information = p_irpsp->Parameters.Write.Length;
	p_irp->IoStatus.Status = STATUS_SUCCESS;
	IoCompleteRequest(p_irp, IO_NO_INCREMENT);
	UNREFERENCED_PARAMETER(p_device);
	return STATUS_SUCCESS;
}