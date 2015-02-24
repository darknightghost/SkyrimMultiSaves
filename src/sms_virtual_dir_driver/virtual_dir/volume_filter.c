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

#include "volume_filter.h"
#include "../data_structure/list.h"
#include "../device.h"
#include <ntdddisk.h>

static	list				vol_filter_dev_list = NULL;
static	KMUTEX				vol_filter_list_lock;
static	KMUTEX				vol_attach_lock;
static	ULONG				filtered_thread_num;
static	FAST_IO_DISPATCH	fast_io_dispatch_table;

static	NTSTATUS			dispatch_func(PDEVICE_OBJECT p_device, PIRP p_irp);

static	NTSTATUS			attach_volume(
    IN PDEVICE_OBJECT p_vol_dev_obj,
    PDEVICE_OBJECT storage_stack_dev_obj);
static	NTSTATUS			is_shadow_copy_volume(
    IN	PDEVICE_OBJECT DeviceObject,
    OUT	PBOOLEAN p_ret);

VOID init_volume_filter_module()
{
	filtered_thread_num = 0;
	KeInitializeMutex(&vol_filter_list_lock, 0);
	KeInitializeMutex(&vol_attach_lock, 0);

	RtlZeroMemory(&fast_io_dispatch_table, sizeof(fast_io_dispatch_table));
	return;
}

NTSTATUS attach_volumes(
    IN PDEVICE_OBJECT p_fs_dev_obj,
    IN PUNICODE_STRING fs_name)
{
	NTSTATUS status;
	ULONG vol_dev_num;
	ULONG i;
	PDEVICE_OBJECT* vol_dev_obj_buf;
	SIZE_T vol_dev_buf_size;
	PDEVICE_OBJECT storage_stack_dev_obj;
	BOOLEAN shadow_copy_volume_flag;

	//Get the number of volume devices
	status = IoEnumerateDeviceObjectList(
	             p_fs_dev_obj->DriverObject,
	             NULL,
	             0,
	             &vol_dev_num);

	if(!NT_SUCCESS(status)
	   && status != STATUS_BUFFER_TOO_SMALL) {
		return status;
	}

	vol_dev_buf_size = vol_dev_num * sizeof(PDEVICE_OBJECT);

	//Get volume devices
	vol_dev_obj_buf = get_memory(vol_dev_buf_size, *((PULONG)"sms"), NonPagedPool);

	if(vol_dev_obj_buf == NULL) {
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	status = IoEnumerateDeviceObjectList(
	             p_fs_dev_obj->DriverObject,
	             vol_dev_obj_buf,
	             (ULONG)vol_dev_buf_size,
	             &vol_dev_num);

	if(!NT_SUCCESS(status)) {
		free_memory(vol_dev_obj_buf);
		return status;
	}

	MEM_CHK(vol_dev_obj_buf);

	for(i = 0; i < vol_dev_num; i++) {

		//Check if the device has been attached
		if((vol_dev_obj_buf[i] == p_fs_dev_obj) ||
		   (vol_dev_obj_buf[i]->DeviceType != p_fs_dev_obj->DeviceType) ||
		   is_dev_attached(vol_dev_obj_buf[i])) {
			ObDereferenceObject(vol_dev_obj_buf[i]);
			continue;
		}

		//Check if the device has a name.If sure ,it must be a control device.
		//Ignore it.
		get_base_device_object_name(vol_dev_obj_buf[i], fs_name);

		if(fs_name->Length > 0) {
			ObDereferenceObject(vol_dev_obj_buf[i]);
			continue;
		}

		//Check if the file sysytem device object is associate with a disk.If not,ignore.
		storage_stack_dev_obj = NULL;
		status = IoGetDiskDeviceObject(vol_dev_obj_buf[i], &storage_stack_dev_obj);

		if(!NT_SUCCESS(status)) {
			ObDereferenceObject(vol_dev_obj_buf[i]);

			if(storage_stack_dev_obj != NULL) {
				ObDereferenceObject(storage_stack_dev_obj);
			}

			continue;
		}

		//Ignore any shadow copy volumes
		if(!NT_SUCCESS(is_shadow_copy_volume(storage_stack_dev_obj, &shadow_copy_volume_flag))
		   || shadow_copy_volume_flag
		  ) {
			ObDereferenceObject(storage_stack_dev_obj);
			ObDereferenceObject(vol_dev_obj_buf[i]);
			continue;
		}

		//Attach volume
		status = attach_volume(vol_dev_obj_buf[i], storage_stack_dev_obj);

		ObDereferenceObject(storage_stack_dev_obj);
		ObDereferenceObject(vol_dev_obj_buf[i]);
	}

	free_memory(vol_dev_obj_buf);
	return STATUS_SUCCESS;
}

VOID clean_volume_filter_devices()
{
	NTSTATUS status;
	PDEVICE_OBJECT p_dev;
	BOOLEAN remove_ret;
	pfilter_device_ext p_dev_ext;

	if(filter_enable_flag) {
		return;
	}

	while(filtered_thread_num > 0) {
		sleep(100);
	}

	status = KeWaitForSingleObject(
	             &vol_filter_list_lock,
	             Executive,
	             KernelMode,
	             FALSE,
	             NULL
	         );

	ASSERT(NT_SUCCESS(status));

	//Clean devices
	while(vol_filter_dev_list != NULL) {
		p_dev = vol_filter_dev_list->p_item;
		p_dev_ext = p_dev->DeviceExtension;
		IoDetachDevice(p_dev_ext->attached_to_device_object);
		sleep(1000);
		IoDeleteDevice(p_dev);
		remove_device(p_dev);
		remove_ret = list_remove_item(&vol_filter_dev_list, p_dev);
		KdPrint(("sms:Detached volume:%wZ\n", &(p_dev_ext->device_name)));
		ASSERT(remove_ret);
	}

	KeReleaseMutex(&vol_filter_list_lock, FALSE);
	return;
}

NTSTATUS is_shadow_copy_volume(
    IN	PDEVICE_OBJECT DeviceObject,
    OUT	PBOOLEAN p_ret)
{
	PIRP irp;
	KEVENT event;
	IO_STATUS_BLOCK iosb;
	NTSTATUS status;

	*p_ret = FALSE;

	if(FILE_DEVICE_VIRTUAL_DISK != DeviceObject->DeviceType) {

		return STATUS_SUCCESS;
	}

	KeInitializeEvent(&event, NotificationEvent, FALSE);

	irp = IoBuildDeviceIoControlRequest(IOCTL_DISK_IS_WRITABLE,
	                                    DeviceObject,
	                                    NULL,
	                                    0,
	                                    NULL,
	                                    0,
	                                    FALSE,
	                                    &event,
	                                    &iosb);

	//If we could not allocate an IRP, return an error
	if(irp == NULL) {

		return STATUS_INSUFFICIENT_RESOURCES;
	}

	//  Call the storage stack and see if this is readonly
	status = IoCallDriver(DeviceObject, irp);

	if(status == STATUS_PENDING) {
		status = KeWaitForSingleObject(&event,
		                               Executive,
		                               KernelMode,
		                               FALSE,
		                               NULL);

		ASSERT(NT_SUCCESS(status));
		status = iosb.Status;
	}

	//  If the media is write protected then this is a shadow copy volume
	if(STATUS_MEDIA_WRITE_PROTECTED == status) {

		*p_ret = TRUE;
		status = STATUS_SUCCESS;
	}

	//  Return the status of the IOCTL.  IsShadowCopy is already set to FALSE
	//  which is what we want if STATUS_SUCCESS was returned or if an error
	//  was returned.
	return status;
}

NTSTATUS attach_volume(
    IN PDEVICE_OBJECT p_vol_dev_obj,
    PDEVICE_OBJECT storage_stack_dev_obj)
{
	NTSTATUS status;
	PDEVICE_OBJECT p_new_dev;
	pfilter_device_ext p_new_dev_ext;

	status = IoCreateDevice(p_sms_driver_object,
	                        sizeof(filter_device_ext),
	                        NULL,
	                        p_vol_dev_obj->DeviceType,
	                        0,
	                        FALSE,
	                        &p_new_dev);

	if(!NT_SUCCESS(status)) {
		return status;
	}

	//Set disk device object
	p_new_dev_ext = p_new_dev->DeviceExtension;
	p_new_dev_ext->attached_to_device_object = p_vol_dev_obj;
	p_new_dev_ext->storage_stack_device_object = storage_stack_dev_obj;

	//Set storage stack device name
	RtlInitEmptyUnicodeString(&p_new_dev_ext->device_name,
	                          p_new_dev_ext->device_name_buffer,
	                          sizeof(p_new_dev_ext->device_name_buffer));

	get_object_name(storage_stack_dev_obj,
	                &p_new_dev_ext->device_name);

	KeWaitForSingleObject(
	    &vol_attach_lock,
	    Executive,
	    KernelMode,
	    FALSE,
	    NULL
	);

	if(!is_dev_attached(p_vol_dev_obj)) {

		//Attach to volume.
		while(p_vol_dev_obj->Flags & DO_DEVICE_INITIALIZING) {
			sleep(100);
		}

		if(FlagOn(p_vol_dev_obj->Flags, DO_BUFFERED_IO)) {

			SetFlag(p_new_dev->Flags, DO_BUFFERED_IO);
		}

		if(FlagOn(p_vol_dev_obj->Flags, DO_DIRECT_IO)) {

			SetFlag(p_new_dev->Flags, DO_DIRECT_IO);
		}

		status = IoAttachDeviceToDeviceStackSafe(
		             p_new_dev,
		             p_vol_dev_obj,
		             &(p_new_dev_ext->attached_to_device_object));

		if(!NT_SUCCESS(status)) {
			IoDeleteDevice(p_new_dev);
			KeReleaseMutex(&vol_attach_lock, FALSE);
			return status;
		}

	} else {

		//We were already attached, cleanup this device object.
		IoDeleteDevice(p_new_dev);
		KeReleaseMutex(&vol_attach_lock, FALSE);
		return STATUS_SUCCESS;
	}

	//Add new device to volume filter device list
	status = KeWaitForSingleObject(
	             &vol_filter_list_lock,
	             Executive,
	             KernelMode,
	             FALSE,
	             NULL
	         );
	ASSERT(NT_SUCCESS(status));

	if(list_add_item(&vol_filter_dev_list, p_new_dev) == NULL) {
		IoDetachDevice(p_new_dev_ext->attached_to_device_object);
		IoDeleteDevice(p_new_dev);
		KeReleaseMutex(&vol_filter_list_lock, FALSE);
		KeReleaseMutex(&vol_attach_lock, FALSE);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	//Add new device to dispatch list
	if(!add_device(p_new_dev, dispatch_func, &fast_io_dispatch_table)) {
		list_remove_item(&vol_filter_dev_list, p_new_dev);
		IoDetachDevice(p_new_dev_ext->attached_to_device_object);
		IoDeleteDevice(p_new_dev);
		KeReleaseMutex(&vol_filter_list_lock, FALSE);
		KeReleaseMutex(&vol_attach_lock, FALSE);
		return STATUS_INSUFFICIENT_RESOURCES;
	}

	KeReleaseMutex(&vol_filter_list_lock, FALSE);

	ClearFlag(p_new_dev->Flags, DO_DEVICE_INITIALIZING);

	KeReleaseMutex(&vol_attach_lock, FALSE);
	KdPrint(("sms:Attached to volume:%wZ\n", &(p_new_dev_ext->device_name)));
	return STATUS_SUCCESS;
}

NTSTATUS dispatch_func(PDEVICE_OBJECT p_device, PIRP p_irp)
{
	KIRQL irql;
	NTSTATUS status;
	PIO_STACK_LOCATION p_irpsp = IoGetCurrentIrpStackLocation(p_irp);

	KeAcquireSpinLock(&filter_flag_lock, &irql);

	if(filter_enable_flag) {
		//If the filter is enabled
		filter_thread_num++;
		filtered_thread_num++;
		KeReleaseSpinLock(&filter_flag_lock, irql);

		//Dispatch IRP
		switch(p_irpsp->MajorFunction) {
		default:
			status = pass_through(p_device, p_irp);
		}

		filtered_thread_num--;
		filter_thread_num--;

		return STATUS_SUCCESS;

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