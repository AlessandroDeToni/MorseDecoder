NodeMCU setup:
1) download drivers for cp2102 https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers. For windows 64bit I chose CP210x VCP Windows
2) In arduino IDE: preferences->additional board manager URLs-> write: http://arduino.esp8266.com/stable/package_esp8266com_index.json
3) In arduino IDE: tools->board->select NODEMCU v1.0

You should be done, try to load an example code to see if everything works. For example try blink.
