// Application layer protocol implementation

#include <stdio.h>

#include "application_layer.h"

FRAME_SIZE = 1000;
INPUT_SIZE = 450;

typedef struct
{
    char serialPort[50];
    LinkLayerRole role;
    int baudRate;
    int nRetransmissions;
    int timeout;
} LinkLayer;

enum CONTROL_TYPE {DATA = 1, START, END};

void printByte(unsigned char byte) {
    printf("%hhx\n", byte);
}
void printByteSequence(unsigned char *sequence, int size) {
    for (size_t i = 0; i < size-1; i++) {
        printf("%hhx-", sequence[i]);
    }
    printf("%hhx\n", sequence[size-1]);
}

void writeToFile(FILE *filePtr, unsigned char *fileContent, int size) {
    fwrite(fileContent, size, 1, filePtr);
}

int createControlPacket(int type, int fileSize, unsigned char *controlPacket) {
    //Size of controlPacket array should be 6
    controlPacket[0] = (unsigned char) type;
    controlPacket[1] = 0;
    int i = 2;
    for (i = 2; i < sizeof(int) + 2; i++) {
        printf("%i", i);
        controlPacket[i] = (unsigned char) fileSize;
        fileSize >>= 8;
    }
    return i; //Size
}

void readControlPacket(int *type, int *fileSize, unsigned char *controlPacket) {
    *type = (int) controlPacket[0];
    int packetNum = (int) controlPacket[1];
    *fileSize = 0;
    for (int i = 2; i < packetNum + 2; i++) {
        *fileSize += (controlPacket[i]);
        *fileSize <<= 8;
    }
}

void startDataPacket(int sequenceNumber, int fileSize, unsigned char *dataPacket) {
    dataPacket[0] = 0x01;
    sequenceNumber = sequenceNumber % 256;
    dataPacket[1] = (unsigned char) sequenceNumber;
    dataPacket[2] = (unsigned char) fileSize;
    dataPacket[3] = (unsigned char) (fileSize >> 8);
}

void interpretDataPacket(int *sequenceNumber, int *fileSize, unsigned char *dataPacket) {
    *sequenceNumber = (int) dataPacket[1];
    *fileSize = (int) dataPacket[2];
    *fileSize <<= 8;
    *fileSize += (int) dataPacket[3];
}

int findSize(char file_name[])
{
    FILE* fp = fopen(file_name, "r");
    if (fp == NULL) {
        printf("File is not in the correct place/doesn't exist\n");
        return -1;
    }
    fseek(fp, 0L, SEEK_END);
    int result = ftell(fp);
    fclose(fp);
    return result;
}


int setupTransmitter(LinkLayer connectionParameters, char file_name[]) {
    if (llopen(connectionParameters) != 1) {
        printf("Error in llopen\n");
        return -1;
    }
    printf("Success in llopen\n");

    //Send control packet
    unsigned char *frame[FRAME_SIZE];
    int size = createControlPacket(START,findSize(file_name),frame);
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
    unsigned char *frame[FRAME_SIZE];
    if (llread((frame)) == -1){
        printf("Error receiving control packet\n");
    }
    int type;
    readControlPacket(&type, &fileSize, frame);
    if (type != START) {
        printf("Error receiving control packet\n");
        return -1;
    }
    printf("First control packet received\n");

    return 0;
}

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename) {
    LinkLayer connectionParameters = {};
    //if role transmitter
    setupTransmitter(connectionParameters, filename);
    FILE *filePtr;
    filePtr = fopen(filename,"rb");
    unsigned char frame[FRAME_SIZE];
    int counter = 0;
    while(!feof(filePtr)) {
        fread(frame,INPUT_SIZE,1,filePtr);
        for (size_t i = INPUT_SIZE; i >= 0; i--) {
            frame[i+4] = frame[i];
        }
        startDataPacket(counter, INPUT_SIZE, frame);
        if (llwrite(frame, INPUT_SIZE) == -1) {
            printf("Error sending data packet\n");
        }
        counter++;
    }
    fclose(filePtr);
    if (llclose(0) == -1) {
        printf("Error in llclose\n");
    }

    //else
    int fileSize;
    unsigned char frame[FRAME_SIZE]; //Error because if isnt done
    FILE *filePtr;                             //""
    filePtr = fopen(filename,"wb");
    setupReceiver(connectionParameters, &fileSize);
    while (1) {
        int opt = llread(frame);
        if (opt == -1) {
            printf("Error receiving data packet\n");
        }
        else if (opt == 0) {  //llclose
            fclose(filePtr);
            printf("Transfer complete\n");
            break;
        }
        int sequenceNumber;
        int size;
        interpretDataPacket(&sequenceNumber, &size, frame);
        for (size_t i = 0; i < size; i++) {
            frame[i] = frame[i+4];
        }
        fwrite(frame, INPUT_SIZE, 1, filePtr);
    }

}