rm -f Pi74595 Pi74595.o paplay_8c.o
g++ -ggdb -Wall paplay_8c.c -c -o paplay_8c.o -I/home/pi/pulseaudio/src -L/home/pi/pulseaudio/src/.libs -lpulse -lsndfile -lpthread -fpermissive
g++ Pi74595.cpp -c -o Pi74595.o -lbcm2835 -pthread -fpermissive
g++ Pi74595.o paplay_8c.o -o Pi74595 -L./ -lbcm2835 -pthread -fpermissive -I/home/pi/pulseaudio/src -L/home/pi/pulseaudio/src/.libs -lpulse -lsndfile