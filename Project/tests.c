#include <iostream>

enum MACHINE {TRANSMITTER = 0, RECEIVER = 1};                                           //Machine constants
enum HEADER_TYPE {INVALID = -1, INFO, SET, DISC, UA, RR, REJ};
enum BYTE {FLAG = 0x7e, ESCAPE = 0x7d, ESC_FLAG = 0x5e, ESC_ESCAPE = 0x5d,              //Flag and byte stuffing
        A_TRANSMITTER_COMMAND = 0x03, A_RECEIVER_COMMAND = 0x01,                        //Address
        CNTRL_INFO_0 = 0x00, CNTRL_INFO_1 = 0x40, CNTRL_SET = 0x03, CNTRL_DISC = 0x0b,  //Control Commands
        CNTRL_UA = 0x07, CNTRL_RR_0 = 0x05, CNTRL_RR_1 = 0x85,
        CNTRL_REJ_0 = 0x01, CNTRL_REJ_1 = 0x81};                                        //Control responses

int machine;    //0 if transmitter or 1 if receiver

unsigned char getBCC(unsigned char *content, int size) {
    unsigned char result = content[0];
    for (size_t i = 1; i < size; i++){
        result = result ^ content[i];
    }
    return result;
}

int getHeaderType(unsigned char *header, int *responseParity) {
    //1- Check BCC
    if (header[3] != getBCC(header, 3)) {
        return INVALID;
    }
    //2- Check address and control, fill parity
    *responseParity = -1; //Default value if not used
    if (machine == TRANSMITTER) {   //Transmitter receiving: receiver sending
        if (header[1] == A_TRANSMITTER_COMMAND) {  //Receiver Responses: RR, REJ and UA
            if (header[2] == CNTRL_UA) {
                return UA;
            }
            else if (header[2] == CNTRL_REJ_0) {
                *responseParity = 0;
                return REJ;
            }
            else if (header[2] == CNTRL_REJ_1) {
                *responseParity = 1;
                return REJ;
            }
            else if (header[2] == CNTRL_RR_0) {
                *responseParity = 0;
                return RR;
            }
            else if (header[2] == CNTRL_RR_1) {
                *responseParity = 1;
                return RR;
            }
        }
        else if (header[1] == A_RECEIVER_COMMAND) { //Receiver Command: DISC
            if (header[2] == CNTRL_DISC) {
                return DISC;
            }
        }
    }
    else if (machine == RECEIVER) { //Receiver receiving: transmitter sending
        if (header[1] == A_TRANSMITTER_COMMAND) {   //Transmitter Commands: SET, I and DISC
            if (header[2] == CNTRL_INFO_0) {
                *responseParity = 0;
                return INFO;
            }
            else if (header[2] == CNTRL_INFO_1) {
                *responseParity = 1;
                return INFO;
            }
            else if (header[2] == CNTRL_SET) {
                return SET;
            }
            else if (header[2] == CNTRL_DISC) {
                return DISC;
            }
        }
        else if (header[1] == A_RECEIVER_COMMAND) { //Transmitter response: UA
            if (header[2] == CNTRL_UA) {
                return UA;
            }
        }
    }
    return INVALID;
}

int createHeader(unsigned char *header, int type, int messageParity) {
    header[0] = FLAG;
    if (machine == TRANSMITTER) {
        if (type == UA) {
            header[1] = A_RECEIVER_COMMAND;
            header[2] = CNTRL_UA;           //Transmitter Response, only UA
        }
        else {
            header[1] = A_TRANSMITTER_COMMAND;  //Transmitter Command: SET, INFO and DISC
            if (type == SET) {
                header[2] = CNTRL_SET;
            }
            else if (type == INFO) {
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
            else if (type == DISC) {
                header[2] = CNTRL_DISC;
            }
            else {
                return -1;
            }
        }

    }
    else if (machine == RECEIVER) {
        if (type == DISC) {
            header[1] = A_RECEIVER_COMMAND;
            header[2] = CNTRL_DISC;           //Receiver Command, only DISC
        }
        else {
            header[1] = A_TRANSMITTER_COMMAND; //Receiver Response: UA, RR and REJ
            if (type == UA) {
                header[2] = CNTRL_UA;
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
        }
    }
    header[3] = getBCC(header, 3);
    return 0;
}

int addStuffing(unsigned char *content, int size) {  //Content should be WAY BIGGER than size
    //content includes final flag
    int newSize = 1;
    for (size_t i = 0; i < size - 1; i++) { //For loop excludes last byte
        if ( (content[i] == FLAG || content[i] == ESCAPE)) {
            newSize++;
        }
        newSize++;
    }                       //Checks size
    unsigned char result[newSize];
    size_t t = 0;
    for (size_t i = 0; i < size - 1; i++){  //For loop excludes last byte
        if (content[i] == FLAG) {
            result[t] = ESCAPE;
            result[t+1] = ESC_FLAG;
            t += 2;
        }
        else if (content[i] == ESCAPE) {
            result[t] = ESCAPE;
            result[t+1] = ESC_ESCAPE;
            t += 2;
        }
        else {
            result[t] = content[i];
            t++;
        }
    }
    result[newSize-1] = FLAG;

    for (size_t i = 0; i < newSize; i++) {
        content[i] = result[i];     //Copy memory
    }

    return newSize;
}

int removeStuffing(unsigned char *content, int size) {
    int newSize = 1;
    for (size_t i = 0; i < size - 1; i++){
        if ( (content[i] == ESCAPE && content[i+1] == ESC_FLAG) || (content[i] == ESCAPE && content[i+1] == ESC_ESCAPE) ) {
            newSize--;
        }
        newSize++;
    }                       //Checks size
    unsigned char result[newSize];
    size_t t = 0;
    for (size_t i = 0; i < size; i++){
        if (content[i] == ESCAPE && content[i+1] == ESC_FLAG) {
            result[t] = FLAG;
            i++;
        }
        else if (content[i] == ESCAPE && content[i+1] == ESC_ESCAPE) {
            result[t] = ESCAPE;
            i++;
        }
        else {
            result[t] = content[i];
        }
        t++;
    }

    for (size_t i = 0; i < newSize; i++){
        content[i] = result[i];
    }

    return newSize;
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

int main() {
    printf("-----getHeaderType Tests-----\n");
    machine = RECEIVER;
    int parity = 9;
    unsigned char header1[4] = {FLAG, A_TRANSMITTER_COMMAND, CNTRL_SET, 0};
    header1[3] = getBCC(header1, 3);
    printf("%i-> ",getHeaderType(header1, &parity));
    printf("%i\n\n", parity);         //Correct transmitter command SET, parity = -1

    header1[3] = 0xFF;
    printf("%i-> ",getHeaderType(header1, &parity));
    printf("%i\n\n", parity);         //Incorrect transmitter command SET, parity = -1

    header1[2] = CNTRL_INFO_0;
    header1[3] = getBCC(header1, 3);
    printf("%i-> ",getHeaderType(header1, &parity));
    printf("%i\n\n", parity);         //Correct transmitter command INFO, parity = 0

    header1[2] = A_RECEIVER_COMMAND;
    header1[3] = getBCC(header1, 3);
    printf("%i-> ",getHeaderType(header1, &parity));
    printf("%i\n\n", parity);         //Incorrect transmitter command INFO, parity = ?

    //Passed

    printf("-----createHeader and BCC Tests-----\n");
    machine = RECEIVER;
    parity = 0;

    printf("%i\n\n", createHeader(header1, SET, parity)); //Invalid SET sent by receiver

    machine = TRANSMITTER;
    printf("%i\n", createHeader(header1, SET, parity)); //Valid SET sent by transmitter
    printByteSequence(header1, 4); //0x7e-0x03-0x03-0x7e

    machine = TRANSMITTER;
    printf("%i\n", createHeader(header1, INFO, parity)); //Valid INFO sent by transmitter, parity 0
    printByteSequence(header1, 4); //0x7e-0x03-0x00-0x7d

    printf("%i\n", createHeader(header1, DISC, parity)); //Valid INFO sent by transmitter
    printByteSequence(header1, 4); //0x7e-0x03-0x0B-0x76

    machine = RECEIVER;
    printf("%i\n", createHeader(header1, DISC, parity)); //Valid INFO sent by receiver
    printByteSequence(header1, 4); //0x7e-0x01-0x0B-0x74

    //Passed

    printf("-----addStuffing Tests-----\n");

    unsigned char content[10] = {0x01, 0x02, FLAG, ESCAPE, 0x03, FLAG, 0,0,0,0};
    printByteSequence(content, 10);
    printf("%i : ", addStuffing(content, 6));
    printByteSequence(content, 10);
    //Passed

    printf("-----removeStuffing Tests-----\n");

    printByteSequence(content, 10);
    printf("%i : ", removeStuffing(content, 8));
    printByteSequence(content, 10);
    //Passed

    return 0;
}
