#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/select.h>

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

int main(void){



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
	
	dataPacket data;
	ackPacket ack;
	char buffer[bufferLen];
	char message[bufferLen];
	int state = -2,slen = sizeof(serverAddress);
	int windowSize;
	char temp[bufferLen];
	int i = 0;
	char *status;
	dataPacket array[1000];
	memset(array,0,sizeof(array));
	char *a = "abc";
	int maxPackets = 0;
	FILE *filePointer;
	filePointer = fopen("./in.txt","r");
	while(a!=NULL){
		a = fgets(array[maxPackets].data,bufferLen,(FILE*)filePointer);
		//printf("%s\n",array[maxPackets].data);
		maxPackets++;
	}
	fclose(filePointer);
	printf("Max Packets Are : %d\n",maxPackets);
	struct stat st;
	stat("./in.txt",&st);
	//printf("File Size is %d bytes\n",st.st_size);
	int numberPackets = (int)st.st_size / (bufferLen-1);
	//printf("Number Of Packets in File are : %d\n",numberPackets);
	int count = 0,j = 0;
	//printf("%s\n",array[0].data);
	int base = 0;
	int ackArray[100];
	for(int k = 0;k<100;k++){
		ackArray[k] = -1;
	}
	int packetNum = 0;
	int ackNum = 0;
	int flag = -1;
	while(1){
		//printf("State Is %d \n",state);
		if(i+base==maxPackets){
			break;
		}
		switch(state){
			case -2:
				//Get Window Size From User 
				printf("Enter The Window Size\n");
				fgets(temp,sizeof(temp),stdin);
				windowSize = atoi(temp);
				//Send Window Size TO Client	
				if(sendto(clientSocket,&temp,sizeof(temp),0,(struct sockaddr *)&serverAddress,sizeof(serverAddress))==-1){
					error("Error While Sending Window Size\n");				
				}
				count = numberPackets/windowSize;
				//printf("Number Of Windows %d\n",count);
				state = 0;
				break;
			/*case -1:
				j = 0;
				//Read From File
				
				while(j < windowSize){
					a = fgets(array[j].data,bufferLen,(FILE*)filePointer);
					j = j + 1;
				}
				state = 0;
				break;*/	
			case 0:
				//Send Packet To Server
				state = -999;
				strcpy(data.data,array[i+base].data);
				data.sequenceNumber = i;
				//printf("%s\n",array[i+base].data);	
				if(sendto(clientSocket,&data,sizeof(data),0,(struct sockaddr *)&serverAddress,sizeof(serverAddress))==-1){
					error("Error While Sending Data Packet\n");	
				}
				printf("SEND PACKET %d : BASE %d \n",packetNum,base);
				packetNum++;
				if(i==windowSize-1){
					i = 0;
					state = 1;
					break;
				}else{
					i++;
					state = 0;
					break;
				}
			case 1:
				//Receive Ack From SERVER
				state = -999;
				if(recvfrom(clientSocket,&ack,sizeof(ack),0,(struct sockaddr *)&serverAddress,&slen)==-1){
					error("Error Receiving The Ack\n");
				}
				if(ack.sequenceNumber!=-1)
					printf("RECEIVE ACK %d : BASE %d\n",ackNum,base);
				ackNum++;
				ackArray[i] = ack.sequenceNumber;
				if(ack.sequenceNumber==-1 || ack.sequenceNumber==i){
					if(i==windowSize-1){
						i = 0;
						state = 2;
						//printf("count is %d\n",count);
						break;
					}else{
						i++;
						state = 1;
						break;	
					}
				}
			case 2:
				//Check If The Packets Are Dropped
				sleep(1);
				flag = -1;
				for(int k = 0;k < windowSize;k++){
					if(ackArray[k]==-1){
						printf("Time Out : %d\n",k+base);
						flag = k;
					}
				}
				if(flag==-1){
					base  = base + windowSize;
					state = 0;
					break;
				}else{
					state = 3;
					break;
				}
			case 3:
				// Send Dropped Packet Again
				data.sequenceNumber = flag;
				strcpy(data.data,array[flag+base].data);
				if(sendto(clientSocket,&data,sizeof(data),0,(struct sockaddr *)&serverAddress,sizeof(serverAddress))==-1){
					error("Error While Sending Data Packet\n");	
				}else{
					printf("SEND PACKET %d : BASE %d\n",base + flag,base);
					if(recvfrom(clientSocket,&ack,sizeof(ack),0,(struct sockaddr *)&serverAddress,&slen)==-1){
						error("Error Receiving The Ack\n");
					}
					printf("RECEIVE ACK %d : BASE %d\n",base + ack.sequenceNumber,base);
					base  = base + windowSize;
					flag = -1;
					state = 0;
					break;
				}
				
		}
	}
	close(clientSocket);
	return 0;
}
