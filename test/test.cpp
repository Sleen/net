#include "Socket.h"
#include "Time.h"

#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <thread>

#include <assert.h>

#ifdef _WIN32
#	define SHOW_MESSAGE(msg) MessageBoxA(NULL, msg, "Assertion Failed", 0)
#else
#	define SHOW_MESSAGE(msg) printf(msg)
#endif

#define ASSERTION_MESSAGE(exp, msg) "Assertion Failed!\n\tFile: \t" __FILE__ "\n\tExpression: \t" #exp "\n\tMessage: \t" msg

#ifdef NO_ASSERT
#	define ASSERT(exp, msg) exp;
#else
#	ifdef _MSC_VER
#		define ASSERT(exp, msg) if(exp){}else{SHOW_MESSAGE(ASSERTION_MESSAGE(exp, msg)); __debugbreak();}
#	else
#		define ASSERT(exp, msg) if(exp){}else{SHOW_MESSAGE(ASSERTION_MESSAGE(exp, msg)); assert(exp);}
#	endif
#endif

using namespace std;

#define PORT			19945

//void receive(shared_ptr<Socket> server){
//	while (server->IsConnected()){
//		cout << server->Receive()->ReadToEnd() << endl;
//	}
//
//	cout << server->GetIpString() << ":" << server->GetPort() << " disconnected" << endl;
//}
//
//void server(){
//	Socket server(INADDR_ANY, PORT);
//	
//	if (!server.Listen()){
//		cout << "failed to listen" << endl;
//		return;
//	}
//
//	while (true){
//		auto client = server.Accept();
//		if (client == nullptr){
//			cout << "failed to accept" << endl;
//			continue;
//		}
//
//		cout << "accept from " << client->GetIpString() << ":" << client->GetPort() << endl;
//
//		new thread(receive, client);
//	}
//}
//
//void client(){
//	Socket client;
//	if (!client.Connect("127.0.0.1", PORT)){
//		cout << "failed to connect" << endl;
//		return;
//	}
//
//	cout << "success, now you can send message to server" << endl;
//	string s;
//	while (true){
//		cin >> s;
//		
//		if (s == "*")
//			break;
//
//		if (!client.Send(s.c_str(), s.length())){
//			cout << "failed to send" << endl;
//			return;
//		}
//	}
//}

int main(){
	char a[] = "1234";
	MemoryStream ms(a, 4);
	char b[4];
	assert(ms.ReadToEnd() == "1234");
	assert(ms.Seek(1) == 0);
	assert(ms.Seek(-4) == -4);
	assert(ms.Seek(1) == 1);
	assert(ms.Seek(1, Stream::SeekLocation::Begin) == 0);
	assert(ms.Write("1234", 4) == 3);
	assert(ms.Seek(1, Stream::SeekLocation::Begin) == -3);
	assert(ms.Peek(b, 4) == 3);
	assert(ms.Read(b, 2) == 2);
	assert(ms.ReadToEnd() == "3");

	FileStream fs("E:\\Git\\SE\\src\\core\\Macro.h");
	cout << fs.ReadToEnd() << endl;

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
/*
	if (n == 1){
		server();
	}
	else{
		client();
	}
*/
	Socket::Cleanup();
	
	return 0;
}
