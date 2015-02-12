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

#include "common.h"

VOID get_object_name(
    IN PVOID Object,
    IN OUT PUNICODE_STRING Name)
{
	NTSTATUS status;
	CHAR nibuf[512];        //buffer that receives name information and name
	POBJECT_NAME_INFORMATION name_info = (POBJECT_NAME_INFORMATION)nibuf;
	ULONG ret_len;

	status = ObQueryNameString(Object, name_info, sizeof(nibuf), &ret_len);

	Name->Length = 0;

	if(NT_SUCCESS(status)) {

		RtlCopyUnicodeString(Name, &name_info->Name);
	}

	return;
}