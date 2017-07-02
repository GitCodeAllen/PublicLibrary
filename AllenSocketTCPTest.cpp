#include "sockets.hpp"
#include<iostream>
#include<string>
#include<vector>

// neccessary static libraries for windows
#ifdef WIN32
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"libevent.lib")
#endif

const unsigned short port = 1010;

void Server()
{
	AllenSocketTCPServer a(port);

	a.AddCallbackConnected([](const char* ip, unsigned short port) {
		std::cout << "connected from " << ip <<
			" and port " << port << std::endl;
	});

	a.AddCallbackReceived([](std::vector<char> *data) {
		std::cout << "recv: ";
		for (auto i : *data)
			std::cout << i;
		std::cout << std::endl;
	});

	std::string str;
	while (true)
	{
		std::cin >> str;
		if (str == "exit") break;
		a.Send(str.c_str(), str.size());
	}
}

void Client()
{
	AllenSocketTCPClient a;

	a.AddCallbackReceived([](std::vector<char> *data) {
		std::cout << "recv: ";
		for (auto i : *data)
			std::cout << i;
		std::cout << std::endl;
	});

	a.Connect("127.0.0.1", port);

	std::string str;
	while (true)
	{
		std::cin >> str;
		if (str == "exit") break;
		a.Send(str.c_str(), str.size());
	}
}

int main()
{
	Server();
	//Client();

	return 0;
}