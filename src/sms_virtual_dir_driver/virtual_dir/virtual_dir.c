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

#include "virtual_dir.h"
#include "../gate/gate.h"

#define	wcslwr				_wcslwr

static	KSPIN_LOCK			hash_table_flag_lock;
static	BOOLEAN				hash_table_readable_flag = TRUE;
static	list				hash_table[0x10000];
static	UINT16				hash_table_reference;
static	UINT32				total_reference;
static	vpath_info			root_node;
static	BOOLEAN				init_flag = FALSE;

static	UINT16				get_hash(PWCHAR str);
static	pvpath_info			get_virtual_path_by_path_always(PWCHAR virtual_path);
static	pvpath_info			get_virtual_path_by_hvdir_always(hvdir hnd);
static	pvpath_info			get_parent_path(PWCHAR path);
static	void				vpath_info_destroy_call_back(pvpath_info p_item);

VOID init_virtual_path()
{
	RtlZeroMemory(hash_table, sizeof(hash_table));
	KeInitializeSpinLock(&hash_table_flag_lock);
	hash_table_readable_flag = TRUE;
	RtlZeroMemory(&root_node, sizeof(vpath_info));
	KeInitializeMutex(&(root_node.lock), 0);
	KeInitializeMutex(&(root_node.child_list_lock), 0);
	hash_table_reference = 0;
	total_reference = 0;
	root_node.flag = FLAG_DIRECTORY;
	root_node.initialized_flag = TRUE;
	init_flag = TRUE;
	return;
}

VOID destroy_virtual_path()
{
	clear_virtual_path();
	free_memory(root_node.virtual_path);
	return;
}

BOOLEAN set_root_path(PWCHAR root_path)
{
	PWCHAR new_path;

	if(!init_flag) {
		return FALSE;
	}

	KeWaitForSingleObject(
	    &(root_node.lock),
	    Executive,
	    KernelMode,
	    FALSE,
	    NULL
	);

	while(root_node.reference > 0) {
		sleep(1000);
	}

	new_path = get_memory((wcslen(root_path) + 1) * sizeof(wchar_t), *(PULONG)"sms", NonPagedPool);

	if(new_path == NULL) {
		KeReleaseMutex(&(root_node.lock), FALSE);
		return FALSE;
	}

	if(root_node.virtual_path != NULL) {
		free_memory(root_node.virtual_path);
	}

	root_node.virtual_path = new_path;
	wcscpy(new_path, root_path);
	wcslwr(new_path);
	KeReleaseMutex(&(root_node.lock), FALSE);
	return TRUE;
}

BOOLEAN is_virtual_path(PWCHAR path)
{
	PWCHAR buf;
	PWCHAR p1, p2;

	PASSIVE_LEVEL_ASSERT;
	KeWaitForSingleObject(
	    &(root_node.lock),
	    Executive,
	    KernelMode,
	    FALSE,
	    NULL
	);

	root_node.reference++;

	KeReleaseMutex(&(root_node.lock), FALSE);
	buf = get_memory((wcslen(path) + 1) * sizeof(wchar_t), *(PULONG)"sms", NonPagedPool);
	buf = get_memory((wcslen(path) + 1) * sizeof(wchar_t), *(PULONG)"sms", NonPagedPool);

	if(buf == NULL) {
		return FALSE;
	}

	wcscpy(buf, path);
	wcslwr(buf);

	//Compare path
	for(p1 = buf, p2 = root_node.virtual_path;
	    *p2 != L'\0';
	    p1++, p2++) {
		if(*p1 != *p2) {
			root_node.reference--;
			return FALSE;
		}
	}

	root_node.reference--;
	return TRUE;
}

hvdir add_virtual_path(PWCHAR path, PWCHAR src, UINT32 flag)
{
	KIRQL irql;
	pvpath_info p_info;
	pvpath_info p_parent;
	PUINT16 p_hash;
	PUINT16 p_num;
	pvpath_info p_new_node;
	plist_node p_list_node;

	PASSIVE_LEVEL_ASSERT;

	if(!init_flag) {
		return 0;
	}

	//Lock hash table.
	while(!hash_table_readable_flag) {
		KeAcquireSpinLock(&hash_table_flag_lock, &irql);

		if(hash_table_readable_flag) {
			hash_table_readable_flag = FALSE;
			KeReleaseSpinLock(&hash_table_flag_lock, irql);
			break;
		}

		KeReleaseSpinLock(&hash_table_flag_lock, irql);

		sleep(500);
	}

	while(hash_table_reference > 0) {
		sleep(1000);
	}

	//Check if the path exists.
	p_info = get_virtual_path_by_path_always(path);

	if(p_info != NULL) {
		decrease_virtual_path_reference(p_info);
		return 0;
	}

	p_parent = get_parent_path(path);

	if(p_parent == NULL) {
		return 0;
	}

	//Create new node.
	//Allocate memory
	p_new_node = get_memory(sizeof(vpath_info), *(PULONG)"sms", NonPagedPool);

	RtlZeroMemory(p_new_node, sizeof(vpath_info));

	if(p_new_node == NULL) {
		decrease_virtual_path_reference(p_parent);
		return 0;
	}

	p_new_node->virtual_path = get_memory((wcslen(path) + 1) * sizeof(wchar_t), *(PULONG)"sms", NonPagedPool);

	if(p_new_node->virtual_path == NULL) {
		free_memory(p_new_node);
		decrease_virtual_path_reference(p_parent);
		return 0;
	}

	p_new_node->src = get_memory((wcslen(src) + 1) * sizeof(wchar_t), *(PULONG)"sms", NonPagedPool);

	if(p_new_node->src == NULL) {
		free_memory(p_new_node->virtual_path);
		free_memory(p_new_node);
		decrease_virtual_path_reference(p_parent);
		return 0;
	}

	//Add node to the directory tree
	KeWaitForSingleObject(
	    &(p_parent->child_list_lock),
	    Executive,
	    KernelMode,
	    FALSE,
	    NULL
	);

	while(p_parent->child_list_ref > 0) {
		sleep(500);
	}

	if(list_add_item(&(p_parent->child_list), p_new_node) == NULL) {
		KeReleaseMutex(&(p_parent->child_list_lock), FALSE);
		free_memory(p_new_node->src);
		free_memory(p_new_node->virtual_path);
		free_memory(p_new_node);
		decrease_virtual_path_reference(p_parent);
		return 0;
	}

	//Add node to hash table
	p_hash = GET_HASH_ADDR_FROM_PHVDIR(&(p_new_node->node_hnd));
	p_num = GET_NUM_ADDR_FROM_PHVDIR(&(p_new_node->node_hnd));

	*p_hash = get_hash(path);
	*p_num = 0;
	p_list_node = hash_table[*p_hash];

	if(p_list_node == NULL) {
		if(list_add_item(&hash_table[*p_hash], p_new_node) == NULL) {
			list_remove_item(&(p_parent->child_list), p_new_node);
			KeReleaseMutex(&(p_parent->child_list_lock), FALSE);
			free_memory(p_new_node->src);
			free_memory(p_new_node->virtual_path);
			free_memory(p_new_node);
			decrease_virtual_path_reference(p_parent);
			return 0;
		}

	} else {
		while(GET_NUM_FROM_HVDIR(
		          ((pvpath_info)(p_list_node->p_item))->node_hnd
		      ) == *p_num) {
			p_list_node = p_list_node->p_next;
			(*p_num)++;
		}

		if(p_list_node == hash_table[*p_hash]) {
			if(list_add_item(&hash_table[*p_hash], p_new_node) == NULL) {
				list_remove_item(&(p_parent->child_list), p_new_node);
				KeReleaseMutex(&(p_parent->child_list_lock), FALSE);
				free_memory(p_new_node->src);
				free_memory(p_new_node->virtual_path);
				free_memory(p_new_node);
				decrease_virtual_path_reference(p_parent);
				return 0;
			}

		} else {
			if(list_insert_item(&hash_table[*p_hash], p_list_node, p_new_node) == NULL) {
				list_remove_item(&(p_parent->child_list), p_new_node);
				KeReleaseMutex(&(p_parent->child_list_lock), FALSE);
				free_memory(p_new_node->src);
				free_memory(p_new_node->virtual_path);
				free_memory(p_new_node);
				decrease_virtual_path_reference(p_parent);
				return 0;
			}
		}

	}

	KeReleaseMutex(&(p_parent->child_list_lock), FALSE);

	//Initialize node
	p_new_node->flag = flag;
	wcscpy(p_new_node->src, src);
	wcscpy(p_new_node->virtual_path, path);
	KeInitializeMutex(&(p_new_node->lock), 0);
	p_new_node->p_parent = p_parent;

	if(flag & FLAG_DIRECTORY) {
		KeInitializeMutex(&(p_new_node->child_list_lock), 0);
	}

	p_new_node->initialized_flag = TRUE;
	hash_table_readable_flag = TRUE;
	decrease_virtual_path_reference(p_parent);

	return p_new_node->node_hnd;

}

pvpath_info get_virtual_path_by_path_always(PWCHAR virtual_path)
{
	pvpath_info p_node;
	UINT16 hash;
	plist_node p_list_node;
	PWCHAR buf1, buf2;

	//Look for node in hash table
	hash = get_hash(virtual_path);

	if(hash_table[hash] == NULL) {
		return NULL;
	}

	p_list_node = hash_table[hash];
	buf2 = get_memory((wcslen(virtual_path) + 1) * sizeof(WCHAR),
	                  *(PULONG)"sms",
	                  NonPagedPool);
	wcscpy(buf2, virtual_path);
	wcslwr(buf2);

	do {
		buf1 = get_memory((wcslen(
		                       ((pvpath_info)(p_list_node->p_item))->virtual_path
		                   ) + 1) * sizeof(WCHAR),
		                  *(PULONG)"sms",
		                  NonPagedPool);
		wcscpy(buf1, ((pvpath_info)(p_list_node->p_item))->virtual_path);
		wcslwr(buf1);

		if(wcscmp(buf1, buf2) == 0) {
			free_memory(buf1);
			p_node = (pvpath_info)(p_list_node->p_item);
			KeWaitForSingleObject(
			    &(p_node->lock),
			    Executive,
			    KernelMode,
			    FALSE,
			    NULL
			);

			if(!p_node->destroy_flag
			   && p_node->initialized_flag) {
				(p_node->reference)++;
				total_reference++;
				KeReleaseMutex(&(p_node->lock), FALSE);
				return p_node;

			}

			KeReleaseMutex(&(p_node->lock), FALSE);
			return NULL;

		}

		free_memory(buf1);

		p_list_node = p_list_node->p_next;
	} while(p_list_node != hash_table[hash]);

	free_memory(buf2);
	return NULL;
}

pvpath_info get_virtual_path_by_hvdir_always(hvdir hnd)
{
	pvpath_info p_node;
	UINT16 hash;
	plist_node p_list_node;


	hash = GET_HASH_FROM_HVDIR(hnd);

	//Look for node in hash table
	if(hash_table[hash] == NULL) {
		return NULL;
	}

	p_list_node = hash_table[hash];

	do {
		p_node = (pvpath_info)(p_list_node->p_item);

		if(p_node->node_hnd == hnd) {
			KeWaitForSingleObject(
			    &(p_node->lock),
			    Executive,
			    KernelMode,
			    FALSE,
			    NULL
			);

			if(!p_node->destroy_flag
			   && p_node->initialized_flag) {
				(p_node->reference)++;
				total_reference++;
				KeReleaseMutex(&(p_node->lock), FALSE);
				return p_node;

			}

			KeReleaseMutex(&(p_node->lock), FALSE);
			return NULL;

		}

		p_list_node = p_list_node->p_next;
	} while(p_list_node != hash_table[hash]);

	return NULL;
}

pvpath_info get_virtual_path_by_path(PWCHAR virtual_path)
{
	KIRQL irql;
	pvpath_info p_node;

	//Increase hash table reference count
	while(!hash_table_readable_flag) {
		KeAcquireSpinLock(&hash_table_flag_lock, &irql);

		if(hash_table_readable_flag) {
			hash_table_reference++;
			KeReleaseSpinLock(&hash_table_flag_lock, irql);
			break;
		}

		KeReleaseSpinLock(&hash_table_flag_lock, irql);

		sleep(500);
	}

	p_node = get_virtual_path_by_path_always(virtual_path);
	hash_table_reference--;
	return p_node;
}

pvpath_info get_virtual_path_by_hvdir(hvdir hnd)
{
	KIRQL irql;
	pvpath_info p_node;

	//Increase hash table reference count
	while(!hash_table_readable_flag) {
		KeAcquireSpinLock(&hash_table_flag_lock, &irql);

		if(hash_table_readable_flag) {
			hash_table_reference++;
			KeReleaseSpinLock(&hash_table_flag_lock, irql);
			break;
		}

		KeReleaseSpinLock(&hash_table_flag_lock, irql);

		sleep(500);
	}

	p_node = get_virtual_path_by_hvdir_always(hnd);
	hash_table_reference--;
	return p_node;
}

VOID decrease_virtual_path_reference(pvpath_info p_info)
{
	p_info->reference--;
	total_reference--;
	return;
}

BOOLEAN increase_virtual_path_child_reference(pvpath_info p_info)
{
	PASSIVE_LEVEL_ASSERT;

	if(!(p_info->flag & FLAG_DIRECTORY)) {
		return FALSE;
	}

	KeWaitForSingleObject(
	    &(p_info->child_list_lock),
	    Executive,
	    KernelMode,
	    FALSE,
	    NULL
	);

	if(!p_info->destroy_flag
	   && p_info->initialized_flag) {
		(p_info->child_list_ref)++;
		total_reference++;
		KeReleaseMutex(&(p_info->child_list_lock), FALSE);
		return TRUE;
	}

	KeReleaseMutex(&(p_info->child_list_lock), FALSE);
	return FALSE;
}

VOID decrease_virtual_path_child_reference(pvpath_info p_info)
{
	if(p_info->flag & FLAG_DIRECTORY) {
		(p_info->child_list_ref)--;
		total_reference--;
	}

	return;
}

BOOLEAN remove_virtual_path(pvpath_info p_info)
{
	KIRQL irql;
	PASSIVE_LEVEL_ASSERT;
	//Set destroy flag
	KeWaitForSingleObject(
	    &(p_info->lock),
	    Executive,
	    KernelMode,
	    FALSE,
	    NULL
	);

	if(p_info->destroy_flag
	   || !p_info->initialized_flag
	   || p_info->child_list != NULL) {
		KeReleaseMutex(&(p_info->lock), FALSE);
		return FALSE;
	}

	p_info->destroy_flag = TRUE;
	KeReleaseMutex(&(p_info->lock), FALSE);

	//Remove node from hash table
	//Lock hash table
	while(!hash_table_readable_flag) {
		KeAcquireSpinLock(&hash_table_flag_lock, &irql);

		if(hash_table_readable_flag) {
			hash_table_readable_flag = FALSE;
			KeReleaseSpinLock(&hash_table_flag_lock, irql);
			break;
		}

		KeReleaseSpinLock(&hash_table_flag_lock, irql);

		sleep(500);
	}

	while(hash_table_reference > 0) {
		sleep(1000);
	}

	//Remove item
	list_remove_item(&hash_table[GET_HASH_FROM_HVDIR(p_info->node_hnd)],
	                 p_info);

	hash_table_readable_flag = TRUE;

	//Remove item from parent node
	while(p_info->reference != 0) {
		sleep(1000);
	}

	KeWaitForSingleObject(
	    &(p_info->p_parent->child_list_lock),
	    Executive,
	    KernelMode,
	    FALSE,
	    NULL
	);

	while(p_info->p_parent->child_list_ref != 0) {
		sleep(1000);
	}

	list_remove_item(&(p_info->p_parent->child_list), p_info);
	KeReleaseMutex(&(p_info->p_parent->child_list_lock), FALSE);

	//Free memory
	free_memory(p_info->virtual_path);

	if(p_info->src != NULL) {
		free_memory(p_info->src);
	}

	free_memory(p_info);

	return TRUE;
}

VOID clear_virtual_path()
{
	UINT16 i;
	KIRQL irql;
	PASSIVE_LEVEL_ASSERT;

	//Lock hash table
	while(!hash_table_readable_flag) {
		KeAcquireSpinLock(&hash_table_flag_lock, &irql);

		if(hash_table_readable_flag) {
			hash_table_readable_flag = FALSE;
			KeReleaseSpinLock(&hash_table_flag_lock, irql);
			break;
		}

		KeReleaseSpinLock(&hash_table_flag_lock, irql);

		sleep(500);
	}

	while(hash_table_reference > 0) {
		sleep(1000);
	}

	while(total_reference > 0) {
		sleep(1000);
	}

	for(i = 0; i < 0xFFFF; i++) {
		if(hash_table[i] != NULL) {
			list_destroy(&hash_table[i], (item_destroyer)vpath_info_destroy_call_back);
			hash_table[i] = NULL;
		}
	}

	hash_table_readable_flag = TRUE;
	return;
}

pvpath_info get_parent_path(PWCHAR path)
{
	PWCHAR buf;
	PWCHAR p;
	pvpath_info ret;

	buf = get_memory((wcslen(path) + 1) * sizeof(WCHAR),
	                 *(PULONG)"sms", NonPagedPool);
	p = buf + wcslen(buf) - 1;

	while(*p != L'\\') {
		p--;
	}

	*p = L'\0';
	ret = get_virtual_path_by_path_always(buf);

	if(ret != NULL) {
		if(!(ret->flag & FLAG_DIRECTORY)) {
			decrease_virtual_path_reference(ret);
			ret = NULL;
		}
	}

	free_memory(buf);
	return ret;
}

UINT16 get_hash(PWCHAR str)
{
	UINT32 i;
	PWCHAR buf = get_memory(sizeof(WCHAR) * (wcslen(str) + 1),
	                        *(PULONG)"sms", NonPagedPool);
	char* p = (char*)buf;
	UINT16 hash = 5381;

	wcscpy(buf, str);
	_wcslwr(buf);

	for(i = 0; i < sizeof(WCHAR) * wcslen(str); i++) {
		hash = ((hash << 5) + hash) + *p;
	}

	free_memory(buf);

	return hash;

}

void vpath_info_destroy_call_back(pvpath_info p_item)
{
	if(p_item->child_list != NULL) {
		list_destroy(&(p_item->child_list), do_nothing_call_back);
	}

	free_memory(p_item->virtual_path);

	if(p_item->src != NULL) {
		free_memory(p_item->src);
	}

	free_memory(p_item);

	return;
}