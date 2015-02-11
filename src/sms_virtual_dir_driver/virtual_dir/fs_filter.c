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

#include "fs_filter.h"
#include "../device.h"
#include "../data_structure//list.h"

static	list				fs_filter_dev_list = NULL;
static	KMUTEX				fs_filter_list_lock;
static	ULONG				filtered_thread_num;
static	FAST_IO_DISPATCH	fast_io_dispatch_table;

static	VOID			fs_change_callback(
    IN	PDEVICE_OBJECT DeviceObject,
    IN	BOOLEAN FsActive);
static	VOID			attach_fs(
    IN	PDEVICE_OBJECT DeviceObject,
    IN	BOOLEAN FsActive);

static	NTSTATUS		dispatch_func(PDEVICE_OBJECT p_device, PIRP p_irp);


NTSTATUS install_fs_filter_device(PDRIVER_OBJECT p_driver_object)
{
	NTSTATUS status;

	filtered_thread_num = 0;

	KeInitializeMutex(&fs_filter_list_lock, 0);

	//Regist attach callback
	status = IoRegisterFsRegistrationChange(
	             p_driver_object, fs_change_callback);

	if(!NT_SUCCESS(status)) {
		return status;
	}

	RtlZeroMemory(&fast_io_dispatch_table, sizeof(fast_io_dispatch_table));

	return STATUS_SUCCESS;
}
VOID uninstall_fs_filter_device()
{
	NTSTATUS status;
	PDEVICE_OBJECT p_dev;
	BOOLEAN remove_ret;
	pfilter_device_ext p_dev_ext;

	if(filter_enable_flag) {
		return;
	}

	IoUnregisterFsRegistrationChange(
	    p_sms_driver_object, fs_change_callback);

	while(filtered_thread_num > 0) {
		sleep(100);
	}

	status = KeWaitForSingleObject(
	             &fs_filter_list_lock,
	             Executive,
	             KernelMode,
	             FALSE,
	             NULL
	         );

	ASSERT(NT_SUCCESS(status));

	//Clean devices
	while(fs_filter_dev_list != NULL) {
		p_dev = fs_filter_dev_list->p_item;
		p_dev_ext = p_dev->DeviceExtension;
		IoDetachDevice(p_dev_ext->attached_to_device_object);
		sleep(1000);
		remove_device(p_dev);
		IoDeleteDevice(p_dev);
		remove_ret = list_remove_item(&fs_filter_dev_list, p_dev);
		KdPrint(("sms:Detached to filesystem %p\"%wZ\".\n", p_dev, &(p_dev_ext->device_name)));
		ASSERT(remove_ret);
	}

	clean_volume_filter_devices();
	KeReleaseMutex(&fs_filter_list_lock, FALSE);
	return;

}


VOID fs_change_callback(
    IN	PDEVICE_OBJECT DeviceObject,
    IN	BOOLEAN FsActive)
{
	attach_fs(DeviceObject, FsActive);

	return;
}

VOID attach_fs(
    IN	PDEVICE_OBJECT DeviceObject,
    IN	BOOLEAN FsActive)
{
	NTSTATUS status;
	pfilter_device_ext p_dev_ext;
	PDEVICE_OBJECT p_new_device;
	UNICODE_STRING fsrec_name;
	UNICODE_STRING fs_name;
	WCHAR tmp_name_buf[MAX_DEVNAME_LENGTH];

	//Check the type of the device.
	if(DeviceObject->DeviceType != FILE_DEVICE_DISK_FILE_SYSTEM) {
		return;
	}

	RtlInitEmptyUnicodeString(&fs_name,
	                          tmp_name_buf,
	                          sizeof(tmp_name_buf));

	//See if this is one of the standard Microsoft file system recognizer
	//devices.If so skip it.
	RtlInitUnicodeString(&fsrec_name, L"\\FileSystem\\Fs_Rec");
	get_object_name(DeviceObject->DriverObject, &fs_name);

	if(RtlCompareUnicodeString(&fs_name, &fsrec_name, TRUE) == 0) {
		return;
	}

	//Create FDO
	status = IoCreateDevice(
	             p_sms_driver_object,
	             sizeof(filter_device_ext),
	             NULL,
	             DeviceObject->DeviceType,
	             0,
	             FALSE,
	             &p_new_device);

	if(!NT_SUCCESS(status)) {
		return;
	}

	//Copy flags
	if(FlagOn(DeviceObject->Flags, DO_BUFFERED_IO)) {
		SetFlag(p_new_device->Flags, DO_BUFFERED_IO);
	}

	if(FlagOn(DeviceObject->Flags, DO_DIRECT_IO)) {
		SetFlag(p_new_device->Flags, DO_DIRECT_IO);
	}

	if(FlagOn(DeviceObject->Characteristics, FILE_DEVICE_SECURE_OPEN)) {
		SetFlag(p_new_device->Characteristics, FILE_DEVICE_SECURE_OPEN);
	}

	p_dev_ext = p_new_device->DeviceExtension;
	RtlInitEmptyUnicodeString(&p_dev_ext->device_name,
	                          p_dev_ext->device_name_buffer,
	                          sizeof(p_dev_ext->device_name_buffer));

	RtlCopyUnicodeString(&(p_dev_ext->device_name), &(fs_name));

	//Add device to dispatch list
	if(!add_device(p_new_device, dispatch_func, &fast_io_dispatch_table)) {
		IoDeleteDevice(p_new_device);
		return;
	}

	//Attach device
	status = IoAttachDeviceToDeviceStackSafe(
	             p_new_device,
	             DeviceObject,
	             &(p_dev_ext->attached_to_device_object));

	if(!NT_SUCCESS(status)) {
		remove_device(p_new_device);
		IoDeleteDevice(p_new_device);
		return;
	}

	p_new_device->Flags &= ~DO_DEVICE_INITIALIZING;

	status = KeWaitForSingleObject(
	             &fs_filter_list_lock,
	             Executive,
	             KernelMode,
	             FALSE,
	             NULL
	         );

	if(!NT_SUCCESS(status)) {
		IoDetachDevice(p_dev_ext->attached_to_device_object);
		remove_device(p_new_device);
		IoDeleteDevice(p_new_device);
		return;
	}

	//Add new device to list
	if(list_add_item(&fs_filter_dev_list, p_new_device) == NULL) {
		IoDetachDevice(p_dev_ext->attached_to_device_object);
		remove_device(p_new_device);
		IoDeleteDevice(p_new_device);
		KeReleaseMutex(&fs_filter_list_lock, FALSE);
		return;
	}

	KeReleaseMutex(&fs_filter_list_lock, FALSE);

	KdPrint(("sms:Attached to filesystem %p\"%wZ\".\n", DeviceObject, &(p_dev_ext->device_name)));

	attach_volumes(DeviceObject, &fs_name);

	UNREFERENCED_PARAMETER(FsActive);
	return;
}

NTSTATUS dispatch_func(PDEVICE_OBJECT p_device, PIRP p_irp)
{
	KIRQL irql;
	NTSTATUS status;
	PIO_STACK_LOCATION p_irpsp = IoGetCurrentIrpStackLocation(p_irp);

	KeAcquireSpinLock(&filter_flag_lock, &irql);

	if(filter_enable_flag) {
		//If the filter is enabled
		filtered_thread_num++;
		filter_thread_num++;
		KeReleaseSpinLock(&filter_flag_lock, irql);

		//Dispatch IRP
		switch(p_irpsp->MajorFunction) {
		default:
			status = pass_through(p_device, p_irp);
		}

		filtered_thread_num--;
		filter_thread_num--;

		return status;

	} else {
		//If the filter is disabled
		filter_thread_num++;
		KeReleaseSpinLock(&filter_flag_lock, irql);
		//Pass through
		status = pass_through(p_device, p_irp);
		filter_thread_num--;
		return status;
	}
}