#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include <bcm2835.h>
#include <string>
#include <thread>


//#include "paplay_8c.h"
#include <sys/shm.h>
#include <alsa/asoundlib.h>

// https://github.com/mignev/shiftpi
/*

SER = 25 (GPIO RPI) #pin 14 on the 75HC595
DS

RCLK = 24 (GPIO RPI) #pin 12 on the 75HC595
ST_CP

SRCLK = 23 (GPIO RPI) #pin 11 on the 75HC595
SH_CP

*/

//data DS 
//#define DI_PIN RPI_GPIO_P1_18 
#define DI_PIN RPI_BPLUS_GPIO_J8_31 

//clock SH_CP
//#define CL_PIN RPI_GPIO_P1_16
#define CL_PIN RPI_BPLUS_GPIO_J8_26 

//latch ST_CP
//#define CE_PIN RPI_GPIO_P1_22
#define CE_PIN RPI_BPLUS_GPIO_J8_29


#define INPUT_PIN RPI_BPLUS_GPIO_J8_40 

#define FORK_SIZE 5
#define QUEUE_SIZE 8
#define WAV_SIZE 8000
#define KEY_SIZE 48

struct KeyStartSet {
	int ForkFlag;
	
	bool QueueLock;
    int KeyStart[QUEUE_SIZE];
	int QueueHead;
	int QueueTail;
	
	short WavData[QUEUE_SIZE][WAV_SIZE];
};

#define SAMPLE_RATE   8000
#define NUM_CHANNELS  1
#define SILENCE_LENGTH 400

// get bcm2835
// http://www.raspberry-projects.com/pi/programming-in-c/io-pins/bcm2835-by-mike-mccauley
// g++ Pi74595.cpp -lbcm2835 -pthread -lasound -fpermissive

// rm -f Pi74595 Pi74595.o paplay_8c.o
// g++ -ggdb -Wall paplay_8c.c -c -o paplay_8c.o -I/home/pi/pulseaudio/src -L/home/pi/pulseaudio/src/.libs -lpulse -lsndfile -lpthread -fpermissive
// g++ Pi74595.cpp -c -o Pi74595.o -lbcm2835 -pthread -fpermissive
// g++ Pi74595.o paplay_8c.o -o Pi74595 -L./ -lbcm2835 -pthread -fpermissive -I/home/pi/pulseaudio/src -L/home/pi/pulseaudio/src/.libs -lpulse -lsndfile


// killall Pi74595

// 要先打開pulseaudio
// pulseaudio -D --system 

// https://appelsiini.net/2012/driving-595-shift-registers/

// i+21-9

// 刪thread
// https://www.bo-yang.net/2017/11/19/cpp-kill-detached-thread

// 音源
// https://www.raspberrypi.org/blog/tinkernut-diy-pi-zero-audio/

using namespace std;

int ffff(){
	
	int a = fork();
	printf("fork return %d\n", a);
	return a;
}

bool CheckKey(int key);

void Play(int key);
 
void AplayString(string s, int key);

int SetAlsa(int flag);

int PlayAlsaSHM(short* wavData, KeyStartSet* keyStartSet);
int PlayPA(int key);
int PlayPAWithThread(void* key); 
 
int main(int argc, char **argv) {
	printf("Start program\n");
	
	
	
	snd_pcm_t *handle;
		
	// Open the PCM output
	int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	
	// Configure parameters of PCM output
	err = snd_pcm_set_params(handle,
		SND_PCM_FORMAT_S16_LE,
		SND_PCM_ACCESS_RW_INTERLEAVED,
		NUM_CHANNELS,
		SAMPLE_RATE,
		0,			// Allow software resampling
		50000);		// 0.05 seconds per buffer
	
	short silence[SILENCE_LENGTH];
	for(int i = 0; i < SILENCE_LENGTH; i++)	// memset?
		silence[i] = 0;
		
	short wavData1[WAV_SIZE];
	
	string s = string("mono_audio/German_Concert_D_0") + to_string(38) + string("_083.wav");
	
	FILE *file = fopen(s.c_str(), "r");
	if (file == NULL) {
		fprintf(stderr, "ERROR: Unable to open file %s.\n", s.c_str());
		exit(EXIT_FAILURE);
	}
	
	int sizeInBytes = ftell(file);
	printf("size of file %d\n", sizeInBytes);
	
	fseek(file, 44, SEEK_SET);	// header 44 byte
	fread(wavData1, sizeof(short), WAV_SIZE, file);
	
	fclose(file);
	snd_pcm_sframes_t frames = snd_pcm_writei(handle, wavData1, WAV_SIZE);
	
	// Check for errors
	if (frames < 0)
		frames = snd_pcm_recover(handle, frames, 0);
	if (frames < 0) {
		fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n", frames);
		exit(EXIT_FAILURE);
	}
	if (frames > 0 && frames < WAV_SIZE)
		printf("Short write (expected %d, wrote %li)\n", WAV_SIZE, frames);
	
	return 0;
	
	
	
	
	
	
	/* fork幾個播音樂的程式 */
	for(int i = 0; i < 5; i++){
		if(SetAlsa(i) == 0)
			return 0;
	}
	
	printf("fork done\n");
	printf("size of keystartset %d\n", sizeof(KeyStartSet));
	// share memory
	
	int shmid;
	key_t key;
	if((key = ftok(".", 1)) < 0){
		printf("ftok error:%s\n", strerror(errno));
		return -1;
    }
	
	if((shmid = shmget(key, sizeof(KeyStartSet), SHM_R|SHM_W|IPC_CREAT)) < 0){
		printf("shmget error:%s\n", strerror(errno));
		return -1;
    }
	
	KeyStartSet* keyStartSet = NULL;
	if((keyStartSet = (KeyStartSet*)shmat(shmid, NULL, 0)) == (void*)-1){
		printf("shmat error:%s\n", strerror(errno));
		return -1;
	}
	
	keyStartSet->ForkFlag = 0;
	keyStartSet->QueueLock = false;
	keyStartSet->QueueHead = 0;
	keyStartSet->QueueTail = 0;
   
	printf("SHM done\n");
   // share memory
	
	// setup
	
	bool keyPlaying[48];
	short wavData[KEY_SIZE][WAV_SIZE];
	for(int i = 0; i < 48; i++) {
		keyPlaying[i] = false;
		
		int pitch = i + 24;
		
		string s = string("mono_audio/German_Concert_D_0") + to_string(pitch+21-9) + string("_083.wav");
		
		FILE *file = fopen(s.c_str(), "r");
		if (file == NULL) {
			fprintf(stderr, "ERROR: Unable to open file %s.\n", s.c_str());
			exit(EXIT_FAILURE);
		}
		
		fseek(file, 44, SEEK_SET);	// header 44 byte
		fread(wavData[i], sizeof(short), WAV_SIZE, file);
		
		fclose(file);
	}
	
	// setup
	
	// bcm2835
	
	if (!bcm2835_init())return 1;
	
	bcm2835_gpio_fsel(DI_PIN, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CL_PIN, BCM2835_GPIO_FSEL_OUTP);
	bcm2835_gpio_fsel(CE_PIN, BCM2835_GPIO_FSEL_OUTP);
	
	// Sets the pin as input.
    bcm2835_gpio_fsel(INPUT_PIN, BCM2835_GPIO_FSEL_INPT);
    // Sets the Pull-up mode for the pin.
    bcm2835_gpio_set_pud(INPUT_PIN, BCM2835_GPIO_PUD_UP);
	
	// bcm2835
	
	// running
	
	bool running = true;
	while(running){
		for(int i = 0; i < 48; i++){
			if(CheckKey(i)){
				if(!keyPlaying[i]){
					PlayAlsaSHM(wavData[i], keyStartSet);
				}
				keyPlaying[i] = true;
			}
			else{
				keyPlaying[i] = false;
			}
		}
	}
	
	// running
	
	bcm2835_close();
	return 0;
}
	
	
bool CheckKey(int key){
	
	int registerNumber = key / 12 * 2 + (key % 12) / 8;
	int highBit = key % 12 % 8;
	
	//printf("%d %d\n", registerNumber, highBit);
	
	bcm2835_gpio_write(CE_PIN, LOW);
	
	for(int i = 7; i >= 0; i--){
		
		if(i == registerNumber){
			
			// register是從最後一顆開始往回存，7->0
			for( int j = 7; j >= 0; j--){
				//usleep(1000);
				bcm2835_gpio_write(DI_PIN, j == highBit ? HIGH : LOW);
				//usleep(1000);
				bcm2835_gpio_write(CL_PIN, LOW);
				//usleep(1000);
				bcm2835_gpio_write(CL_PIN, HIGH);
			}
		}
		else{
			for( int j = 0; j < 8; j++){
				//usleep(1000);
				bcm2835_gpio_write(DI_PIN, LOW);
				//usleep(1000);
				bcm2835_gpio_write(CL_PIN, LOW);
				//usleep(1000);
				bcm2835_gpio_write(CL_PIN, HIGH);
			}
		}
	}
	//usleep(1000);
	bcm2835_gpio_write(CE_PIN, HIGH);
	
	usleep(50);
	if(bcm2835_gpio_lev(INPUT_PIN) == HIGH)
		return true;
	
	return false;
	
}

int SetAlsa(int flag){
	
	printf("start fork #%d\n", flag); 
	
	int fpid = fork();  
    if (fpid < 0)  
        printf("error in fork!");  
    if (fpid == 0)  {
		
		for(int i = 0; i < 10; i++)
			printf("process %d start!", flag); 
		
		printf("process %d start!\n", flag); 
		/* share memory */
		int shmid;
		key_t key;
		if((key = ftok(".", 1)) < 0){
			printf("ftok error:%s\n", strerror(errno));
			return fpid;
		}
		
		if((shmid = shmget(key, sizeof(KeyStartSet), SHM_R|SHM_W)) < 0){
			printf("shmget error:%s\n", strerror(errno));
			return fpid;
		}
		
		KeyStartSet* keyStartSet = NULL;
		if((keyStartSet = (KeyStartSet*)shmat(shmid, NULL, 0)) == (void*)-1){
			printf("shmat error:%s\n", strerror(errno));
			return fpid;
		}
		
		printf("start alsa #%d\n", flag);
		/* alsa */
		snd_pcm_t *handle;
		
		// Open the PCM output
		int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
		
		// Configure parameters of PCM output
		err = snd_pcm_set_params(handle,
			SND_PCM_FORMAT_S16_LE,
			SND_PCM_ACCESS_RW_INTERLEAVED,
			NUM_CHANNELS,
			SAMPLE_RATE,
			0,			// Allow software resampling
			50000);		// 0.05 seconds per buffer
		
		short silence[SILENCE_LENGTH];
		for(int i = 0; i < SILENCE_LENGTH; i++)	// memset?
			silence[i] = 0;
			
		short wavData[WAV_SIZE];
		
		string s = string("mono_audio/German_Concert_D_0") + to_string(38) + string("_083.wav");
		
		FILE *file = fopen(s.c_str(), "r");
		if (file == NULL) {
			fprintf(stderr, "ERROR: Unable to open file %s.\n", s.c_str());
			exit(EXIT_FAILURE);
		}
		
		fseek(file, 44, SEEK_SET);	// header 44 byte
		fread(wavData, sizeof(short), WAV_SIZE, file);
		
		fclose(file);
		snd_pcm_writei(handle, wavData, WAV_SIZE);
		
		/* loop */
		
		printf("start loop #%d\n", flag);
		while(1){
			
			while(keyStartSet->ForkFlag  != flag 					|| 
				  keyStartSet->QueueHead == keyStartSet->QueueTail  || 
				  keyStartSet->QueueLock)
				snd_pcm_writei(handle, silence, SILENCE_LENGTH);
			
			printf("receive play at %d\n", flag);
			
			memcpy(wavData, keyStartSet->WavData[keyStartSet->QueueHead], sizeof(wavData));
			
			printf("send data: %d %d %d %d %d %d %d %d\n", wavData[0], wavData[1], wavData[2], wavData[3], wavData[4], wavData[5], wavData[6], wavData[7]);
			
			keyStartSet->QueueHead = keyStartSet->QueueHead == QUEUE_SIZE-1 ?			 0 			: keyStartSet->QueueHead+1;
			keyStartSet->ForkFlag  = keyStartSet->ForkFlag  == FORK_SIZE-1  ?            0          : keyStartSet->ForkFlag+1;
		
			snd_pcm_writei(handle, wavData, WAV_SIZE);
		}
		
	}	
	
    return fpid;
}

int PlayAlsaSHM(short* wavData, KeyStartSet* keyStartSet){
	
	printf("play!!\n");
	
	keyStartSet->QueueLock = true;
	
	memcpy(keyStartSet->WavData[keyStartSet->QueueTail], wavData, sizeof(short) * WAV_SIZE);
	
	printf("send data: %d %d %d %d %d %d %d %d\n", wavData[0], wavData[1], wavData[2], wavData[3], wavData[4], wavData[5], wavData[6], wavData[7]);
	
	keyStartSet->QueueTail = keyStartSet->QueueTail == QUEUE_SIZE-1 ?			 0 			: keyStartSet->QueueTail+1;
	
	keyStartSet->QueueLock = false;
	
	return 1;
}
	
void Play(int key){
	//printf("%d press!\n", key);
	
	int pitch = key + 24;
	
	string s = string("Audio/German_Concert_D_0") + to_string(pitch+21-9) + string("_083.wav");
	
	thread t(AplayString, s, key);
	
	printf("[%d] ", key);
	
	t.detach();
	
}

void AplayString(string s, int key){
	system(s.c_str());
}
