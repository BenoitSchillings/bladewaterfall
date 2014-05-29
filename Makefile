all: fft_blade


fft_blade: ui.c fft_blade.c blde.h fft.c
	g++  -O4   -lGL -lglut -DENABLE_LIBBLADERF_SYNC -DHAVE_LIBUSB_GET_VERSION -I/home/benoit/bladeRF/host/libraries/libbladeRF/include -I/home/benoit/bladeRF/host/libraries/libbladeRF/src -I/home/benoit/bladeRF/host/build/common/include -I/home/benoit/bladeRF/host/common/include -I/home/benoit/bladeRF/host/common/include/ -I/home/benoit/bladeRF/host/../firmware_common -I/home/benoit/bladeRF/host/libraries/libbladeRF/src -I/opt/local/include/libusb-1.0  -c    fft_blade.c -o fft_blade.c.o
	g++   -O4 -c  -lGL -lglut ui.c -o ui.c.o	
	cc  ui.c.o fft_blade.c.o libbladeRF.so.0  -o fft_blade  -lpthread -lm -lGL -lglut



clean:
	-rm fft_blade
	-rm -rf *.dSYM
