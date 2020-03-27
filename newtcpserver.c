#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <unistd.h>

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define BLU   "\x1B[36m"
#define MAG   "\x1B[35m"
#define RESET "\x1B[0m"

// function declarations
void dostuff (int sock);
void printTable(int *ranArr, int *boolArr);
void createTickets(int *ranArr, int *boolArr);
void firstone(int balance, int *ranArr, int *boolArr, int *noFunds, int *tempp);
int buy (int *boolArr);

int main(int argc, char *argv[]) {
	//variables for tcp 
	int sockfd, client1, client2, portno; 
	socklen_t clilen; 
	
	// string buffer
	char buffer[256];
	
	// arrays to hold ticket information
	int ranArr[25];		// hold ticket price
	int boolArr[25];	// holds ticket status
	
	// generate the ticket prices
	createTickets(ranArr, boolArr);
	
	// print the initial table of tickets
	printTable(ranArr, boolArr);
	
	struct sockaddr_in serv_addr, cli_addr;
	fd_set fds; //set for select()
	
	//check for 2 arguments
	if  (argc != 2) {
		fprintf(stderr, "Error, no port provided\n");
		exit(1);
	}
	
	//create socket
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sockfd < 0) 
		perror("ERROR opening socket");
	
	//resets socket
	int optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));
	
	//add server information
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = atoi(argv[1]);
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(portno);
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	
	socklen_t size = sizeof(serv_addr);
	
	//bind to socket
	if (bind(sockfd, (struct sockaddr *) &serv_addr, size) < 0) 
		perror("ERROR on binding");
	//listen for connection
	listen(sockfd,5);
	
	
	clilen = sizeof(cli_addr);
	
	
	//wait for first and second connection
	printf("0 clients connected. Waiting on CLIENT 1 to connect.\n");
	
	if ((client1 = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0) {
		perror("accept");
		exit(1);
	}

	printf("1 client connected . Waiting on CLIENT 2 to connect.\n");
	
	/* Accept another connection. */
	if ((client2 = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen)) < 0) {
		perror("accept");
		exit(1);
	}
	
	printf("2 clients connected. Ready for incoming requests...\n");
	
	
	int maxfd = (client1 > client2 ? client1 : client2) + 1;
	
	// variables
	int clientready;
	int clientread;
	int balance;
	int balance2;
	int noFunds = 0; // 0 = false, 1 = true
	int ticket = 0;
	
	// messages to let clients know they are both connected
	clientread = send(client1, buffer, strlen(buffer), 0);
	if(clientread < 1) {
		close(client1);
		close(client2);
	}
	clientread = send(client2, buffer, strlen(buffer), 0);
	if(clientread < 1) {
		close(client1);
		close(client2);
	}
	
	// main loop to accept/send messages
	while (1) 
	{
		FD_ZERO(&fds);
		FD_SET(client1, &fds);
		FD_SET(client2, &fds);
		
		if ((clientready = select(maxfd, &fds, (fd_set *) 0, (fd_set *) 0, (struct timeval *) 0)) < 0)
		{
			perror("select error");
			exit(1);
		}
		
		bzero(buffer,256);
		
		// client 1 part
		if(FD_ISSET(client1, &fds)) {
			clientread = recv(client1, buffer, sizeof(buffer), 0);
			if(clientread < 1) {
				close(client1);
				close(client2);
				break;
			}
			
			// separate buffer string into two parts "BUY" and <balance>
			char *balanceStr[1];
			char *t = strtok(buffer, " ");
			balanceStr[0] = t;
			t = strtok(NULL, " ");
			balanceStr[1]= t;
			
			// convert client's balance str to int
			balance = atoi(balanceStr[1]);
				
			// check if message is buy
			if (strcmp(balanceStr[0], "BUY") == 0)
			{
				// call ticket function which will return the first
				// available ticket
				ticket = buy(boolArr);
				
				// make sure ticket is above -1 (if it is -1 server is soldout)
				if (ticket > -1)
				{
					// call function to check if user has enough money
					firstone(balance, ranArr, boolArr, &noFunds, &ticket);
				
					printf(BLU "[<- Client 1]: %s %d\n" RESET, balanceStr[0], balance);
				
					bzero(buffer,256);
					
					// check if user has NOFUNDS
					if (noFunds == 1)		
					{
						// set buffer to NOFUNDS
						strcpy(buffer, "NOFUNDS");
						printf(GRN "[-> SERVER  ]: CLIENT 1 BUY NOFUNDS\n" RESET);
					}
					else if (noFunds == 0)
					{
						// set buffer to ticket buy info
						snprintf(buffer, sizeof(buffer), "%d %d", ticket, ranArr[ticket]);
						printf(GRN "[-> SERVER  ]: CLIENT 1 BUY %d $%d OK\n" RESET, ticket + 10000, ranArr[ticket]);
					}
				}
				// soldout
				else
				{
					// set buffer to SOLDOUT
					strcpy(buffer, "SOLDOUT");
					
				}
				
			}
			// check if message is SELL
			else if (strcmp(balanceStr[0], "SELL") == 0)
			{
				// set tempTick to the ticket the client is trying to sell
				int tempTick = atoi(balanceStr[1]);
				
				printf(BLU "[<- Client 1]: %s %d\n" RESET, balanceStr[0], tempTick + 10000);
				
				// set ticket to available in boolArr
				boolArr[tempTick] = 1;
				
				// set noFunds to false
				noFunds= 0;
		
				// set buffer to ticket sell info
				snprintf(buffer, sizeof(buffer), "%d %d", tempTick, ranArr[tempTick]);
				printf(GRN "[-> SERVER  ]: CLIENT 1 SELL %d OK\n" RESET, tempTick + 10000);
			}
			
			// send the message to the client 
			clientread = send(client1, buffer, strlen(buffer), 0);
			if(clientread < 1) {
				close(client1);
				close(client2);
				break;
			}
			
		}
		// end client 1 part
		
		bzero(buffer,256);
		
		// client 2 part
		if(FD_ISSET(client2, &fds)) {
			clientread = recv(client2, buffer, sizeof(buffer), 0);
			if(clientread < 1) {
				close(client1);
				close(client2);
				break;
			}
			
			// separate buffer string into two parts "BUY" and <balance>
			char *balanceStr2[1];
			char *t = strtok(buffer, " ");
			balanceStr2[0] = t;
			t = strtok(NULL, " ");
			balanceStr2[1]= t;
			
			// convert client's balance str to int
			balance2 = atoi(balanceStr2[1]);
			
			// check if message is buy
			if (strcmp(balanceStr2[0], "BUY") == 0)
			{
				// call ticket function which will return the firstone
				// available ticket
				ticket = buy(boolArr);
				
				// make sure ticket is above -1 (if it is -1 server is soldout)
				if (ticket > -1)
				{
					// call function to check if user has enough money
					firstone(balance2, ranArr, boolArr, &noFunds, &ticket);
				
					printf(BLU "[<- Client 2]: %s %d\n" RESET, balanceStr2[0], balance2);
				
					bzero(buffer,256);
					
					// check if user has NOFUNDS
					if (noFunds == 1)		
					{
						// set buffer to NOFUNDS
						strcpy(buffer, "NOFUNDS");
						printf(GRN "[-> SERVER  ]: CLIENT 2 BUY NOFUNDS\n" RESET);
					}
					else if (noFunds == 0)
					{
						// set buffer to ticker buy info
						snprintf(buffer, sizeof(buffer), "%d %d", ticket, ranArr[ticket]);
						printf(GRN "[-> SERVER  ]: CLIENT 2 BUY %d $%d OK\n" RESET, ticket + 10000, ranArr[ticket]);
					}
				}
				// soldout
				else
				{
					// set buffer to SOLDOUT
					strcpy(buffer, "SOLDOUT");
					
				}
				
			}
			// check if message is sell
			else if (strcmp(balanceStr2[0], "SELL") == 0)
			{
				// set tempTick to the ticket the client is trying to sell
				int tempTick2 = atoi(balanceStr2[1]);
				
				printf(BLU "[<- Client 2]: %s %d\n" RESET, balanceStr2[0], tempTick2 + 10000);
				
				// set ticket to available in boolArr
				boolArr[tempTick2] = 1;
				
				// set noFunds to false
				noFunds = 0;
				
				// set buffer to ticket sell info
				snprintf(buffer, sizeof(buffer), "%d %d", tempTick2, ranArr[tempTick2]);
				printf(GRN "[-> SERVER  ]: CLIENT 2 SELL %d OK\n" RESET, tempTick2 + 10000);
			}
			
			// send the message to the client
			clientread = write(client2, buffer, strlen(buffer));
		}	
		// end client 2 part
	}
	printf("Both clients disconnected. Preparing to shut down...\n");
	
	// print the final table
	printTable(ranArr, boolArr);
	
	return 0;
}

// dostuff?
void dostuff (int sock) {
	int n;
	char buffer[256];
	  
	bzero(buffer,256);
	n = read(sock,buffer,255);
	if (n < 0) perror("ERROR reading from socket");
		printf("Here is the message: %s\n",buffer);
	n = write(sock,"I got your message",18);
	if (n < 0) 
		perror("ERROR writing to socket");
}


// function to generate the ticket prices and populate the arrays
void createTickets(int *ranArr, int *boolArr)
{
	// variables to hold min and max $ amount for tickets
	int min = 200, max = 400; 
	 
	// seed rand()
	srand(time(NULL));
	
	// generate random number between min and max and put it in the array[i]
	int i; 
	for (i = 0; i < 25; i++) 
	{
            int ranNum = (rand() % (max - min + 1)) + min;
            ranArr[i] = ranNum;
	} 
	
	// set all tickets to available
	for (i = 0; i < 25; i++)
	{
		boolArr[i] = 1;
	}
}

// function to print the table for any given status
void printTable(int *ranArr, int *boolArr) 
{
	printf(GRN "[<> SERVER ]: Database Table:\n" RESET);
	printf("TICKET NUMBER  PRICE  STATUS\n----------------------------\n");
	int i;
	for (i = 0; i < 25; i++)
	{
		// check ticket status
		char *status = malloc(5);
		if (boolArr[i] == 0)
		{
			strcpy(status, "SOLD");
		}
		else
		{
			strcpy(status, "AVAIL");
		}
		
		// print tickets
		int ticketNum = i;
		printf("[Tkt # %d]: $ %d  %s\n", 10000 + ticketNum, ranArr[i], status);
		free(status);
	}
	printf("----------------------------\n");
	
}

// function to check if client has enough funds to purchase ticket
void firstone(int balance, int *ranArr, int *boolArr, int *noFunds, int *tempp)
{
	if (balance >= ranArr[*tempp])
	{
		// enough funds
		*noFunds = 0; 
		boolArr[*tempp] = 0;
	}
	else if (balance < ranArr[*tempp])
	{		
		// not enough funds
		*noFunds = 1;
	}
}

// Bassam did this part
// this function searches through boolArr and returns the
// index of the first ticket that is available
// and returns -1 if none are available
int buy (int *boolArr)
{
	int i;
	for (i = 0; i < 25; i++)
	{
		if (boolArr[i] == 1)
		{
			return i;
		}
	}
	
	// soldout
	return -1;
}
