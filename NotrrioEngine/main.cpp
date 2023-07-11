#include "Notrrio/Server.h"
#include "Notrrio/ProcessScanner.h"

#include <map>

std::map<std::string, std::string> GetTokensFromQuery(std::string query) // will very easily break
{
	std::map <std::string, std::string> variables;

	std::vector<std::string> tokens;
	int i = 0;
	tokens.push_back("");
	for (auto c : query)
	{
		if (c == '&')
		{
			i++;
			tokens.push_back("");
			continue;
		}
		tokens[i] += c;
	}

	//token = path.substr(0, path.find("?"));
	for (auto str : tokens)
	{
		variables[str.substr(0, str.find("="))] = str.substr(str.find("=") + 1, str.length() - 1);
	}

	return variables;
}

int main()
{
	nottrio::Server server("8000", ".\\frontend\\web");

	server.Run();

	nottrio::ProcessScanner scanner("46");

	while (true)
	{
		nottrio::RequestFlag requestFlag = server.ClientRecieved();

		if (&scanner == nullptr)
		{
			throw "the fuck?";
		}

		if (requestFlag == nottrio::FILE_REQUEST)
		{
			server.ServeFile();
		}
		else if (requestFlag == nottrio::API_REQUEST)
		{
			std::string path = server.GetRequestFilePath();

			std::string token;
			std::string query = "";
			if (path.find("?") != std::string::npos)
			{
				token = path.substr(0, path.find("?"));
				query = path.substr(path.find("?") + 1, path.length() - 1);
				std::cout << query << std::endl;
			}
			else
			{
				token = path;
			}

			std::map<std::string, std::string> params = GetTokensFromQuery(query);



			if (token == "/api/Foo")
			{
				if (params["task"] == "initial_scan" && params["scantype"] == "exact_value")
				{
					volatile int val = std::stoi(params["value"]);
					scanner.BeginScan(nottrio::Scantype::EXACT_VALUE, 4, (unsigned char*)&val);
				}
				else if (params["task"] == "next_scan" && params["scantype"] == "exact_value")
				{
					volatile int val = std::stoi(params["value"]);
					scanner.NextScan(nottrio::Scantype::EXACT_VALUE, (unsigned char*)&val, nullptr);
				}
				else if (params["task"] == "write")
				{
					volatile int val = std::stoi(params["value"]);
					scanner.WriteToAllMatches((unsigned char*)&val);
				}
			}
			else
			{
				server.Respond("404 Not Found", "", "");
			}
		}

		server.CloseConnectionToClient();
	}
}

