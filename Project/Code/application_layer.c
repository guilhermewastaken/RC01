// Application layer protocol implementation

#include "application_layer.h"

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

int readFile(FILE *filePtr, int sizeOfBatch) {
    filePtr = fopen("pinguim.gif","rb");
    unsigned char fileContent[sizeOfBatch * 2];
    FILE *file2;
    file2 = fopen("newpinguim.gif", "wb");
    while(!feof(filePtr))
    {
        fread(fileContent,sizeOfBatch,1,filePtr);
        writeToFile(file2, fileContent, 1000);
        printByteSequence(fileContent, 1000);
        printf("%d", ftell(filePtr));
    }
    fclose(filePtr);
    fclose(file2);
}

int createControlPacket(int type, int fileSize, unsigned char *controlPacket) {
    //Size of controlPacket array should be 6
    controlPacket[0] = (unsigned char) type;
    int i = 2;
    for (i = 2; i < sizeof(int) + 2; i++) {
        printf("%i", i);
        controlPacket[i] = (unsigned char) fileSize;
        fileSize >>= 8;
    }
    controlPacket[1] = (unsigned char) i;
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

void applicationLayer(const char *serialPort, const char *role, int baudRate,
                      int nTries, int timeout, const char *filename)
{

}