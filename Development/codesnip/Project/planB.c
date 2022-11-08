machine = TRANSMITTER;

//llopen
sendPacket(SET, data, 0);

//write file

FILE *filePtr;
filePtr = fopen(filename, "rb");

unsigned char *input[450];

while (!feof(filePtr)) {
            fread(input, 450, 1, filePtr);
		sendPacket(INFO, input, 450);
		sleep(1);
}
fclose(filePtr);
//llclose
sendPacket(DISC, data, 0);
sleep(1);
sendPacket(UA, data, 0);


//--------------

machine = RECEIVER;

unsigned char *output[1000];
int sizeReceived, parityReceived;

//llopen
if (receivePacket(output, &size, &parityReceived) == SET) {
	sendPacket(UA, output, 0);
}

//read file

FILE *filePtr;
filePtr = fopen(filename, "wb");
while (1) {
	int type = receivePacket(output, &size, &parityReceived);
	if (type == DISC) {
		sendPacket(DISC, output, 0);
		break;
		fclose(filePtr);
	}
	else if (type == INFO) {
		fwrite(output, size, 1, filePtr);
	}
}