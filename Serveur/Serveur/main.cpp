
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

SOCKET Initialize(int portNumber)
{
	int result;

	// Initialize WinSock
	WSADATA WSAData;
	result = WSAStartup(MAKEWORD(2, 2), &WSAData);
	if (result != NO_ERROR)
	{
		return INVALID_SOCKET;
	}

	// Create server Socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		WSACleanup();
		return false;
	}

	// Setup NoBlocking Socket
	u_long mode = 1;
	result = ioctlsocket(sock, FIONBIO, &mode);
	if (result != NO_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Bind socket
	SOCKADDR_IN sin;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(portNumber);

	result = bind(sock, (SOCKADDR*)&sin, sizeof(sin));
	if (result != NO_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return INVALID_SOCKET;
	}

	// Listen
	result = listen(sock, SOMAXCONN);
	if (result != NO_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return INVALID_SOCKET;
	}

	std::cout << "Le serveur a fini de demarer" << endl;
	return sock;
}

void AcceptNewClient(SOCKET sock, vector<Client>& clients)
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
}

bool RecvFromClient(SOCKET sock, Client& client, vector<Client>& clients)
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
			if (clients[j].socket != client.socket) // Transmettre le message a tout le monde sauf a l'expediteur
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
			if (clients[j].socket != client.socket) // Transmettre le message a tout le monde sauf a l'expediteur
			{
				send(clients[j].socket, message.c_str(), message.size(), 0);
			}
		}
		return true;
	}

	return false;
}

void RecvClients(SOCKET sock, vector<Client>& clients)
{
	for (size_t i = 0; i < clients.size(); ++i)
	{
		Client& client = clients[i];

		FD_SET readSet;
		FD_SET writeSet;

		FD_ZERO(&readSet);
		FD_ZERO(&writeSet);
		FD_SET(client.socket, &readSet);
		FD_SET(client.socket, &writeSet);

		if (select(0, &readSet, &writeSet, nullptr, nullptr) == SOCKET_ERROR)
		{
			return;
		}

		if (FD_ISSET(client.socket, &readSet))
		{
			if (RecvFromClient(sock, client, clients) == true)
			{
				clients.erase(clients.begin() + i);
				i--;
			}
		}
	}
}

int main()
{
	SOCKET sock = Initialize(7500); //port
	if (sock == INVALID_SOCKET)
		return 1;

	vector<Client> clients;

	while (true)
	{
		AcceptNewClient(sock, clients);

		RecvClients(sock, clients);
	}

	closesocket(sock);
	WSACleanup();
	return 0;
}
