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
#define true 1
#define false 0

#define LAMPS_COUNT 40

// get bcm2835
// http://www.raspberry-projects.com/pi/programming-in-c/io-pins/bcm2835-by-mike-mccauley
// g++ Pi74595.cpp -lbcm2835

// https://appelsiini.net/2012/driving-595-shift-registers/





 
  
int main(int argc, char **argv) {
	
	
	if (!bcm2835_init())return 1;
    // Sets the pin as input.
    bcm2835_gpio_fsel(KEY, BCM2835_GPIO_FSEL_INPT);
    // Sets the Pull-up mode for the pin.
    bcm2835_gpio_set_pud(KEY, BCM2835_GPIO_PUD_UP);
    printf("Key Test Program!!!!\n");  
    while (1)
    {  
        // Reads the current level on the specified pin and returns either HIGH or LOW (0 or 1).
        if(bcm2835_gpio_lev(KEY) == 0)
        {  
            printf ("KEY PRESS\n") ;
            while(bcm2835_gpio_lev(KEY) == 0)
                bcm2835_delay(100);
        }  
        bcm2835_delay(100);
    }  
	
	
	bool change = true;
	lampindex = 0;
    while (1) {
		
				bcm2835_gpio_write(CE_PIN, LOW);
				
				   for (c = 0; c < 8; c++) {
						//usleep(interval);
						bcm2835_gpio_write(CL_PIN, LOW);
						bcm2835_gpio_write(DI_PIN,change ? HIGH : LOW );
						//bcm2835_gpio_write(DI_PIN, LOW);
						bcm2835_gpio_write(CL_PIN, HIGH);
						//usleep(interval);
					}
					change = !change;
					printf(" %d\n", change?1:0);
				//usleep(50 * interval);
				bcm2835_gpio_write(CE_PIN, HIGH);
			//}
		}

    bcm2835_close();
    return 0;
}
