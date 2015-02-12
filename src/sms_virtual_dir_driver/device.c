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

#include "device.h"
#include "data_structure/list.h"

#pragma warning(disable:4127)

static	list				device_list = NULL;
static	BOOLEAN				dev_list_readable_flag;
static	UINT16				dev_list_count;
static	KSPIN_LOCK			device_list_lock;

static	NTSTATUS			dispatch_func(PDEVICE_OBJECT p_device, PIRP p_irp);
static	BOOLEAN				remove_condition(PVOID p_item, PVOID p_arg);
static	PFAST_IO_DISPATCH	find_fast_io_by_device(PDEVICE_OBJECT p_device);

//Fast I/O functions
static	BOOLEAN				fast_io_check_if_possible_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_read_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_write_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_query_basic_info_func(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_query_standard_info_func(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_lock(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_unlock_single_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_unlock_all_func(
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_unlock_all_by_key_func(
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_device_control_func(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

static	VOID				fast_io_detach_device_func(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
);

static	BOOLEAN				fast_io_query_network_open_info_func(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_mdl_read_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);


static	BOOLEAN				fast_io_mdl_read_complete_func(
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_prepare_mdl_write_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_mdl_write_complete_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_read_compressed_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_write_compressed_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_mdl_read_complete_compressed_func(
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_mdl_write_complete_compressed_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
);

static	BOOLEAN				fast_io_query_open_func(
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    IN PDEVICE_OBJECT DeviceObject
);

BOOLEAN init_dispatch_func(PDRIVER_OBJECT p_driver_object)
{
	INT i;
	PFAST_IO_DISPATCH p_fast_io_dispatch;

	KeInitializeSpinLock(&device_list_lock);
	dev_list_readable_flag = TRUE;
	dev_list_count = 0;

	//Fill IRP Dispatch functions
	for(i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		p_driver_object->MajorFunction[i] = dispatch_func;
	}

	//Fill Fast I/O functions
	p_fast_io_dispatch =
	    get_memory(sizeof(FAST_IO_DISPATCH), *(PULONG)"sms", NonPagedPool);

	if(p_fast_io_dispatch == NULL) {
		return FALSE;
	}

	RtlZeroMemory(p_fast_io_dispatch, sizeof(FAST_IO_DISPATCH));

	p_fast_io_dispatch->SizeOfFastIoDispatch = sizeof(FAST_IO_DISPATCH);
	p_fast_io_dispatch->FastIoCheckIfPossible = fast_io_check_if_possible_func;
	p_fast_io_dispatch->FastIoRead = fast_io_read_func;
	p_fast_io_dispatch->FastIoWrite = fast_io_write_func;
	p_fast_io_dispatch->FastIoQueryBasicInfo = fast_io_query_basic_info_func;
	p_fast_io_dispatch->FastIoQueryStandardInfo = fast_io_query_standard_info_func;
	p_fast_io_dispatch->FastIoLock = fast_io_lock;
	p_fast_io_dispatch->FastIoUnlockSingle = fast_io_unlock_single_func;
	p_fast_io_dispatch->FastIoUnlockAll = fast_io_unlock_all_func;
	p_fast_io_dispatch->FastIoUnlockAllByKey = fast_io_unlock_all_by_key_func;
	p_fast_io_dispatch->FastIoDeviceControl = fast_io_device_control_func;
	p_fast_io_dispatch->FastIoDetachDevice = fast_io_detach_device_func;
	p_fast_io_dispatch->FastIoQueryNetworkOpenInfo = fast_io_query_network_open_info_func;
	p_fast_io_dispatch->MdlRead = fast_io_mdl_read_func;
	p_fast_io_dispatch->MdlReadComplete = fast_io_mdl_read_complete_func;
	p_fast_io_dispatch->PrepareMdlWrite = fast_io_prepare_mdl_write_func;
	p_fast_io_dispatch->MdlWriteComplete = fast_io_mdl_write_complete_func;
	p_fast_io_dispatch->FastIoReadCompressed = fast_io_read_compressed_func;
	p_fast_io_dispatch->FastIoWriteCompressed = fast_io_write_compressed_func;
	p_fast_io_dispatch->MdlReadCompleteCompressed = fast_io_mdl_read_complete_compressed_func;
	p_fast_io_dispatch->MdlWriteCompleteCompressed = fast_io_mdl_write_complete_compressed_func;
	p_fast_io_dispatch->FastIoQueryOpen = fast_io_query_open_func;

	p_driver_object->FastIoDispatch = p_fast_io_dispatch;
	return TRUE;
}

VOID clean_dispatch_func(PDRIVER_OBJECT p_driver_object)
{
	free_memory(p_driver_object->FastIoDispatch);
	p_driver_object->FastIoDispatch = NULL;
	return;
}

BOOLEAN add_device(PDEVICE_OBJECT p_device, NTSTATUS(*p_dispatch_func)(PDEVICE_OBJECT, PIRP), PFAST_IO_DISPATCH p_fast_io_dispatch)
{
	pdispatch_table p_new_item;
	KIRQL irql;
	PFAST_IO_DISPATCH p_new_fast_io_dispatch;

	KeAcquireSpinLock(&device_list_lock, &irql);
	dev_list_readable_flag = FALSE;
	KeReleaseSpinLock(&device_list_lock, irql);

	while(dev_list_count > 0) {
		sleep(100);
	}

	//Allocate memory for new item
	p_new_item = get_memory(sizeof(dispatch_table), *(PULONG)"sms", NonPagedPool);

	if(p_new_item == NULL) {
		dev_list_readable_flag = TRUE;
		return FALSE;
	}

	p_new_item->p_device = p_device;
	p_new_item->dispatch_func = p_dispatch_func;

	if(p_fast_io_dispatch == NULL) {
		p_new_item->p_fast_io_dispatch = NULL;

	} else {
		//Allocate memory for Fast I/O
		p_new_fast_io_dispatch =
		    get_memory(sizeof(FAST_IO_DISPATCH), *(PULONG)"sms", NonPagedPool);

		if(p_new_fast_io_dispatch == NULL) {
			free_memory(p_new_item);
			dev_list_readable_flag = TRUE;
			return FALSE;
		}

		RtlCopyMemory(p_new_fast_io_dispatch, p_fast_io_dispatch, sizeof(FAST_IO_DISPATCH));
		p_new_item->p_fast_io_dispatch = p_new_fast_io_dispatch;
	}

	//Insert item
	if(list_add_item(&device_list, p_new_item) == NULL) {
		if(p_new_item->p_fast_io_dispatch != NULL) {
			free_memory(p_new_item->p_fast_io_dispatch);
		}

		free_memory(p_new_item);
		dev_list_readable_flag = TRUE;
		return FALSE;
	}

	dev_list_readable_flag = TRUE;
	return TRUE;
}

VOID remove_device(PDEVICE_OBJECT p_device)
{
	KIRQL irql;
	pdispatch_table p_item;

	KdPrint(("sms:Removing device %P from dispatch list.\n", p_device));
	KeAcquireSpinLock(&device_list_lock, &irql);
	dev_list_readable_flag = FALSE;
	KeReleaseSpinLock(&device_list_lock, irql);

	while(dev_list_count > 0) {
		KdPrint(("sms:The reference count of,dispatch list is %u.\n", dev_list_count));
		sleep(1000);
	}

	//Remove item
	p_item = (pdispatch_table)list_remove_item_by_maching(&device_list, remove_condition, p_device);

	if(p_item != NULL) {
		if(p_item->p_fast_io_dispatch != NULL) {
			free_memory(p_item->p_fast_io_dispatch);
		}

		free_memory(p_item);
	}

	dev_list_readable_flag = TRUE;

	return;
}

NTSTATUS dispatch_func(PDEVICE_OBJECT p_device, PIRP p_irp)
{
	KIRQL irql;
	plist_node p_node;
	NTSTATUS status;
	pdispatch_func p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for the destination of IRP
	p_node = device_list;

	if(p_node != NULL) {
		do {
			if(((pdispatch_table)p_node->p_item)->p_device == p_device) {
				p_func = ((pdispatch_table)p_node->p_item)->dispatch_func;
				dev_list_count--;
				status = p_func(p_device, p_irp);
				return status;
			}

			p_node = p_node->p_next;
		} while(p_node != device_list);
	}

	dev_list_count--;

	//There's no device for the IRP
	p_irp->IoStatus.Information = 0;
	p_irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
	IoCompleteRequest(p_irp, IO_NO_INCREMENT);
	return STATUS_NO_SUCH_DEVICE;

}

BOOLEAN remove_condition(PVOID p_item, PVOID p_arg)
{
	if(((pdispatch_table)p_item)->p_device == p_arg) {
		return TRUE;
	}

	return FALSE;
}

PFAST_IO_DISPATCH find_fast_io_by_device(PDEVICE_OBJECT p_device)
{
	plist_node p_node;
	p_node = device_list;

	if(p_node != NULL) {
		do {
			if(((pdispatch_table)p_node->p_item)->p_device == p_device) {
				return ((pdispatch_table)p_node->p_item)->p_fast_io_dispatch;
			}

			p_node = p_node->p_next;
		} while(p_node != device_list);
	}

	return NULL;
}

//Fast I/O functions
BOOLEAN fast_io_check_if_possible_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN BOOLEAN CheckForReadOperation,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_CHECK_IF_POSSIBLE p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoCheckIfPossible != NULL) {
		p_func = p_fast_io->FastIoCheckIfPossible;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          FileOffset,
		          Length,
		          Wait,
		          LockKey,
		          CheckForReadOperation,
		          IoStatus,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;

}

BOOLEAN fast_io_read_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_READ p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoRead != NULL) {
		p_func = p_fast_io->FastIoRead;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          FileOffset,
		          Length,
		          Wait,
		          LockKey,
		          Buffer,
		          IoStatus,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_write_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN BOOLEAN Wait,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_WRITE p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoWrite != NULL) {
		p_func = p_fast_io->FastIoWrite;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          FileOffset,
		          Length,
		          Wait,
		          LockKey,
		          Buffer,
		          IoStatus,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_query_basic_info_func(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_BASIC_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_QUERY_BASIC_INFO p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoQueryBasicInfo != NULL) {
		p_func = p_fast_io->FastIoQueryBasicInfo;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          Wait,
		          Buffer,
		          IoStatus,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_query_standard_info_func(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_STANDARD_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_QUERY_STANDARD_INFO p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoQueryStandardInfo != NULL) {
		p_func = p_fast_io->FastIoQueryStandardInfo;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          Wait,
		          Buffer,
		          IoStatus,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_lock(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    BOOLEAN FailImmediately,
    BOOLEAN ExclusiveLock,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_LOCK p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoLock != NULL) {
		p_func = p_fast_io->FastIoLock;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          FileOffset,
		          Length,
		          ProcessId,
		          Key,
		          FailImmediately,
		          ExclusiveLock,
		          IoStatus,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_unlock_single_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PLARGE_INTEGER Length,
    PEPROCESS ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_UNLOCK_SINGLE p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoUnlockSingle != NULL) {
		p_func = p_fast_io->FastIoUnlockSingle;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          FileOffset,
		          Length,
		          ProcessId,
		          Key,
		          IoStatus,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_unlock_all_func(
    IN PFILE_OBJECT FileObject,
    PEPROCESS ProcessId,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_UNLOCK_ALL p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoUnlockAll != NULL) {
		p_func = p_fast_io->FastIoUnlockAll;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          ProcessId,
		          IoStatus,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_unlock_all_by_key_func(
    IN PFILE_OBJECT FileObject,
    PVOID ProcessId,
    ULONG Key,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_UNLOCK_ALL_BY_KEY p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoUnlockAllByKey != NULL) {
		p_func = p_fast_io->FastIoUnlockAllByKey;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          ProcessId,
		          Key,
		          IoStatus,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_device_control_func(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength,
    IN ULONG IoControlCode,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_DEVICE_CONTROL p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoDeviceControl != NULL) {
		p_func = p_fast_io->FastIoDeviceControl;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          Wait,
		          InputBuffer,
		          InputBufferLength,
		          OutputBuffer,
		          OutputBufferLength,
		          IoControlCode,
		          IoStatus,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

VOID fast_io_detach_device_func(
    IN PDEVICE_OBJECT SourceDevice,
    IN PDEVICE_OBJECT TargetDevice
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	PFAST_IO_DETACH_DEVICE p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(SourceDevice);

	if(p_fast_io == NULL) {
		p_fast_io = find_fast_io_by_device(TargetDevice);
	}

	if(p_fast_io != NULL
	   && p_fast_io->FastIoDetachDevice != NULL) {
		p_func = p_fast_io->FastIoDetachDevice;
		dev_list_count--;
		p_func(
		    SourceDevice,
		    TargetDevice);

	} else {
		dev_list_count--;
	}

	return;
}

BOOLEAN fast_io_query_network_open_info_func(
    IN PFILE_OBJECT FileObject,
    IN BOOLEAN Wait,
    OUT PFILE_NETWORK_OPEN_INFORMATION Buffer,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_QUERY_NETWORK_OPEN_INFO p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoQueryNetworkOpenInfo != NULL) {
		p_func = p_fast_io->FastIoQueryNetworkOpenInfo;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          Wait,
		          Buffer,
		          IoStatus,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_mdl_read_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_MDL_READ p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->MdlRead != NULL) {
		p_func = p_fast_io->MdlRead;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          FileOffset,
		          Length,
		          LockKey,
		          MdlChain,
		          IoStatus,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_mdl_read_complete_func(
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_MDL_READ_COMPLETE p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->MdlReadComplete != NULL) {
		p_func = p_fast_io->MdlReadComplete;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          MdlChain,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_prepare_mdl_write_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_PREPARE_MDL_WRITE p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->PrepareMdlWrite != NULL) {
		p_func = p_fast_io->PrepareMdlWrite;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          FileOffset,
		          Length,
		          LockKey,
		          MdlChain,
		          IoStatus,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_mdl_write_complete_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_MDL_WRITE_COMPLETE p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->MdlWriteComplete != NULL) {
		p_func = p_fast_io->MdlWriteComplete;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          FileOffset,
		          MdlChain,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_read_compressed_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    OUT PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    OUT struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_READ_COMPRESSED p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoReadCompressed != NULL) {
		p_func = p_fast_io->FastIoReadCompressed;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          FileOffset,
		          Length,
		          LockKey,
		          Buffer,
		          MdlChain,
		          IoStatus,
		          CompressedDataInfo,
		          CompressedDataInfoLength,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_write_compressed_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN ULONG Length,
    IN ULONG LockKey,
    IN PVOID Buffer,
    OUT PMDL *MdlChain,
    OUT PIO_STATUS_BLOCK IoStatus,
    IN struct _COMPRESSED_DATA_INFO *CompressedDataInfo,
    IN ULONG CompressedDataInfoLength,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_WRITE_COMPRESSED p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoWriteCompressed != NULL) {
		p_func = p_fast_io->FastIoWriteCompressed;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          FileOffset,
		          Length,
		          LockKey,
		          Buffer,
		          MdlChain,
		          IoStatus,
		          CompressedDataInfo,
		          CompressedDataInfoLength,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_mdl_read_complete_compressed_func(
    IN PFILE_OBJECT FileObject,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_MDL_READ_COMPLETE_COMPRESSED p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->MdlReadCompleteCompressed != NULL) {
		p_func = p_fast_io->MdlReadCompleteCompressed;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          MdlChain,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_mdl_write_complete_compressed_func(
    IN PFILE_OBJECT FileObject,
    IN PLARGE_INTEGER FileOffset,
    IN PMDL MdlChain,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_MDL_WRITE_COMPLETE_COMPRESSED p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->MdlWriteCompleteCompressed != NULL) {
		p_func = p_fast_io->MdlWriteCompleteCompressed;
		dev_list_count--;
		ret = p_func(
		          FileObject,
		          FileOffset,
		          MdlChain,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}

BOOLEAN fast_io_query_open_func(
    IN PIRP Irp,
    OUT PFILE_NETWORK_OPEN_INFORMATION NetworkInformation,
    IN PDEVICE_OBJECT DeviceObject
)
{
	KIRQL irql;
	PFAST_IO_DISPATCH p_fast_io;
	BOOLEAN ret;
	PFAST_IO_QUERY_OPEN p_func;

	//If the list cannot be read,wait
	while(TRUE) {
		KeAcquireSpinLock(&device_list_lock, &irql);

		if(dev_list_readable_flag) {
			dev_list_count++;
			KeReleaseSpinLock(&device_list_lock, irql);
			break;
		}

		KeReleaseSpinLock(&device_list_lock, irql);

		if(irql <= APC_LEVEL) {
			sleep(100);
		}
	}

	//Look for fast I/O functions
	p_fast_io = find_fast_io_by_device(DeviceObject);

	if(p_fast_io != NULL
	   && p_fast_io->FastIoQueryOpen != NULL) {
		p_func = p_fast_io->FastIoQueryOpen;
		dev_list_count--;
		ret = p_func(
		          Irp,
		          NetworkInformation,
		          DeviceObject);

	} else {
		dev_list_count--;
		ret = FALSE;
	}

	return ret;
}