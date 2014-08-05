#include "Socket.h"
#include "Time.h"
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <thread>

using namespace std;

#define PORT			19945

void receive(shared_ptr<Socket> server){
	while (server->IsConnected()){
		cout << server->Receive()->ReadToEnd() << endl;
	}

	cout << server->GetIpString() << ":" << server->GetPort() << " disconnected" << endl;
}

void server(){
	Socket server(INADDR_ANY, PORT);
	
	if (!server.Listen()){
		cout << "failed to listen" << endl;
		return;
	}

	while (true){
		auto client = server.Accept();
		if (client == nullptr){
			cout << "failed to accept" << endl;
			continue;
		}

		cout << "accept from " << client->GetIpString() << ":" << client->GetPort() << endl;

		new thread(receive, client);
	}
}

void client(){
	Socket client;
	if (!client.Connect("127.0.0.1", PORT)){
		cout << "failed to connect" << endl;
		return;
	}

	cout << "success, now you can send message to server" << endl;
	string s;
	while (true){
		cin >> s;
		
		if (s == "*")
			break;

		if (!client.Send(s.c_str(), s.length())){
			cout << "failed to send" << endl;
			return;
		}
	}
}

int main(){
	if(!Socket::Startup()){
		cout << "failed to startup socket" << endl;
		return -1;
	}
	
	cout << "1.server or 2.client: " << endl;
	int n = 0;
	while (!(cin >> n) || n<1 || n>2){
		cout << "error input!" << endl;
		cin.clear();
		cin.sync();
	}

	if (n == 1){
		server();
	}
	else{
		client();
	}

	Socket::Cleanup();
	
	return 0;
}
