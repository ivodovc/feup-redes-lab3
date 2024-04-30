/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "link_library.h"

int main(int argc, char** argv)
{

    if ( (argc < 2) ||
         ((strcmp("/dev/ttyS0", argv[1])!=0) &&
          (strcmp("/dev/ttyS1", argv[1])!=0) )) {
        printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
        exit(1);
    }

    char* port = argv[1];

    int fd;
    linkLayer connectionParameters;
    strcpy(connectionParameters.serialPort,port);
    connectionParameters.role = TRANSMITTER;
    connectionParameters.baudRate = 38400;
    fd = llopen(connectionParameters);
    printf("Transmitter: Port opened\n");

    char data[10] = {1,2,3,4,5,6,7,8,9};
    //llwrite(fd, data, 10);

    llclose(fd, connectionParameters, 0);
    printf("Transmitter: Port Closed\n");

   sleep(1);



    return 0;
}
