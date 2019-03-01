ORG := jeremyhahn
PACKAGE := harvest.nutes
TARGET_OS := linux

.PHONY: clean flash-usbasp flash-mega monitor

default: clean flash-usbasp flash-serial monitor

clean:
	rm -rf build/

flash-usbasp:
	avrdude -c usbasp -p m328p -u -U flash:w:build/Nano/harvest-nutes.hex

flash-nano:
	avrdude -v -patmega328p -carduino -P/dev/ttyUSB0 -b115200 -D -U flash:w:build/Nano/harvest-nutes.hex

flash-mega:
	avrdude -c usbasp -p m2560 -u -U flash:w:build/Mega/harvest-nutes.hex

monitor:
	screen /dev/ttyUSB0 115200
