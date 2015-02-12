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

#include "pipe.h"

static	BOOLEAN			is_pipe_empty(ppipe p_pipe);
static	BOOLEAN			is_pipe_full(ppipe p_pipe);
static	VOID			write_buf(ppipe p_pipe, PCHAR p_input, PSIZE_T p_length_written, SIZE_T length_to_write);

BOOLEAN initialize_pipe(ppipe p_pipe, SIZE_T buf_size)
{
	//Allocate memory
	if((p_pipe->buf
	    = (PCHAR)get_memory(buf_size))
	   == NULL) {
		return FALSE;
	}

	p_pipe->buf_size = buf_size;

	//Initialize flags
	p_pipe->count = 0;
	p_pipe->destroy_flag = FALSE;
	p_pipe->empty_flag = TRUE;

	//Initialize pointers
	p_pipe->p_end = p_pipe->p_start = p_pipe->buf;

	//Initialize locks
	InitializeCriticalSection(&(p_pipe->buf_lock));
	p_pipe->read_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	p_pipe->write_event = CreateEvent(NULL, TRUE, FALSE, NULL);
	return TRUE;;
}

VOID destroy_pipe(ppipe p_pipe)
{
	EnterCriticalSection(&(p_pipe->buf_lock));
	p_pipe->destroy_flag = TRUE;
	LeaveCriticalSection(&(p_pipe->buf_lock));

	while(p_pipe->count > 0) {
		SetEvent(p_pipe->read_event);
		SetEvent(p_pipe->write_event);
		Sleep(100);
	}

	CloseHandle(p_pipe->read_event);
	CloseHandle(p_pipe->write_event);
	DeleteCriticalSection(&(p_pipe->buf_lock));
	free_memory(p_pipe->buf);
	return;
}

BOOLEAN read_pipe(ppipe p_pipe, PVOID buf, SIZE_T buf_size, PSIZE_T p_length_read)
{
	PCHAR p_pipe_buf_end;
	PCHAR p_output;


	EnterCriticalSection(&(p_pipe->buf_lock));
	(p_pipe->count)++;

	if(p_pipe->destroy_flag) {
		SetEvent(p_pipe->write_event);
		SetEvent(p_pipe->read_event);
		EnterCriticalSection(&(p_pipe->buf_lock));
		(p_pipe->count)--;
		return FALSE;
	}

	p_output = (PCHAR)buf;
	p_pipe_buf_end = p_pipe->buf + p_pipe->buf_size - 1;

	//If the pipe is empty,wait for data.
	while(is_pipe_empty(p_pipe)
	      && !p_pipe->destroy_flag) {
		ResetEvent(p_pipe->write_event);
		SetEvent(p_pipe->read_event);
		LeaveCriticalSection(&(p_pipe->buf_lock));
		WaitForSingleObject(
		    p_pipe->write_event,
		    INFINITE
		);

		EnterCriticalSection(&(p_pipe->buf_lock));
	}

	//Test if the pipe is being destroyed
	if(p_pipe->destroy_flag) {
		SetEvent(p_pipe->write_event);
		SetEvent(p_pipe->read_event);
		LeaveCriticalSection(&(p_pipe->buf_lock));
		(p_pipe->count)--;
		return FALSE;
	}

	//Read data
	*p_length_read = 0;

	if(p_pipe->p_start > p_pipe->p_end) {
		while(p_pipe->p_start <= p_pipe_buf_end
		      && *p_length_read < buf_size) {
			*p_output = *(p_pipe->p_start);
			p_output++;
			(p_pipe->p_start)++;
			(*p_length_read)++;
		}
	}

	if(p_pipe->p_start > p_pipe_buf_end) {
		//Jump to the head of buf
		p_pipe->p_start = p_pipe->buf;
	}

	if(*p_length_read < buf_size) {

		while(p_pipe->p_start <= p_pipe->p_end
		      && *p_length_read < buf_size) {
			*p_output = *(p_pipe->p_start);
			p_output++;
			(p_pipe->p_start)++;
			(*p_length_read)++;
		}

		if(p_pipe->p_start > p_pipe->p_end) {
			//The buffer is empty.Reset the pointers
			p_pipe->p_end = p_pipe->p_start = p_pipe->buf;
			p_pipe->empty_flag = TRUE;
		}
	}

	SetEvent(p_pipe->read_event);
	LeaveCriticalSection(&(p_pipe->buf_lock));
	(p_pipe->count)--;
	return TRUE;
}

BOOLEAN write_pipe(ppipe p_pipe, PVOID data, SIZE_T length_to_write)
{
	PCHAR p_input;
	SIZE_T length_written;

	EnterCriticalSection(&(p_pipe->buf_lock));
	(p_pipe->count)++;

	//Test if the pipe is being destroyed
	if(p_pipe->destroy_flag) {
		SetEvent(p_pipe->write_event);
		SetEvent(p_pipe->read_event);
		LeaveCriticalSection(&(p_pipe->buf_lock));
		(p_pipe->count)--;
		return FALSE;
	}

	p_input = (PCHAR)data;
	length_written = 0;

	while(length_written < length_to_write) {
		//If the pipe is full,wait for space.
		while(is_pipe_full(p_pipe)
		      && !p_pipe->destroy_flag) {
			ResetEvent(p_pipe->read_event);
			SetEvent(p_pipe->write_event);
			LeaveCriticalSection(&(p_pipe->buf_lock));
			WaitForSingleObject(
			    p_pipe->read_event,
			    INFINITE
			);

			EnterCriticalSection(&(p_pipe->buf_lock));

		}

		//Test if the pipe is being destroyed
		if(p_pipe->destroy_flag) {
			SetEvent(p_pipe->read_event);
			SetEvent(p_pipe->write_event);
			LeaveCriticalSection(&(p_pipe->buf_lock));
			(p_pipe->count)--;
			return FALSE;
		}

		//Write data
		write_buf(p_pipe, p_input, &length_written, length_to_write);
		p_input = (PCHAR)data + length_written;
	}

	SetEvent(p_pipe->write_event);
	LeaveCriticalSection(&(p_pipe->buf_lock));
	(p_pipe->count)--;
	return TRUE;
}

BOOLEAN is_pipe_empty(ppipe p_pipe)
{
	if(p_pipe->empty_flag) {
		return TRUE;

	} else {
		return FALSE;
	}
}
BOOLEAN is_pipe_full(ppipe p_pipe)
{
	if(p_pipe->p_start - p_pipe->p_end == 1
	   || (p_pipe->p_start == p_pipe->buf
	       && p_pipe->p_end == p_pipe->buf + p_pipe->buf_size - 1)) {
		return TRUE;

	} else {
		return FALSE;
	}
}

VOID write_buf(ppipe p_pipe, PCHAR p_input, PSIZE_T p_length_written, SIZE_T length_to_write)
{
	PCHAR p_pipe_buf_end;

	p_pipe_buf_end = p_pipe->buf + p_pipe->buf_size - 1;

	//Test if the buf is empty
	if(p_pipe->empty_flag) {
		if(length_to_write != *p_length_written) {
			p_pipe->empty_flag = FALSE;

		} else {
			return;
		}

	} else {
		(p_pipe->p_end)++;
	}

	if(p_pipe->p_end >= p_pipe->p_start) {
		while(p_pipe->p_end <= p_pipe_buf_end
		      && *p_length_written < length_to_write) {
			*(p_pipe->p_end) = *p_input;
			(*p_length_written)++;
			p_input++;
			(p_pipe->p_end)++;
		}
	}

	MEM_CHK(p_pipe->buf);

	if(*p_length_written < length_to_write
	   && p_pipe->buf != p_pipe->p_start
	   && p_pipe->p_end > p_pipe_buf_end) {
		//Jump to the head of buf
		p_pipe->p_end = p_pipe->buf;
	}

	MEM_CHK(p_pipe->buf);

	while(p_pipe->p_end < p_pipe->p_start
	      && *p_length_written < length_to_write) {
		*(p_pipe->p_end) = *p_input;
		(*p_length_written)++;
		p_input++;
		(p_pipe->p_end)++;
	}

	MEM_CHK(p_pipe->buf);

	(p_pipe->p_end)--;
	return;
}