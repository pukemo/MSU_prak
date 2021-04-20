#include "HttpHeader.h"
#include "HttpResponse-Request-Code.h"
#include <iostream>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <io.h>

using namespace std;

enum {
	REQBUF = 2048,

};

class Socket {
	int fd;
	int port;
	char req[REQBUF];
public:
	void SetPort(int port) {
		this->port = port;
	}
	void SetFd(int fd) {
		dup2(fd, this->fd);
	}
	void SetReq(char* req) {
		for (int i = 0; i < REQBUF; i++) {
			this->req[i] = req[i];
		}
	}

	int GetPort() {
		return port;
	}
	int GetFd() {
		return fd;
	}
	char* GetReq() {
		char res[REQBUF];
		for (int i = 0; i < REQBUF; i++) {
			res[i] = req[i];
		}
		return res;
	}

	void CloseFd() {
		close(fd);
	}

	~Socket() {
		close(fd);
	}
};

class ConnectedSocket :Socket {

};

class ServerSocket :Socket {
	Socket(int port) {
		this->SetPort(port);
		int Server_fd;
		Server_fd = socket(AF_INET, SOCK_STREAM, 0);
		if (Server_fd < 0) {
			//печатаем "не получилось создать сокет"
			exit(1);//503  500
		}

		struct sockaddr_in ServerAddress;
		memset(&ServerAddress, 0, sizeof(ServerAddress));
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = htonl(INADDR_ANY);

		if (bind(Server_fd, (struct sockaddr*)&ServerAddress, sizeof(ServerAddress)) < 0) {
			//cerr << "cant bind socket" << endl;
			close(Server_fd);
			exit(1);//500
		}

		if (listen(Server_fd, BACKLOG) < 0) {
			//cerr << "cant listen" << endl;
			exit(1);//500
		}
		this->SetFd(Server_fd);
	}

	void Run() {
		for (;;) {
			//try
			struct sockaddr_in ClientAddress;
			size_t ClAddrLen = sizeof(ClientAddress);
			int Client_fd = accept(Server_fd, (struct sockaddr*)&ClientAddress, (socklen_t*)&ClAddrLen);
			if (Client_fd < 0) {
				//cerr << "Client error" << endl;
				close(Server_fd);
				this->CloseFd();
				exit(1);//503  500
				//fill Code
				//HttpResponse(500/503, NULL);
			}

			char request[REQBUF];
			int req_flag = recv(Client_fd, request, REQBUF, 0);
			if (req_flag < 0) {
				//cerr << "Server error" << endl;
				close(Client_fd);
				close(Server_fd);
				this->CloseFd();
				exit(1);//500
			}
			HttpRequest req(request);
			HttpResponse resp(0, req);
			//cout << resp
		}
	}

	
};

class ClientSocket :ServerSocket {

};