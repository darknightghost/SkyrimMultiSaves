#! /bin/python
# -*- coding: utf-8 -*-

import os
import re
import shutil
import subprocess

g_sys_file_name = "[^\n]*\.sys"
g_exe_file_name = "[^\n]*\.exe"
g_dll_file_name = "[^\n]*\.dll"
g_driver_debug_path = "../../bin/driver/Debug"
g_driver_release_path = "../../bin/driver/Release"
g_win32_debug_path = "../../bin/Win32/Debug"
g_win32_release_path = "../../bin/Win32/Release"
g_x64_debug_path = "../../bin/x64/Debug"
g_x64_release_path = "../../bin/x64/Release"
g_package_path = "../../bin/package"
g_package_win32_debug = "../../bin/package/sms_win32_debug"
g_package_win32_release = "../../bin/package/sms_win32_release"
g_package_x64_debug = "../../bin/package/sms_x64_debug"
g_package_x64_release = "../../bin/package/sms_x64_release"

def pack(src,package_name):
        global g_package_path
        os.system("7z.exe a \""+g_package_path+"/"+package_name+"\" \""+src+"/\" -mx=9 -mf -mhc -mhcf -ms -mmt -r")

def copy_files(src,dest):
        global g_sys_file_name
        global g_exe_file_name
        global g_dll_file_name
        
        s = re.compile(g_sys_file_name)
        e = re.compile(g_exe_file_name)
        d = re.compile(g_dll_file_name)

        file_list=os.listdir(src)

        for file_name in file_list:

                #Match file names
                s_match=s.match(file_name)
                e_match=e.match(file_name)
                d_match=d.match(file_name)
                if s_match != None or e_match != None or d_match != None:
                        print("Copying \""+src+"/"+file_name+"\" to \""+dest+"/"+file_name+"\".\n")
                        shutil.copyfile(src+"/"+file_name,dest+"/"+file_name)

def main():
        global g_driver_debug_path
        global g_driver_release_path
        global g_win32_debug_path
        global g_win32_release_path
        global g_x64_debug_path
        global g_x64_release_path
        global g_package_path
        global g_package_win32_debug
        global g_package_win32_release
        global g_package_x64_debug
        global g_package_x64_release

        shutil.rmtree(g_package_path,True)
        #Create folders
        os.mkdir(g_package_win32_debug)
        os.mkdir(g_package_win32_release)
        os.mkdir(g_package_x64_debug)
        os.mkdir(g_package_x64_release)

        #Copy files
        print("Copying files...\n")
        #Win32 Debug
        copy_files(g_driver_debug_path,g_package_win32_debug)
        copy_files(g_win32_debug_path,g_package_win32_debug)
        #Win32 release
        copy_files(g_driver_release_path,g_package_win32_release)
        copy_files(g_win32_release_path,g_package_win32_release)
        #X64 Debug
        copy_files(g_driver_debug_path,g_package_x64_debug)
        copy_files(g_x64_debug_path,g_package_x64_debug)
        #X64 Release
        copy_files(g_driver_release_path,g_package_x64_release)
        copy_files(g_x64_release_path,g_package_x64_release)

        pack(g_package_win32_debug,"sms_2_0_0_alpha_win32_debug.7z")
        pack(g_package_win32_release,"sms_2_0_0_alpha_win32_release.7z")
        pack(g_package_x64_debug,"sms_2_0_0_alpha_x64_debug.7z")
        pack(g_package_x64_release,"sms_2_0_0_alpha_x64_release.7z")

main()
