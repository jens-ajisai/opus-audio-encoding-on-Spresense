echo if the calls fail with oggz/oggz.h: No such file or directory then you forgot to adapt the platform.txt and boards.txt of the Arduino SDK folder.
echo 
echo 
cd MainAudio
arduino-cli compile --fqbn SPRESENSE_local:spresense:spresense --board-options "Core=Main,Memory=896,Debug=Disabled,UploadSpeed=230400"
arduino-cli upload -p /dev/tty.SLAB_USBtoUART --fqbn SPRESENSE_local:spresense:spresense --board-options "Core=Main,Memory=896,Debug=Disabled,UploadSpeed=230400" 

cd ../EncoderSub
arduino-cli compile --fqbn SPRESENSE_local:spresense:spresense --board-options "Core=Sub1,Memory=896,Debug=Disabled,UploadSpeed=230400"
arduino-cli upload -p /dev/tty.SLAB_USBtoUART --fqbn SPRESENSE_local:spresense:spresense --board-options "Core=Sub1,Memory=896,Debug=Disabled,UploadSpeed=230400" 

ffplay -i udp://192.168.1.200:20003 &
python scripts/serialProxy.py
