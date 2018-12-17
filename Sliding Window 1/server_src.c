#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>

#define bufferLen 20
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

int main(int argc ,char *argv){
	//printf("%c\n",argv[1]);
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
	int state = -1;
	int recvLen;
	int sLen = sizeof(clientAddress);
	int drop = 0;
	int windowSize = 0;
	char temp[bufferLen];
	dataPacket array[100];
	int i = 0,j = 0;
	//int count = 5;
	int base = 0;
	int packetNum = 0;
	int ackNum = 0;
	int flag = -1;
	int random = 0;
	int random1 = 0;
	printf("Enter \n 0 : No Packet Drop\n 1 : Packet Drop\n");
	scanf("%d",&drop);
	while(1){
		switch(state){
			case -1:
				//Receive Window Size From Client
				printf("Waiting For Window Size From Client..\n");
				fflush(stdout);
				if((recvLen = recvfrom(serverSocket,&temp,sizeof(temp),0,(struct sockaddr *)&clientAddress,&sLen))==-1){
					error("Error While Receiving Window Size\n");
				}
				windowSize = atoi(temp);
				if(windowSize > 0){
					printf("Received The Window Size Of %d\n",windowSize);
					state = 0;
					break;
				}
			case 0:	
				//Receiving Packets From Client
				state = -999;
				//printf("Waiting For Packet %d From Sender..\n",i);
				fflush(stdout);
				if((recvLen = recvfrom(serverSocket,&data,sizeof(data),0,(struct sockaddr *)&clientAddress,&sLen))==-1){
					error("Error While Receiving\n");
				}
				array[i] = data;
				if(data.sequenceNumber==i){
					if(i==windowSize-1){
						i = 0;
						state = 1;
						break;
					}else{
						i++;
						state = 0;
						break;
					}
				}
			case 1:
				//Send Ack To Client
				state = -999;
				if(array[i].sequenceNumber==i){
					packetNum++;
					ack.sequenceNumber = i;
					random = (rand()+i+1)%windowSize;
					if(i==random && drop==1 && flag==-1){
						printf("RECEIVE PACKET %d DROP : BASE %d\n",i+base,base);
						ack.sequenceNumber = -1;
						flag = random;	
					}else{
						printf("RECEIVE PACKET %d : BASE %d\n",i + base,base);
					}
					if(sendto(serverSocket,&ack,recvLen,0,(struct sockaddr*)&clientAddress,sLen)==-1){
						error("Error While Sending Ack\n");
					}
					if(flag==-1 || flag!=i)
						printf("SEND ACK %d : BASE %d\n",i + base,base);
					ackNum++;
					if(i==windowSize-1){
						i = 0;
						if(flag==-1){
							state = 2;
							break;
						}else{
							state = 3;
							break;
						}
					}else{
						i++;
						state = 1;
						break;
					}
				
				}
			case 2:
				//Writing To The File
				state = -999;
				j = 0;
				FILE *filePointer;
				filePointer = fopen("./out.txt","a");
				while(j < windowSize){
					fprintf(filePointer,array[j].data);
					j = j + 1;
				}
				fclose(filePointer);
				//count = count - 1;
				//printf("count is %d\n",count);
				state = 0;
				base = base + windowSize;
				break;	
			case 3:
				// Receiving Retransmitted Packets
				if((recvLen = recvfrom(serverSocket,&data,sizeof(data),0,(struct sockaddr *)&clientAddress,&sLen))==-1){
					error("Error While Receiving\n");
				}
				printf("RECEIVE PACKET %d : BASE %d\n",flag + base,base);
				if(data.sequenceNumber==flag){
					ack.sequenceNumber = flag;
					if(sendto(serverSocket,&ack,recvLen,0,(struct sockaddr*)&clientAddress,sLen)==-1){
						error("Error While Sending Ack\n");
					}
					printf("SEND ACK %d : BASE %d\n",flag + base,base);
				}
				flag = -1;
				state = 2;
				break;
		}
	}
	close(serverSocket);
	return 0;
}
