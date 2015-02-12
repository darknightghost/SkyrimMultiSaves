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

#include "driver.h"
#include <VersionHelpers.h>
#include "../../common/driver/device_name.h"
#include <stdio.h>
#include <string.h>

static	wchar_t*		get_driver_file_name();

static	SC_HANDLE		l_scm_hnd = NULL;
static	SC_HANDLE		l_service_hnd = NULL;
static	HANDLE			dev_hnd = NULL;

bool load_driver()
{
	wchar_t* driver_path;
	wchar_t* driver_name;
	SERVICE_STATUS srv_status;
	DWORD path_length;

	//Open SCM
	if((l_scm_hnd = OpenSCManager(
	                    NULL,
	                    NULL,
	                    SC_MANAGER_ALL_ACCESS)) == NULL) {
		MessageBox(NULL, L"Fail to open SCM.Cannot initialize the daemon.", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	PRINTF("SCM opened.\n");

	//Test if the service exists
	if((l_service_hnd = OpenService(
	                        l_scm_hnd,
	                        SERVICE_NAME,
	                        SERVICE_ALL_ACCESS)) != NULL) {
		ControlService(l_service_hnd, SERVICE_CONTROL_STOP, &srv_status);
		DeleteService(l_service_hnd);
		CloseServiceHandle(l_service_hnd);

	} else {
		if(GetLastError() != ERROR_SERVICE_DOES_NOT_EXIST) {
			CloseServiceHandle(l_scm_hnd);
			MessageBox(NULL, L"Fail to open the service.Cannot initialize the daemon.", L"Error", MB_OK | MB_ICONERROR);
			return false;
		}
	}

	//Get driver file name
	if((driver_name = get_driver_file_name()) == NULL) {
		CloseServiceHandle(l_scm_hnd);
		MessageBox(NULL, L"Fail to create the service.Cannot initialize the daemon.", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	path_length = GetFullPathName(driver_name, 0, NULL, NULL) + 1;
	path_length *= sizeof(wchar_t);

	if((driver_path = (wchar_t*)malloc(path_length)) == NULL) {
		CloseServiceHandle(l_scm_hnd);
		MessageBox(NULL, L"Fail to allocate memory.Cannot initialize the daemon.", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	path_length = GetFullPathName(driver_name, path_length / sizeof(wchar_t), driver_path, NULL);

	PRINTF("Driver path:%S.\n", driver_path);

	//Create the service.
	if((l_service_hnd = CreateService(
	                        l_scm_hnd,
	                        SERVICE_NAME,
	                        SERVICE_NAME,
	                        SERVICE_ALL_ACCESS,
	                        SERVICE_KERNEL_DRIVER,
	                        SERVICE_DEMAND_START,
	                        SERVICE_ERROR_IGNORE,
	                        driver_path,
	                        NULL,
	                        NULL,
	                        NULL,
	                        NULL,
	                        NULL)) == NULL) {
		PRTNT_ERRCODE;
		free(driver_path);
		CloseServiceHandle(l_scm_hnd);
		MessageBox(NULL, L"Fail to create the service.Cannot initialize the daemon.", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	free(driver_path);
	PRINTF("Service created.\n");

	//Start service
	if(!StartService(l_service_hnd, 0, NULL)) {
		PRTNT_ERRCODE;
		DeleteService(l_service_hnd);
		CloseServiceHandle(l_service_hnd);
		CloseServiceHandle(l_scm_hnd);
		MessageBox(NULL, L"Fail to start the service.Cannot initialize the daemon.", L"Error", MB_OK | MB_ICONERROR);
		return false;
	}

	PRINTF("Service started.\n");
	return true;
}

void unload_driver()
{
	SERVICE_STATUS srv_status;
	ControlService(l_service_hnd, SERVICE_CONTROL_STOP, &srv_status);
	DeleteService(l_service_hnd);
	CloseServiceHandle(l_scm_hnd);
	return;
}

bool open_device()
{
	if(dev_hnd != NULL) {
		return true;
	}

	dev_hnd = CreateFile(
	              R3_SYMBOLLINK_NAME,
	              GENERIC_READ | GENERIC_WRITE, 0, 0,
	              OPEN_EXISTING,
	              FILE_ATTRIBUTE_SYSTEM, 0);

	if(dev_hnd == INVALID_HANDLE_VALUE) {
		dev_hnd = NULL;
		return false;
	}

	return true;
}
void close_device()
{
	if(dev_hnd == NULL) {
		return;
	}

	CloseHandle(dev_hnd);
}

bool read_device(char* buf, DWORD len, LPDWORD p_length_read)
{
	if(dev_hnd == NULL) {
		return false;
	}

	if(ReadFile(
	       dev_hnd, buf, len,
	       p_length_read, NULL)) {
		return true;
	}

	return false;
}

bool write_device(char* buf, DWORD size, LPDWORD p_length_written)
{
	if(dev_hnd == NULL) {
		return false;
	}

	if(WriteFile(dev_hnd, buf, size,
	             p_length_written, NULL)) {
		return true;
	}

	return false;
}

wchar_t* get_driver_file_name()
{
	BOOL sys_64_flag;

	//Test Windows platform
	#ifdef _AMD64
	sys_64_flag = TRUE;;
	#endif // _AMD64

	#ifndef _AMD64
	sys_64_flag = FALSE;

	if(IsWow64Process(
	       GetCurrentProcess(),
	       &sys_64_flag)) {
		MessageBox(NULL, L"Cannot test the version of Windows!", L"Error", MB_OK | MB_ICONERROR);
		return NULL;
	}

	#endif // !_AMD64

	//Test Windows version
	if(IsWindows8Point1OrGreater()) {
		if(sys_64_flag) {
			return L"sms_virtual_dir_driver_x64_8_1.sys";

		} else {
			return L"sms_virtual_dir_driver_Win32_8_1.sys";
		}

	} else if(IsWindows8OrGreater()) {
		if(sys_64_flag) {
			return L"sms_virtual_dir_driver_x64_8.sys";

		} else {
			return L"sms_virtual_dir_driver_Win32_8.sys";
		}

	} else if(IsWindows7OrGreater()) {
		if(sys_64_flag) {
			return L"sms_virtual_dir_driver_x64_7.sys";

		} else {
			return L"sms_virtual_dir_driver_Win32_7.sys";
		}

	} else {
		MessageBox(NULL, L"The version of Windows is too low!\r\nThe daemon needs Windows 7 or greater.",
		           L"Error", MB_OK | MB_ICONERROR);
		return NULL;
	}

	return NULL;
}