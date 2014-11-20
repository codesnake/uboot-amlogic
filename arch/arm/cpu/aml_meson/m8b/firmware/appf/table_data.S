; Copyright (C) 2010 ARM Limited                           
;
; This software is provided 'as-is', without any express or implied
; warranties including the implied warranties of satisfactory quality, 
; fitness for purpose or non infringement.  In no event will  ARM be 
; liable for any damages arising from the use of this software.
;
; Permission is granted to anyone to use, copy and modify this software for 
; any purpose, and to redistribute the software, subject to the following 
; restrictions:
;
; 1. The origin of this software must not be misrepresented; you must not
;    claim that you wrote the original software. If you use this software
;    in a product, an acknowledgment in the product documentation would be
;    appreciated but is not required.                                       
; 2. Altered source versions must be plainly marked as such, and must not be
;    misrepresented as being the original software.
; 3. This notice may not be removed or altered from any source distribution.

	AREA  APPF_TABLE_DATA, DATA, ALIGN=12
	PRESERVE8

	EXPORT main_table
	
main_table	
	SPACE	256


	AREA  APPF_TRANSLATION_TABLE2, DATA, ALIGN=10
	PRESERVE8

	EXPORT appf_translation_table2a
	EXPORT appf_translation_table2b
	
appf_translation_table2a
	SPACE	1024
appf_translation_table2b
	SPACE	1024

	
	AREA  APPF_TRANSLATION_TABLE1, DATA, ALIGN=14
	PRESERVE8

	EXPORT appf_translation_table1
	
appf_translation_table1
	SPACE	16384

	AREA  APPF_DEVICE_MEMORY, DATA, ALIGN=12
	PRESERVE8

	EXPORT appf_device_memory
	
appf_device_memory
	SPACE	4096


	END
