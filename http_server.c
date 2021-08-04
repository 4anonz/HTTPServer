#include "http_server.h"

#define MAXREQUEST 2015

// A strcuture that stores information about every connected client

struct client_info {
	SOCKET socket;
	struct sockaddr_storage address; 
	socklen_t address_length;
	char address_buffer[100];
	char request[MAXREQUEST+1];
	int received;
	struct client_info *next;
};

// for creating a server socket

SOCKET create_socket(const char *host, const char *port) {

	printf("[*] Configuring local address.....");
	struct addrinfo hints;
	memset(&hints, 0, sizeof(hints));
	
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	
	struct addrinfo *bind_address;
	if(getaddrinfo(host, port, &hints, &bind_address) < 0) {
		fprintf(stderr, "[!] getaddrinfo() failed (%d)\n", GetErrorNo());
		exit(1);
	}
	printf("Done\n");
	
	printf("[*] Creating socket....");
	
	SOCKET server_socket;
	server_socket = socket(bind_address->ai_family, bind_address->ai_socktype,
			bind_address->ai_protocol);
	
	if(!IsValidSocket(server_socket)) {
	
		fprintf(stderr, "[!] socket() failed (%d)\n", GetErrorNo());
		exit(1);
	}
	printf("Done\n");
	printf("[*] Binding socket to hostname and port....");
	if(bind(server_socket, bind_address->ai_addr, bind_address->ai_addrlen)) {
		fprintf(stderr, "[!] bind() failed (%s)\n", GetErrorNo());
		exit(1);
	}
	printf("Done\n");
	freeaddrinfo(bind_address);
	
	printf("[*] Putting server ocket in listening mode...");
	if(listen(server_socket, 10) < 0) {
		printf("[!] listen() failed (%d)\n");
		exit(1);
	}
	printf("Done\n");
	
	return server_socket;
}

// use to search for a specific client, or create a new client

struct client_info *get_client(struct client_info **client_list, SOCKET s) {

	struct client_info *cl = *client_list;
	// Search for the client, if found, return the client struct
	while(cl) {
		if(cl->socket == s)
			break;
		cl = cl->next;
	}
	if(cl) return cl;
	
	// if not found then create a new client and allocate some memory
	struct client_info *n = (struct client_info *) calloc(1, sizeof(struct client_info));
	
	if(!n) {
		printf("[!] Out of memory!\n");
		exit(1);
	}
	
	n->address_length = sizeof(n->address);
	n->next = *client_list;
	*client_list = n;
	
	return n;
}

// use to close and free allocated memory for a specific client

void drope_client(struct client_info **client_list, struct client_info *client) {


	CloseSocket(client->socket);
	
	struct client_info **p = client_list;
	// loop through and free the allocated memory for that client
	while(*p) {
		if(*p == client) {
			*p = client->next;
			free(client);
			return;
		}
		p = &(*p)->next;
	}

	fprintf(stderr, "[!] drop_client not found.\n");
	exit(1);
}

// use for returning the IP of the connected client

const char *get_client_address(struct client_info *client) {
	getnameinfo((struct sockaddr*)&client->address,
		    client->address_length,
		    client->address_buffer,
		    sizeof(client->address_buffer),
		    0, 0,
		    NI_NUMERICHOST);
	return client->address_buffer;
}

// use for checking on clients, if one has data set or a new connection 

fd_set wait_on_clients(struct client_info **client_list, SOCKET server) {


	fd_set reads;
	FD_ZERO(&reads);
	FD_SET(server, &reads);
	
	SOCKET max_socket = server;
	
	struct client_info *cl = *client_list;
	// set all the clients sockets in fd
	
	while(cl) {
		FD_SET(cl->socket, &reads);
		if(cl->socket > max_socket)
			max_socket = cl->socket;
		cl = cl->next;
	}
	
	if(select(max_socket+1, &reads, 0, 0, 0) < 0) {
		fprintf(stderr, "[!] select() failed (%d)\n", GetErrorNo());
		exit(1);
	}

	return reads;
}

// Send 400 (Bad Request)

void send_400(struct client_info **client_list, struct client_info *client) {

	const char *c400 = "HTTP/1.1 400 Bad Request\r\n"
			   "Connection: close\r\n"
			   "Content-Length: 11\r\n"
			   "Content-Type: text/plain\r\n"
			   "\r\nBad Request";
			   
   	send(client->socket, c400, strlen(c400), 0);
   	drope_client(client_list, client);
}


// Send 404 (Not Found)

void send_404(struct client_info **client_list, struct client_info *client) {

	const char *c404 = "HTTP/1.1 404 Not Found\r\n"
			   "Connection: close\r\n"
			   "Content-Length: 9\r\n"
			   "Content-Type: text/plain\r\n"
			   "\r\nNot Found";
			   
   	send(client->socket, c404, strlen(c404), 0);
   	drope_client(client_list, client);
}
// send 405 (Method Not Allowed)
void send_405(struct client_info **client_list, struct client_info *client) {

	const char *c405 = "HTTP/1.1 405 Method Not Allowed\r\n"
			   "Connection: close\r\n"
			   "Content-Length: 18\r\n"
			   "Content-Type: text/plain\r\n"
			   "\r\nMethod Not Allowed";
			   
   	send(client->socket, c405, strlen(c405), 0);
   	drope_client(client_list, client);
}


// use for getting content type header field
const char *get_content_type(const char *path) {

	const char *last_dot = strrchr(path, '.');
	if(last_dot) {
		if(strcmp(last_dot, ".css") == 0) return "text/css";
		if(strcmp(last_dot, ".csv") == 0) return "text/csv";
		if(strcmp(last_dot, ".htm") == 0) return "text/html";
		if(strcmp(last_dot, ".html") == 0) return "text/html";
		if(strcmp(last_dot, ".txt") == 0) return "text/plain";
		if(strcmp(last_dot, ".gif") == 0) return "image/gif";
		if(strcmp(last_dot, ".png") == 0) return "image/png";
		if(strcmp(last_dot, ".jpg") == 0) return "image/jpeg";
		if(strcmp(last_dot, "jpeg") == 0) return "image/jpeg";
		if(strcmp(last_dot, ".ico") == 0) return "image/x-icon";
		if(strcmp(last_dot, ".svg") == 0) return "image/svg+xml";
		if(strcmp(last_dot, ".js") == 0) return "application/javascript";
		if(strcmp(last_dot, ".json") == 0) return "application/json";
		if(strcmp(last_dot, ".pdf") == 0) return "application/pdf";
	}
	
	return "application/octet-stream";
}

// This function does the actual duty of a server
// this function servers the resource

void server_resource(struct client_info **client_list, struct client_info *client, const char *path) {

	printf("[*] Serving resource (%s) for %s....\n", path, get_client_address(client));
	
	if(strcmp(path, "/") == 0) path = "/index.html";
	
	// if request is greater than 100, send 400
	if(strlen(path) > 100) {
		send_400(client_list, client);
		return;
	}
	
	if(strstr(path, "..")) {
		send_404(client_list, client);
		return;
	}
	
	char full_path[130];

	sprintf(full_path, "/home/zayyad/www%s", path); // edit this line the path where your local webpages are
	
	#if defined(_WIN32)
		char *p = full_path;
		while(*p) {
			if(*p == '/') *p = '\\';
			++p;
		}
	
	#endif

	FILE *fp = fopen(full_path, "rb");
	
	if(!fp) {
		send_404(client_list, client);
		return;
	}
	
	fseek(fp, 0L, SEEK_END);
	size_t cl = ftell(fp);
	rewind(fp);
	
	const char *ct = get_content_type(full_path);

#define BSIZE 2024

	char buffer[BSIZE];
	
	sprintf(buffer, "HTTP/1.1 200 OK\r\n");
	send(client->socket, buffer, strlen(buffer), 0);
	
	sprintf(buffer, "Connection: close\r\n");
	send(client->socket, buffer, strlen(buffer), 0);
	
	sprintf(buffer, "Content-Type: %s\r\n", ct);
	send(client->socket, buffer, strlen(buffer), 0);
	
	sprintf(buffer, "Content-Length: %d\r\n", cl);
	send(client->socket, buffer, strlen(buffer), 0);
	
	sprintf(buffer, "\r\n");
	send(client->socket, buffer, strlen(buffer), 0);
	
	int r = fread(buffer, 1, BSIZE, fp);
	while(r) {
		send(client->socket, buffer, r, 0);
		r = fread(buffer, 1, BSIZE, fp);
	}
	
	fclose(fp);
	drope_client(client_list, client);
}

//Now the main program flow

int main() {

// if on windows, we need to initialize winsock first

#if defined(_WIN32)
	system("cls");
	printf("[*] Initializing winsock....\n");
	WSADATA d;
	if(WSAStartup(MAKEWORD(2, 2), &d)) {
		fprintf(stderr, "[!] Failed to initialize winsock\n");
		return 1;
	}
	
#else
	system("clear");
#endif

	printf("[*] Starting server...\n");
	SOCKET server = create_socket(0, "80");
	
	printf("[*] Server started listening on port 80\n");
	printf("[*] Waiting for connections...\n");
	
	struct client_info *client_list = 0;
	
	// Let's enter our infinite while loop
	
	while(1) {
	
		fd_set reads;
		reads = wait_on_clients(&client_list, server);
		
		// let check if any socket is set in fd
		
		if(FD_ISSET(server, &reads)) {
			printf("[*] Got a connection attemp\n");
			//server has gotten a new connection
			struct client_info *client = get_client(&client_list, -1);
			
			client->socket = accept(server, 
				(struct sockaddr*)&(client->address),
				&(client->address_length));
				
			if(!IsValidSocket(client->socket)) {
			
				fprintf(stderr, "[!] accept() failed (%d)\n");
				exit(1);
			}
			printf("[*] New connection establsihed for %s\n", get_client_address(client));
		}
		
		// loop through all the already connected client and check which one has data
		
		struct client_info *client = client_list;
		while(client) {
			struct client_info *next = client->next;
			
			if(FD_ISSET(client->socket, &reads)) {
				if(MAXREQUEST == client->received) {
					send_400(&client_list, client);
					client = next;
					continue;
				}
				
				int r = recv(client->socket, client->request + client->received,
					     MAXREQUEST - client->received, 0);
			     	if (r < 1) {
			     		printf("[*] Unexpected disconnect from %s\n", get_client_address(client));
			     		drope_client(&client_list, client);
			     	} else {
			     		client->received += r;
			     		client->request[client->received] = 0;
			     		
			     		// check if hole request header is received
			     		char *q = strstr(client->request, "\r\n\r\n");
			     		if(q) {
			     			*q = 0;
			     			if(strncmp("GET /", client->request, 5)) {
			     				send_405(&client_list, client);
			     			} else {
			     				char *path = client->request + 4;
			     				char *end_path = strstr(path, " ");
			     				if(!end_path)
			     					send_400(&client_list, client);
		     					else{
		     						
		     						*end_path = 0;
		     						server_resource(&client_list, client, path);
		     					}
			     			}
			     		}
			     	}
			}
			
			client = next;
		
		} //while(client)
	
	}//while(1)

	printf("[*] Closing socket...\n");
	CloseSocket(server);
	
#if defined(_WIN32)
	WSACleanup();
#endif
	printf("[*] Finished\n");
	
	return 0;	

}

