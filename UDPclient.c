#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include "UDPclient.h"

//this is the client

#define BUFLEN 2048
#define MSGS 5 /* number of messages to send */
#define SERVICE_PORT 21235

int recvlen;	  //# of bytes in acknowledgment message
struct sockaddr_in myaddr, remaddr;
int fd, i, slen;

void init(char *server_addr)
{
	
	slen = sizeof(remaddr);
	
	
	char *server = server_addr;  //change this to use a different server

	//create a socket
	if ((fd=socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("socket created\n");

	//bind it to all local addresses and pick any port number
	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);

	if (bind(fd, (struct sockaddr *) &myaddr, sizeof(myaddr)) < 0)
	{
		perror("bind failed");
		//return 0;
	}

	//now define remaddr, the address to whom we want to send messages
	//for convenience, the host address is expressed as a numeric IP address
	//that we will convert to a binary format via inet_aton

	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(SERVICE_PORT);
	if (inet_aton(server, &remaddr.sin_addr)==0)
	{
		fprintf(stderr, "inet_aton() failed\n");
		//exit(1);
	}
}

void sendMessage(char *buf)
{
	if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *) &remaddr, slen)==-1)
	{
		perror("sendto");
	}
}

char* sendRequest(char *buf) //same as sendMessage, but we care about the server response now
{
	//now send the messages
	
	if (sendto(fd, buf, strlen(buf), 0, (struct sockaddr *) &remaddr, slen)==-1)
	{
		perror("sendto");
	}
		
		
	//now receive an acknowledgement from the server
	//char* buffer = malloc (sizeof (char) * 8);
	char* buffer = malloc (2048);
	recvlen = recvfrom(fd, buffer, BUFLEN, 0, (struct sockaddr *) &remaddr, &slen);
	if (recvlen >= 0)
	{
		buffer[recvlen] = 0; //expect a printable string - terminate it
		return buffer;
		//printf("received message: \"%s\"\n", buf);
	}
	
}

void closeSocket()
{
	close(fd);
}

