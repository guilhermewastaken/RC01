#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// Baudrate settings are defined in <asm/termbits.h>, which is
// included by <termios.h>
#define BAUDRATE B38400
#define _POSIX_SOURCE 1 // POSIX compliant source

#define FALSE 0
#define TRUE 1

#define BUF_SIZE 5

volatile int STOP = FALSE;
int fd = -1;

void printByte(unsigned char byte[]) {
    for (size_t i = 0; i < sizeof(byte); i++){
        printf("%hhx - ", byte[i]);
    }
    printf("\n");
}

int compareBytes(unsigned char byte01[], unsigned char byte02[]) {
    if (sizeof(byte01) != 8 || sizeof(byte02) != 8) {
        return 1;
    }
    if ( memcmp(byte01, byte02, 8) ){
        return 0;
    }
    return 1;
}

int sendFrame(unsigned char message[]) {
    int bytes = write(fd, message, sizeof(message));
    if ( bytes == sizeof(message) ) {
        return 0;
    }
    return 1;
}

int receiveFrame()

int main(int argc, char *argv[])
{
    // Program usage: Uses either COM1 or COM2
    const char *serialPortName = argv[1];

    if (argc < 2)
    {
        printf("Incorrect program usage\n"
               "Usage: %s <SerialPort>\n"
               "Example: %s /dev/ttyS1\n",
               argv[0],
               argv[0]);
        exit(1);
    }

    // Open serial port device for reading and writing, and not as controlling tty
    // because we don't want to get killed if linenoise sends CTRL-C.
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
    newtio.c_cc[VTIME] = 0; // Inter-character timer unused
    newtio.c_cc[VMIN] = 5;  // Blocking read until 5 chars received

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

    // Create string to send
    unsigned char buf[BUF_SIZE];
    buf[0] = 'F';
    buf[1] = 'A';
    buf[2] = 'C';
    buf[3] = 'B';
    buf[4] = 'F';



    // In non-canonical mode, '\n' does not end the writing.
    // Test this condition by placing a '\n' in the middle of the buffer.
    // The whole buffer must be sent even with the '\n'.
    int bytes = write(fd, buf, BUF_SIZE);

    unsigned char aux[BUF_SIZE];
    printf("%d bytes written\n", bytes);

    // Wait until all bytes have been written to the serial portt
    sleep(1);

    alarm(3);
    while (1) {
        for (size_t i = 0; i < 5; i++) {
            int bytes = read(fd, aux, 1);
            printf("%c", aux);
        }
        if (memcmp(aux, buf, 5)) {
            printf("Noice");
            alarm(0);
        }
    }

    // Restore the old port settings
    if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    close(fd);
    return 0;
}
