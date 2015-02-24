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

#include "filter.h"
#include "fs_filter.h"

BOOLEAN				filter_enable_flag = FALSE;
KSPIN_LOCK			filter_flag_lock;
PDRIVER_OBJECT		p_sms_driver_object;
ULONG				filter_thread_num;

NTSTATUS install_filter_device(PDRIVER_OBJECT p_driver_object)
{
	if(filter_enable_flag) {
		return STATUS_SUCCESS;
	}

	filter_thread_num = 0;

	KeInitializeSpinLock(&filter_flag_lock);

	p_sms_driver_object = p_driver_object;

	init_volume_filter_module();

	return install_fs_filter_device(p_driver_object);
}

VOID enable_filter()
{
	KIRQL irql;
	KeAcquireSpinLock(&filter_flag_lock, &irql);
	filter_enable_flag = TRUE;
	KeReleaseSpinLock(&filter_flag_lock, irql);
	return;
}

VOID disable_filter()
{
	KIRQL irql;
	KeAcquireSpinLock(&filter_flag_lock, &irql);
	filter_enable_flag = FALSE;
	KeReleaseSpinLock(&filter_flag_lock, irql);
	return;
}

VOID uninstall_filter_device()
{
	KIRQL irql;

	//Disable filter module
	KeAcquireSpinLock(&filter_flag_lock, &irql);
	filter_enable_flag = FALSE;
	KeReleaseSpinLock(&filter_flag_lock, irql);
	uninstall_fs_filter_device();
	return;
}

NTSTATUS pass_through(PDEVICE_OBJECT p_device, PIRP p_irp)
{
	IoSkipCurrentIrpStackLocation(p_irp);
	return IoCallDriver(((pfilter_device_ext)
	                     p_device->DeviceExtension)->attached_to_device_object, p_irp);
}

BOOLEAN is_dev_attached(PDEVICE_OBJECT dev_obj)
{
	PDEVICE_OBJECT current_dev_obj;
	PDEVICE_OBJECT next_dev_obj;

	//Get the device object at the TOP of the attachment chain.
	current_dev_obj = IoGetAttachedDeviceReference(dev_obj);

	while(current_dev_obj != NULL) {

		//If the device object is my device object
		if(current_dev_obj != NULL
		   && current_dev_obj->DriverObject == p_sms_driver_object
		   && current_dev_obj->DeviceExtension != NULL) {
			ObDereferenceObject(current_dev_obj);
			return TRUE;

		}

		//Get next device
		next_dev_obj = IoGetLowerDeviceObject(current_dev_obj);
		ObDereferenceObject(current_dev_obj);

		current_dev_obj = next_dev_obj;
	}

	return FALSE;
}

VOID get_base_device_object_name(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name)
{
	DeviceObject = IoGetDeviceAttachmentBaseRef(DeviceObject);
	get_object_name(DeviceObject, Name);
	ObDereferenceObject(DeviceObject);
	return;
}