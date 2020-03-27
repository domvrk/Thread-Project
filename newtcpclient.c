#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>

// define colors to set print colors
#define MAXLINE 4096
#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define BLU   "\x1B[36m"
#define MAG   "\x1B[35m"
#define RESET "\x1B[0m"


// function declarations
int getSellTick (int *ticketArr);
void clientudp();
void *serverudp();
void printtable();

// global variables
int ticketArr[25] = {0};
int portno;
char address[14];
int balance;
int sockfd;

int main(int argc, char *argv[]) {
	int n;
	struct sockaddr_in serv_addr; //server address
	struct hostent *server;	
	int noFunds = 0; // 0 = false, 1 = true
	
	// set client balance
	balance = 4000;

	//create a thread for udp server
	pthread_t tidserver;
	if(pthread_create(&tidserver, NULL, serverudp, NULL) != 0) {
				printf("Thread Failed");
	}
	
	// ticket index number
	int ticketI = 0;
	char buffer[256];

	//check for 4 arguments
	if(argc != 4) {
		fprintf(stderr, "usage %s <hostname> <port> <client 2 ip>\n", argv[0]);
		exit(0);
	}
	
	//open socket on input port
	strcpy(address, argv[3]);
	portno = atoi(argv[2]);
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        perror("ERROR opening socket");
	
	//get server using input hostname
	server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }
	
	//fill in server struct 
	bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
	
	//connect to socket 
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		perror("ERROR connecting (make sure server is running)");
		exit(1);
	}

	// wait for message from server so it knows both clients are conencted 
	n = read(sockfd,buffer,256);
	
	// for loop to send 15 messages
	int i;
	//char *msg;
	for (i = 0; i < 15; i++)
	{
		// buy message
		char buy[] = "BUY";
		char sell[] = "SELL";
		
		// message string to send to svr
		char msg[150];
		
		bzero(buffer,256);
		
		// check if noFunds
		if (noFunds == 0)
		{
			// false
			snprintf(msg, sizeof(msg), "%s %d", buy, balance);
			printf(BLU "[-> CLIENT ]: BUY %d\n" RESET, balance);
		}
		else if (noFunds == 1)
		{
			// true
			// set sell ticket to first available ticket
			int sellTick = getSellTick(ticketArr);
			snprintf(msg, sizeof(msg), "%s %d", sell, sellTick);
			printf(BLU "[-> CLIENT ]: SELL %d\n" RESET, sellTick + 10000);
			
			n = write(sockfd, msg,strlen(msg));
			
			if (n < 0) 
			 perror("ERROR writing to socket");
			
			n = read(sockfd, buffer,256);
			if (n < 0) 
				perror("ERROR reading from socket");
			
			// parse string buffer
			char *sellStr[2];
			char *p = strtok(buffer, " ");
			sellStr[0] = p;
			p = strtok(NULL, " ");
			sellStr[1] = p;
			
			
			int sellPrice = atoi(sellStr[1]);
			
			ticketArr[sellTick] = 0;
			
			// add sale to balance
			balance = balance + sellPrice;
			// reset noFunds
			noFunds = 0;
			
			printf(GRN "[<- SERVER ]: %d %d\n" RESET, sellTick + 10000, sellPrice);
			printf(BLU "[<> CLIENT ]: SELL %d $%d OK\n" RESET, sellTick + 10000, sellPrice);
			
			bzero(buffer, 256);
			i--;
			continue;
		}
		//send sell or buy 
		n = write(sockfd, msg,strlen(msg));	
		
		if (n < 0) 
			 perror("ERROR writing to socket");
		 
		bzero(buffer,256);
		//receive response from server	
		n = read(sockfd, buffer,256);
		if (n < 0) 
			perror("ERROR reading from socket");
		
		//if soldout then call udp function to buy to other client
		if (strcmp(buffer, "SOLDOUT") == 0)
		{
			printf(GRN "[<- SERVER ]: SOLDOUT\n" RESET);
			clientudp();
			
			continue;
		}
		//if anything but soldout or nofunds then ticket was bought
		else if (strcmp(buffer, "NOFUNDS") != 0)
		{
			noFunds = 0;
			char *priceStr[2];
			char *t = strtok(buffer, " ");
			priceStr[0] = t;
			t = strtok(NULL, " ");
			priceStr[1] = t;
		
			int price = atoi(priceStr[1]);
			ticketI = atoi(priceStr[0]);
			ticketArr[ticketI] = price;
			
			
			balance = balance - price;
			
			
			printf(GRN "[<- SERVER ]: %d %d\n" RESET, ticketI + 10000, price);
			printf(BLU "[<> CLIENT ]: BUY %d $%d OK\n" RESET, ticketI + 10000, price);
		}
		//if no funds then set nofunds = 1 to true so next loop will sell.
		else if (strcmp(buffer, "NOFUNDS") == 0)
		{
			printf(GRN "[<- SERVER ]: NOFUNDS\n" RESET);
			noFunds = 1;
		}
		
		bzero(msg, 150);
	}
	
	// sleep for 2s so both clients finish
	
	sleep(2);
	
	// print final table
	printtable();
	
    return 0;
	
}
//function to print tickets.
void printtable() {
	printf(BLU "[<> CLIENT ]: Database Table:\n" RESET);
	printf("TICKET NUMBER  PRICE\n----------------------------\n");
	int i;
	for (i = 0; i < 25; i++)
	{
		if(ticketArr[i] > 0) {
			int ticketNum = i;
			printf("[Tkt # %d]: $ %d \n", 10000 + ticketNum, ticketArr[i]);	
		}
	}
	printf("----------------------------\n");
	printf("BALANCE     :  $ %d\n", balance);
}

//client for udp connection
void clientudp() {
	int n; //error checking int
	int cliesock; 
	struct hostent *findserv;
	char buffer[4096]; //read and send through socket
	socklen_t len;	//length of udp socket
	struct sockaddr_in serv_addr;
	
	//open dgram socket
	cliesock = socket(AF_INET, SOCK_DGRAM, 0);
	if(cliesock < 0) {
		perror("ERROR socket");
	}
	//get host by using ip address
	findserv = gethostbyname(address);
	
	//set up server 
	bzero((char *) &serv_addr, 8);
	serv_addr.sin_family = AF_INET;
	
	bcopy((char *)findserv->h_addr, (char *)&serv_addr.sin_addr.s_addr, findserv->h_length);
	serv_addr.sin_port = htons(portno+1);
	
	bzero(buffer, MAXLINE);
	strcpy(buffer, "SCALP");
	len = sizeof(serv_addr); //get length of server
	
	//send scalp to other client
	snprintf(buffer, sizeof(buffer), "SCALP %d", balance);
	printf(MAG "[-> BUYER  ]: %s\n" RESET, buffer);
	n = sendto(cliesock, buffer, strlen(buffer), 0, (struct sockaddr *) &serv_addr, len);
	if(n < 0)
		perror("error in sendto");
	
	//receiver answer
	bzero(buffer, 256);
	n = recvfrom(cliesock, buffer, MAXLINE, 0, (struct sockaddr *)&serv_addr, &len);
	if (n < 0) 
		perror("ERROR read from socket");
	
	// check if buyer has correct balance
	if(strcmp(buffer, "NOMONEY") == 0) {
		// index of ticket to be sold
		int sellTick = getSellTick(ticketArr);
		
		// message to send buyer
		char newmsg[150];
		// set newmsg to SELL and the price
		snprintf(newmsg, sizeof(newmsg), "SELL %d", sellTick);
		printf(BLU "[-> CLIENT ]: SELL %d\n" RESET, sellTick + 10000);
		
		// send msg to buyer
		n = write(sockfd, newmsg,strlen(newmsg));
		if (n < 0) 
		 perror("ERROR writing to socket");
		
		bzero(buffer, 256);
		n = read(sockfd, buffer,256);
		if (n < 0) 
			perror("ERROR reading from socket");
		
		
		// parse string buffer
		char *newsellStr[2];
		char *q = strtok(buffer, " ");
		newsellStr[0] = q;
		q = strtok(NULL, " ");
		newsellStr[1] = q;
		
		int sellPrice = atoi(newsellStr[1]);
		
		ticketArr[sellTick] = 0;
		
		balance = balance + sellPrice;
		
		printf(GRN "[<- SERVER ]: %d %d\n" RESET, sellTick + 10000, sellPrice);
		printf(BLU "[<> CLIENT ]: SELL %d $%d OK\n" RESET, sellTick + 10000, sellPrice);
		
		bzero(buffer, 256);
		
	}
	//if there is enough money then buy the ticket and update table
	else {
		
		char *balanceStr2[2];
		char *t = strtok(buffer, " ");
		balanceStr2[0] = t;
		t = strtok(NULL, " ");
		balanceStr2[1]= t;
		int balance2;
		balance2 = atoi(balanceStr2[1]);
		int ticketnum = atoi(balanceStr2[0]);
		
		printf(RED "[<- SCALPER]: %i %i\n" RESET, ticketnum+10000, balance2);
		
		ticketArr[ticketnum] = balance2;
		balance = balance - balance2;
		
		
		printf(MAG "[<> BUYER  ]: SCALP %i $%i OK\n" RESET, ticketnum+10000, balance2);
	}
}

//thread function to have listening udp on port + 1
void *serverudp() {
	
	int servsock; //server socket
	char buffer[4096]; //read and send 
	struct sockaddr_in serv_addr, cli_addr;
	int n;
	
	//make socket
	servsock = socket(AF_INET, SOCK_DGRAM, 0);
	
	if(servsock < 0) {
		perror("ERROR opening socket");
	}
	
	//fill in server struct
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno+1);
	bzero(&(serv_addr.sin_zero), 8);
	
	//bind to socket
	if (bind(servsock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR on binding");
	}
	
	socklen_t len = sizeof(cli_addr);
	
	//wait for scalp message
	for( ; ; ) {
		bzero(buffer, MAXLINE);
		n = recvfrom(servsock, buffer, MAXLINE, 0, (struct sockaddr *)&cli_addr, &len);
		if (n < 0) 
			perror("ERROR read from socket");
		
		printf(MAG "[<- BUYER  ]: %s\n" RESET, buffer);
		
		char *balanceStr2[1];
		char *t = strtok(buffer, " ");
		balanceStr2[0] = t;
		t = strtok(NULL, " ");
		balanceStr2[1]= t;
		int balance2;
		balance2 = atoi(balanceStr2[1]);
		
		//printf("%i", balance2);
		
		int lowi = 0;
		int lowp = 400;
		
		//check table for lowest cost ticket and double price
		int j;
		for(j = 0; j < 25; j++) {
			if((ticketArr[j] < lowp) && (ticketArr[j] > 0)) {
				lowi = j;
				lowp = ticketArr[j];
			}
		}
		
		//if buyer does not have neough money then send message
		if((lowp*2) > balance2) {
			bzero(buffer, 256);
			strcpy(buffer, "NOMONEY");
			n = sendto(servsock, buffer, strlen(buffer), 0, (struct sockaddr *) &cli_addr, len);
			if(n < 0)
				perror("error in sendto");
			//printf("NOMONEY\n");
		}
		//if client has enough money let them buy and update table
		else {
			snprintf(buffer, sizeof(buffer), "%d %d", lowi, lowp*2);
			n = sendto(servsock, buffer, strlen(buffer), 0, (struct sockaddr *) &cli_addr, len);
			if(n < 0)
				perror("error in sendto");
			
			ticketArr[lowi] = 0;
			balance = balance + lowp*2;
			
			printf(RED "[-> SCALPER]: SCALP %i $%i\n" RESET, lowi+10000, lowp*2);
		}
	}
}

//gets ticket to sell to server
int getSellTick (int *ticketArr)
{
	int i;
	for (i = 0; i < 25; i++)
	{
		if (ticketArr[i] != 0)
		{
			return i;
		}
	}
	
	// should never get here
	printf("No tickets available\n");
	return 0;
}