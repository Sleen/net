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

#ifdef __MINGW32__		// std::thread does not supported well on mingw32 on my computer
		receive(client);
#else
		new thread(receive, client);
#endif
	}
}

void client(){
	Socket client;
	if (!client.Connect("127.0.0.1", PORT)){
		cout << "failed to connect" << endl;
		return;
	}

	FileStream fs("E:\\Git\\SE\\src\\core\\Macro.h");
	client.Send(fs.ReadToEnd());

	cout << "success, now you can send message to server" << endl;
	string s;
	while (true){
		cin >> s;
		
		if (s == "*")
			break;

		if (!client.Send(s)){
			cout << "failed to send" << endl;
			return;
		}
	}
}

bool exists_file(const char* file){
	FILE* f = fopen(file, "r");
	if (f) fclose(f);;
	return f != NULL;
}

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
	ms = MemoryStream("1234", 4);
	assert(!ms.IsWritable());

	const char* path = "E:/test.txt";

	if (exists_file(path))
		remove(path);

	// readable, seekable
	FILE* file = fopen(path, "rb");
	assert(file == nullptr);

	// writable, seekable
	file = fopen(path, "wb");
	assert(file != nullptr);
	assert(fwrite("test", 1, 4, file) == 4);
	assert(fseek(file, -1, SEEK_CUR) == 0);
	assert(fread(a, 1, 1, file) == 0);
	assert(fclose(file) == 0);
	assert(FileStream(path).ReadToEnd() == "test");

	file = fopen(path, "wb");
	assert(file != nullptr);
	assert(fwrite("hehehe", 1, 6, file) == 6);
	assert(fclose(file) == 0);
	assert(FileStream(path).ReadToEnd() == "hehehe");

	// writable
	file = fopen(path, "ab");
	assert(file != nullptr);
	assert(fseek(file, -2, SEEK_CUR) == -1);
	assert(fwrite("test", 1, 4, file) == 4);
	assert(fclose(file) == 0);
	assert(FileStream(path).ReadToEnd() == "hehehetest");
	
	// readable, writable, seekable
	file = fopen(path, "rb+");
	assert(file != nullptr);
	assert(fwrite("test", 1, 4, file) == 4);
	assert(fclose(file) == 0);
	assert(FileStream(path).ReadToEnd() == "testhetest");

	// readable, writable, seekable  (clear)
	file = fopen(path, "wb+");
	assert(file != nullptr);
	assert(fwrite("test", 1, 4, file) == 4);
	assert(fclose(file) == 0);
	assert(FileStream(path).ReadToEnd() == "test");

	// readable, writable
	file = fopen(path, "ab+");
	assert(file != nullptr);
	assert(fseek(file, -2, SEEK_CUR) == -1);
	assert(fread(a, 1, 4, file) == 4);
	//assert(fwrite("hehe", 1, 4, file) == 4);				// ???
	assert(MemoryStream(a, 4).ReadToEnd() == "test");
	assert(fclose(file) == 0);
	//assert(FileStream(path).ReadToEnd() == "testhehe");	// ???

	file = fopen(path, "wb");
	assert(fclose(file) == 0);
	assert(FileStream(path).ReadToEnd() == "");

	assert(remove(path) == 0);

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
