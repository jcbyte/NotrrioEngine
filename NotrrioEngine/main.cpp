#include "Notrrio/Server.h"

int main()
{
	nottrio::Server server("8000", ".\\frontend\\web");

	server.Run();

	while (true)
	{
		nottrio::RequestFlag requestFlag = server.ClientRecieved();

		if (requestFlag == nottrio::FILE_REQUEST)
		{
			server.ServeFile();
		}
		else if (requestFlag == nottrio::API_REQUEST)
		{
			std::cout << "Api request" << std::endl;

			if (server.GetRequestFilePath() == "/api/Foo")
			{
				std::cout << server.GetRequestFilePath() << std::endl;

				std::string data = "{hello:\"world\"}";

				server.Respond("200 OK", "text/json", data);
			}
			else
			{
				server.Respond("404 Not Found", "", "");
			}
		}

		server.CloseConnectionToClient();
	}
}