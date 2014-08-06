
#include <stdint.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <string>
#include <sstream>
#include <memory>

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
#undef  min
class Stream{
public:
	enum class SeekLocation{
		Begin,
		Current,
		End
	};

protected:
	bool writable = false;
	bool readable = false;
	bool seekable = false;
	int size = 0;
	int position = 0;

	virtual int read(void *buf, int len){
		throw std::logic_error("called unimplemented method read()");
	}

	virtual int write(const void *data, int len){
		throw std::logic_error("called unimplemented method write()");
	}

	virtual int seek(int offset, SeekLocation relativeTo){
		throw std::logic_error("called unimplemented method seek()");
	}

public:
	bool IsWritable(){
		return writable;
	}

	bool IsReadable(){
		return readable;
	}

	bool IsSeekable(){
		return seekable;
	}

	virtual int Peek(void *buf, int len){
		if (!readable)
			throw std::logic_error("Peek() called on a un-readable stream!");

		if (!seekable)
			throw std::logic_error("Peek() called on a un-seekable stream!");

		int read_size = Read(buf, len);
		Seek(-read_size);
		
		return read_size;
	}

	virtual int Seek(int offset, SeekLocation relativeTo = SeekLocation::Current){
		if (!seekable)
			throw std::logic_error("Seek() called on a un-seekable stream!");

		int real_offset = seek(offset, relativeTo);
		position += real_offset;

		return real_offset;
	}

	virtual int Write(const void *data, int len){
		if (!writable)
			throw std::logic_error("Write() called on a un-writable stream!");

		int s = write(data, len);

		if (seekable){
			position += s;
		}
		else{
			size -= s;
		}
		
		return s;
	}

	virtual int Read(void *buf, int len){
		if (!readable)
			throw std::logic_error("Read() called on a un-readable stream!");

		int s = read(buf, len);

		if (seekable){
			position += s;
		}
		else{
			size -= s;
		}
		
		return s;
	}

	std::string ReadToEnd(){
		if (RestSize() < 0)
			throw std::logic_error("ReadToEnd() called on a un-measurable stream!");

		char* buf = new char[RestSize() + 1];
		char* read_buf = buf;

		while (RestSize() > 0)
		{
			read_buf += Read(read_buf, RestSize());
		}

		read_buf[0] = 0;

		std::string s(buf);
		delete[] buf;

		return std::move(s);
	}

	int RestSize() const{
		return size - position;
	}

	int TotalSize() const{
		return size;
	}
};

class MemoryStream : public Stream{
protected:
	char *buf;

public:
	MemoryStream(char* buf, int size){
		this->buf = buf;
		this->size = size;
		readable = true;
		writable = true;
		seekable = true;
	}

	MemoryStream(const char* buf, int size){
		this->buf = const_cast<char*>(buf);
		this->size = size;
		readable = true;
		writable = false;
		seekable = true;
	}

	int read(void *buf, int len) override{
		int read_size = RestSize();
		if (len < read_size)
			read_size = len;
		memcpy(buf, this->buf + position, read_size);
		return read_size;
	}

	int write(const void *data, int len) override{
		int write_size = RestSize();
		if (len < write_size)
			write_size = len;
		memcpy(this->buf + position, buf, write_size);
		return write_size;
	}

	int seek(int offset, SeekLocation relativeTo) override{
		int t = 0;
		switch (relativeTo)
		{
		case Stream::SeekLocation::Begin:
			t = 0;
			break;
		case Stream::SeekLocation::Current:
			t = position;
			break;
		case Stream::SeekLocation::End:
			t = size;
			break;
		}

		t += offset;
		if (t < 0)
			t = 0;
		else if (t > size)
			t = size;

		return t - position;
	}
};

class FileStream : public Stream{
protected:
	FILE* file;

public:
	FileStream(const std::string& file){
		this->file = fopen(file.c_str(), "rb+");

#ifdef _WIN32
#	define stat _stat
#endif
		struct stat st;
		if (stat(file.c_str(), &st))
			size = 0;
		else
			size = st.st_size;

		readable = true;
		writable = true;
		seekable = true;
	}

	int read(void *buf, int len) override{
		return fread(buf, 1, len, file);
	}

	int write(const void *data, int len) override{
		return fwrite(data, 1, len, file);
	}

	int seek(int offset, SeekLocation relativeTo) override{
		int t = 0;
		int s = SEEK_CUR;

		switch (relativeTo)
		{
		case Stream::SeekLocation::Begin:
			t = 0;
			s = SEEK_SET;
			break;
		case Stream::SeekLocation::Current:
			t = position;
			s = SEEK_CUR;
			break;
		case Stream::SeekLocation::End:
			t = size;
			s = SEEK_END;
			break;
		}

		t += offset;
		if (t < 0)
			t = 0;
		else if (t > size)
			t = size;

		if (fseek(file, offset, s) != 0)
			t = position;

		return t - position;
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
	bool closed = false;
	
	Socket(SOCKET _socket, struct sockaddr_in addr){
		this->addr = addr;
		this->m_socket = _socket;
		bind(m_socket, (struct sockaddr *)&addr, sizeof(addr));
	}
	
	bool has_error()
	{
#ifdef _WIN32 
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

	bool send(const char *data, int length){
		while (length > 0){
			int r = ::send(m_socket, data, length, 0);

			if (r < 0){
				return false;
			}

			length -= r;
		}

		return true;
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
	
	~Socket(){
		Close();
	}

	/**
	* get the ip address
	* @return		ip address
	*/
	uint32_t GetIp(){
		return addr.sin_addr.s_addr;
	}

	/**
	* get the ip address in string
	* @return		ip address in string
	*/
	std::string GetIpString(){
		uint32_t ip = addr.sin_addr.s_addr;
		std::stringstream s;
		s << (ip & 0xff) << "." << ((ip >> 8) & 0xff) << "." << ((ip >> 16) & 0xff) << "." << ((ip >> 24) & 0xff);
		return s.str();
	}

	/**
	 * get the port number
	 * @return		port number
	 */
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

	/**
	 * start to listen for connection.
	 * @param		backlog
	 * @return		whether the operation succeed or not.
	 */
	bool Listen(int backlog = 5){
		return listen(m_socket, backlog) == 0;
	}

	/**
	 * waiting for a connection and return it.
	 * @return		a wrap for the connection
	 */
	std::shared_ptr<Socket> Accept(){
		struct sockaddr_in clientAddr;
		socklen_t length = sizeof(clientAddr);
		SOCKET s = accept(m_socket, (struct sockaddr*)&clientAddr, &length);

		if (s == INVALID_SOCKET){
			return nullptr;
		}

		return std::shared_ptr<Socket>(new Socket(s, clientAddr));
	}

	/**
	 * waiting for some message from current connection
	 * @return		a stream which contains the data we received, 
	 *				you must read all for it before calling Receive()
	 *				again.
	 */
	std::shared_ptr<SocketReadStream> Receive(){
		static int len;
		int r = recv(m_socket, (char*)&len, 4, 0);
		if (r <= 0)
			return nullptr;
		
		return std::shared_ptr<SocketReadStream>(new SocketReadStream(this, len));
	}

	/**
	 * send some data to remote end.
	 * @param data		pointer to data
	 * @param length	length of data to send
	 * @return			whether the operation succeed or not.
	 */
	bool Send(const char *data, int length){
		return send((const char*)&length, 4) && send(data, length);
	}

	/**
	 * trying to start connection with remote end.
	 * @param ip		remote ip address
	 * @param port		remote port number
	 * @return			whether the operation succeed or not.
	 */
	bool Connect(uint32_t ip, int port){
		struct sockaddr_in serverAddr;
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(port);
		serverAddr.sin_addr.s_addr = ip;
		return connect(m_socket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == 0;
	}

	/**
	* trying to start connection with remote end.
	* @param ip		remote ip address
	* @param port		remote port number
	* @return			whether the operation succeed or not.
	*/
	bool Connect(std::string ip, int port){
		return Connect(inet_addr(ip.c_str()), port);
	}

	/**
	 * check if connection is available
	 * @return			available or not
	 */
	bool IsConnected(){
		if (m_socket == INVALID_SOCKET || closed) {
			return false;
		}

		char buf[1];
		int ret = recv(m_socket, buf, 1, MSG_PEEK);

		if (ret == 0 || (ret < 0 && has_error())) {
			Close();
			return false;
		}

		return true;
	}

	/**
	 * close current connection. you don' t need to call this
	 * method since it will called while destructing.
	 */
	void Close(){
		if (closed)
			return;

#ifdef _WIN32
		closesocket(m_socket);
#else
		close(m_socket);
#endif

		closed = true;
	}
};

int SocketReadStream::read(void *buf, int len){
	return recv(s->m_socket, (char *)buf, len, 0);
}
