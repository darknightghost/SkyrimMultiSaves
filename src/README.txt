
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

 ------------------------------------------------------------------------

The directory structure of the tool:

-|-sms_gui.exe			The main program
 |
 |-savesets.sfg			sevesets
 |
 |-config.sfg			The config
 |
 |-mods.sfg				The info of mods
 |
 |-mods					mods
 | |
 | |-xxxx				modsxxx
 ...
 |
 |-skyrim				The directory of the game
 |
 |-savesets				The directory of savesets
 | |
 | |-0
 | |
 | |-1
 | |
 | |-2
 ...

 ------------------------------------------------------------------------
 Data in config.sfg:

 root{
	version			"version\0";
	skyrim_path		"path\0";
	main_window	""{
		x			x;
		y			y;
		height		height_of_the_main_windows;
		width		width_of_the_main_window;
	}
	save_sets	""{
		actived_save_set	id(unsigned int);
	}
	skyrim_tree		"num(unsigned long)"{
		//xxxxx is a number
		filexxxxx		"file_name\0mod_name\0src_name\0";
		dirxxxxx		"dir_name\0"{
			...
		}
		...
	}
 }

------------------------------------------------------------------------
Data in savesets.sfg:

root{
		id_0			"name(unsigned int)"{
			symbollink_tree	"num(unsigned long)"{
				//xxxxx is a number
				filexxxxx		"file_name\0mod_name\0src_name\0"{
				//If conflict
					filexxxxx		"file_name\0mod_name\0src_name\0"
				}
				dirxxxxx		"dir_name\0"{
					...
				}
				...
			}
			mods		"mod_0\0mod_1\0...mod_n\0";
			esp_1ist	"num(unsigned long)"{
				espxxxxxx		"filename\0true/false(bool)";	//False if not actived
				...
			}
		}
		id_1			"name"{
		...
		}
		...
}

------------------------------------------------------------------------
Data in mods.sfg

root{
	classifications	"class1\0class2\0class3\0...classN\0";
	mods	""{
		mod_1	"name\0classification\0"{
			mode	"fomod\0";
			count	"(unsigned int)num";
		}
		mod_2	"name\0classification\0"{
			mode	"file_list\0";
			count	"(unsigned int)num";
		}
		...
	}
}

------------------------------------------------------------------------