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
#include "driver/driver.h"

#define	SMS_MUTEX_NAME	L"sms_virtual_dir_mutex"

static	bool		is_running();
static	HANDLE		mutex_hnd;

//<test>
void	test();
//</test>

int __cdecl main(int argc, char* argn[])
{
	BREAKPOINT;

	if(is_running()) {
		PRINTF("The daemon process is running...\n");
		PAUSE;
		return 0;
	}

	if(!load_driver()) {
		MessageBox(NULL, L"Cannot load driver!", L"Error", MB_OK | MB_ICONWARNING);
		PAUSE;
		return -1;
	}

	if(!open_device()) {
		MessageBox(NULL, L"Cannot open control device!", L"Error", MB_OK | MB_ICONWARNING);
		unload_driver();
		PAUSE;
		return -1;
	}

	ReleaseMutex(mutex_hnd);
	return 0;
	//<test>
	test();
	//</test>
	close_device();
	unload_driver();
	ReleaseMutex(mutex_hnd);
	PAUSE;
	CloseHandle(mutex_hnd);
	PAUSE;
	return 0;
}

bool is_running()
{
	DWORD ret;
	mutex_hnd = CreateMutex(NULL, FALSE, SMS_MUTEX_NAME);

	if(mutex_hnd) {
		ret = WaitForSingleObject(mutex_hnd, 0);

		if(ret == WAIT_TIMEOUT) {
			return true;
		}

	} else {
		return true;
	}

	return false;
}

//<test>
void test()
{
	char a[] = "aaaaaaaaaaaaaaaaaaaaaaa";
	int i;
	DWORD len;
	write_device("hello\ntest\n", 11, &len);
	PRINTF("%d|\n", len);
	PAUSE;

	for(i = 0; i < 100; i++) {
		read_device(a, 6, &len);
		a[6] = '\0';
		PRINTF("%d|%s\n", len, a);
		write_device("hello\n", 6, &len);
		read_device(a, 5, &len);
		a[5] = '\0';
		PRINTF("%d|%s\n", len, a);
		write_device("test\n", 5, &len);

	}


	//PAUSE;
	return;
}
//</test>