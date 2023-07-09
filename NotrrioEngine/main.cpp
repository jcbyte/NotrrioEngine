#include <iostream>
#include <fstream>
#include <string>

#include <winSock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

int main()
{
	WSADATA wsaData;

	int errorResult;
	errorResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (errorResult != 0)
	{
		std::cout << "WSAStartup failed " << errorResult << std::endl;
		return 1;
	}

	struct addrinfo* result = NULL, * ptr = NULL, hints; // tf does this line of code mean?

	const char* defaultPort = "8000";

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	errorResult = getaddrinfo(NULL, defaultPort, &hints, &result);
	if (errorResult != 0)
	{
		std::cout << "getaddrinfo failed " << errorResult << std::endl;
		WSACleanup();
		return 1;
	}

	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);

	if (ListenSocket == INVALID_SOCKET)
	{
		std::cout << "Error at socket() " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}

	errorResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (errorResult == SOCKET_ERROR)
	{
		std::cout << "bind failed with error " << WSAGetLastError() << std::endl;
		freeaddrinfo(result);
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	freeaddrinfo(result);

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR)
	{
		std::cout << "Listen failed with error " << WSAGetLastError() << std::endl;
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	const char* frontendPath = ".\\frontend\\web";
	while (true)
	{
		SOCKET ClientSocket = INVALID_SOCKET;

		ClientSocket = accept(ListenSocket, NULL, NULL);
		if (ClientSocket == INVALID_SOCKET)
		{
			std::cout << "accept failed " << WSAGetLastError() << std::endl;
			closesocket(ListenSocket);
			WSACleanup();
			return 1;
		}

		char buffer[1024*50];
		int bytesRead = recv(ClientSocket, buffer, sizeof(buffer), 0);

		if (bytesRead > 0)
		{
			std::string request(buffer, bytesRead);

			// Extract requested file path from the request
			size_t start = request.find("GET ") + 4;
			size_t end = request.find(" HTTP/");
			std::string filePath = request.substr(start, end - start);
			if (filePath.empty() || filePath == "/") filePath = "/index.html";

			std::ifstream file(frontendPath + filePath);
			if (!file.is_open())
			{
				std::cout << "Failed to open file: " << frontendPath + filePath << std::endl;
			}

			std::string fileContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
			file.close();

			std::string response;

			if (fileContent.empty())
			{
				response = "HTTP/1.1 404 Not Found\r\n\r\n";
			}
			else
			{
				response = "HTTP/1.1 200 OK\r\n";
				response += "Content-Type: text/html\r\n";
				response += "Content-Length: " + std::to_string(fileContent.length()) + "\r\n";
				response += "\r\n";
				response += fileContent;
			}

			int bytesSent = send(ClientSocket, response.c_str(), response.length(), 0);
			if (bytesSent == SOCKET_ERROR)
			{
				std::cout << "Failed to send message " << WSAGetLastError() << std::endl;
			}
		}

		closesocket(ClientSocket);
	}

	WSACleanup();
	return 0;
}