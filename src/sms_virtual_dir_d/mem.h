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

#ifndef MEM_H_INCLUDE
#define	MEM_H_INCLUDE

#include "debug.h"

#ifdef _DEBUG
	void*			get_memory_dbg(size_t size, unsigned int line, char* file_name);
	void			free_memory(void* p_mem);
	void			check_memory(void* p_mem);
	void			check_memory_leaks();
	void			mem_init();

	#define	get_memory(size)	get_memory_dbg((size),(__LINE__),(__FILE__))
	#define	MEM_CHK(p_mem)		check_memory((p_mem))
	#define	MEM_LEAK_CHK		check_memory_leaks()
	#define	MEM_INIT			mem_init()

#endif // _DEBUG

#ifndef _DEBUG

	void*			get_memory(size_t size);
	void			free_memory(void* p_mem);

	#define	MEM_CHK(p_mem)
	#define	MEM_LEAK_CHK
	#define	MEM_INIT

#endif // !_DEBUG

#endif // !MEM_H_INCLUDE
