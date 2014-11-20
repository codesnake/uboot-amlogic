ddwin32 --list
ddwin32 of=temp.bin if=\\?\Device\Harddisk3\Partition0 count=1

ddwin32 if=%1 of=t.bin	bs=1 count=444
ddwin32 of=t.bin if=temp.bin  bs=1 skip=444 seek=444 
dir t.bin
copy t.bin j.bin
ddwin32 of=t.bin if=%1   bs=512 skip=1 seek=1

ddwin32 if=/dev/zero of=t.bin  bs=512 seek=1023 count=1
ddwin32 if=t.bin of=\\?\Device\Harddisk3\Partition0 
del *.bin
sync
date /t
time /t
