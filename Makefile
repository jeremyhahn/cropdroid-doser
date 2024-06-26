
AVRDUDE=/home/jhahn/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino14/bin/avrdude
CONF=-C/home/jhahn/.arduino15/packages/arduino/tools/avrdude/6.3.0-arduino14/etc/avrdude.

.PHONY: clean flash-usbasp flash-mega monitor

default: clean flash-serial monitor

clean:
	rm -rf build/

download:
	avrdude -pm328p -cusbasp -u -U flash:r:flash.hex:i # save flash as "flash.hex", in Intel format

flash-usbasp:
	avrdude -c usbasp -p m328p -u -U flash:w:build/Nano/cropdroid-doser.hex

flash-nano:
	avrdude -v -patmega328p -carduino -P/dev/ttyUSB0 -b115200 -D -U flash:w:build/Nano/cropdroid-doser.hex

flash-mega:
	avrdude -c usbasp -p m2560 -u -U flash:w:build/Mega/cropdroid-doser.hex

monitor:
	screen /dev/ttyUSB0 115200

bootloader:
	$(AVRDUDE) $(CONF) -v -patmega328p -cusbasp -Pusb -e -Ulock:w:0x3F:m -Uefuse:w:0xFD:m -Uhfuse:w:0xDA:m -Ulfuse:w:0xFF:m
	$(AVRDUDE) $(CONF) -v -patmega328p -cusbasp -Pusb -Uflash:w:bootloader/optiboot_atmega328.hex:i -Ulock:w:0x0F:m
