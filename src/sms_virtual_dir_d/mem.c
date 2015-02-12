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

#include "mem.h"
#include "stdio.h"

#define	FILL_STR			"SMSDABN"

#ifdef _DEBUG

	static	WCHAR				error_buf[2048];
	static	ULONG				mem_leak_num = 0;
	static	CRITICAL_SECTION	num_lock;

#endif // _DEBUG

#ifdef _DEBUG

void mem_init()
{
	InitializeCriticalSection(&num_lock);
}

void* get_memory_dbg(size_t size, unsigned int line, char* file_name)
{
	void* p_buf;
	unsigned long*	p_line;
	char** p_p_file_name;
	size_t* p_size;
	char* p_ret;
	char* p_fill;

	//Allocate memory
	p_buf = malloc(sizeof(unsigned long) + sizeof(char*)
	               + sizeof(size_t) + size + strlen(FILL_STR) + 1);

	if(p_buf == NULL) {
		return NULL;
	}

	p_line = p_buf;
	p_p_file_name = (char**)(p_line + 1);
	p_size = (size_t*)(p_p_file_name + 1);
	p_ret = (char*)(p_size + 1);
	p_fill = p_ret + size;

	//Fill info
	*p_line = line;
	*p_p_file_name = file_name;
	*p_size = size;
	strcpy(p_fill, FILL_STR);
	EnterCriticalSection(&num_lock);
	mem_leak_num++;
	LeaveCriticalSection(&num_lock);
	return p_ret;
}

void free_memory(void* p_mem)
{
	check_memory(p_mem);
	free((char *)p_mem - sizeof(size_t) - sizeof(char*) - sizeof(unsigned long));
	EnterCriticalSection(&num_lock);
	mem_leak_num--;
	LeaveCriticalSection(&num_lock);
	return;
}

void check_memory(void* p_mem)
{
	unsigned long*	p_line;
	char** p_p_file_name;
	size_t* p_size;
	char* p_fill;

	//Locate infomations of memory block
	p_size = (size_t*)p_mem - 1;
	p_p_file_name = (char**)((char*)p_size - 1);
	p_line = (unsigned long*)p_p_file_name - 1;
	p_fill = (char*)p_mem + *p_size;

	if(strcmp(p_fill, FILL_STR) != 0) {
		wsprintf(error_buf, L"Buffer overflow at %P.\r\nThe memory block was allocted in file\r\n\"%s\",\r\nLine:%u.",
		         p_mem,
		         *p_p_file_name,
		         *p_line);
		PRINT_ERR(error_buf);
	}

	return;
}

void check_memory_leaks()
{
	if(mem_leak_num != 0) {
		wsprintf(error_buf, L"%u memory leaks detected.", mem_leak_num);
	}

	DeleteCriticalSection(&num_lock);
	return;
}

#endif // _DEBUG

#ifndef _DEBUG

void* get_memory(size_t size)
{
	return malloc(size);
}

void free_memory(void* p_mem)
{
	free(p_mem);
}

#endif // !_DEBUG