;---------------------------------------------------------------------------------
;	Copyright 2015,∞µ“π”ƒ¡È <darknightghost.cn@gmail.com>
;
;	This program is free software: you can redistribute it and/or modify
;   it under the terms of the GNU General Public License as published by
;   the Free Software Foundation, either version 3 of the License, or
;   (at your option) any later version.
;
;   This program is distributed in the hope that it will be useful,
;   but WITHOUT ANY WARRANTY; without even the implied warranty of
;   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;   GNU General Public License for more details.
;
;   You should have received a copy of the GNU General Public License
;   along with this program.  If not, see <http://www.gnu.org/licenses/>.
;---------------------------------------------------------------------------------
				.code
public			driver_caller
extrn			kernel_mode_func_table:near

TYPE_BOOLEAN		equ	0001h
TYPE_HVDIR			equ	0002h
TYPE_UINT32			equ	0003h
TYPE_WCHAR_STRING	equ	0004h


;void*			driver_caller(void* buf);
driver_caller:
				push		rbp
				mov			rbp,rsp
				push		rbx
				push		rsi
				push		rdi
				push		r12
				push		r13
				push		r14
				;rsi=buf
				mov			rsi,rcx
				;rax=Address of function
				mov			rbx,offset kernel_mode_func_table
				xor			rax,rax
				mov			ax,[rsi]
				imul		rax,8
				add			rbx,rax
				mov			rax,[rbx]
				add			rsi,2
				;r12=Number of arguments
				xor			r12,r12
				mov			r12b,[rsi]
				add			rsi,10
				;r13=Number of arguments dealed
				xor			r13,r13
				push		rbp
				mov			rbp,rsp
				;while(r13!=r12)
				WHILE_0:
				cmp			r12,r13
				je			WHILE_END_0
					;switch(r13)
					cmp		r13,0
					je		SWITCH_0_0
					cmp		r13,1
					je		SWITCH_0_1
					cmp		r13,2
					je		SWITCH_0_2
					cmp		r13,3
					je		SWITCH_0_3
					jmp		SWITCH_0_DEFAULT
					;case	0:
					SWITCH_0_0:
						;if(*rsi==TYPE_WCHAR_STRING)
						mov		ebx,[rsi]
						cmp		ebx,TYPE_WCHAR_STRING
						jne		ELSE_0
						IF_0:
							add		rsi,4
							;r14=Size of argument
							xor		r14,r14
							mov		r14d,[rsi]
							add		rsi,4
							;rcx=First argument
							mov		rcx,rsi
							add		rsi,r14
						jmp		ENDIF_0
						;else
						ELSE_0:
							add		rsi,4
							;switch(*rsi)
							mov		ebx,[rsi]
							cmp		ebx,1
							je		SWITCH_1_0
							cmp		ebx,2
							je		SWITCH_1_1
							cmp		ebx,4
							je		SWITCH_1_2
							cmp		ebx,8
							je		SWITCH_1_3
							;case	1:
							SWITCH_1_0:
								add		rsi,4
								xor		rcx,rcx
								mov		cl,[rsi]
								inc		rsi
								jmp		SWITCH_1_END
							;case	2:
							SWITCH_1_1:
								add		rsi,4
								xor		rcx,rcx
								mov		cx,[rsi]
								add		rsi,2
								jmp		SWITCH_1_END
							;case	4:
							SWITCH_1_2:
								add		rsi,4
								xor		rcx,rcx
								mov		ecx,[rsi]
								add		rsi,4
								jmp		SWITCH_1_END
							;case	8:
							SWITCH_1_3:
								add		rsi,4
								mov		rcx,[rsi]
								add		rsi,8
							SWITCH_1_END:
						ENDIF_0:
						inc		r13
						sub		rsp,8
						jmp		SWITCH_0_END
					;case	1:
					SWITCH_0_1:
					;if(*rsi==TYPE_WCHAR_STRING)
						mov		ebx,[rsi]
						cmp		ebx,TYPE_WCHAR_STRING
						jne		ELSE_1
						IF_1:
							add		rsi,4
							;r14=Size of argument
							xor		r14,r14
							mov		r14d,[rsi]
							add		rsi,4
							;rdx=Second argument
							mov		rdx,rsi
							add		rsi,r14
						jmp		ENDIF_1
						;else
						ELSE_1:
							add		rsi,4
							;switch(*rsi)
							mov		ebx,[rsi]
							cmp		ebx,1
							je		SWITCH_2_0
							cmp		ebx,2
							je		SWITCH_2_1
							cmp		ebx,4
							je		SWITCH_2_2
							cmp		ebx,8
							je		SWITCH_2_3
							;case	1:
							SWITCH_2_0:
								add		rsi,4
								xor		rdx,rdx
								mov		dl,[rsi]
								inc		rsi
								jmp		SWITCH_2_END
							;case	2:
							SWITCH_2_1:
								add		rsi,4
								xor		rdx,rdx
								mov		dx,[rsi]
								add		rsi,2
								jmp		SWITCH_2_END
							;case	4:
							SWITCH_2_2:
								add		rsi,4
								xor		rdx,rdx
								mov		edx,[rsi]
								add		rsi,4
								jmp		SWITCH_2_END
							;case	8:
							SWITCH_2_3:
								add		rsi,4
								mov		rdx,[rsi]
								add		rsi,8
							SWITCH_2_END:
						ENDIF_1:
						inc		r13
						sub		rsp,8
						jmp		SWITCH_0_END
					SWITCH_0_2:
						;if(*rsi==TYPE_WCHAR_STRING)
						mov		ebx,[rsi]
						cmp		ebx,TYPE_WCHAR_STRING
						jne		ELSE_2
						IF_2:
							add		rsi,4
							;r14=Size of argument
							xor		r14,r14
							mov		r14d,[rsi]
							add		rsi,4
							;r8=Third argument
							mov		r8,rsi
							add		rsi,r14
						jmp		ENDIF_2
						;else
						ELSE_2:
							add		rsi,4
							;switch(*rsi)
							mov		ebx,[rsi]
							cmp		ebx,1
							je		SWITCH_3_0
							cmp		ebx,2
							je		SWITCH_3_1
							cmp		ebx,4
							je		SWITCH_3_2
							cmp		ebx,8
							je		SWITCH_3_3
							;case	1:
							SWITCH_3_0:
								add		rsi,4
								xor		r8,r8
								mov		r8b,[rsi]
								inc		rsi
								jmp		SWITCH_3_END
							;case	2:
							SWITCH_3_1:
								add		rsi,4
								xor		r8,r8
								mov		r8w,[rsi]
								add		rsi,2
								jmp		SWITCH_3_END
							;case	4:
							SWITCH_3_2:
								add		rsi,4
								xor		r8,r8
								mov		r8d,[rsi]
								add		rsi,4
								jmp		SWITCH_3_END
							;case	8:
							SWITCH_3_3:
								add		rsi,4
								mov		r8,[rsi]
								add		rsi,8
							SWITCH_3_END:
						ENDIF_2:
						inc		r13
						sub		rsp,8
						jmp		SWITCH_0_END
					SWITCH_0_3:
						;if(*rsi==TYPE_WCHAR_STRING)
						mov		ebx,[rsi]
						cmp		ebx,TYPE_WCHAR_STRING
						jne		ELSE_3
						IF_3:
							add		rsi,4
							;r14=Size of argument
							xor		r14,r14
							mov		r14d,[rsi]
							add		rsi,4
							;r9=Fourth argument
							mov		r9,rsi
							add		rsi,r14
						jmp		ENDIF_3
						;else
						ELSE_3:
							add		rsi,4
							;switch(*rsi)
							mov		ebx,[rsi]
							cmp		ebx,1
							je		SWITCH_4_0
							cmp		ebx,2
							je		SWITCH_4_1
							cmp		ebx,4
							je		SWITCH_4_2
							cmp		ebx,8
							je		SWITCH_4_3
							;case	1:
							SWITCH_4_0:
								add		rsi,4
								xor		r9,r9
								mov		r9b,[rsi]
								inc		rsi
								jmp		SWITCH_4_END
							;case	2:
							SWITCH_4_1:
								add		rsi,4
								xor		r9,r9
								mov		r9w,[rsi]
								add		rsi,2
								jmp		SWITCH_4_END
							;case	4:
							SWITCH_4_2:
								add		rsi,4
								xor		r9,r9
								mov		r9d,[rsi]
								add		rsi,4
								jmp		SWITCH_4_END
							;case	8:
							SWITCH_4_3:
								add		rsi,4
								mov		r9,[rsi]
								add		rsi,8
							SWITCH_4_END:
						ENDIF_3:
						inc		r13
						sub		rsp,8
						jmp		SWITCH_0_END
					SWITCH_0_DEFAULT:
						int		3
					SWITCH_0_END:
				jmp			WHILE_0
				WHILE_END_0:
				;Call function
				call		rax
				leave
				pop			r14
				pop			r13
				pop			r12
				pop			rdi
				pop			rsi
				pop			rbx
				leave
				ret
				end