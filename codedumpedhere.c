#include <stdio.h>

enum MACHINE {TRANSMITTER = 0, RECEIVER = 1};                                           //Machine constants
enum HEADER_TYPE {INVALID = -1, INFO, SET, DISC, UA, RR, REJ};
enum BYTE {FLAG = 0x7e, ESCAPE = 0x7d, ESC_FLAG = 0x5e, ESC_ESCAPE = 0x5d,              //Flag and byte stuffing
        A_TRANSMITTER_COMMAND = 0x03, A_RECEIVER_COMMAND = 0x01,                        //Address
        CNTRL_INFO_0 = 0x00, CNTRL_INFO_1 = 0x40, CNTRL_SET = 0x03, CNTRL_DISC = 0x0b,  //Control Commands
        CNTRL_UA = 0x07, CNTRL_RR_0 = 0x05, CNTRL_RR_1 = 0x85,
        CNTRL_REJ_0 = 0x01, CNTRL_REJ_1 = 0x81};                                        //Control responses

int machine;    //0 if transmitter or 1 if receiver
int messageParity = 0;  //0 or 1, switches

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

int getHeaderType(unsigned char *header) {
    //1- Check BCC
    if (header[3] != getBCC(header, 3)) {
        return INVALID;
    }
    //2- Check address and control
    if (machine == TRANSMITTER) {
        if (header[1] == A_TRANSMITTER_COMMAND) {  //Only UA, RR and REJ
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

int addStuffing(unsigned char *content, int size) {  //Content should be WAY BIGGER than size
    //It should have 2* maximum message size for a worse case scenario (?)
    int newSize = 0;
    for (size_t i = 0; i < size; i++){
        if ( (content[i] == FLAG || content[i] == ESCAPE) && i != size - 1) {
            newSize++;
            printByte(content[i]);
        }
        newSize++;
    }                       //Checks size
    unsigned char result[newSize];
    size_t t = 0;
    for (size_t i = 0; i < size; i++){
        if (content[i] == FLAG && i != size - 1) {
            result[t] = ESCAPE;
            result[t+1] = ESC_FLAG;
            t += 2;
        }
        else if (content[i] == ESCAPE && i != size - 1) {
            result[t] = ESCAPE;
            result[t+1] = ESC_ESCAPE;
            t += 2;
        }
        else {
            result[t] = content[i];
            t++;
        }
    }
    printByteSequence(result, newSize);
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


    return newSize;
}

void alarmHandler(int signal)
{
    alarmEnabled = 0;
    alarmCount++;

}
int receivePacket(unsigned char *data, int *size) {
    //Internal State Machine
    enum STATE_MACHINE {WAIT_FOR_FLAG = 0, BUILDING_HEADER, WAIT_FOR_LAST_FLAG, FILLING_INFO, OVER};
    unsigned char byteReceived;
    int bytes;
    int counter = 0;
    int state = WAIT_FOR_FLAG;
    int header = INVALID;

    while(alarmEnabled == 1 && state != OVER){
        switch (state){
            case WAIT_FOR_FLAG:
                bytes = read(fd,byteReceived,1);
                if (bytes != 0 && byteReceived == FLAG) {
                    data[0] = FLAG;
                    state = BUILDING_HEADER;
                }
                break;
            case BUILDING_HEADER:
                bytes = read(fd,byteReceived,1);
                if (bytes != 0) {
                    if (byteReceived == FLAG) {
                        counter = 0;
                    }
                    else {
                        counter++;
                        data[counter] = byteReceived;
                    }
                }
                if (counter == 3) {
                    header = getHeaderType(data);
                    if (header == INVALID) {
                        state = WAIT_FOR_FLAG;
                        counter = 0;
                    }
                    else if (header == INFO) {
                        counter = 0;
                        state = FILLING_INFO;
                    }
                    else {
                        state = WAIT_FOR_LAST_FLAG;
                    }
                }
            case WAIT_FOR_LAST_FLAG:
                bytes = read(fd,byteReceived,1);
                if (bytes != 0 && byteReceived == FLAG) {
                    state = OVER;
                }
                else if (bytes != 0) {
                    state = WAIT_FOR_FLAG;
                    counter = 0;
                }
            case FILLING_INFO:
                bytes = read(fd,byteReceived,1);
                if (bytes != 0 && byteReceived == FLAG) {
                    *size = counter;
                    state = OVER;
                }
                else if (bytes != 0) {
                    data[counter] = byteReceived;
                    counter++;
                }
        }
    }
    //Check BCC and send REJ if wrong
    if (data[counter - 2] != getBCC(data, counter -3)) {
        //Should also send REJ
        return INVALID;
    }
    return header;
}

int sendPacket(int type, unsigned char * data, int dataSize) {
    unsigned char msg[20];
    unsigned char newMsg[20];
    int acknowledge = 0;
    int alarmCount = 0;
    int alarmEnabled = 1;

    (void)signal(SIGALRM, alarmHandler);

    while (!acknowledge) {
        if (createHeader(msg, INFO) != 0) {
            return -1;
        }

        if (write(fd, msg, 4) != 4) {
            return -1;
        }

        if (type == INFO) {
            for (int i = 0; i < dataSize; i++) {
                msg[i] = data[i];
            }
            msg[dataSize] = getBCC(data, dataSize);
            msg[dataSize + 1] = FLAG;
            newSize = addStuffing(msg, dataSize);
            if (write(fd, msg, newSize) != newSize) {
                return -1;
            }
        }

        else { // if it doesn't have data
            msg[0] = FLAG;
            if (write(fd, msg, 1) != 1) {
                return -1;
            }
        }
        alarm(10);
        int typeResponse;
        int size_not_applicable;
        typeResponse = receivePacket(newMsg, size_not_applicable);

        if (type == SET) {
            
        }

        if (type == DISC) {
            if (machine == TRANSMITTER) {
                if (typeResponse == DISC) {
                    acknowledge = 1;
                    alarm(0);
                }
            }
            else if (machine == RECEIVER) {
                if (typeResponse = UA) {
                    acknowledge = 1;
                    alarm(0);
                }
            }
        }

        if (type == info) {
            if (messageParity == 0) {
                // TO DO
            }
        }

        if (type == UA || type == RR || type == REJ) {
            acknowledge = 1;
            alarm(0);
        }

        alarmEnabled = 1;    
    }

    if (messageParity == 0) {
        messageParity = 1;
    }
    else if (messageParity == 1) {
        messageParity = 0;
    }
    return 0;
}

int main() {
    unsigned char array1[10] = {0x00, ESCAPE,0x0F, FLAG, 0xFF, FLAG, 0x00, 0x00, 0x00, 0x00};
    printf("%i", addStuffing(array1, 6));
    return 0;
}


