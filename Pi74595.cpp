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

    //struct sockaddr_in si_local, si_remote;
    int s, i, j, interval ,n , lastpacketsize, lampindex, c;
	
	interval = 1000;
	
    int port;
    size_t slen;
    //char buf[BUFLEN];
    //char laststate[BUFLEN];
	int lamps[LAMPS_COUNT];
	int map[LAMPS_COUNT];
	map[0] = 33;
	map[1] = 34; 
	map[2] = 35; 
	map[3] = 36; 
	map[4] = 37; 
	map[5] = 38; 
	map[6] = 39; 
	map[7] = 3;
	map[8] = 4; 
	map[9] = 5; 
	map[10] = 6; 
	map[11] = 7; 
	map[12] = 8; 
	map[13] = 9; 
	map[14] = 10; 
	map[15] = 11; 
	map[16] = 12; 
	map[17] = 13; 
	map[18] = 14; 
	map[19] = 15; 
	map[20] = 16; 
	map[21] = 17; 
	map[22] = 18; 
	map[23] = 19; 
	map[24] = 20; 
	map[25] = 21; 
	map[26] = 22; 
	map[27] = 23; 
	map[28] = 24; 
	map[29] = 25; 
	map[30] = 26; 
	map[31] = 27; 
	map[32] = 29; 
	map[33] = 28; 
	map[34] = 30; 
	map[35] = 31; 
	map[36] = 32;
	map[37] = 0;
	map[38] = 1; 
	map[39] = 2;
	
	
 	if (!bcm2835_init()){
		return 1;
	}
	 
	lastpacketsize = 0;
	bcm2835_gpio_fsel(DI_PIN, BCM2835_GPIO_FSEL_OUTP); 
	bcm2835_gpio_fsel(CL_PIN, BCM2835_GPIO_FSEL_OUTP); 
	bcm2835_gpio_fsel(CE_PIN, BCM2835_GPIO_FSEL_OUTP); 

    //slen = sizeof (si_remote);
    //if (argc != 3) {
    //    fprintf(stderr, "Usage: %s <port number>  <interval> \n", argv[0]);
    //    exit(EXIT_FAILURE);
    //}

    //interval = atoi(argv[2]);
    //port = atoi(argv[1]);
    //if (port < 1024) {
    //    fprintf(stderr, "Usage: %s <port number>  <time delay>  \n", argv[0]);
    //    fprintf(stderr, "\twhere <port number> shall be > 1023\n");
    //    exit(EXIT_FAILURE);
    //}

/*  demonize(argv[0]); */
//	demonize(argv[0]);
	
    //if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
    //    perror("socket");
    //    exit(EXIT_FAILURE);
    //}

    //memset((char *) &si_local, 0, sizeof (si_local));
    //si_local.sin_family = AF_INET;
    //si_local.sin_port = htons(port);
    //si_local.sin_addr.s_addr = htonl(INADDR_ANY);
    //if (bind(s, (const struct sockaddr *) &si_local, sizeof (si_local)) == -1) {
    //    perror("bind");
    //    exit(EXIT_FAILURE);
    //}
	
	bool change = true;
	lampindex = 0;
    while (1) {
			//memset(buf, 0, sizeof (char) *BUFLEN);
			//n=recvfrom(s, buf, BUFLEN, 0, (struct sockaddr *) &si_remote, &slen);
			/* 
			//if (
			== -1) {
				perror("recvfrom()");
				exit(EXIT_FAILURE);
			}*/
			
			/*  printf("Data received %i ", buf[0] ); */
			
			//if(buf[0]==50){
			//	sendto(s,laststate,lastpacketsize,0,(struct sockaddr *)&si_remote,slen);
			//	printf("state request %s:%d \n", inet_ntoa(si_remote.sin_addr), ntohs(si_remote.sin_port));
			//}else{
				/* printf("%s:%d ", inet_ntoa(si_remote.sin_addr), ntohs(si_remote.sin_port)); */
				//laststate[0]=buf[0]; 
				//lastpacketsize = n;
                //
				//lampindex = 0;
				//for (j = 1; j < buf[0]; j++) {
				//	laststate[j]=buf[j];
				//   for (i = 0; i < 8; i++) {
				//		/* printf(" %d - %d \n", lampindex , map[lampindex]); */
				//		lamps[map[lampindex]] = (buf[j] & (0x80 >> i))>0;
				//		lampindex++;
				//	}
				//}
				bcm2835_gpio_write(CE_PIN, LOW);
				
				   for (c = 0; c < 8; c++) {
				   /*
						printf("%d" , lamps[c]);
						if(lamps[c]){
							printf(" %d - %d \n", c , lamps[c]);
						}
					*/
						//usleep(interval);
						bcm2835_gpio_write(CL_PIN, LOW);
						bcm2835_gpio_write(DI_PIN,change ? HIGH : LOW );
						//bcm2835_gpio_write(DI_PIN, LOW);
						bcm2835_gpio_write(CL_PIN, HIGH);
						//usleep(interval);
					}
					change = !change;
					printf(" %d\n", change?1:0);
				/*printf("\n"); */
				//usleep(50 * interval);
				bcm2835_gpio_write(CE_PIN, HIGH);
			//}
		}

    close(s);
    exit(EXIT_SUCCESS);
}
