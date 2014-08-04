
#include <stdint.h>

#include <string>
#include <sstream>

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

class Stream{
public:
	enum class PeekLocation{
		Begin,
		Current,
		End
	};

protected:
	bool writable, readable, peekable;
	int size;

	virtual int read(void *buf, int len){
		throw std::exception("called unimplemented method read()");
	}

	virtual bool write(void *data, int len){
		throw std::exception("called unimplemented method write()");
	}

	virtual void peek(int offset, PeekLocation relativeTo){
		throw std::exception("called unimplemented method peek()");
	}

public:
	bool IsWritable(){
		return writable;
	}

	bool IsReadable(){
		return readable;
	}

	bool IsPeekable(){
		return peekable;
	}

	virtual void Peek(int offset, PeekLocation relativeTo){
		if (!peekable)
			throw std::exception("Peek() called on a un-peekable stream!");

		peek(offset, relativeTo);
	}

	virtual bool Write(void *data, int len){
		if (!writable)
			throw std::exception("Write() called on a un-writable stream!");

		return write(data, len);
	}

	int Read(void *buf, int len){
		if (!readable)
			throw std::exception("Read() called on a un-readable stream!");

		int s = read(buf, len);
		this->size -= s;
		if (this->size < 0) this->size = 0;
		return s;
	}

	std::string ReadToEnd(){
		if (size<0)
			throw std::exception("ReadToEnd() called on a un-measurable stream!");

		char* buf = new char[size+1];
		int rest = size;
		while (rest>0)
		{
			int r = Read(buf + (size - rest), size);
			rest -= r;
		}
		
		buf[size] = 0;

		std::string s(buf);
		delete[] buf;

		return std::move(s);
	}

	int Size() const{
		return size;
	}
};

class Socket;

class SocketReadStream : public Stream{
protected:
	Socket* s;

	int read(void *buf, int len) override;

public:
	SocketReadStream(Socket* s, int size){
		this->s = s;
		this->size = size;
		readable = true;
	}
};

class Socket{
protected:
	friend class SocketReadStream;

	struct sockaddr_in addr;
	SOCKET m_socket;
	
	Socket(SOCKET _socket, struct sockaddr_in addr){
		this->addr = addr;
		this->m_socket = _socket;
		bind(m_socket, (struct sockaddr *)&addr, sizeof(addr));
	}
	
public:
	Socket(std::string ip, int port) : Socket(inet_addr(ip.c_str()), port){
		
	}
	
	Socket(uint32_t ip, int port) : Socket(){
		addr.sin_family = AF_INET;
		addr.sin_port = htons(port);
		addr.sin_addr.s_addr = ip;
		bind(m_socket, (struct sockaddr *)&addr, sizeof(addr));
	}

	Socket(){
		m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	}
	
	uint32_t GetIp(){
		return addr.sin_addr.s_addr;
	}

	std::string GetIpString(){
		uint32_t ip = addr.sin_addr.s_addr;
		std::stringstream s;
		s << (ip & 0xff) << "." << ((ip >> 8) & 0xff) << "." << ((ip >> 16) & 0xff) << "." << ((ip >> 24) & 0xff);
		return s.str();
	}

	int GetPort(){
		return addr.sin_port;
	}

	/**
	 * initialization for socket library on windows. do nothing on other systems.
	 * @param version	which version of socket is going to use on windows.
	 * @return			whether the operation succeed or not.
	 */
	static bool Startup(int16_t version = 0x202){
#ifdef _WIN32
		WSADATA data;
		int err = WSAStartup(version, &data);
		if(err || data.wVersion != version){
			return false;
		}
#endif
		
		return true;
	}
	
	/**
	 * terminal socket library when socket is no longer to use.
	 * @return		whether the operation succeed or not.
	 */
	static bool Cleanup(){
#ifdef _WIN32
		return WSACleanup() == 0;
#else
		return true;
#endif
	}

	bool Listen(int backlog = 5){
		return listen(m_socket, backlog) == 0;
	}

	Socket *Accept(){
		struct sockaddr_in clientAddr;
		socklen_t length = sizeof(clientAddr);
		SOCKET s = accept(m_socket, (struct sockaddr*)&clientAddr, &length);

		if (s == INVALID_SOCKET){
			return nullptr;
		}

		return new Socket(s, clientAddr);
	}

	SocketReadStream* Receive(){
		static int len;
		int r = recv(m_socket, (char*)&len, 4, 0);
		if (r <= 0)
			return nullptr;
		
		return new SocketReadStream(this, len);
		//return recv(m_socket, buffer, length, 0);
	}

	bool Send(const char *data, int length){
		return send(m_socket, (const char*)&length, 4, 0) == 0
			&& send(m_socket, data, length, 0) == 0;
	}

	bool Connet(uint32_t ip, int port){
		struct sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(port);
		serverAddr.sin_addr.s_addr = ip;
		return connect(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == 0;
	}

	bool Connet(std::string ip, int port){
		return Connet(inet_addr(ip.c_str()), port);
	}

	void Close(){
#ifdef _WIN32
		closesocket(m_socket);
#else
		close(m_socket);
#endif
	}
};

int SocketReadStream::read(void *buf, int len){
	return recv(s->m_socket, (char *)buf, len, 0);
}