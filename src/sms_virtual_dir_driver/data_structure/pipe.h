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

#ifndef PIPE_H_INCLUDE
#define	PIPE_H_INCLUDE

#include "../mem.h"

typedef	struct _pipe {
	PCHAR			buf;					//Buffer.
	SIZE_T			buf_size;				//Size of buffer.
	PCHAR			p_start;				//Point to the begining of data.
	PCHAR			p_end;					//Point to the end of the data.
	UINT32			count;					//The number of thread which are reading and writing the pipe
	BOOLEAN			destroy_flag;			//True if being destroyed
	BOOLEAN			empty_flag;				//True if the buffer is empty
	KMUTEX			buf_lock;				//Must be acquired before operation the buf
	KEVENT			read_event;				//Be set when the pipe is read
	KEVENT			write_event;			//Be set when the pipe is written
} pipe, *ppipe;


NTSTATUS	initialize_pipe(ppipe p_pipe, SIZE_T buf_size);
VOID		destroy_pipe(ppipe p_pipe);
NTSTATUS	read_pipe(ppipe p_pipe, PVOID buf, SIZE_T buf_size, PSIZE_T p_length_read);
NTSTATUS	write_pipe(ppipe p_pipe, PVOID data, SIZE_T length_to_write);


#endif // !PIPE_H_INCLUDE
