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

#include "list.h"
#include "../mem.h"

VOID item_only_delete_func(void* p_item)
{
	free_memory(p_item);
	return;
}

plist_node list_add_item(list* p_list, void* p_item)
{
	plist_node p_new_node;

	//If the list is empty
	if(*p_list == NULL) {
		*p_list = get_memory(sizeof(list_node), *(PULONG)"sms", NonPagedPool);

		if(p_list == NULL) {
			return NULL;
		}

		(*p_list)->p_next = (*p_list)->p_prev = *p_list;
		(*p_list)->p_item = p_item;
		return *p_list;
		//If there is only one item in the list

	} else if((*p_list)->p_next == *p_list) {
		p_new_node = get_memory(sizeof(list_node), *(PULONG)"sms", NonPagedPool);

		if(p_list == NULL) {
			return NULL;
		}

		(*p_list)->p_next = (*p_list)->p_prev = p_new_node;
		p_new_node->p_next = p_new_node->p_prev = (*p_list);
		p_new_node->p_item = p_item;
		//Else

	} else {
		p_new_node = get_memory(sizeof(list_node), *(PULONG)"sms", NonPagedPool);

		if(p_list == NULL) {
			return NULL;
		}

		p_new_node->p_prev = (*p_list)->p_prev;
		p_new_node->p_next = *p_list;
		(*p_list)->p_prev->p_next = p_new_node;
		(*p_list)->p_prev = p_new_node;
		p_new_node->p_item = p_item;
	}

	return p_new_node;
}

VOID list_destroy(list* p_list, item_destroyer item_destroy_func)
{
	plist_node	p_node, p_next_node;
	p_node = *p_list;

	//Destroy the list
	if(*p_list == NULL) {
		return;
	}

	do {
		p_next_node = p_node->p_next;

		//Destroy the item
		if(item_destroy_func != NULL) {
			item_destroy_func(p_node->p_item);
		}

		free_memory(p_node);
		p_node = p_next_node;
	} while(p_node != *p_list);

	*p_list = NULL;
	return;
}

BOOLEAN list_remove_item(list* p_list, void* p_item)
{
	plist_node p_node;
	//Search for the item
	p_node = *p_list;

	do {
		//If the item has been found
		if(p_node->p_item == p_item) {
			//Remove the item
			//If this item is the only one item
			if(p_node->p_next == p_node) {
				*p_list = NULL;
				//If the item is the first item

			} else if(p_node == *p_list) {
				*p_list = p_node->p_next;
				p_node->p_next->p_prev = p_node->p_prev;
				p_node->p_prev->p_next = p_node->p_next;
				//Else

			} else {
				p_node->p_next->p_prev = p_node->p_prev;
				p_node->p_prev->p_next = p_node->p_next;
			}

			free_memory(p_node);
			return TRUE;
		}

		p_node = p_node->p_next;
	} while(p_node != *p_list);

	return FALSE;
}

PVOID list_remove_item_by_maching(list* p_list, BOOLEAN(*condition)(PVOID p_item, PVOID arg), PVOID p_arg)
{
	plist_node p_node;
	PVOID p_item;

	//Search for the item
	p_node = *p_list;

	do {
		//If the item has been found
		if(condition(p_node->p_item, p_arg)) {
			//Remove the item
			//If this item is the only one item
			if(p_node->p_next == p_node) {
				*p_list = NULL;
				//If the item is the first item

			} else if(p_node == *p_list) {
				*p_list = p_node->p_next;
				p_node->p_next->p_prev = p_node->p_prev;
				p_node->p_prev->p_next = p_node->p_next;
				//Else

			} else {
				p_node->p_next->p_prev = p_node->p_prev;
				p_node->p_prev->p_next = p_node->p_next;
			}

			p_item = p_node->p_item;
			free_memory(p_node);
			return p_item;
		}

		p_node = p_node->p_next;
	} while(p_node != *p_list);

	return NULL;
}

VOID list_swap_item(plist_node p1, plist_node p2)
{
	void* tmp;
	tmp = p1->p_item;
	p1->p_item = p2->p_item;
	p2->p_item = tmp;
	return;
}