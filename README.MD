Bassam Metwally

Jeremiah Spears

Mithilesh Thakurdesai

Dominic Whiting

Organization:
    - Bassam
        - Program Logic
        - Buy/sell portion in server
        
    - Jeremiah
        - Parsing messages sent between clients/server
        - Buy/sell portion in client and server
        - Error handling/fixing warnings
        - Setup data structures to hold tickets in server
        - Randomly seeded prices for tickets
        - NOFUNDS in server
        - Comments
        - Colored output
        
    - Dominic
        - Full TCP engine for server client
        - Client(client UDP)
        - Client(server UDP)
        - Threads in client
        - Error handling
        - Setup data structures to hold tickets in client
        - Select in server
        - Makefile
        - NOFUNDS in client
        - Comments
        - Scalp in client
        - Buyer in client
        
    Mithilesh
        - Program Logic
        - Scalp in client
        - Client(server UDP)

    
Design Overview
    We set up a TCP connection between the server and two clients. 
The server uses select() to differentiate between the two clients. The
clients both create a thread that creates a SOCKET for UDP connection. This
allows the clients to connect to eachother when they receive a SOLDOUT 
message. The two clients SCALP and then then disconnect. If another SCALP 
occurs the UDP client-server is still open for second SCALP.
    
    Our code has two separate parts the client and the server.  The server 
starts out by printing the tickets with the correct info and then waits for
both clients to connect. The server then accepts buy and sell messages from
the clients and responds accordingly.  
    
    The server stores the ticket info in 2 arrays (boolArr and ranArr.) It 
uses the index of the array to keep track of the ticket number.  To determine
the ticket number to send to the client the server iterates through the 
boolArr to find the first available ticket.  The client keeps the ticket in
the equivalent array and stores it in the same index as the server to keep
track of it.  
    
    Data between the clients/server is parsed using the strtok function which
allows the data to be sent between each entity with one single string.  The
server determines if it is sold out by using the buy function.  This function
will return -1 if it does not find any available tickets.
    
    The clients use a similar array system to keep track of their tickets,
but do not require a boolArr since they can delete the tickets from
their system once they no longer own them.
    
    
Specification
    - Clients wait for message from the server to start sending requests
    - Clients wait 2s after they finish buy requests to exit to make sure they both
    finish 
    
    
Known Bugs or Problems:
    - No known bugs.