#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/select.h>

#define bufferLen 10
#define portNo 8882

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
	//File Reading Code
	FILE *filePointer;
	filePointer = fopen("./in.txt","r");
	dataPacket array[1000];
	ackPacket ackArray[1000];
	char *a = "a";
	int maxPackets;
	while(a!=NULL){
		a = fgets(array[maxPackets].data,bufferLen,(FILE*)filePointer);
		array[maxPackets].sequenceNumber = maxPackets++;
	}
	fclose(filePointer);	
	//
	
	//Socket Creation Code
	
	struct sockaddr_in serverAddress;
	memset((char*)&serverAddress,0,sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(portNo);
	serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	int clientSocket;
	clientSocket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	
	if(clientSocket==-1){
		error("Error While Creating Client Socket\n");
	}
	int slen = sizeof(serverAddress);
	int base = 0;
	int windowEnd = 0;
	char temp[bufferLen];
	int state = 0;
	int windowSize = 0;
	int i = 0;
	int flag = 0;
	dataPacket data;
	int j=0;
	ackPacket ack;
	fd_set readFds;
	struct timeval timeOut;
	int ret;
	int choice = -1;
	while(1){
		if(state==8){
			break;
		}
		switch(state){
			case 0:
				//Sending Window Size
				fflush(stdin);
				printf("Enter The Window Size\n");
				fgets(temp,sizeof(temp),stdin);
				windowSize = atoi(temp);	
				if(sendto(clientSocket,&temp,sizeof(temp),0,(struct sockaddr *)&serverAddress,sizeof(serverAddress))==-1){
					error("Error While Sending Window Size\n");				
				}
				windowEnd = windowSize-1;
				state = 1;
				break;
			case 1:
				//Sending MaxPackets
				if(sendto(clientSocket,&maxPackets,sizeof(maxPackets),0,(struct sockaddr *)&serverAddress,sizeof(serverAddress))==-1){
					error("Error While Sending Window Size\n");				
				}
				for(int k = 0;k<maxPackets;k++){
					//array[k].sequenceNumber = -1;
					ackArray[k].sequenceNumber = -1;
				}
				state = 2;
				break;
			case 2: 
				//Sending Packets
				i = base;
				while(i <= windowEnd){
					data.sequenceNumber = array[i].sequenceNumber;
					strcpy(data.data,array[i].data);
					if(sendto(clientSocket,&data,sizeof(data),0,(struct sockaddr *)&serverAddress,sizeof(serverAddress))==-1){
						error("Error While Sending Data Packet\n");	
					}
					printf("SEND PACKET %d : BASE %d \n",data.sequenceNumber,base);
					i = i + 1;
				}
				state = 4;
				break;
			case 3:
				if(i < maxPackets){
					data.sequenceNumber = array[i].sequenceNumber;
					strcpy(data.data,array[i].data);
					if(sendto(clientSocket,&data,sizeof(data),0,(struct sockaddr *)&serverAddress,sizeof(serverAddress))==-1){
						error("Error While Sending Data Packet\n");	
					}
					printf("SEND PACKET %d : BASE %d \n",data.sequenceNumber,base);
					i = i + 1;
					state = 4;
					break;
				}else{
					state = 7;
					break;
				}
			case 4:
				//Receiving Ack
				FD_ZERO(&readFds);
				FD_SET(clientSocket,&readFds);
		
				timeOut.tv_sec = 2;
				timeOut.tv_usec = 0;
		
				ret = select(clientSocket+1,&readFds,NULL,NULL,&timeOut);
				if(ret < 0){
					printf("TIMEOUT %d \n",base);
					state = 5;
					break;
				}else{
					if(recvfrom(clientSocket,&ack,sizeof(ack),0,(struct sockaddr *)&serverAddress,&slen)==-1){
						error("Error Receiving The Ack\n");
					}
					ackArray[ack.sequenceNumber].sequenceNumber = 1;
					printf("RECEIVE ACK %d \n",ack.sequenceNumber);
					if(ack.sequenceNumber==base){
						base = base + 1;
						windowEnd = windowEnd + 1;
						state = 3;
						break;
					}
				}
			case 5:
				//Sending Droped Packets Again
				
				data.sequenceNumber = array[base].sequenceNumber;
				strcpy(data.data,array[base].data);
				if(sendto(clientSocket,&data,sizeof(data),0,(struct sockaddr *)&serverAddress,sizeof(serverAddress))==-1){
					error("Error While Sending Data Packet\n");	
				}
				printf("SEND PACKET %d : BASE %d \n",data.sequenceNumber,base);
				state = 6;
				break;
			case 6:
				if(recvfrom(clientSocket,&ack,sizeof(ack),0,(struct sockaddr *)&serverAddress,&slen)==-1){
					error("Error Receiving The Ack\n");
				}
				ackArray[ack.sequenceNumber].sequenceNumber = 1;
				printf("RECEIVE ACK %d \n",ack.sequenceNumber);
				if(ack.sequenceNumber==base){
					base = base + 1;
					state = 3;
					break;
				}
			case 7:
				j = 0;
				while(j < windowSize){
					if(recvfrom(clientSocket,&ack,sizeof(ack),0,(struct sockaddr *)&serverAddress,&slen)==-1){
						error("Error Receiving The Ack\n");
					}
					ackArray[ack.sequenceNumber].sequenceNumber = 1;
					printf("RECEIVE ACK %d \n",ack.sequenceNumber);
					j = j + 1;
				}
				state = 8;
				break;
		}
	}
	
	close(clientSocket);
	return 0;
}
