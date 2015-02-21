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
#include "driver/gate.h"

#define	SMS_MUTEX_NAME	L"sms_virtual_dir_mutex"

static	bool		is_running();
static	HANDLE		mutex_hnd;

//<test>
void	test();
//</test>

int __cdecl main(int argc, char* argn[])
{
	BREAKPOINT;
	MEM_INIT;

	if(is_running()) {
		PRINTF("The daemon process is running...\n");
		PAUSE;
		return 0;
	}

	if(!init_call_gate()) {
		MessageBox(NULL, L"Cannot initialize call gate!", L"Error", MB_OK | MB_ICONWARNING);
		ReleaseMutex(mutex_hnd);
		CloseHandle(mutex_hnd);
		PAUSE;
		return -1;
	}

	if(!start_call_gate()) {
		MessageBox(NULL, L"Cannot start call gate module!", L"Error", MB_OK | MB_ICONWARNING);
		destroy_call_gate();
		ReleaseMutex(mutex_hnd);
		CloseHandle(mutex_hnd);
		PAUSE;
		return -1;
	}

	//<test>
	test();
	//</test>
	stop_call_gate();
	destroy_call_gate();
	ReleaseMutex(mutex_hnd);
	CloseHandle(mutex_hnd);
	MEM_LEAK_CHK;
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
	char* p;
	p = NULL;
	PAUSE;
	PRINTF("Testing functions...\n");
	/*k_enable_filter();
	k_disable_filter();
	PRINTF("k_set_base_dir returned %u\n.", k_set_base_dir(L"c:\\"));*/
	PRINTF("k_add_virtual_path returned 0x%.8X\n", k_add_virtual_path(L"D:\\", L"aassddd", 1));
	PAUSE;
	//	*p = '\0';
	return;
}
//</test>