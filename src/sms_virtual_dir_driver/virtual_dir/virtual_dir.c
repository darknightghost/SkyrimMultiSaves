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

static	KSPIN_LOCK			hash_table_flag_lock;
static	BOOLEAN				hash_table_readable_flag = TRUE;
static	list				hash_table[0xFFFF];
static	UINT16				hash_table_reference;
static	vpath_info			root_node;
static	BOOLEAN				init_flag = FALSE;

static	UINT16				get_hash(PWCHAR str);
static	pvpath_info			get_virtual_path_by_path_always(PWCHAR virtual_path);
static	pvpath_info			get_virtual_path_by_hvdir_always(hvdir hnd);
static	pvpath_info			get_parent_path(PWCHAR path);

VOID init_virtual_path()
{
	RtlZeroMemory(hash_table, sizeof(hash_table));
	KeInitializeSpinLock(&hash_table_flag_lock);
	hash_table_readable_flag = TRUE;
	RtlZeroMemory(&root_node, sizeof(vpath_info));
	KeInitializeMutex(&(root_node.lock), 0);
	KeInitializeMutex(&(root_node.child_list_lock), 0);
	hash_table_reference = 0;
	root_node.initialized_flag = TRUE;
	init_flag = TRUE;
	return;
}

BOOLEAN add_virtual_path(PWCHAR path, PWCHAR src, UINT32 flag)
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
		return FALSE;
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
		return FALSE;
	}

	p_parent = get_parent_path(path);

	if(p_parent == NULL) {
		return FALSE;
	}

	//Create new node.
	//Allocate memory
	p_new_node = get_memory(sizeof(vpath_info), *(PULONG)"sms", NonPagedPool);

	RtlZeroMemory(p_new_node, sizeof(vpath_info));

	if(p_new_node == NULL) {
		decrease_virtual_path_reference(p_parent);
		return FALSE;
	}

	p_new_node->virtual_path = get_memory((wcslen(path) + 1) * sizeof(wchar_t), *(PULONG)"sms", NonPagedPool);

	if(p_new_node->virtual_path == NULL) {
		free_memory(p_new_node);
		decrease_virtual_path_reference(p_parent);
		return FALSE;
	}

	p_new_node->src = get_memory((wcslen(src) + 1) * sizeof(wchar_t), *(PULONG)"sms", NonPagedPool);

	if(p_new_node->src == NULL) {
		free_memory(p_new_node->virtual_path);
		free_memory(p_new_node);
		decrease_virtual_path_reference(p_parent);
		return FALSE;
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
		return FALSE;
	}

	//Add node to hash table
	p_hash = GET_HASH_ADDR_FROM_PHVDIR(&(p_new_node->node_hnd));
	p_num = GET_NUM_ADDR_FROM_PHVDIR(&(p_new_node->node_hnd));

	*p_hash = get_hash(path);
	*p_num = 0;
	p_list_node = hash_table[*p_hash];

	if(p_list_node == NULL) {
		if(!list_add_item(&hash_table[*p_hash], p_new_node) == NULL) {
			list_remove_item(&(p_parent->child_list), p_new_node);
			KeReleaseMutex(&(p_parent->child_list_lock), FALSE);
			free_memory(p_new_node->src);
			free_memory(p_new_node->virtual_path);
			free_memory(p_new_node);
			decrease_virtual_path_reference(p_parent);
			return FALSE;
		}

	} else {
		while(GET_NUM_FROM_HVDIR(
		          ((pvpath_info)(p_list_node->p_item))->node_hnd
		      ) == *p_num) {
			p_list_node = p_list_node->p_next;
			(*p_num)++;
		}

		if(p_list_node == hash_table[*p_hash]) {
			if(!list_add_item(&hash_table[*p_hash], p_new_node) == NULL) {
				list_remove_item(&(p_parent->child_list), p_new_node);
				KeReleaseMutex(&(p_parent->child_list_lock), FALSE);
				free_memory(p_new_node->src);
				free_memory(p_new_node->virtual_path);
				free_memory(p_new_node);
				decrease_virtual_path_reference(p_parent);
				return FALSE;
			}

		} else {
			if(!list_insert_item(&hash_table[*p_hash], p_list_node, p_new_node) == NULL) {
				list_remove_item(&(p_parent->child_list), p_new_node);
				KeReleaseMutex(&(p_parent->child_list_lock), FALSE);
				free_memory(p_new_node->src);
				free_memory(p_new_node->virtual_path);
				free_memory(p_new_node);
				decrease_virtual_path_reference(p_parent);
				return FALSE;
			}
		}

	}

	KeReleaseMutex(&(p_parent->child_list_lock), FALSE);

	//Initialize node
	p_new_node->flag = flag;
	p_new_node->reference = 0;
	KeInitializeMutex(&(p_new_node->lock), 0);
	KeInitializeMutex(&(p_new_node->child_list_lock), 0);
	p_new_node->initialized_flag = TRUE;
	hash_table_readable_flag = TRUE;
	decrease_virtual_path_reference(p_parent);

	return TRUE;

}

pvpath_info		get_virtual_path_by_path_always(PWCHAR virtual_path);
pvpath_info		get_virtual_path_by_hvdir_always(hvdir hnd);
pvpath_info		get_virtual_path_by_path(PWCHAR virtual_path);
pvpath_info		get_virtual_path_by_hvdir(hvdir hnd);
VOID			decrease_virtual_path_reference(pvpath_info p_info);
BOOLEAN			remove_virtual_path();
VOID			clear_virtual_path();

pvpath_info		get_parent_path(PWCHAR path);

UINT16 get_hash(PWCHAR str)
{
	UINT32 i;
	PWCHAR buf = get_memory(sizeof(WCHAR) * (wcslen(str) + 1),
	                        *(PLONG)"sms", NonPagedPool);
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