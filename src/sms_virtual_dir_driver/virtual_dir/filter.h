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

#ifndef FILTER_H_INCLUDE
#define	FILTER_H_INCLUDE

#include "virtual_dir.h"
#include "../common.h"

#define		MAX_DEVNAME_LENGTH					64

typedef struct _filter_device_ext {

	//Pointer to the file system device object we are attached to.
	PDEVICE_OBJECT	attached_to_device_object;

	//Pointer to the real (disk) device object that is associated with
	//the file system device object we are attached to.
	PDEVICE_OBJECT	storage_stack_device_object;

	//Name for this device.  If attached to a Volume Device Object it is the
	//name of the physical disk drive.  If attached to a Control Device
	//Object it is the name of the Control Device Object.

	UNICODE_STRING	device_name;

	//Buffer used to hold the above unicode strings
	WCHAR			device_name_buffer[MAX_DEVNAME_LENGTH];
} filter_device_ext, *pfilter_device_ext;

extern	BOOLEAN			filter_enable_flag;
extern	KSPIN_LOCK		filter_flag_lock;
extern	PDRIVER_OBJECT	p_sms_driver_object;
extern	ULONG			filter_thread_num;


NTSTATUS	install_filter_device(PDRIVER_OBJECT p_driver_object);
VOID		uninstall_filter_device();
VOID		enable_filter();
VOID		disable_filter();
NTSTATUS	pass_through(PDEVICE_OBJECT p_device, PIRP p_irp);
BOOLEAN		is_dev_attached(PDEVICE_OBJECT dev_obj);
VOID		get_base_device_object_name(
    IN PDEVICE_OBJECT DeviceObject,
    IN OUT PUNICODE_STRING Name);

#endif // !FILTER_H_INCLUDE
