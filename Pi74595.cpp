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


#include "paplay_8c.h"
#include <sys/shm.h>

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




// get bcm2835
// http://www.raspberry-projects.com/pi/programming-in-c/io-pins/bcm2835-by-mike-mccauley
// g++ Pi74595.cpp -lbcm2835 -pthread

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

pthread_t* handler;


bool CheckKey(int key);

void Play(int key);
 
void AplayString(string s, int key);

int SetPA(int key);
int PlayPA(int key);
int PlayPAWithThread(void* key); 
 
int main(int argc, char **argv) {
	
	// 把thread址標清掉
	handler = NULL;
	KeyStartSet* keyStartSet = NULL;
	
	// share memory
	
	int shmid;
	key_t key;
	if((key = ftok(".", 1)) < 0){
		printf("ftok error:%s\n", strerror(errno));
		return -1;
    }
	
	if((shmid = shmget(key, BUFFER_SIZE, SHM_R|SHM_W|IPC_CREAT)) < 0){
		printf("shmget error:%s\n", strerror(errno));
		return -1;
    }
	
	if((keyStartSet = (KeyStartSet*)shmat(shmid, NULL, 0)) == (void*)-1){
		printf("shmat error:%s\n", strerror(errno));
		return -1;
	}
   
   // share memory
	
	// setup PA
	
	int pid[48];
	
	bool keyPlaying[48];
	for(int i = 0; i < 48; i++) {
		keyPlaying[i] = false;
	}
	for(int i = 0; i < 48; i++) {
		if(!(pid[i] = SetPA(i)))
			return 0;
	}
	
	// setup PA
	
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
					//Play(i);
					
					// 叫tread來playPA，看會不會快一點
					pthread_t pt;
					int* tempPlayingKey = new int(i);
					pthread_create(&pt, NULL, PlayPAWithThread, tempPlayingKey);
					
					// 直接呼叫playPA，速度有點慢，試試看用thread
					//if(!(pid[i] = PlayPA(i)))
					//	return 0;
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
	for(int i = 0; i < 48; i++){
		string s = string("kill ") + to_string(pid[i]);
		system(s.c_str());
	}
	if(shmdt(keyStartSet) < 0){
		perror("shmdt");
		return -1;
	}
	shmctl(shmid, IPC_RMID, NULL);
	system("ipcs -m");
	
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
	
	usleep(200);
	if(bcm2835_gpio_lev(INPUT_PIN) == HIGH)
		return true;
	
	return false;
	
}

int SetPA(int key){
	
	int pitch = key + 24;
	
	int fpid = fork();  
    if (fpid < 0)  
        printf("error in fork!");  
    else if (fpid == 0)  {
        printf("process id %d, Setting my pitch [%d]\n", getpid(), pitch); 
		
		char* part1 = "audio_cut/German_Concert_D_0";
		char* part2 = malloc(3);
		char* part3 = "_083.wav";
		sprintf(part2, "%ld", pitch+21-9);

		char* path = malloc(strlen(part1) + strlen(part2) + strlen(part3) + 1); /* make space for the new string (should check the return value ...) */
		strcpy(path, part1); /* copy name into the new var */
		strcat(path, part2); /* add the extension */
		strcat(path, part3); /* add the extension */
		
		SetSound(pitch, path);
	}	
    else  {
        // no-op
	}
    return fpid;  
}

int PlayPAWithThread(void* key){
	
	int pitch = *((int*)key) + 24;
	
	delete (int*)key;
	
	printf("Pitch [%d] played!!!!\n", pitch);
	
	PlayPaSound(pitch);
	
	int fpid = fork();  
    if (fpid < 0)  
        printf("error in fork!");  
    else if (fpid == 0)  {
        printf("process id %d, Setting my pitch [%d]\n", getpid(), pitch); 
		
		char* part1 = "audio_cut/German_Concert_D_0";
		char* part2 = malloc(3);
		char* part3 = "_083.wav";
		sprintf(part2, "%ld", pitch+21-9);

		char* path = malloc(strlen(part1) + strlen(part2) + strlen(part3) + 1); /* make space for the new string (should check the return value ...) */
		strcpy(path, part1); /* copy name into the new var */
		strcat(path, part2); /* add the extension */
		strcat(path, part3); /* add the extension */
		
		SetSound(pitch, path);
	}	
    else  {
        //no-op
	}
    return fpid;  
}

int PlayPA(int key){
	
	int pitch = key + 24;
	
	printf("Pitch [%d] played!!!!\n", pitch);
	
	PlayPaSound(pitch);
	
	int fpid = fork();  
    if (fpid < 0)  
        printf("error in fork!");  
    else if (fpid == 0)  {
        printf("process id %d, Setting my pitch [%d]\n", getpid(), pitch); 
		
		char* part1 = "audio_cut/German_Concert_D_0";
		char* part2 = malloc(3);
		char* part3 = "_083.wav";
		sprintf(part2, "%ld", pitch+21-9);

		char* path = malloc(strlen(part1) + strlen(part2) + strlen(part3) + 1); /* make space for the new string (should check the return value ...) */
		strcpy(path, part1); /* copy name into the new var */
		strcat(path, part2); /* add the extension */
		strcat(path, part3); /* add the extension */
		
		SetSound(pitch, path);
	}	
    else  {
        //no-op
	}
    return fpid;  
}
	
void Play(int key){
	//printf("%d press!\n", key);
	
	int pitch = key + 24;
	
	string s = string("Audio/German_Concert_D_0") + to_string(pitch+21-9) + string("_083.wav");
	
	thread t(AplayString, s, key);
	
	printf("[%d] ", key);
	
	if(handler[key]){
		pthread_cancel(handler[key]);
		printf("The last process num is %d. ", handler[key]);
	}
	handler[key] = t.native_handle();
	printf("The new process num is %d.\n", handler[key]);
	
	t.detach();
	
}

void AplayString(string s, int key){
	system(s.c_str());
	printf("[%d] Process %d ends.\n", key, handler[key]);
	handler[key] = NULL;
}
