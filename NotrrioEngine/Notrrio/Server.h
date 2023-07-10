#pragma once

#include <iostream>
#include <string>
#include <fstream>

#include <winSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

namespace nottrio
{
	enum RequestFlag
	{
		NO_REQUEST, FILE_REQUEST, API_REQUEST
	};

	class Server
	{
	public:
		Server(const char* port, const char* frontendPath);
		~Server();

		void Run();
		RequestFlag ClientRecieved();

		void Respond(std::string status, std::string contentType, std::string data);
		void ServeFile();
		void CloseConnectionToClient();

		std::string GetRequest();
		std::string GetRequestFilePath();

	private:
		WSADATA wsaData;

		SOCKET listenSocket;
		SOCKET clientSocket;

		const char* frontendPath;
		std::string request;
		std::string filePath;
	};
}