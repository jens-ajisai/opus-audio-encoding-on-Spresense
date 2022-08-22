echo if the calls fail with oggz/oggz.h: No such file or directory then you forgot to adapt the platform.txt and boards.txt of the Arduino SDK folder.
echo 
echo 
cd opusRecorderMain
arduino-cli compile --fqbn SPRESENSE_local:spresense:spresense --board-options "Core=Main,Memory=896,Debug=Disabled,UploadSpeed=230400"
arduino-cli upload -p /dev/tty.SLAB_USBtoUART --fqbn SPRESENSE_local:spresense:spresense --board-options "Core=Main,Memory=896,Debug=Disabled,UploadSpeed=230400" 

cd ../opusenc
arduino-cli compile --fqbn SPRESENSE_local:spresense:spresense --board-options "Core=Sub1,Memory=896,Debug=Disabled,UploadSpeed=230400" -v

echo 
echo 
echo Rename the opusenc.ino.elf file to OPUSENC and copy it to SD card.
echo Info: This does not yet work.
