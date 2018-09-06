#ifndef PAPLAY_8C_H
#define PAPLAY_8C_H


static bool startPlay[100];

int PlayPaSound(int pitch);
int SetSound(int pitch, char *argv);


typedef struct {
	int pitch;
	char* path;
} PaSoundSet;

#endif