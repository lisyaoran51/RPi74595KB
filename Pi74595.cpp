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

//#include "clientserver.h"
//#include "demonize.h"

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
#define DI_PIN RPI_BPLUS_GPIO_J8_37 

//clock SH_CP
//#define CL_PIN RPI_GPIO_P1_16
#define CL_PIN RPI_BPLUS_GPIO_J8_33 

//latch ST_CP
//#define CE_PIN RPI_GPIO_P1_22
#define CE_PIN RPI_BPLUS_GPIO_J8_35


#define INPUT_PIN RPI_BPLUS_GPIO_J8_40 



// get bcm2835
// http://www.raspberry-projects.com/pi/programming-in-c/io-pins/bcm2835-by-mike-mccauley
// g++ Pi74595.cpp -lbcm2835

// https://appelsiini.net/2012/driving-595-shift-registers/


bool CheckKey(int key);

void Play(int key);
 
  
int main(int argc, char **argv) {
	
	
	if (!bcm2835_init())return 1;
	// Sets the pin as input.
    bcm2835_gpio_fsel(INPUT_PIN, BCM2835_GPIO_FSEL_INPT);
    // Sets the Pull-up mode for the pin.
    bcm2835_gpio_set_pud(INPUT_PIN, BCM2835_GPIO_PUD_UP);
	
	bool keyPlaying[48];
	for(int i = 0; i < 48; i++) keyPlaying[i] = false;
	
	bool running = true;
	while(running){
		for(int i = 0; i < 48; i++){
			if(CheckKey(i)){
				if(!keyPlaying[i])
					Play(i);
				
				keyPlaying[i] = true;
			}
			else{
				keyPlaying[i] = false;
			}
		}
	}
	
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
				bcm2835_gpio_write(DI_PIN, j == highBit ? HIGH : LOW);
				bcm2835_gpio_write(CL_PIN, LOW);
				bcm2835_gpio_write(CL_PIN, HIGH);
			}
		}
		else{
			for( int j = 0; j < 8; j++){
				bcm2835_gpio_write(CL_PIN, LOW);
				bcm2835_gpio_write(DI_PIN, LOW);
				bcm2835_gpio_write(CL_PIN, HIGH);
			}
		}
	}
	bcm2835_gpio_write(CE_PIN, HIGH);
	//usleep(500000);
	
	if(bcm2835_gpio_lev(INPUT_PIN) == HIGH)
		return true;
	
	return false;
	
}
	
void Play(int key){
	printf("%d press!\n", key);
}
