/*
 *  Small program to read a 16-bit, signed, 44.1kHz wave file and play it.
 *  Written by Brian Fraser, heavily based on code found at:
 *  http://www.alsa-project.org/alsa-doc/alsa-lib/_2test_2pcm_min_8c-example.html
 */

#include <alsa/asoundlib.h>
#include <pthread.h>
#include <unistd.h>

// g++ wave_player.c -fpermissive -lasound -lpthread


// File used for play-back:
// If cross-compiling, must have this file available, via this relative path,
// on the target when the application is run. This example's Makefile copies the wave-files/
// folder along with the executable to ensure both are present.
#define SOURCE_FILE "wave-files/100060__menegass__gui-drum-splash-hard.wav"
//#define SOURCE_FILE "wave-files/100053__menegass__gui-drum-cc.wav"

#define SAMPLE_RATE   8000
#define NUM_CHANNELS  1
#define SAMPLE_SIZE   (sizeof(short)) 	// bytes per sample
#define RESAMPLE_RATE 10

// Store data of a single wave file read into memory.
// Space is dynamically allocated; must be freed correctly!
typedef struct {
	int numSamples;
	short *pData;
} wavedata_t;

typedef struct {
	snd_pcm_t *handle;
	short *pData;
	int bufNum;
} AudioPiece;

// Prototypes:
snd_pcm_t *Audio_openDevice();
void Audio_readWaveFileIntoMemory(char *fileName, wavedata_t *pWaveStruct);
void Audio_playFile(snd_pcm_t *handle, wavedata_t *pWaveData);
void Audio_playFile_Cut(snd_pcm_t *handle, wavedata_t *pWaveData);
void Audio_playMultiFile_Cut(snd_pcm_t *handle, wavedata_t *pWaveData1, wavedata_t *pWaveData2);
void Audio_playFile_Piece(AudioPiece* aPiece);
void Audio_playMultiFile(snd_pcm_t *handle, wavedata_t *pWaveData1,  wavedata_t *pWaveData2);


int main(void)
{
	printf("Beginning play-back of %s\n", SOURCE_FILE);
	
	
	//pthread_t t; // 宣告 pthread 變數
	//pthread_create(&t, NULL, child, "Child"); // 建立子執行緒
	
	
	// Configure Output Device
	snd_pcm_t *handle = Audio_openDevice();

	char file1[] = "Audio/German_Concert_D_021_083.wav";
	char file2[] = "Audio/German_Concert_D_055_083.wav";
	char file3[] = "Audio/German_Concert_D_033_083.wav";
	char file4[] = "Audio/German_Concert_D_048_083.wav";
	char file5[] = "Audio/German_Concert_D_071_083.wav";
	char file6[] = "Audio/German_Concert_D_078_083.wav";
	char file7[] = "Audio/German_Concert_D_085_083.wav";
	char file8[] = "Audio/German_Concert_D_100_083.wav";
	
	
	// Load wave file we want to play:
	wavedata_t sampleFile1;
	wavedata_t sampleFile2;
	
	Audio_readWaveFileIntoMemory(file1, &sampleFile1);
	Audio_readWaveFileIntoMemory(file2, &sampleFile2);
	
	/***
	wavedata_t sampleFile3;
	sampleFile3.numSamples = sampleFile1.numSamples < sampleFile2.numSamples ? sampleFile1.numSamples : sampleFile2.numSamples;
	sampleFile3.pData = malloc(sampleFile3.numSamples * SAMPLE_SIZE);
	
	for(int i = 0; i < sampleFile3.numSamples; i++){
		
		if( *(sampleFile1.pData+i) + *(sampleFile2.pData+i) < -32768 ){
			*(sampleFile3.pData+i) = -32768;
		}
		else if( *(sampleFile1.pData+i) + *(sampleFile2.pData+i) > 32767 ){
			*(sampleFile3.pData+i) = 32767;
		}
		else{
			*(sampleFile3.pData+i) = *(sampleFile1.pData+i) + *(sampleFile2.pData+i);
		}
		
	}
	printf("Added\n");
	***/
	
	//printf("Play1\n");
	Audio_playFile(handle, &sampleFile1);
	//printf("Play2\n");
	//Audio_playFile(handle, &sampleFile2);
	//printf("Play3\n");
	//Audio_playFile(handle, &sampleFile3);
	//Audio_playFile_Cut(handle, &sampleFile1);
	//Audio_playMultiFile(handle, &sampleFile1, &sampleFile2);
	//Audio_playMultiFile_Cut(handle, &sampleFile1, &sampleFile2);
	

	// Cleanup, letting the music in buffer play out (drain), then close and free.
	snd_pcm_drain(handle);
	printf("Drained\n");
	snd_pcm_hw_free(handle);
	printf("Free\n");
	snd_pcm_close(handle);
	printf("Close\n");
	free(sampleFile1.pData);
	//free(sampleFile2.pData);

	printf("Done!\n");
	
	handle = Audio_openDevice();
	Audio_playFile(handle, &sampleFile1);
	snd_pcm_drain(handle);
	snd_pcm_hw_free(handle);
	snd_pcm_close(handle);
	free(sampleFile1.pData);
	
	
	
	
	return 0;
}



// Open the PCM audio output device and configure it.
// Returns a handle to the PCM device; needed for other actions.
snd_pcm_t *Audio_openDevice()
{
	snd_pcm_t *handle;

	// Open the PCM output
	int err = snd_pcm_open(&handle, "default", SND_PCM_STREAM_PLAYBACK, 0);
	if (err < 0) {
		printf("Play-back open error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}
	
	// 這邊要改
	// Configure parameters of PCM output
	err = snd_pcm_set_params(handle,
			SND_PCM_FORMAT_S16_LE,
			SND_PCM_ACCESS_RW_INTERLEAVED,
			NUM_CHANNELS,
			SAMPLE_RATE,
			1,			// Allow software resampling
			50000);		// 0.05 seconds per buffer
	if (err < 0) {
		printf("Play-back configuration error: %s\n", snd_strerror(err));
		exit(EXIT_FAILURE);
	}

	return handle;
}

// Read in the file to dynamically allocated memory.
// !! Client code must free memory in wavedata_t !!
void Audio_readWaveFileIntoMemory(char *fileName, wavedata_t *pWaveStruct)
{
	assert(pWaveStruct);

	// Wave file has 44 bytes of header data. This code assumes file
	// is correct format.
	const int DATA_OFFSET_INTO_WAVE = 44;

	// Open file
	FILE *file = fopen(fileName, "r");
	if (file == NULL) {
		fprintf(stderr, "ERROR: Unable to open file %s.\n", fileName);
		exit(EXIT_FAILURE);
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	int sizeInBytes = ftell(file) - DATA_OFFSET_INTO_WAVE;
	fseek(file, DATA_OFFSET_INTO_WAVE, SEEK_SET);
	pWaveStruct->numSamples = sizeInBytes / SAMPLE_SIZE;
	
	printf("The total size in byte is %d\n", sizeInBytes);

	// Allocate Space
	pWaveStruct->pData = malloc(sizeInBytes);
	if (pWaveStruct->pData == NULL) {
		fprintf(stderr, "ERROR: Unable to allocate %d bytes for file %s.\n",
				sizeInBytes, fileName);
		exit(EXIT_FAILURE);
	}

	// Read data:
	int samplesRead = fread(pWaveStruct->pData, SAMPLE_SIZE, pWaveStruct->numSamples, file);
	if (samplesRead != pWaveStruct->numSamples) {
		fprintf(stderr, "ERROR: Unable to read %d samples from file %s (read %d).\n",
				pWaveStruct->numSamples, fileName, samplesRead);
		exit(EXIT_FAILURE);
	}
	
	/***
	unsigned char* stuff8;
	for(int i = 0; i < pWaveStruct->numSamples; i++){
		stuff8 = (unsigned char*)&(pWaveStruct->pData[i]);
		printf("%x%x ", stuff8[0], stuff8[1]);
		if(i % 16 == 0)
			printf("\n");
	}
	***/

	fclose(file);
}

// 一次播放好幾個音檔
void Audio_playMultiFile(snd_pcm_t *handle, wavedata_t *pWaveData1,  wavedata_t *pWaveData2)
{
	// If anything is waiting to be written to screen, can be delayed unless flushed.
	fflush(stdout);
	
	snd_pcm_sframes_t frames;
	
	// Write data and play sound (blocking)
	frames = snd_pcm_writei(handle, pWaveData1->pData, pWaveData1->numSamples);
	frames = snd_pcm_writei(handle, pWaveData2->pData, pWaveData2->numSamples);
	

	// Check for errors
	if (frames < 0)
		frames = snd_pcm_recover(handle, frames, 0);
	if (frames < 0) {
		fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n", frames);
		exit(EXIT_FAILURE);
	}
	if (frames > 0 && frames < pWaveData1->numSamples)
		printf("Short write (expected %d, wrote %li)\n", pWaveData1->numSamples, frames);
}


// Play the audio file (blocking)
void Audio_playFile(snd_pcm_t *handle, wavedata_t *pWaveData)
{
	// If anything is waiting to be written to screen, can be delayed unless flushed.
	fflush(stdout);
	printf("play!!!\n");
	// Write data and play sound (blocking)
	snd_pcm_sframes_t frames = snd_pcm_writei(handle, pWaveData->pData, pWaveData->numSamples);
	frames = snd_pcm_writei(handle, pWaveData->pData, pWaveData->numSamples);

	// Check for errors
	if (frames < 0)
		frames = snd_pcm_recover(handle, frames, 0);
	if (frames < 0) {
		fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n", frames);
		exit(EXIT_FAILURE);
	}
	if (frames > 0 && frames < pWaveData->numSamples)
		printf("Short write (expected %d, wrote %li)\n", pWaveData->numSamples, frames);
}


// Play the audio file (blocking)
void Audio_playFile_Cut(snd_pcm_t *handle, wavedata_t *pWaveData)
{
	// If anything is waiting to be written to screen, can be delayed unless flushed.
	fflush(stdout);
	
	pthread_t t[pWaveData->numSamples / 32 / (SAMPLE_RATE / 100)]; // 宣告 pthread 變數
	
	AudioPiece aPiece[2];
	
	pthread_t t1, t2;
	
	//aPiece.handle = handle;
	//aPiece.pData = pWaveData->pData;
	//aPiece.bufNum = 32 * (SAMPLE_RATE / 100);
	//printf("1\n");
	//pthread_create(&t1, NULL, Audio_playFile_Piece, &aPiece ); // 建立子執行緒
	//
	//usleep(10000);
	//printf("2\n");
	//aPiece.pData = pWaveData->pData + 1 * 32 * (SAMPLE_RATE / 100);
	//pthread_create(&t2, NULL, Audio_playFile_Piece, &aPiece ); // 建立子執行緒
	
	//return;
	
	printf("start!!\n");
	
	snd_pcm_sframes_t frames;
	for(int i = 0; i < pWaveData->numSamples / 32 / (SAMPLE_RATE / 100); i++){
		
		aPiece[i%2].handle = handle;
		aPiece[i%2].pData = pWaveData->pData + i * 32 * (SAMPLE_RATE / 100);
		aPiece[i%2].bufNum = 32 * (SAMPLE_RATE / 100);
		
		//frames = snd_pcm_writei(aPiece.handle, aPiece.pData, aPiece.bufNum);
		pthread_create(t + i, NULL, Audio_playFile_Piece, &(aPiece[i%2]) ); // 建立子執行緒
		//printf("%d", i);
		//break;
		usleep(100000);
	
	}
	
}

// Play the audio file (blocking)
void Audio_playMultiFile_Cut(snd_pcm_t *handle, wavedata_t *pWaveData1, wavedata_t *pWaveData2)
{
	// If anything is waiting to be written to screen, can be delayed unless flushed.
	fflush(stdout);
	
	
	
	AudioPiece aPiece;
	
	wavedata_t sampleFile3;
	sampleFile3.numSamples = pWaveData1->numSamples < pWaveData2->numSamples ? pWaveData1->numSamples : pWaveData2->numSamples;
	sampleFile3.pData = malloc(sampleFile3.numSamples * SAMPLE_SIZE);
	
	//pthread_t t[pWaveData->numSamples / 32 / (SAMPLE_RATE / 100)]; // 宣告 pthread 變數
	pthread_t t[sampleFile3.numSamples / NUM_CHANNELS / (SAMPLE_RATE / RESAMPLE_RATE)];
	
	bool thread_alive = false;
	snd_pcm_sframes_t frames;
	for(int i = 0; i < sampleFile3.numSamples / NUM_CHANNELS / (SAMPLE_RATE / RESAMPLE_RATE); i++){
		
		for(int j = 0; j < NUM_CHANNELS * (SAMPLE_RATE / RESAMPLE_RATE); j++){
			
			int k = i * NUM_CHANNELS * (SAMPLE_RATE / RESAMPLE_RATE) + j;
			
			if( *(pWaveData1->pData+k) + *(pWaveData2->pData+k) < -32768 ){
				*(sampleFile3.pData+k) = -32768;
			}
			else if( *(pWaveData1->pData+k) + *(pWaveData2->pData+k) > 32767 ){
				*(sampleFile3.pData+k) = 32767;
			}
			else{
				*(sampleFile3.pData+k) = *(pWaveData1->pData+k) + *(pWaveData2->pData+k);
			}
			
		}
		
		aPiece.handle = handle;
		aPiece.pData = &(sampleFile3.pData[i * NUM_CHANNELS * (SAMPLE_RATE / RESAMPLE_RATE)]);
		aPiece.bufNum = NUM_CHANNELS * (SAMPLE_RATE / RESAMPLE_RATE);
		
		//printf("count done!!\n");
		
		//Audio_playFile_Piece(&aPiece);
		
		///***
		//if(thread_alive)
		//	pthread_join(t, NULL);
		
		if(pthread_create( &(t[i]), NULL, Audio_playFile_Piece, &aPiece ) < 0)
			printf("fail!!\n");
		thread_alive = true;
		usleep(1000000);
		//***/
		
		//frames = snd_pcm_writei(aPiece.handle, aPiece.pData, aPiece.bufNum);
		//pthread_create(&(t[i]), NULL, Audio_playFile_Piece, &aPiece ); // 建立子執行緒
		//printf("%d", i);
		////break;
		//usleep(10000);
	
	}
	
}

void Audio_playFile_Piece(AudioPiece* aPiece){
	
	// snd_pcm_t *handle, short *buf, int bufNum
	//printf("piece\n");
	// Write data and play sound (blocking)
	snd_pcm_sframes_t frames = snd_pcm_writei(aPiece->handle, aPiece->pData, aPiece->bufNum);
	
	//printf("write done!!\n");
	// Check for errors
	if (frames < 0)
		frames = snd_pcm_recover(aPiece->handle, frames, 0);
	if (frames < 0) {
		fprintf(stderr, "ERROR: Failed writing audio with snd_pcm_writei(): %li\n", frames);
		exit(EXIT_FAILURE);
	}
	if (frames > 0 && frames < aPiece->bufNum)
		printf("Short write (expected %d, wrote %li)\n", aPiece->bufNum, frames);
}
