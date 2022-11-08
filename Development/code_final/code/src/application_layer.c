// Application layer protocol implementation

#include <stdio.h>
#include "link_layer.h"
#include <string.h>

#include "application_layer.h"

int FRAME_SIZE = 200;
int INPUT_SIZE = 100;

enum CONTROL_TYPE {DATA = 1, START, END};
 

int createControlPacket(int type, int fileSize, unsigned char *controlPacket) {
    //Returns the size of the controlPacket
    controlPacket[0] = (unsigned char) type;
    controlPacket[1] = 0; //We are only sending file size, not name
    int i = 2;
    while (fileSize > 0) {
        i++; //At the beginning so i is the number of bytes actually used
        controlPacket[i] = (unsigned char) (fileSize % 256);
        fileSize = (int) fileSize / 256;
    }
    controlPacket[2] = i - 2; //Number of bytes needed for file size
    return i + 1; //Size (last index + 0)
}

void readControlPacket(int *type, int *fileSize, unsigned char *controlPacket) {
    *type = (int) controlPacket[0]; //controlPacket[1] is irrelevant since we are only sending the file size
    *fileSize = 0;
    for (int i = ((int) controlPacket[2]) + 2; i > 2; i--) {
        *fileSize = *fileSize * 256 + ((int) controlPacket[i]);
        //We are rebuilding the size in the opposite direction
    }
    //No need to return size since we are saving fileSize already
}

void createDataPacket(int sequenceNumber, int fileSize, unsigned char *dataPacket, unsigned char *inputData) {
    dataPacket[0] = DATA;
    dataPacket[1] = (unsigned char) (sequenceNumber % 256);
    dataPacket[2] = (unsigned char) (fileSize / 256);
    dataPacket[3] = (unsigned char) (fileSize % 256); //All 4 direct from requirements
    for (size_t i = 0; i < fileSize; i++) {
        dataPacket[i+4] = inputData[i];
    }
}

void readDataPacket(int *sequenceNumber, int *fileSize, unsigned char *dataPacket, unsigned char *outputData) {
    *sequenceNumber = (int) dataPacket[1];
    *fileSize = ((int) dataPacket[2]) * 256 + ((int) dataPacket[3]);
    for (size_t i = 0; i < *fileSize; i++) {
        outputData[i] = dataPacket[i+4];
    }
}

int findSize(const char file_name[]) {
    FILE *fp;
    fp = fopen(file_name, "rb");
    if (fp == NULL) {
        printf("File is not in the correct place/doesn't exist\n");
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    int result = ftell(fp);
    fclose(fp);
    return result;
}


int setupTransmitter(LinkLayer connectionParameters, char filename[]) {
    if (llopen(connectionParameters) != 1) {
        printf("Error in llopen\n");
        return -1;
    }
    printf("Success in llopen\n");

    //Send control packet
    unsigned char frame[FRAME_SIZE];
    int size = createControlPacket(START,findSize(filename),frame);
    if (llwrite(frame, size) == -1) {
        printf("Error sending control packet\n");
    }
    printf("First control packet sent\n");

    return 0;
}

int setupReceiver(LinkLayer connectionParameters, int *fileSize) {
    if (llopen(connectionParameters) != 1) {
        printf("Error in llopen\n");
        return -1;
    }
    printf("Success in llopen\n");

    //Receive control packet
    unsigned char frame[FRAME_SIZE];
    if (llread((frame)) == -1){
        printf("Error receiving control packet\n");
    }
    int type;
    readControlPacket(&type, fileSize, frame);
    if (type != START) {
        printf("Error receiving control packet\n");
        return -1;
    }
    printf("First control packet received\n");

    return 0;
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {
    LinkLayer connectionParameters;
    if (role[0] == 'r') {
        connectionParameters.role = LlRx;
    }
    else if (role[0] == 't') {
        connectionParameters.role = LlTx;
    }
    strcpy(connectionParameters.serialPort, serialPort);
    connectionParameters.baudRate = baudRate;
    connectionParameters.nRetransmissions = nTries;
    connectionParameters.timeout = timeout;

    if (connectionParameters.role == LlTx) {  //Transmitter
        setupTransmitter(connectionParameters, "penguin.gif");
        printf("setup done\n");
        FILE *filePtr;
        filePtr = fopen("penguin.gif", "rb");
        unsigned char frame[FRAME_SIZE];
        unsigned char input[INPUT_SIZE];
        int counter = 0;
        while (!feof(filePtr)) {
            printf("Sent a frame\n");
            fread(input, INPUT_SIZE, 1, filePtr);
            createDataPacket(counter, INPUT_SIZE, frame, input);
            if (llwrite(frame, INPUT_SIZE + 4) == -1) {
                printf("Error sending data packet\n");
            }
            counter++;
        }
        fclose(filePtr);
        printf("Penguin sent\n");

        //Send control packet
        int size = createControlPacket(END,findSize(filename),frame);
        if (llwrite(frame, size) == -1) {
            printf("Error sending final control packet\n");
        }
        printf("Final control packet sent\n");

        if (llclose(0) == -1) {
            printf("Error in llclose\n");
        }
    }
    else if (connectionParameters.role == LlRx) { //Receiver
        int fileSize;
        unsigned char frame[FRAME_SIZE];
        unsigned char output[FRAME_SIZE];
        FILE *filePtr;
        filePtr = fopen("penguin.gif", "wb");
        setupReceiver(connectionParameters, &fileSize);

        int filledSize = 0;

        while (1) {
            if (llread(frame) == -1) {
                printf("Error receiving data packet\n");

            } else if (frame[0] == END) {  //Last control packet
                fclose(filePtr);
                printf("Transfer complete\n");
                llread(frame); //Receive DISC
                printf("Disconnecting\n");
                printf("File Size: %i\n", fileSize);
                break;
            }
            int sequenceNumber;
            int size;
            readDataPacket(&sequenceNumber, &size, frame, output);

            printf("FILLING: %i bytes with %i new bytes", filledSize, size);
            filledSize = filledSize + size;
            printf(". Result: %i\n", filledSize);

            if (filledSize > fileSize) {
                fwrite(output, (size -(filledSize - fileSize)), 1, filePtr);
            }
            else {
                fwrite(output, size, 1, filePtr);
            }

            printf("Succesfully wrote %i frame\n", sequenceNumber);
        }
    }
    else {
        printf("Error in role");
    }
}
