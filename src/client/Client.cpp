#include "Client.h"

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<arpa/inet.h>
#include<sys/socket.h>

void die(char *s)
{
	perror(s);
	exit(1);
}

int main(void)
{
	struct sockaddr_in si_other;
	int s, i, slen=sizeof(si_other);
	char buf[MSG_BUFFER_MAX_SIZE];
	char message[MSG_BUFFER_MAX_SIZE];
    if (i) {}

	if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
	{
        char s[] = "socket";
		die(s);
	}

	memset((char *) &si_other, 0, sizeof(si_other));
	si_other.sin_family = AF_INET;
	si_other.sin_port = htons(SERVER_DEFAULT_PORT);
	
	if (inet_aton(SERVER_DEFAULT_ADDRESS , &si_other.sin_addr) == 0) 
	{
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	while(1)
	{
		printf("Enter message : ");
		fgets(message, MSG_BUFFER_MAX_SIZE, stdin);
		
		//send the message
		if (sendto(s, message, strlen(message) , 0 , (struct sockaddr *) &si_other, slen)==-1)
		{
            char st[] = "sendto()";
			die(st);
		}
		
		//receive a reply and print it
		//clear the buffer by filling null, it might have previously received data
		memset(buf,'\0', MSG_BUFFER_MAX_SIZE);
		//try to receive some data, this is a blocking call
		if (recvfrom(s, buf, MSG_BUFFER_MAX_SIZE, 0, (struct sockaddr *) &si_other, (socklen_t*)&slen) == -1)
		{
            char r[] = "recvfrom()";
			die(r);
		}
		
		puts(buf);
	}

	close(s);
	return 0;
}