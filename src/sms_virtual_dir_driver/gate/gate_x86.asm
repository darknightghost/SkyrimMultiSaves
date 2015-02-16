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
				.386
				.model flat,stdcall
				option casemap:none

;---------------------------------------------------------------------------------
				.code
public			driver_caller@4
extrn			kernel_mode_func_table:near

TYPE_BOOLEAN		equ	0001h
TYPE_HVDIR			equ	0002h
TYPE_UINT32			equ	0003h
TYPE_WCHAR_STRING	equ	0004h

;void*			driver_caller(void* buf);
driver_caller@4:
				push	ebp
				mov		ebp,esp
				sub		esp,4
				pushad
				sub		esp,16
				;esi=buf
				mov		esi,[ebp-8]
				;eax=Function address
				xor		eax,eax
				mov		ax,[esi]
				mov		ebx,offset kernel_mode_func_table
				add		ebx,eax
				mov		eax,[ebx]
				mov		esi,[ebp+8]
				add		esi,2
				;edx=Number of arguments
				xor		edx,edx
				mov		dl,[esi]
				add		esi,9
				;r13=Number of arguments dealed
				xor		ecx,ecx
				;while(ecx!=edx)
				WHILE_0:
				cmp		ecx,edx
				je		WHILE_END_0
					;if(*esi==TYPE_WCHAR_STRING)
					IF_0_0:
					mov		ebx,[esi]
					cmp		ebx,TYPE_WCHAR_STRING
					jne		IF_0_ELSE
						add		esi,4
						mov		ebx,[esi]
						add		esi,4
						mov		edi,ecx
						imul	edi,4
						add		edi,esp
						mov		[edi],esi
						add		esi,ebx
					jmp		IF_0_END
					;else
					IF_0_ELSE:
						add		esi,4
						;switch(*esi)
						mov		ebx,[esi]
						cmp		ebx,1
						je		SWITCH_0_0
						cmp		ebx,2
						je		SWITCH_0_1
						cmp		ebx,4
						je		SWITCH_0_2
						jmp		SWITCH_0_DEFAULT
						;case	1:
						SWITCH_0_0:
							add		esi,4
							xor		ebx,ebx
							mov		bl,[esi]
							mov		edi,ecx
							imul	edi,4
							add		edi,esp
							mov		[edi],ebx
							inc		esi
							jmp		SWITCH_0_END
						;case	2:
						SWITCH_0_1:
							add		esi,4
							xor		ebx,ebx
							mov		bx,[esi]
							mov		edi,ecx
							imul	edi,4
							add		edi,esp
							mov		[edi],ebx
							add		esi,2
							jmp		SWITCH_0_END
						SWITCH_0_2:
						;case	4:
							add		esi,4
							xor		ebx,ebx
							mov		ebx,[esi]
							mov		edi,ecx
							imul	edi,4
							add		edi,esp
							mov		[edi],ebx
							add		esi,4
							jmp		SWITCH_0_END
						SWITCH_0_DEFAULT:
							int		3
						SWITCH_0_END:
					IF_0_END:
					inc		ecx
				jmp		WHILE_0
				WHILE_END_0:
				;Call function
				call	eax
				mov		[ebp-4],eax
				add		esp,16
				popad
				mov		eax,[ebp-4]
				leave
				ret		4
				end