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

#include "dir.h"

//IRP_MN_NOTIFY_CHANGE_DIRECTORY
static	NTSTATUS			mino_notify_change_directory(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp);

//IRP_MN_QUERY_DIRECTORY
//FileBothDirectoryInformation
static	NTSTATUS			mino_query_file_both_dir_info(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp);

//IRP_MN_QUERY_DIRECTORY
//FileDirectoryInformation
static	NTSTATUS			mino_query_file_dir_info(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp);

//IRP_MN_QUERY_DIRECTORY
//FileFullDirectoryInformation
static	NTSTATUS			mino_query_file_full_dir_info(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp);

//IRP_MN_QUERY_DIRECTORY
//FileIdBothDirectoryInformation
static	NTSTATUS			mino_query_file_id_both_dir_info(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp);

//IRP_MN_QUERY_DIRECTORY
//FileIdFullDirectoryInformation
static	NTSTATUS			mino_query_file_id_full_dir_info(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp);

//IRP_MN_QUERY_DIRECTORY
//FileNamesInformation
static	NTSTATUS			mino_query_file_names_info(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp);

//IRP_MN_QUERY_DIRECTORY
//FileObjectIdInformation
static	NTSTATUS			mino_query_file_obj_id_info(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp);

//IRP_MN_QUERY_DIRECTORY
//FileReparsePointInformation
static	NTSTATUS			mino_query_file_reparse_point_info(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp);




NTSTATUS dispatch_func_dir_control(PDEVICE_OBJECT p_device,
                                   PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(p_device);
	UNREFERENCED_PARAMETER(p_irp);
	UNREFERENCED_PARAMETER(p_irpsp);
	return status;
}

NTSTATUS mino_notify_change_directory(PDEVICE_OBJECT p_device,
                                      PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(p_device);
	UNREFERENCED_PARAMETER(p_irp);
	UNREFERENCED_PARAMETER(p_irpsp);
	return status;
}

NTSTATUS mino_query_file_both_dir_info(PDEVICE_OBJECT p_device,
                                       PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(p_device);
	UNREFERENCED_PARAMETER(p_irp);
	UNREFERENCED_PARAMETER(p_irpsp);
	return status;
}

NTSTATUS mino_query_file_dir_info(PDEVICE_OBJECT p_device,
                                  PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(p_device);
	UNREFERENCED_PARAMETER(p_irp);
	UNREFERENCED_PARAMETER(p_irpsp);
	return status;
}

NTSTATUS mino_query_file_full_dir_info(PDEVICE_OBJECT p_device,
                                       PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(p_device);
	UNREFERENCED_PARAMETER(p_irp);
	UNREFERENCED_PARAMETER(p_irpsp);
	return status;
}

NTSTATUS mino_query_file_id_both_dir_info(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(p_device);
	UNREFERENCED_PARAMETER(p_irp);
	UNREFERENCED_PARAMETER(p_irpsp);
	return status;
}

NTSTATUS mino_query_file_id_full_dir_info(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(p_device);
	UNREFERENCED_PARAMETER(p_irp);
	UNREFERENCED_PARAMETER(p_irpsp);
	return status;
}

NTSTATUS mino_query_file_names_info(PDEVICE_OBJECT p_device,
                                    PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(p_device);
	UNREFERENCED_PARAMETER(p_irp);
	UNREFERENCED_PARAMETER(p_irpsp);
	return status;
}

NTSTATUS mino_query_file_obj_id_info(PDEVICE_OBJECT p_device,
                                     PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(p_device);
	UNREFERENCED_PARAMETER(p_irp);
	UNREFERENCED_PARAMETER(p_irpsp);
	return status;
}

NTSTATUS mino_query_file_reparse_point_info(PDEVICE_OBJECT p_device,
        PIRP p_irp, PIO_STACK_LOCATION p_irpsp)
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(p_device);
	UNREFERENCED_PARAMETER(p_irp);
	UNREFERENCED_PARAMETER(p_irpsp);
	return status;
}
