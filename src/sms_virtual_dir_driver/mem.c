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

#include "mem.h"

#define	FILL_STR	"MEMCHKSTRAAAABBBBCCCC"
#define	FILL_SIZE	(sizeof(FILL_STR)-1)

#ifdef _DEBUG
	static	ULONG		mem_count = 0;
#endif // _DEBUG

#ifndef _DEBUG

PVOID get_memory(SIZE_T size, ULONG tag, POOL_TYPE pool_type)
{

	return ExAllocatePoolWithTag(pool_type, size, tag);
}

VOID free_memory(VOID* p)
{
	ExFreePool(p);
	return;
}

#endif // !_DEBUG

#ifdef _DEBUG
PVOID get_memory_dbg(SIZE_T size, ULONG tag, POOL_TYPE pool_type, UINT32 line, PCHAR file_name)
{
	PSIZE_T	p_size;
	PVOID p_ret;
	PVOID p_mem;
	PCHAR p_fill;
	PUINT32	p_line;
	PCHAR* p_file_name;
	p_mem = ExAllocatePoolWithTag(pool_type, size + sizeof(SIZE_T) + sizeof(UINT32) + sizeof(PCHAR*) + FILL_SIZE, tag);

	if(p_mem == NULL) {
		return NULL;
	}

	p_size = (PSIZE_T)p_mem;
	p_line = (PUINT32)(p_size + 1);
	p_file_name = (PCHAR*)(p_line + 1);
	p_ret = (PCHAR)p_file_name + sizeof(PCHAR*);
	p_fill = (PCHAR)p_ret + size;

	*p_size = size;
	*p_line = line;
	*p_file_name = file_name;
	RtlCopyMemory(p_fill, FILL_STR, FILL_SIZE);
	mem_count++;

	RtlFillMemory(p_ret, size, 0xcc);
	return p_ret;
}

VOID free_memory(VOID* p)
{
	check_mem(p);
	ExFreePool((PCHAR)p - sizeof(UINT32) - sizeof(SIZE_T) - sizeof(PCHAR*));
	mem_count--;
	return;
}

VOID check_mem(PVOID p)
{
	PCHAR p_fill;
	PCHAR p_fill_str;
	PUINT32 p_line;
	PCHAR* p_file_name;
	PSIZE_T	p_size;

	p_file_name = (PCHAR*)p - 1;
	p_line = (PUINT32)p_file_name - 1;
	p_size = (PSIZE_T)p_line - 1;
	p_fill = (PCHAR)p + *p_size;

	for(p_fill_str = FILL_STR;
	    p_fill_str - FILL_STR < FILL_SIZE;
	    p_fill_str++, p_fill++) {
		if(*p_fill != *p_fill_str) {
			KdPrint(("Buffer overflow at %p.The memory was allocated in file:\"%s\",line:%u.\n",
			         p,
			         *p_file_name,
			         *p_line));

			//If Kernel dubugger connected
			if(!KdRefreshDebuggerNotPresent()) {
				//Breakpoint
				RAW_BREAKPOINT;

			} else {
				//Blue screen of death
				KeBugCheckEx(BAD_POOL_HEADER,
				             0x2,
				             (ULONG_PTR)p,
				             *p_size,
				             0);

			}

			break;
		}
	}

	return;
}

VOID check_mem_leaks()
{
	if(mem_count != 0) {
		if(!KdRefreshDebuggerNotPresent()) {
			KdPrint(("%u memory leaks detected!\n", mem_count));
			RAW_BREAKPOINT;
		}
	}
}

#endif // _DEBUG