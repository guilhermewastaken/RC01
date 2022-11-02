// Application layer protocol implementation

#include <stdio.h>

#include "application_layer.h"

FRAME_SIZE = 1000;
INPUT_SIZE = 450;

//vv Delete this afterwards vv
typedef enum
{
    LlTx,
    LlRx,
} LinkLayerRole;

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
    //Returns the size of the controlPacket
    controlPacket[0] = (unsigned char) type;
    controlPacket[1] = 0; //We are only sending file size, not name
    int i = 2;
    while (fileSize > 0) {
        i++; //At the beginning so i is the number of bytes actually used
        controlPacket[i] = (unsigned char) (fileSize % 256);
        fileSize = fileSize / fileSize;
    }
    controlPacket[2] = i - 2; //Number of bytes needed for file size
    return i; //Size
}

void readControlPacket(int *type, int *fileSize, unsigned char *controlPacket) {
    *type = (int) controlPacket[0]; //controlPacket[1] is irrelevant since we are only sending the file size
    *fileSize = 0;
    for (size_t i = controlPacket[2] + 2; i >= 0; i--) {
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
    for (size_t i = 0; i < fileSize; i++) {
        outputData[i] = dataPacket[i+4];
    }
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
    LinkLayer connectionParameters = {serialPort, role, baudRate, nTries, timeout};
    if (connectionParameters.role == LlTx) {    //Transmitter
        setupTransmitter(connectionParameters, filename);
        FILE *filePtr;
        filePtr = fopen(filename, "rb");
        unsigned char frame[FRAME_SIZE];
        unsigned char input[INPUT_SIZE];
        int counter = 0;
        while (!feof(filePtr)) {
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
        filePtr = fopen(filename, "wb");
        setupReceiver(connectionParameters, &fileSize);
        while (1) {
            if (llread(frame) == -1) {
                printf("Error receiving data packet\n");

            } else if (frame[0] == END) {  //Last control packet
                fclose(filePtr);
                printf("Transfer complete\n");
                llread(frame); //Receive DISC
                printf("Disconnecting\n");
                break;
            }
            int sequenceNumber;
            int size;
            readDataPacket(&sequenceNumber, &size, frame, output);
            fwrite(output, INPUT_SIZE, 1, filePtr);
            printf("Succesfully wrote %i frame\n", sequenceNumber);
        }
    }
    else {
        printf("Error in role");
    }
}