#include "Server.h"

namespace nottrio
{
	Server::Server(const char* port, const char* frontendPath)
		:frontendPath(frontendPath)
	{
		clientSocket = INVALID_SOCKET;

		int errorResult;
		errorResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (errorResult != 0)
		{
			std::cout << "WSAStartup failed " << errorResult << std::endl;
			throw "WSAStartup failed";
		}

		struct addrinfo* result = NULL, * ptr = NULL, hints; // tf does this line of code mean?

		ZeroMemory(&hints, sizeof(hints));
		hints.ai_family = AF_INET;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		hints.ai_flags = AI_PASSIVE;

		errorResult = getaddrinfo(NULL, port, &hints, &result);
		if (errorResult != 0)
		{
			std::cout << "getaddrinfo failed " << errorResult << std::endl;
			WSACleanup();
			throw "getaddrinfo failed";
		}

		listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

		if (listenSocket == INVALID_SOCKET)
		{
			std::cout << "Error at socket() " << WSAGetLastError() << std::endl;
			freeaddrinfo(result);
			WSACleanup();
			throw "error at socket()";
		}

		errorResult = bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
		if (errorResult == SOCKET_ERROR)
		{
			std::cout << "bind failed with error " << WSAGetLastError() << std::endl;
			freeaddrinfo(result);
			closesocket(listenSocket);
			WSACleanup();
			throw "bind failed";
		}

		freeaddrinfo(result);
	}

	Server::~Server()
	{
		closesocket(listenSocket);
		WSACleanup();
	}

	void Server::Run()
	{
		if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		{
			std::cout << "Listen failed with error " << WSAGetLastError() << std::endl;
			closesocket(listenSocket);
			WSACleanup();
			throw "listen failed";
		}
	}

	RequestFlag Server::ClientRecieved()
	{
		clientSocket = INVALID_SOCKET;

		clientSocket = accept(listenSocket, NULL, NULL);
		if (clientSocket == INVALID_SOCKET)
		{
			std::cout << "accept failed " << WSAGetLastError() << std::endl;
			closesocket(listenSocket);
			WSACleanup();
			throw "accept failed";
		}

		char buffer[1024 * 10]; // think can call recv multiple times by checking if bytes read is max size of buffer then calling recv again
		int bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);

		if (bytesRead <= 0) return NO_REQUEST;

		request = std::string(buffer, bytesRead);

		size_t start = request.find("GET ") + 4;
		size_t end = request.find(" HTTP/");
		filePath = request.substr(start, end - start);

		if (filePath.empty() || filePath == "/") filePath = "/index.html";

		if (filePath.find("/api") != std::string::npos)
		{
			return API_REQUEST;
		}
		else
		{
			return FILE_REQUEST;
		}
	}

	void Server::Respond(std::string status, std::string contentType, std::string data)
	{
		std::string response;
		response = "HTTP/1.1 " + status + "\r\n";
		response += "Content-Type: " + contentType + "\r\n";
		response += "Content-Length: " + std::to_string(data.length()) + "\r\n";
		response += "\r\n";
		response += data;

		int bytesSent = send(clientSocket, response.c_str(), response.length(), 0);
		if (bytesSent == SOCKET_ERROR)
		{
			std::cout << "Failed to send message " << WSAGetLastError() << std::endl;
			throw "Failed to send message";
		}
	}

	void Server::ServeFile()
	{
		std::ifstream file(frontendPath + filePath, std::ios::binary);
		if (!file.is_open())
		{
			std::cout << "Failed to open file: " << frontendPath + filePath << std::endl;
		}

		std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
		file.close();

		if (fileContent.empty())
		{
			Respond("404 Not Found", "", "");
		}
		else
		{
			Respond("200 OK", "text/html", fileContent);
		}
	}

	void Server::CloseConnectionToClient()
	{
		closesocket(clientSocket);
	}

	std::string Server::GetRequest()
	{
		return request;
	}

	std::string Server::GetRequestFilePath()
	{
		return filePath;
	}
}