#include "Socket.h"
#include "Time.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <thread>

using namespace std;

// Windows
#ifdef _WIN32
#	include <winsock2.h>
#	pragma comment(lib, "ws2_32")
	typedef int socklen_t;

// Linux
#elif defined(__LINUX__) || defined(__linux__)
#	include <unistd.h>
#	include <errno.h> 
#	include <arpa/inet.h> 
#	define SOCKET int
#	define SOCKET_ERROR -1
#	define INVALID_SOCKET -1

// unsupported platform
#else
#	error "platform does not be supported !"
#endif

bool socket_startup(){
#ifdef _WIN32
	WORD version = 0x202;
	WSADATA data;
	int err = WSAStartup(version, &data);
	if(err || data.wVersion != version){
		return false;
	}
#endif
	
	return true;
}

bool socket_cleanup(){
#ifdef _WIN32
	return WSACleanup() == 0;
#else
	return true;
#endif
}

void socket_close(SOCKET s){
#ifdef _WIN32
	closesocket(s);
#else
	close(s);
#endif
}

#define BUFFER_SIZE 	8
#define PORT			19945

string getAddr(unsigned long addr){
	stringstream s;
	s << (addr & 0xff) << "." << ((addr >> 8) & 0xff) << "." << ((addr >> 16) & 0xff) << "." << ((addr >> 24) & 0xff);
	return s.str();
}

int error(SOCKET s, string msg, int code){
	cout << msg << endl;
	if (s != INVALID_SOCKET)
		socket_close(s);
	socket_cleanup();
	return code;
}

bool socket_has_error()
{
#ifdef WIN32 
	int err = WSAGetLastError();
	if (err != WSAEWOULDBLOCK) {
#else 
	int err = errno;
	if (err != EINPROGRESS && err != EAGAIN) {
#endif 
		return true;
	}

	return false;
}

bool socket_connected(SOCKET socket){
	// 检查状态 
	if (socket == INVALID_SOCKET) {
		return false;
	}

	char buf[1];
	int ret = recv(socket, buf, 1, MSG_PEEK);
	if (ret == 0) {
		socket_close(socket);
		return false;
	} else if(ret < 0) {
		if (socket_has_error()) {
			socket_close(socket);
			return false;
		}
		else {    // 阻塞 
			return true;
		}
	}
	else {    // 有数据 
		return true;
	}

	return true;
}

void listen1(Socket* server){

	char buffer[BUFFER_SIZE + 1];

	while (true){
		SocketReadStream *s = server->Receive();
		cout << s->ReadToEnd() << endl;
	}

	server->Close();
}

int server(){
	Socket listener(INADDR_ANY, PORT);
	
	if (!listener.Listen()){
		return error(INVALID_SOCKET, "failed to listen", -4);
	}

	cout << "waiting" << endl;

	while (true){
		Socket* server = listener.Accept();
		if (server == nullptr){
			error(INVALID_SOCKET, "failed to accept", -5);
			continue;
		}

		cout << "accept from " << server->GetIpString() << ":" << server->GetPort() << endl;

		thread *t = new thread(listen1, server);
	}

	listener.Close();

	return 0;
}

int client(){
	Socket client;
	if (!client.Connet("127.0.0.1", PORT)){
		return error(INVALID_SOCKET, "failed to connect", -6);
	}

	cout << "success, now you can send message to server" << endl;
	string s;
	while (true){
		cin >> s;
		if (s == "*")
			break;
		if (!client.Send(s.c_str(), s.length())){
			return error(INVALID_SOCKET, "failed to send", -7);
		}
	}

	client.Close();

	return 0;
}

int main(){
	if(!Socket::Startup()){
		cout << "failed to startup socket" << endl;
		return -1;
	}
	
	cout << "1.server or 2.client: " << endl;
	int n = 0;
	while (!(cin >> n) || n<1 || n>2)
		cout << "error input!" << endl;

	int ret = n == 1 ? server() : client();

	Socket::Cleanup();
	
	return ret;
}
