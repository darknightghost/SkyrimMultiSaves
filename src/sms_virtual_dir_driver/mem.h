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

#include "common.h"

#ifdef _DEBUG

PVOID		get_memory_dbg(
    SIZE_T	size,			//Input.Size of memory.
    ULONG	tag,			//Input.The pool tag for the allocated memory.
    POOL_TYPE	pool_type,	//Input.The type of pool memory to allocate.
    UINT32	line,
    PCHAR	file_name);

VOID		free_memory(
    PVOID	p);				//Input.The address of the block of pool memory being deallocated.

VOID		check_mem(
    PVOID	p);				//Input.The address of the block of pool memory being checked.

VOID		check_mem_leaks();

#define	get_memory(size,tag,pool_type)	get_memory_dbg((size),(tag),(pool_type),(__LINE__),(__FILE__))

#define	MEM_CHK(addr)	check_mem(addr)
#define	MEM_LEAK_CHK	check_mem_leaks()

#endif // _DEBUG

#ifndef _DEBUG

PVOID		get_memory(
    SIZE_T	size,			//Input.Size of memory.
    ULONG	tag,			//Input.The pool tag for the allocated memory.
    POOL_TYPE	pool_type);	//Input.The type of pool memory to allocate.

VOID		free_memory(
    PVOID	p);				//Input.The address of the block of pool memory being deallocated.

#define	MEM_CHK(addr)
#define	MEM_LEAK_CHK

#endif // !_DEBUG

#endif // !MEM_H_INCLUDE
