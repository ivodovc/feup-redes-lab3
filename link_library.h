#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1


#define FLAG 0x5C
#define SENDER_ADDRESS 0x03
#define RECEIVER_ADDRESS 0x01
#define CONTROLS_SET 0x08
#define CONTROL_UA 0x06
#define CONTROL_DISC 0x0A

typedef struct linkLayer{
 char serialPort[50];
 int role; //defines the role of the program: 0==Transmitter, 1=Receiver
 int baudRate;
 int numTries;
 int timeOut;
} linkLayer;

//ROLE
#define NOT_DEFINED -1
#define TRANSMITTER 0
#define RECEIVER 1

//SIZE of maximum acceptable payload; maximum number of bytes that application layer should send to link layer
#define MAX_PAYLOAD_SIZE 1000
//CONNECTION deafault values
#define BAUDRATE_DEFAULT B38400
#define MAX_RETRANSMISSIONS_DEFAULT 3
#define TIMEOUT_DEFAULT 4
#define _POSIX_SOURCE 1 /* POSIX compliant source */

// states
#define START  0
#define FLAG_RCV 1
#define A_RCV 2
#define C_RCV 3
#define BCC1_OK 4
#define DATA_RCV 5
#define BCC2_OK 6


#define DATA_SENT 5
#define POSITIVE_ACK 6

int llopen(linkLayer connectionParameters);
int llwrite(int fd, char * buffer, int length);
int llread(int fd, char * buffer);
int llclose(int fd, linkLayer connectionParameters, int showStatistics);
