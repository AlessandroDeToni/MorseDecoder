# MorseDecoder

 ## NodeMCU setup:
1) download drivers for cp2102 https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers. For windows 64bit I chose CP210x VCP Windows
2) In arduino IDE: preferences->additional board manager URLs-> write: http://arduino.esp8266.com/stable/package_esp8266com_index.json
3) In arduino IDE: tools->board->select NODEMCU v1.0

You should be done, try to load an example code to see if everything works. For example try blink.

## OLED display connessione PIN (sinistra = display, destra = NodeMCU):
1) SDA a D2
2) SCL a D1 
3) GND a GND
4) Vcc a 3v3

## connessione bottone a NodeMCU:
1) Posizionare il bottone al centro della breadboard
2) Scegliere un lato e collegare un pin a 3.3V attraverso un cavo
3) Nello stesso lato, collegare l'altro pin a 0V attrverso una resistenza da 1kOhm
4) Passare al lato opposto e collegare il pin del bottone, che è in cortocircuito con la resistenza, al pin D6 del NodeMCU

Dopo queste parole spese inutilmente, la foto di seguito sarà sicuramente di maggiore aiuto:
<img src="https://user-images.githubusercontent.com/51931398/156933949-fe1de5bd-bd80-405c-b1ec-181c3da9cd92.png" alt="Alt text" style = "max-width: 5px">
