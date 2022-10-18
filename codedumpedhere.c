#include <stdio.h>

//THE IDEA IS TO COPY THIS TO THE MAIN PROJECT LATER


enum MACHINE {TRANSMITTER = 0, RECEIVER = 1};                                           //Machine constants
enum HEADER_TYPE {INVALID = -1, INFO, SET, DISC, UA, RR, REJ};
enum BYTE {FLAG = 0x7e, ESCAPE = 0x7d, ESC_FLAG = 0x5e, ESC_ESCAPE = 0x5d,              //Flag and byte stuffing
        A_TRANSMITTER_COMMAND = 0x03, A_RECEIVER_COMMAND = 0x01,                        //Address
        CNTRL_INFO_0 = 0x00, CNTRL_INFO_1 = 0x40, CNTRL_SET = 0x03, CNTRL_DISC = 0x0b,  //Control Commands
        CNTRL_UA = 0x07, CNTRL_RR_0 = 0x05, CNTRL_RR_1 = 0x85,
        CNTRL_REJ_0 = 0x01, CNTRL_REJ_1 = 0x81};                                        //Control responses

int machine;    //0 if transmitter or 1 if receiver
int messageParity = 0;  //0 or 1, switches

unsigned char getBCC(unsigned char *content, int size) {
    unsigned char result = content[0];
    for (size_t i = 1; i < size; i++){
        result = result ^ content[i];
    }
    return result;
}

int createHeader(unsigned char *header, int type) { //Tested, header must have 4 bytes (or more) , to be used in building
    header[0] = FLAG;
    header[1] = A_TRANSMITTER_COMMAND; //Most are transmitter commands, will change when they aren't
    if (type == INFO) {
        if (messageParity == 0) {
            header[2] = CNTRL_INFO_0;
        }
        else if (messageParity == 1) {
            header[2] = CNTRL_INFO_1;
        }
        else {
            return -1;
        }
    }
    else if (type == SET) {
        header[2] = CNTRL_SET;
    }
    else if (type == DISC) {
        header[2] = CNTRL_DISC;
        if (machine == RECEIVER ) {
            header[1] = A_RECEIVER_COMMAND;
        }
    }
    else if (type == UA) {
        header[2] = CNTRL_UA;
        if (machine == TRANSMITTER ) {
            header[1] = A_RECEIVER_COMMAND;
        }
    }
    else if (type == RR) {
        if (messageParity == 0) {
            header[2] = CNTRL_RR_0;
        }
        else if (messageParity == 1) {
            header[2] = CNTRL_RR_1;
        }
        else {
            return -1;
        }
    }
    else if (type == REJ) {
        if (messageParity == 0) {
            header[2] = CNTRL_REJ_0;
        }
        else if (messageParity == 1) {
            header[2] = CNTRL_REJ_1;
        }
        else {
            return -1;
        }
    }
    else {
        return -1;
    }
    header[3] = getBCC(header, 3);
    return 0;
}

int getHeaderType(unsigned char *header, int machine) {
    //1- Check BCC
    if (header[3] != getBCC(header, 3)) {
        return INVALID;
    }
    //2- Check address and control
    if (machine == TRANSMITTER) {
        if (header[1] == A_TCOMMAND) {  //Only UA, RR and REJ
            if (header[2] == CNTRL_UA) {
                return UA;
            }
            else if (header[2] == CNTRL_REJ_0 || header[2] == CNTRL_REJ_1) {
                return REJ;
            }
            else if (header[2] == CNTRL_RR_0 || header[2] == CNTRL_RR_1) {
                return RR;
            }
        }
        else if (header[1] == A_RECEIVER_COMMAND) { //only DISC is possible
            if (header[2] == CNTRL_DISC) {
                return DISC;
            }
        }
    }
    else if (machine == RECEIVER) {
        if (header[1] == A_TRANSMITTER_COMMAND) {   //Information, SETs and DISCs
            if (header[2] == CNTRL_INFO_0 || header[2] == CNTRL_INFO_1) {
                return INFO;
            }
            else if (header[2] == CNTRL_SET) {
                return SET;
            }
            else if (header[2] == CNTRL_DISC) {
                return DISC;
            }
        }
        else if (header[1] == A_RECEIVER_COMMAND) { //Only UA is possible
            if (header[2] == CNTRL_UA) {
                return UA;
            }
        }
    }
    return INVALID;
}

//Debug Functions
void printByte(unsigned char byte) {
    printf("%hhx\n", byte);
}
void printByteSequence(unsigned char *sequence, int size) {
    for (size_t i = 0; i < size-1; i++) {
        printf("%hhx-", sequence[i]);
    }
    printf("%hhx\n", sequence[size-1]);
}

int main() { //DELETE ME
    unsigned char array1[3] = {0x00, 0x0F, 0xFF};
    return 0;
}
