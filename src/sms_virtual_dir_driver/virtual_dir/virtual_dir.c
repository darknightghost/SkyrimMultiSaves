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
#include "../data_structure/list.h"

static	KMUTEX				hash_table_lock;
static	list				hash_table[0xFFFF];
static	UINT16				get_hash(PWCHAR str);

VOID init_virtual_dir()
{
	RtlZeroMemory(hash_table, sizeof(hash_table));
	KeInitializeMutex(&hash_table_lock, 0);
}

BOOLEAN			create_virtual_dir(PWCHAR path, UINT32 flag);
BOOLEAN			remove_virtual_dir();
VOID			clear_virtual_dir();

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