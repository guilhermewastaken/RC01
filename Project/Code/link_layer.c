#include <iostream>

enum MACHINE {TRANSMITTER = 0, RECEIVER = 1};                                           //Machine constants
enum HEADER_TYPE {INVALID = -1, INFO, SET, DISC, UA, RR, REJ};
enum BYTE {FLAG = 0x7e, ESCAPE = 0x7d, ESC_FLAG = 0x5e, ESC_ESCAPE = 0x5d,              //Flag and byte stuffing
    A_TRANSMITTER_COMMAND = 0x03, A_RECEIVER_COMMAND = 0x01,                        //Address
    CNTRL_INFO_0 = 0x00, CNTRL_INFO_1 = 0x40, CNTRL_SET = 0x03, CNTRL_DISC = 0x0b,  //Control Commands
    CNTRL_UA = 0x07, CNTRL_RR_0 = 0x05, CNTRL_RR_1 = 0x85,
    CNTRL_REJ_0 = 0x01, CNTRL_REJ_1 = 0x81};                                        //Control responses

int machine;    //0 if transmitter or 1 if receiver
int ARRAY_SIZE = 800;
int messageParity = 0;  //0 or 1, switches
int alarmEnabled = 1;
int fd;


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
    if (type != INFO) {
        header[4] = FLAG;
    }
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

//----------------TESTED AND VALIDATED UNTIL HERE---------------

int receivePacket(unsigned char *data, int *size, int *parityReceived) {
    //Internal State Machine
    enum STATE_MACHINE {WAIT_FOR_FLAG = 0, BUILDING_HEADER, WAIT_FOR_LAST_FLAG, FILLING_INFO, INFO_FILLED, OVER};
    unsigned char byteReceived;
    int bytes;
    int counter = 0;
    int state = WAIT_FOR_FLAG;
    int header = INVALID;

    while(alarmEnabled == 1 && state != OVER){
        switch (state) {
            case WAIT_FOR_FLAG:
                bytes = read(fd,byteReceived,1);
                if (bytes == 1 && byteReceived == FLAG) {
                    data[0] = FLAG;
                    state = BUILDING_HEADER;
                }
                break;
            case BUILDING_HEADER:
                bytes = read(fd,byteReceived,1);
                if (bytes == 1) {
                    if (byteReceived == FLAG) {
                        counter = 0;
                    }
                    else {
                        counter++;
                        data[counter] = byteReceived;
                    }
                }
                if (counter == 3) {
                    header = getHeaderType(data, &parityReceived);
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
                if (bytes == 1 && byteReceived == FLAG) {
                    state = OVER;
                }
                else if (bytes == 1) {
                    state = WAIT_FOR_FLAG;
                    counter = 0;
                }
                break;
            case FILLING_INFO:
                bytes = read(fd,byteReceived,1);
                if (bytes == 1 && byteReceived == FLAG) {
                    *size = counter;
                    state = INFO_FILLED;
                }
                else if (bytes != 0) {
                    data[counter] = byteReceived;
                    counter++;
                }
                break;
            case INFO_FILLED:
                *size = removeStuffing(data, *size);
                if (getBCC(data, (*size) - 2) != data[(*size) - 2]) {
                    counter = 0;
                    sendPacket(REJ, 0, 0);
                    state = WAIT_FOR_FLAG;
                }
                break;
        }
    }
    return header;
}

int sendPacket(int type, unsigned char * data, int dataSize) {
    unsigned char header[5];
    unsigned char msg[ARRAY_SIZE];
    int acknowledge = 0;
    alarmEnabled = 1;
    int newSize;

    (void)signal(SIGALRM, alarmHandler);

    while (!acknowledge) {
        if (createHeader(header, type) != 0) {
            return -1;
        }
        if (type != INFO) {
            if (write(fd, header, 5) != 5) {
                return -1;
            }
        }
        else {
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

        alarm(10);
        int typeResponse;
        int parityReceived;

        if (type == UA || type == RR || type == REJ) {
            acknowledge = 1;
            alarm(0);
            break;
        }

        typeResponse = receivePacket(newMsg, 0, &parityReceived);

        if (type == SET) {
            if (typeResponse == UA) {
                acknowledge = 1;
                alarm(0);
            }
        }
        else if (type == DISC) {
            if (machine == TRANSMITTER) {
                if (typeResponse == DISC) {
                    acknowledge = 1;
                    alarm(0);
                }
            }
            else if (machine == RECEIVER) {
                if (typeResponse == UA) {
                    acknowledge = 1;
                    alarm(0);
                }
            }
        }
        else if (type == INFO) {
            if (typeResponse == REJ) {
                continue;
            }
            if (typeResponse == RR && parityReceived != messageParity) {
                acknowledge = 1;
                alarm(0);
                if (messageParity == 0) {
                    messageParity = 1;
                }
                else if (messageParity == 1) {
                    messageParity = 0;
                }
            }
        }
        alarmEnabled = 1;
    }
    return 0;
}

int llopen(LinkLayer connectionParameters)
{
    if (connectionParameters.tx) {
        machine = TRANSMITTER;
    }
    else if (connectionParameters.rx) {
        machine = RECEIVER;
    }

    fd = open(serialPortName, O_RDWR | O_NOCTTY);

    if (fd < 0)
    {
        perror(serialPortName);
        exit(-1);
    }

    struct termios oldtio;
    struct termios newtio;

    // Save current port settings
    if (tcgetattr(fd, &oldtio) == -1)
    {
        perror("tcgetattr");
        exit(-1);
    }

    // Clear struct for new port settings
    memset(&newtio, 0, sizeof(newtio));

    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    // Set input mode (non-canonical, no echo,...)
    newtio.c_lflag = 0;
    newtio.c_cc[VTIME] = 1; // Inter-character timer unused
    newtio.c_cc[VMIN] = 0;  // Blocking read until 5 chars received

    // VTIME e VMIN should be changed in order to protect with a
    // timeout the reception of the following character(s)

    // Now clean the line and activate the settings for the port
    // tcflush() discards data written to the object referred to
    // by fd but not transmitted, or data received but not read,
    // depending on the value of queue_selector:
    //   TCIFLUSH - flushes data received but not read.
    tcflush(fd, TCIOFLUSH);

    // Set new port settings
    if (tcsetattr(fd, TCSANOW, &newtio) == -1)
    {
        perror("tcsetattr");
        exit(-1);
    }

    printf("New termios structure set\n");

    if(machine == TRANSMITTER){
        if(sendPacket(SET, 0, 0) != 0){
            return -1;
        }
    }
    else if (machine == RECEIVER){
        int type = receivePacket(0, 0);
        if(type == SET){
            if(sendPacket(UA, 0, 0) != 0){
                return -1;
            }
        }
        return 1;
    }

////////////////////////////////////////////////
// LLWRITE
////////////////////////////////////////////////
    int llwrite(const unsigned char *buf, int bufSize)
    {
        if(machine == TRANSMITTER){
            if(sendPacket(INFO, buf, bufSize) != 0){
                return -1;
            }
        }

        return 0;
    }

////////////////////////////////////////////////
// LLREAD
////////////////////////////////////////////////
    int llread(unsigned char *packet)
    {
        if(machine == RECEIVER){
            int msgSize;
            int parityReceived;
            int type = receivePacket(packet, &msgSize, &parityReceived);
            if(type == INFO){
                if (parityReceived == messageParity){
                    if (messageParity == 0) {
                        messageParity = 1;
                    }
                    else if (messageParity == 1) {
                        messageParity = 0;
                    }
                    else {
                        return -1;
                    }
                    if(sendPacket(RR, 0, 0) != 0){
                        return -1;
                    }
                }
                else {
                    return -1;
                }
            }
        }
        else if(type == DISC){
            if(sendPacket(DISC, 0, 0) != 0){
                return -1;
            }
        }
        return 0;
    }


////////////////////////////////////////////////
// LLCLOSE
////////////////////////////////////////////////
    int llclose(int showStatistics)
    {
        if(machine == TRANSMITTER){
            if(sendPacket(DISC, 0, 0) != 0){
                return -1;
            }
            int type = receivePacket(0, 0);
            if(type == DISC){
                if(sendPacket(UA, 0, 0) != 0){
                    return -1;
                }
            }
        }
        return 1;
    }