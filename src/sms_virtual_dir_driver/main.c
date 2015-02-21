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

#include "common.h"
#include "virtual_dir/filter.h"
#include "gate/control_device.h"
#include "gate/gate.h"
#include "../common/driver/device_name.h"
#include "device.h"

VOID DriverUnload(PDRIVER_OBJECT driver);

NTSTATUS DriverEntry(
    PDRIVER_OBJECT DriverObject,		//In
    PUNICODE_STRING	RegistryPath		//In
)
{
	NTSTATUS status;
	BREAKPOINT;
	KdPrint(("sms:Kernel module sms_virtual_dir_drive starting...\n"));

	//Initialize IRP and Fast I/O dispatch module
	if(!init_dispatch_func(DriverObject)) {
		KdPrint(("sms:Init dispatch module failed.\n"));
		return STATUS_UNSUCCESSFUL;
	}

	/*//Initialize filter device
	status = install_filter_device(DriverObject);

	if(!NT_SUCCESS(status)) {
		KdPrint(("sms:Init filter devices failed.\n"));
		destroy_control_device();
		clean_dispatch_func(DriverObject);
		return status;
	}*/

	//Start command interpreter
	status = start_call_gate(DriverObject);

	if(!NT_SUCCESS(status)) {
		KdPrint(("sms:Init command interpreter failed.\n"));
		destroy_control_device();
		uninstall_filter_device();
		clean_dispatch_func(DriverObject);
		return status;
	}

	DriverObject->DriverUnload = DriverUnload;
	UNREFERENCED_PARAMETER(RegistryPath);
	KdPrint(("sms:Kernel module sms_virtual_dir_drive started.\n"));
	return status;
}

VOID DriverUnload(PDRIVER_OBJECT driver)
{
	BREAKPOINT;

	sleep(1000);
	stop_call_gate();
	/*uninstall_filter_device();*/
	clean_dispatch_func(driver);
	KdPrint(("sms:Kernel module sms_virtual_dir_drive unloaded.\n"));
	MEM_LEAK_CHK;
	return;
}