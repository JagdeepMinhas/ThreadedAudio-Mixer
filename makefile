CFLAGS = -g -Wall
LDFLAGS = -pthread

myChannels: myChannels.c
	gcc $(CFLAGS) -o myChannels myChannels.c $(LDFLAGS) -lm

clean:
	rm -f myChannels
