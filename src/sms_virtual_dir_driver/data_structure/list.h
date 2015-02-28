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

#ifndef LIST_H_INCLUDE
#define	LIST_H_INCLUDE
#include "../common.h"

typedef	struct list_node_ {
	struct	list_node_*		p_prev;
	struct	list_node_*		p_next;
	PVOID					p_item;
} list_node, *plist_node, *list;

typedef		void (*item_destroyer)(void*);

VOID			free_item_call_back(void* p_item);
VOID			do_nothing_call_back(void* p_item);
plist_node		list_add_item(list* p_list, void* p_item);
plist_node		list_insert_item(list* p_list, plist_node p_insert_before, void* p_item);
VOID			list_destroy(list* p_list, item_destroyer item_destroy_func);
BOOLEAN			list_remove_item(list* p_list, void* p_item);
PVOID			list_remove_item_by_maching(list* p_list, BOOLEAN(*condition)(PVOID p_item, PVOID arg), PVOID p_arg);
VOID			list_swap_item(plist_node p1, plist_node p2);


#endif //LIST_H_INCLUDE
