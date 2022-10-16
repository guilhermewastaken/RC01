//Untested! Code for analyzing (and validating) headers

#define H_INVALID -1
#define H_INFO 0
#define H_SET 1
#define H_DISC 2
#define H_UA 3
#define H_RR 4
#define H_REJ 5

#define CNTRL_SET 0b00000011
#define CNTRL_DISC 0b00001011
#define CNTRL_UA 0b00000111
#define CNTRL_RR0 0b00000101
#define CNTRL_RR1 0b10000101
#define CNTRL_REJ0 0b00000001
#define CNTRL_REJ1 0b10000001

#define A_TCOMMAND 0b00000011
#define A_RCOMMAND 0b00000001

SET (set up) 0 0 0 0 0 0 1 1
DISC (disconnect) 0 0 0 0 1 0 1 1
UA (unnumbered acknowledgment) 0 0 0 0 0 1 1 1
RR (receiver ready / positive ACK) R 0 0 0 0 1 0 1
REJ (reject / negative ACK) R 0 0 0 0 0 0 1 

int machine = -1; //0- transmitter 1- receiver

int getHeaderType(unsigned char header[], int machine) {
	//1- Check BCC

	//2- Check address and control
	if (machine == 0) {
		if (header[1] == A_TCOMMAND) {
			//CODE
		}
		else if (header[1] == A_RCOMMAND) {
			//CODE
		}
		
	}
	else if (machine == 1) {
		if (header[1] == A_TCOMMAND) {
			//CODE
		}
		else if (header[1] == A_RCOMMAND) {
			//CODE
		}
		
	}
}