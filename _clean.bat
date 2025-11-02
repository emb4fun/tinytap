@echo off

del *.bak /S
cd bootloader
call _clean.bat
cd ..
cd firmware
call _clean.bat
cd ..
cd tools\tinytap
call _clean.bat
cd ..\..
