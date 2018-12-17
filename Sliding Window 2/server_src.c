#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <time.h>

#define bufferLen 10	
#define portNo 8882
#define r 0.25

typedef struct packetAck{
	int sequenceNumber;
}ackPacket;

typedef struct packetData{
	int sequenceNumber;
	char data[bufferLen];
}dataPacket;

void error(char *s){
	printf("%s",s);
	exit(1);
}

int main(void){
	
	//Socket Creation Code
	struct sockaddr_in serverAddress,clientAddress;
	memset((char *)&serverAddress,0,sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(portNo);
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	int serverSocket;
	serverSocket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	
	if(serverSocket==-1){
		error("Error While Creating Server Sockert\n");
	}
	
	if(bind(serverSocket,(struct sockaddr *)&serverAddress,sizeof(serverAddress))==-1){
		error("Error While Binding\n");
	}
	dataPacket data;
	ackPacket ack;
	int recvLen;
	int sLen = sizeof(clientAddress);
	int base = 0;
	char temp[bufferLen];
	int state = -1;
	int windowSize = 0;
	dataPacket array[1000];
	ackPacket ackArray[1000];
	int maxPackets = 0;
	int random = 0;
	int ackNotSent = 0;
	int choice = 1;
	int j = 0,i=0;
	while(1){
		if(state==5)
			break;
		switch(state){
			case -1:
				//Enter Choice
				//printf("Enter The Choice\n1 : No Packet Drop\n2 : Drop Packets\n");
				//scanf("%d",&choice);
				state = 0;
				break;
			case 0:
				//Receiving Window Size
				printf("Waiting For Window Size From Client..\n");
				fflush(stdout);
				if((recvLen = recvfrom(serverSocket,&temp,sizeof(temp),0,(struct sockaddr *)&clientAddress,&sLen))==-1){
					error("Error While Receiving Window Size\n");
				}
				windowSize = atoi(temp);
				if(windowSize > 0){
					printf("Received The Window Size Of %d\n",windowSize);
					state = 1;
					break;
				}
			case 1:
				//Receiving MaxPackets
				if((recvLen = recvfrom(serverSocket,&maxPackets,sizeof(maxPackets),0,(struct sockaddr *)&clientAddress,&sLen))==-1){
					error("Error While Receiving Window Size\n");
				}
				printf("Max Packets Are %d\n",maxPackets);
				for(int k = 0;k<maxPackets;k++){
					array[k].sequenceNumber = -1;
					ackArray[k].sequenceNumber = -1;
				}
				state = 2;
				break;
			case 2:
				//Receiving Packets
				if((recvLen = recvfrom(serverSocket,&data,sizeof(data),0,(struct sockaddr *)&clientAddress,&sLen))==-1){
					error("Error While Receiving\n");
				}
				array[data.sequenceNumber].sequenceNumber = data.sequenceNumber;
				strcpy(array[data.sequenceNumber].data,data.data);
				printf("RECEIVE PACKET %d \n",data.sequenceNumber);
				//base = base + 1;
				state = 3;
				break;
			case 3:
				srand(time(NULL));
				random = rand()%2;
				ack.sequenceNumber = data.sequenceNumber;
				if(ack.sequenceNumber==15 && choice==2){
					j = 0;
					ackNotSent = ack.sequenceNumber;
					ack.sequenceNumber = -1; 
					state = 4;
					break;
				}
				if(sendto(serverSocket,&ack,recvLen,0,(struct sockaddr*)&clientAddress,sLen)==-1){
					error("Error While Sending Ack\n");
				}
				printf("SEND ACK %d \n",ack.sequenceNumber);
				if(ack.sequenceNumber==maxPackets-1){
					state = 5;
					break;
				}else{
					state = 2;
					break;
				}
			case 4:
				if((recvLen = recvfrom(serverSocket,&data,sizeof(data),0,(struct sockaddr *)&clientAddress,&sLen))==-1){
					error("Error While Receiving\n");
				}
				printf("RECEIVE PACKET %d \n",data.sequenceNumber);
				if(ackNotSent==data.sequenceNumber){
					ack.sequenceNumber = data.sequenceNumber;
					if(sendto(serverSocket,&ack,recvLen,0,(struct sockaddr*)&clientAddress,sLen)==-1){
						error("Error While Sending Ack\n");
					}
					printf("SEND ACK %d \n",ack.sequenceNumber);
					state = 2;
					break;
				}
			case 6:
				i = base;
				if(i < (base + windowSize) && i < maxPackets){
					if((recvLen = recvfrom(serverSocket,&data,sizeof(data),0,(struct sockaddr *)&clientAddress,&sLen))==-1){
						error("Error While Receiving\n");
					}
					array[data.sequenceNumber].sequenceNumber = data.sequenceNumber;
					strcpy(array[data.sequenceNumber].data,data.data);
					printf("RECEIVE PACKET %d \n",data.sequenceNumber);
					i = i + 1;
					state = 6;
					break;
				}else{
					state = 7;
					break;
				}
			case 7:
				while(j < (base + windowSize) && j < maxPackets){
					ack.sequenceNumber = j;
					if(sendto(serverSocket,&ack,recvLen,0,(struct sockaddr*)&clientAddress,sLen)==-1){
						error("Error While Sending Ack\n");
					}
					printf("SEND ACK %d \n",ack.sequenceNumber);
					j = j + 1;
				}
				if(j < maxPackets){
					base = base + windowSize;
					state = 6;
					break;
				}else{
					state = 5;
					break;
				}
				
		}
	}
	
	//File Writing Code
	j = 0;
	FILE *filePointer;
	filePointer = fopen("./out.txt","a");
	while(j < maxPackets){
		fprintf(filePointer,array[j].data);
		j = j + 1;
	}
	fclose(filePointer);
	printf("Data Written In File\n");
	close(serverSocket);
	return 0;
}
