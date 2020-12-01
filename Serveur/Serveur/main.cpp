
#include <iostream>
#include <vector>

#include <winsock2.h>

using namespace std;

#pragma comment(lib,"Ws2_32.lib")

struct Client
{
	SOCKET socket = INVALID_SOCKET;
	string name;
};

int main()
{
	int result;

	WSADATA WSAData;
	result = WSAStartup(MAKEWORD(2, 2), &WSAData);
	if (result != NO_ERROR)
	{
		return 1;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		WSACleanup();
		return 1;
	}

	u_long mode = 1;
	result = ioctlsocket(sock, FIONBIO, &mode);
	if (result != NO_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	std::cout << mode << std::endl;

	SOCKADDR_IN sin;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(7500);

	result = bind(sock, (SOCKADDR*)&sin, sizeof(sin));
	if (result != NO_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	result = listen(sock, SOMAXCONN);
	if (result != NO_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	std::cout << "Le serveur a fini de demarer" << endl;

	vector<Client> clients;

	int val = 0;
	while (1)
	{
		SOCKADDR csin;
		int sizeof_csin = sizeof(csin);
		SOCKET sockClient = accept(sock, (SOCKADDR*)&csin, &sizeof_csin);
		if (sockClient != INVALID_SOCKET)
		{
			Client client;
			client.name = "";
			client.socket = sockClient;

			clients.push_back(client);
			std::cout << "Un nouveau client s'est connecte" << endl;
		}

		for (size_t i = 0; i < clients.size(); ++i)
		{
			Client& client = clients[i];

			FD_SET writeSet;
			FD_SET readSet;

			FD_ZERO(&writeSet);
			FD_ZERO(&readSet);

			FD_SET(client.socket, &readSet);
			FD_SET(client.socket, &writeSet);

			if (select(0, &readSet, &writeSet, nullptr, nullptr) == SOCKET_ERROR)
			{
				return 1;
			}

			if (FD_ISSET(client.socket, &readSet))
			{
				char buffer[4096];
				int recvLen = recv(client.socket, buffer, sizeof(buffer), 0);
				if (recvLen > 0)
				{
					if (recvLen > 4096 - 1)
					{
						recvLen = 4096 - 1;
					}
					buffer[recvLen] = '\0';

					string message;

					if (client.name == "")
					{
						client.name = buffer;
						message = client.name + " a rejoin le chat !";
					}
					else
					{
						message = client.name + ": " + buffer;	
					}

					cout << message << endl;

					for (size_t j = 0; j < clients.size(); ++j)
					{
						if (j != i) // Transmettre le message a tout le monde sauf a l'expediteur
						{
							send(clients[j].socket, message.c_str(), message.size(), 0);
						}
					}
				}
				else if (recvLen == 0 || recvLen == SOCKET_ERROR)
				{
					if (recvLen == SOCKET_ERROR)
					{
						//std::cout << "Error code : " << WSAGetLastError() << endl;
					}

					string message = client.name + " a quitter le chat";
					cout << message << endl;

					for (size_t j = 0; j < clients.size(); ++j)
					{
						if (j != i) // Transmettre le message a tout le monde sauf a l'expediteur
						{
							send(clients[j].socket, message.c_str(), message.size(), 0);
						}
					}

					clients.erase(clients.begin() + i);
					break;
				}
			}
		}
	}

	closesocket(sock);
	WSACleanup();
	return 0;
}
