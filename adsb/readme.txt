run

./fft_blade -f 1090500000 -b 8 -g 35

this will do a pretty good job outputing the raw adsb-b message with checksum and error correction done.


I do have a hackedup version of rtl1090 which can be used to pipe the output to get a real time display.
I will update the dump1090 code, all I added is a dum mode which will pipe stdin as a list of adsb messges.


just run

./fft_blade -f 1090500000 -b 8 -g 35 | dump1090x


