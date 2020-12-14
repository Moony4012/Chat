
#include <iostream>
#include <vector>

#include <WS2tcpip.h>
#include <winsock2.h>
#pragma comment(lib,"Ws2_32.lib")

using namespace std;

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

	cout << "Server is setup." << endl;
	return sock;
}

void SendToAllClientExceptMe(const string& message, vector<Client>& clients, const Client& me)
{
	for (size_t j = 0; j < clients.size(); ++j)
	{
		if (clients[j].socket != me.socket) // Transmettre le message a tout le monde sauf a l'expediteur
		{
			send(clients[j].socket, message.c_str(), (int)message.size(), 0);
		}
	}
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
		cout << "New client connection... waiting his name..." << endl;
	}
}

void ParseType(const char* buffer, int bufferSize, SOCKET sock, Client& client, vector<Client>& clients)
{
	unsigned char messageType = buffer[0];
	++buffer;
	--bufferSize;

	if (messageType == 0) // SetUsername
	{
		string previousName = client.name;
		client.name.assign(buffer, bufferSize);

		string message;
		if (previousName.empty() == true)
		{
			message = client.name + " join the chat !";
		}
		else
		{
			message = previousName + " renamed as " + client.name;
		}

		cout << message << endl;
		SendToAllClientExceptMe(message, clients, client);
	}
	else if (messageType == 1) // Message
	{
		string message;
		message.assign(buffer, bufferSize);

		message.insert(0, client.name + " : ");

		cout << message << endl;
		SendToAllClientExceptMe(message, clients, client);
	}
	else
	{
		cerr << "Unknown message type !" << endl;
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
		//buffer[recvLen] = '\0';

		ParseType(buffer, recvLen, sock, client, clients);
	}
	else if (recvLen == 0 || recvLen == SOCKET_ERROR)
	{
		string message = client.name + " left.";
		cout << message << endl;

		SendToAllClientExceptMe(message, clients, client);
		return true;
	}

	return false;
}

void RecvClients(SOCKET sock, vector<Client>& clients)
{
	// For each client check his FileDescriptor
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

		// Check if client has sent a message
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

void PrintExternIpAdress()
{
	// Create server Socket
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		return;
	}

	// Try to connect
	SOCKADDR_IN sin;
	sin.sin_addr.s_addr = inet_addr("108.171.202.203");
	sin.sin_family = AF_INET;
	sin.sin_port = htons(80);

	int connectionResult = connect(sock, (SOCKADDR*)&sin, sizeof(sin));
	if (connectionResult == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
	{
		closesocket(sock);
		return;
	}

	const char* httpRequest = "GET / HTTP/1.0\r\nHost: api.ipify.org\r\nUser-Agent: MonSuperServeurDeLaMortQuiTueDeOuf\r\n\r\n";

	if (send(sock, httpRequest, strlen(httpRequest), 0) <= 0)
	{
		closesocket(sock);
		return;
	}

	char buffer[256] = { '\0' };
	int recvLen = recv(sock, buffer, sizeof(buffer), 0);
	if (recvLen > 0)
	{
		buffer[recvLen] = '\0';

		char* bufferOffset = buffer;

		while (bufferOffset[0] != '\r' || bufferOffset[1] != '\n' || bufferOffset[2] != '\r' || bufferOffset[3] != '\n')
			++bufferOffset;

		bufferOffset += 4;

		cout << "External Client connection Ip : " << bufferOffset << endl;
	}

	closesocket(sock);
}

void PrintLocalIpAdress()
{
	char hostname[255];
	gethostname(hostname, 255);

	hostent* host = gethostbyname(hostname);
	char* localIp = inet_ntoa(*(in_addr*)*host->h_addr_list);
	cout << "Local Client connection Ip : " << localIp << endl;
}

int main()
{
	SOCKET sock = Initialize(7500); //port
	if (sock == INVALID_SOCKET)
	{
		return 1;
	}

	PrintExternIpAdress();
	PrintLocalIpAdress();

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
