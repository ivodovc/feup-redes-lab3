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

struct termios oldtio,newtio;
/**
 * Open Port 'port'
 * and return file descriptor as int
 * if 'transmitter' is 1, we are a transmitter, if 'transmitter' is 0, we are a receiver
*/
int llopen(linkLayer connectionParameters){
   /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
    */
    printf("Opening Port\n");

   char port[50];
   strcpy(port, connectionParameters.serialPort);
   int role = connectionParameters.role;

    int fd, c, res;
    char buf[5];
    int i, sum = 0, speed = 0;

    fd = open(port, O_RDWR | O_NOCTTY );
    if (fd < 0) { perror(port); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
        perror("tcgetattr");
        exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = connectionParameters.baudRate | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */

    tcflush(fd, TCIOFLUSH);


    if (tcsetattr(fd,TCSANOW,&newtio) == -1) {
        perror("tcsetattr");
        return -1;
    }

    if (role == TRANSMITTER){
        buf[0]=FLAG;
        buf[1]=RECEIVER_ADDRESS;
        buf[2]=CONTROLS_SET;
        buf[3]=buf[1]^buf[2];
        buf[4]=FLAG;
        res = write(fd,buf,5);
    }
	
	char recv[32];
    char received_byte;
    char state;
    int recv_i=0;

    // check if correct bytes arrived
    char expected_address = (role==TRANSMITTER) ? SENDER_ADDRESS : RECEIVER_ADDRESS;
    char expected_control = (role==TRANSMITTER) ? CONTROL_UA : CONTROLS_SET;
	
    int STOP=FALSE;
    while (STOP==FALSE) {       /* loop for input */
        res = read(fd,(&received_byte),1);   /* read byte by byte */
        recv[recv_i] = received_byte;
        recv_i++;
        printf("Received: %x\n", received_byte);

        //UA state machine
        switch (state){
            case START:
                if (received_byte == FLAG)
                    state = FLAG_RCV;
                break;

            case FLAG_RCV:
                if (received_byte == expected_address)
                    state = A_RCV;
                else{
                    state = START;
                    recv_i = 0;
                }
                break;

            case A_RCV:
                if (received_byte == expected_control)
                    state = C_RCV;
                else{
                    state = START;
                    recv_i = 0;
                }
                break;

            case C_RCV:
                if (received_byte == (recv[recv_i-2] ^ recv[recv_i-3]))
                    state = BCC_OK;
                else{
                    state = START;
                    recv_i = 0;
                }
                break;

            case BCC_OK:
                if (received_byte == FLAG)
                    state = STOP;
                    STOP = TRUE;
                    break;
        }
    }

    if ((role==RECEIVER)){
        buf[0]=FLAG;
        buf[1]=SENDER_ADDRESS;
        buf[2]=CONTROL_UA;
        buf[3]=buf[1]^buf[2];
        buf[4]=FLAG;
        res = write(fd,buf,5);
    }
    
    return fd;
}

int llclose(int fd, linkLayer connectionParameters, int showStatistics){
    tcflush(fd, TCIOFLUSH);
   char port[50];
   int role = connectionParameters.role;

    // variable initialization
    int res;
    char buf[10];
    char recv[10]; // receive buffer
    char received_byte;
    char state;
    int recv_i=0;
    int STOP = FALSE;

    // Transmitter sends DISC and waits for DISC from RECEIVER
    if (role==TRANSMITTER){
        buf[0]=FLAG;
        buf[1]=RECEIVER_ADDRESS;
        buf[2]=CONTROL_DISC;
        buf[3]=buf[1]^buf[2];
        buf[4]=FLAG;
        res = write(fd,buf,5);
        printf("TRANSMITTER SENDING DISC, %d bytes written\n", res);

        // check for DISC from RECEIVER
        STOP=FALSE;
        recv_i = 0;
        while (STOP==FALSE) {       /* loop for input */
            printf("State: %d\n", state);
            res = read(fd,(&received_byte),1);   /* read byte by byte */
            recv[recv_i] = received_byte;
            recv_i++;
            printf("last: %x, before last: %x\n", *(recv-1), *(recv-2));

            printf("Received: %x\n", received_byte);
            
            //state machine
            switch (state){
                case START:
                    if (received_byte == FLAG)
                        state = FLAG_RCV;
                    break;

                case FLAG_RCV:
                    if (received_byte == SENDER_ADDRESS)
                        state = A_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case A_RCV:
                    if (received_byte == CONTROL_DISC)
                        state = C_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case C_RCV:
                    if (received_byte == (recv[recv_i-2] ^ recv[recv_i-3]))
                        state = BCC_OK;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case BCC_OK:
                    if (received_byte == FLAG)
                        state = STOP;
                        STOP = TRUE;
                        break;
        }
        // if everything is ok, send back UA

        buf[0]=FLAG;
        buf[1]=RECEIVER_ADDRESS;
        buf[2]=CONTROL_UA;
        buf[3]=buf[1]^buf[2];
        buf[4]=FLAG;
        res = write(fd,buf,5);
        printf("TRANSMITTER, SENDING UA, %d bytes written\n", res);
        printf("TRANSMITTER DISCONNECT done \n");

     }
    }else if (role==RECEIVER){
    // RECEIVER WAITS FOR DISC, then sends back DISC and waits for UA from TRANSMITTER
       
       // 1. step check for DISC from RECEIVEr
        STOP=FALSE;
        recv_i = 0;
        while (STOP==FALSE) {       /* loop for input */
            res = read(fd,(&received_byte),1);   /* read byte by byte */
            recv[recv_i] = received_byte;
            recv_i++;           
            printf("Receiver state: %d \n", state);
            printf("Receiver read byte: %x\n", received_byte);

            //WAIT FOR DISC from TRANSMITTER 
            switch (state){
                case START:
                    if (received_byte == FLAG)
                        state = FLAG_RCV;
                    break;

                case FLAG_RCV:
                    if (received_byte == RECEIVER_ADDRESS)
                        state = A_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case A_RCV:
                    if (received_byte == CONTROL_DISC)
                        state = C_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case C_RCV:
                    if (received_byte == (recv[recv_i-2] ^ recv[recv_i-3]))
                        state = BCC_OK;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case BCC_OK:
                    if (received_byte == FLAG)
                        state = STOP;
                        STOP = TRUE;
                        break;
            }
        }

        // DISC received, send back DISC
        buf[0]=FLAG;
        buf[1]=SENDER_ADDRESS;
        buf[2]=CONTROL_DISC;
        buf[3]=buf[1]^buf[2];
        buf[4]=FLAG;
        res = write(fd,buf,5);
        printf("RECEIVER SENDING DISC, %d bytes written\n", res);

        // WAIT FOR UA from TRANSMITTER
        // 1. step check for DISC from RECEIVEr
        int STOP=FALSE;
        recv_i = 0;
        while (STOP==FALSE) {       /* loop for input */
            res = read(fd,(&received_byte),1);   /* read byte by byte */
            recv[recv_i] = received_byte;
            recv_i++;

            //WAIT FOR DISC from TRANSMITTER 
            switch (state){
                case START:
                    if (received_byte == FLAG)
                        state = FLAG_RCV;
                    break;

                case FLAG_RCV:
                    if (received_byte == RECEIVER_ADDRESS)
                        state = A_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case A_RCV:
                    if (received_byte == CONTROL_UA)
                        state = C_RCV;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case C_RCV:
                    if (received_byte == (recv[recv_i-2] ^ recv[recv_i-3]))
                        state = BCC_OK;
                    else{
                        state = START;
                        recv_i = 0;
                    }
                    break;

                case BCC_OK:
                    if (received_byte == FLAG)
                        state = STOP;
                        STOP = TRUE;
                        break;
            }
        }
        printf("RECEIVED DISCONNECT done \n");
    }

    if ( tcsetattr(fd,TCSANOW,&oldtio) == -1) {
        perror("tcsetattr");
        exit(-1);
    }
    close(fd);
}

int llwrite(int fd, char * buffer, int length){
    
    if (length>MAX_PAYLOAD_SIZE){
        return -1;
    }

    // 6 control bytes: flag, address, control, bcc, ..... , bcc, flag
    int send_buffer_len = length+6;
    char buf[send_buffer_len];
    buf[0] = FLAG;
    buf[1] = RECEIVER_ADDRESS; // we assume that only transmitter uses llwrite, but otherwise its strange
   
    char Ns = 0; // TODO keep track of Ns
    char control = 0b1000000 | (Ns<<7);
    buf[2] = control;

    buf[3] = buf[1]^buf[2];

    // copy the data into sending buffer
    memcpy((buf+4), buffer, length);

    buf[send_buffer_len-2] = 0; // TODO BCC VALUE
    buf[send_buffer_len-1] = FLAG;

    int res = write(fd,buf,send_buffer_len);
    printf("WRITING, %d bytes written\n", res);

}

int llread(int fd, char * buffer){
    // read character by character until flag is encountered
}
