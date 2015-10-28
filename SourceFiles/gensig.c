/** CUSTOM GENERATE SIGNAL
 * $Id: generate.c 1246 2014-06-02 09:07am pdorazio $
 *
 * @brief Red Pitaya simple signal/function generator with pre-defined
 *        signal types.
 *
 * @Author Ales Bardorfer <ales.bardorfer@redpitaya.com>
 *
 * (c) Red Pitaya  http://www.redpitaya.com
 *
 * This part of code is written in C programming language.
 * Please visit http://en.wikipedia.org/wiki/C_(programming_language)
 * for more details on the language used herein.
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "fpga_awg.h"
#include "version.h"

#define START_BUFF 0x110011
#define STOP_BUFF 0x41

/**
 * GENERAL DESCRIPTION:
 *
 * The code below performs a function of a signal generator, which produces
 * a a signal of user-selectable pred-defined Signal shape
 * [Sine, Square, Triangle], Amplitude and Frequency on a selected Channel:
 *
 *
 *                   /-----\
 *   Signal shape -->|     | -->[data]--+-->[FPGA buf 1]--><DAC 1>
 *   Amplitude ----->| AWG |            |
 *   Frequency ----->|     |             -->[FPGA buf 2]--><DAC 2>
 *                   \-----/            ^
 *                                      |
 *   Channel ---------------------------+ 
 *
 *
 * This is achieved by first parsing the four parameters defining the 
 * signal properties from the command line, followed by synthesizing the 
 * signal in data[] buffer @ 125 MHz sample rate within the
 * generate_signal() function, depending on the Signal shape, Amplitude
 * and Frequency parameters. The data[] buffer is then transferred
 * to the specific FPGA buffer, defined by the Channel parameter -
 * within the write_signal_fpga() function.
 * The FPGA logic repeatably sends the data from both FPGA buffers to the
 * corresponding DACs @ 125 MHz, which in turn produces the synthesized
 * signal on Red Pitaya SMA output connectors labeled DAC1 & DAC2.
 *
 */

/** Maximal signal frequency [Hz] */
const double c_max_frequency = 62.5e6;

/** Minimal signal frequency [Hz] */
const double c_min_frequency = 0;

/** Maximal signal amplitude [Vpp] */
const double c_max_amplitude = 2.0;

/** AWG buffer length [samples]*/
#define n (16*1024)

/** AWG data buffer */
int32_t data[n];

/** Program name */
const char *g_argv0 = NULL;

/** Predefined File name, used for definition of arbitrary Signal Shape. */
const char *gen_waveform_file1="./gen_ch1.csv";
const char *gen_waveform_file2="./gen_ch2.csv";

/** FPGA AWG output signal buffer length */
#define AWG_SIG_LEN   (16*1024)

/*Declarations for a function which reads is a file of arbitrary length goes here*/
#define MAX_NUM_OF_BUFF 30
#define BUFFSIZE (16*1024)
int read_in_file2(int chann , int* num_ofbuff,  float* sigbuff[], int32_t* dataout[]);
void printdata(int in_smpl_len1, int num_ofbuff);
void freebuffspace(int in_smpl_len1, int num_ofbuff);
float* sigbuff[MAX_NUM_OF_BUFF]; /*sigbuff is an array of pointers (pointers to floats)*/
								 /*sigbuff's value is the address of the first element of the array of pointers (pointers to floats)*/
int32_t* dataout[MAX_NUM_OF_BUFF]; 




/** AWG FPGA parameters */
typedef struct {
    int32_t  offsgain;   ///< AWG offset & gain.
    uint32_t wrap;       ///< AWG buffer wrap value.
    uint32_t step;       ///< AWG step interval.
} awg_param_t;

/* Forward declarations */

void synthesize_signal2(int in_data_len, double ampl, double freq, awg_param_t *awg, int num_ofbuff);
void write_data_fpga2(uint32_t ch,
                     const awg_param_t *awg,
					 int num_ofbuff,
					 double freq,
					 int z);

					 


/** Print usage information */
void usage() {

    const char *format =
        "\n\n"
        "Usage: Input arguments:  SampleRate Playtimes\n"
        "\n"
        "\tSampleRate  Sample rate of custom signal [Hz].\n"
        "\tPlayTimes   Number of times to play the signal.\n"
        "\n";

    fprintf( stderr, format,g_argv0);
}


/** Signal generator main */
int main(int argc, char *argv[])
{	
	
	//Variables needed to read file:
	int in_smpl_len1 = 0;
	double ampl = 1; //Hard code ampl!
	double freq = 0;
	double Fs = 0;
	int num_ofbuff = 0;
	int num_ofTimes = 0;
	awg_param_t params;	
	
	if ( argc < 2 ) {
        usage();
		return -1;
	}
	
    g_argv0 = argv[0]; 
	
    /* Signal frequency argument parsing */
    Fs = strtod(argv[1], NULL);
	freq = Fs / ((double)BUFFSIZE);
	num_ofTimes = strtod(argv[2], NULL);
	

	
    in_smpl_len1 = read_in_file2(1, &num_ofbuff, sigbuff, dataout); 
	//printdata(in_smpl_len1, num_ofbuff);
	synthesize_signal2(in_smpl_len1, ampl, freq, &params, num_ofbuff);
	//printdata(in_smpl_len1, num_ofbuff);
	write_data_fpga2(0, &params, num_ofbuff, freq, num_ofTimes);
	freebuffspace(in_smpl_len1, num_ofbuff);
    
}

/**
 * Synthesize a desired signal.
 *
 * Generates/synthesized  a signal, based on three pre-defined signal
 * types/shapes, signal amplitude & frequency. The data[] vector of 
 * samples at 125 MHz is generated to be re-played by the FPGA AWG module.
 *
 * @param ampl  Signal amplitude [Vpp].
 * @param freq  Signal frequency [Hz].
 * @param type  Signal type/shape [Sine, Square, Triangle].
 * @param data  Returned synthesized AWG data vector.
 * @param awg   Returned AWG parameters.
 *
 */


/**
 * Write synthesized data[] to FPGA buffer.
 *
 * @param ch    Channel number [0, 1].
 * @param data  AWG data to write to FPGA.
 * @param awg   AWG paramters to write to FPGA.
 */



/*----------------------------------------------------------------------------------*/
/**
 * @brief Read Time Based Signal Definition from the file system
 *
 * The gen_waveform_file is a simple text file, constituted from lines of triple
 * data: time_base, channel A and channel B values. At most AWG_SIG_LEN lines are
 * parsed and apparent data are put to the specified buffers. It is expected the
 * specified buffers are large enough, no check is made within a function. After the
 * Signal Definition is read from a file, the final Signal Shape is calculated with
 * calculate_data() function.
 *
 * @param[out]  time_vect Time base vector
 * @param[out]  ch1_data  Channel A buffer
 * @param[out]  ch2_data  Channel B buffer
 * @retval      -1        Failure, error message is output on standard error
 * @retval      >0        Number of parsed lines
 */


int read_in_file2(int chann , int* num_ofbuff,  float* sigbuff[], int32_t* dataout[]){
	
	FILE *fi = NULL;
    int i, k, read_size; 
	float tmpstorage;
	float* tmpptr = NULL;
	int file_size =0; /*denotes how many samples are in a file*/
	chann = 1; /*Hardcoded for now*/
	
    /* open file */
    if (chann == 1) {

        fi = fopen(gen_waveform_file1, "r+");
        if (fi == NULL) {
            fprintf(stderr, "read_in_file(): Can not open input file (%s): %s\n",
                    gen_waveform_file1, strerror(errno));
            return -1;
        }

    } else {

        fi = fopen(gen_waveform_file2, "r+");
        if (fi == NULL) {
            fprintf(stderr, "read_in_file(): Can not open input file (%s): %s\n",
                    gen_waveform_file2, strerror(errno));
            return -1;
        }
    }
	
	/*Determine the file size*/
	while( fscanf(fi, "%f \n", &tmpstorage) != EOF ){
		file_size += 1;
	}
	
	/* close a file */
    fclose(fi);
	
	/* Re-open file */
    if (chann == 1) {

        fi = fopen(gen_waveform_file1, "r+");
        if (fi == NULL) {
            fprintf(stderr, "read_in_file(): Can not open input file (%s): %s\n",
                    gen_waveform_file1, strerror(errno));
            return -1;
        }

    } else {

        fi = fopen(gen_waveform_file2, "r+");
        if (fi == NULL) {
            fprintf(stderr, "read_in_file(): Can not open input file (%s): %s\n",
                    gen_waveform_file2, strerror(errno));
            return -1;
        }
    }
	
	
	/*Determine how many 16k large buffers are needed*/
	*num_ofbuff = file_size/BUFFSIZE;
	
	/*Create the needed buffers*/
	for(i =0 ; i < *num_ofbuff; i++){
		*sigbuff = calloc( BUFFSIZE, sizeof (float) );
		sigbuff++;
		*dataout = calloc( BUFFSIZE, sizeof (int32_t) );
		dataout++;
	}
	
	/*"Rewind" the buffers*/
	for(i =0 ; i < *num_ofbuff; i++){
		sigbuff--;
		dataout--;
	}
	
	/*Copy the contents of the file into the buffers*/
	for( i = 0; i < *num_ofbuff; i++){
		tmpptr = *sigbuff; /*start tmpptr at first element of array of buffers*/

		for (k = 0; k < BUFFSIZE; k++) {
			read_size = fscanf(fi, "%f \n", tmpptr);
			tmpptr++; /*move to next element of buffer*/
			if((read_size == EOF) || (read_size != 1)) {
            	k--;
            	break;
        	}
		}
		sigbuff++; /*move to first element of next buffer*/
	}
	
	
	/* close a file,and exit */
    fclose(fi);
	return file_size;
	
	
	
}/*Endof read_in_file2*/

void printdata(int in_smpl_len1, int num_ofbuff){
	int i =0;
	int k =0;
	float* tmpptr = NULL; /*tmpptr is used to move within each buffer, from element to element*/
	float** ptr2array = sigbuff; /*ptr2array is used to from buffer to buffer*/
	
	int32_t* output_buff = NULL; 
	int32_t** output_buffptr = dataout;
	
	printf("THIS IS A PRINTF OF THE sigbuff ARRAY\n\r");
	for( i = 0; i < num_ofbuff; i++){
			tmpptr = *ptr2array; /*start tmpptr at first element of array of buffers*/
	
			for (k = 0; k < BUFFSIZE; k++) {
				printf("%f ", *tmpptr);
				tmpptr++; /*move to next element of buffer*/
			
			}
			ptr2array++; /*move to first element of next buffer*/
			printf("\n\r");
	}
	
	
	printf("THIS IS A PRINTF OF THE dataout ARRAY\n\r");
	for( i = 0; i < num_ofbuff; i++){
			output_buff = *output_buffptr; /*start tmpptr at first element of array of buffers*/
	
			for (k = 0; k < BUFFSIZE; k++) {
				printf("%d ", *output_buff);
				output_buff++; /*move to next element of buffer*/
			
			}
			output_buffptr++; /*move to first element of next buffer*/
			printf("\n\r");
	}
	
}

void freebuffspace(int in_smpl_len1, int num_ofbuff){
	int i =0;
	int k =0;
	float* next = NULL;
	float* tmpptr = NULL;
	float** ptr2array = sigbuff; /*Initialize ptr*/
	
	int32_t* nextout = NULL;
	int32_t* output_buff = NULL; 
	int32_t** output_buffptr = dataout;
	

	for( i = 0; i < num_ofbuff; i++){
			tmpptr = *ptr2array; /*start tmpptr at first element of array of buffers*/
			output_buff = *output_buffptr;
			
	
			for (k = 0; k < BUFFSIZE; k++) {
				next = tmpptr + 1; /*store the address of next memory location*/
				free(tmpptr);
				tmpptr = next; /*move to next element of buffer*/
				
				nextout = output_buff + 1;
				free(output_buff);
				output_buff = nextout;
			}
			
			ptr2array++; /*move to first element of next buffer*/
			output_buffptr++;
			printf("Buffer Space Cleared!!\n\r");
	}
	
}


void synthesize_signal2(int in_data_len, double ampl, double freq, awg_param_t *awg, int num_ofbuff){
	

    /* Various locally used constants - HW specific parameters */
    const int dcoffs = -155;
    const int trans0 = 30;
    const int trans1 = 300;
    //const double tt2 = 0.249;
	
	int j =0;
	int k =0;
	float* tmpptr = NULL;
	float** ptr2array = sigbuff; /*Initialize ptr*/	
	
	int32_t* output_buff = NULL; 
	int32_t** output_buffptr = dataout;
	
	in_data_len = in_data_len/num_ofbuff;
	/* This is where frequency is used... */
    awg->offsgain = (dcoffs << 16) + 0x1fff;
    awg->step = round(65536 * freq/c_awg_smpl_freq * in_data_len);
    awg->wrap = round(65536 * (in_data_len-1));

    int trans = freq / 1e6 * trans1; /* 300 samples at 1 MHz */
    uint32_t amp = ampl * 4000.0;    /* 1 Vpp ==> 4000 DAC counts */
    if (amp > 8191) {
        /* Truncate to max value if needed */
        amp = 8191;
    }

    if (trans <= 10) {
        trans = trans0;
    }


	
	
	
	for( j = 0; j < num_ofbuff; j++){
		
			tmpptr = *ptr2array; /*start tmpptr at first element of array of buffers*/
			output_buff = *output_buffptr;
			
			for (k = 0; k < BUFFSIZE; k++) {
				*output_buff = amp*(*tmpptr); /*action on each sample happens here!*/
				tmpptr++; /*move to next element of one of the input buffers*/
				output_buff++; /*move to next element of one of the output buffers*/
			
			}
			ptr2array++; /*move to first element of next input buffer*/
			output_buffptr++; /*move to first element of next output buffer*/ 
			
			
	}
	
	printf("synthesize_signal() function is complete \n\r");
	
	
}

void write_data_fpga2(uint32_t ch,
                     const awg_param_t *awg,
					 int num_ofbuff,
					 double freq,
					 int z) {



    
    uint32_t state_machine;
    int mode_mask = 0;
	int j =0;
	int h =0;
	int sleeptime = 1000000;
	int32_t* output_buff = NULL; 
	int32_t** output_buffptr = dataout;

    fpga_awg_init();
    state_machine = g_awg_reg->state_machine_conf;
    mode_mask = 0x20;
    state_machine &= ~0xff;
	sleeptime = sleeptime / freq;

    if(ch == 0) {
        /* Channel A */
        g_awg_reg->state_machine_conf = state_machine | 0xC0;
		state_machine = g_awg_reg->state_machine_conf;
        g_awg_reg->cha_scale_off      = awg->offsgain;
        g_awg_reg->cha_count_wrap     = awg->wrap;
        g_awg_reg->cha_count_step     = awg->step;
        g_awg_reg->cha_start_off      = 0;
	}

       
    /*Fill the FPGA buffer starts here-------------------------------------*/
	
	while(z/*Repeat signal z times*/){
	
	for( j = 0; j < num_ofbuff; j++){
		
			
			output_buff = *output_buffptr;
			
			for (h = 0; h < BUFFSIZE; h++) {
				/*action on each sample happens here!*/
				g_awg_cha_mem[h] = *output_buff ;
				output_buff++; /*move to next element of one of the output buffers*/
			}
			output_buffptr++; /*move to first element of next output buffer*/ 
			
			//Now I can play that freshly-filled buffer  
			g_awg_reg->cha_start_off      = 0; /*Start playing at index[0]*/
			g_awg_reg->state_machine_conf = START_BUFF;
			usleep(sleeptime);    
			g_awg_reg->state_machine_conf = STOP_BUFF;
			//And repeat untill all buffers are played
	}
	output_buffptr = dataout; //reset pointer to the first buffer
	z--; //and repeat the signal!
	
	
	}/*END OF WHILE!*/
	
	
	
	
	/*Fill the FPGA buffer ENDS here-------------------------------------*/


    g_awg_reg->state_machine_conf = 0x11010100 | mode_mask;
	


    fpga_awg_exit();
    
}
