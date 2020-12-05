
#include <iostream>
#include <vector>
#include <string>

#include <winsock2.h>
#pragma comment(lib,"Ws2_32.lib")

#include <Windows.h>
#include <conio.h>

using namespace std;

SOCKET Initialize()
{
	int result;

	WSADATA WSAData;
	result = WSAStartup(MAKEWORD(2, 2), &WSAData);
	if (result != NO_ERROR)
	{
		return INVALID_SOCKET;
	}

	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock == INVALID_SOCKET)
	{
		WSACleanup();
		return INVALID_SOCKET;
	}

	u_long mode = 1;
	result = ioctlsocket(sock, FIONBIO, &mode);
	if (result != NO_ERROR)
	{
		closesocket(sock);
		WSACleanup();
		return INVALID_SOCKET;
	}

	return sock;
}

bool ConsoleInput(int& sendIndex, char* sendBuffer)
{
	// Check if user pressed a key
	if (_kbhit() != 0)
	{
		char c = _getch();

		// Special case of Backspace
		if (c == '\b')
		{
			if (sendIndex > 0)
			{
				cout << "\b \b";
				--sendIndex;
				sendBuffer[sendIndex] = '\0';
			}
		}
		else
		{
			putchar(c);
			sendBuffer[sendIndex] = c;
			++sendIndex;
		}

		return c == '\r' || c == '\n';
	}

	return false;
}

void RecvFromServer(SOCKET sock, int sendIndex, char* sendBuffer, const string& name)
{
	FD_SET writeSet;
	FD_SET readSet;

	FD_ZERO(&writeSet);
	FD_ZERO(&readSet);

	FD_SET(sock, &writeSet);
	FD_SET(sock, &readSet);

	if (select(0, &readSet, &writeSet, nullptr, nullptr) == SOCKET_ERROR)
	{
		return;
	}

	// Check if server has sent a message
	if (FD_ISSET(sock, &readSet))
	{
		char buffer[4096];
		int recvLen = recv(sock, buffer, sizeof(buffer), 0);
		if (recvLen > 0)
		{
			// Erase current input
			for (size_t i = 0; i < sendIndex + name.size() + 2; ++i)
			{
				cout << "\b \b";
			}

			if (recvLen > 4096 - 1)
			{
				recvLen = 4096 - 1;
			}
			buffer[recvLen] = '\0';

			// Print server message
			cout << buffer << endl;

			// Print current input
			sendBuffer[sendIndex] = '\0';
			cout << name << ": " << sendBuffer;
		}
	}
}

bool CheckConnection(SOCKET sock)
{
	FD_SET writeSet;
	FD_SET exceptSet;

	FD_ZERO(&writeSet);
	FD_ZERO(&exceptSet);

	FD_SET(sock, &writeSet);
	FD_SET(sock, &exceptSet);

	if (select(0, nullptr, &writeSet, &exceptSet, nullptr) != SOCKET_ERROR)
	{
		if (FD_ISSET(sock, &writeSet))
		{
			cout << "Connected !" << endl;
			return true;
		}
		if (FD_ISSET(sock, &exceptSet))
		{
			cout << "Connect failed !" << endl;
			return false;
		}
	}

	return false;
}

bool TryToConnect(SOCKET sock)
{
	int inputIndex = 0;
	char inputBuffer[4096] = { "127.0.0.1" };

	cout << "Server's ip : ";

	// Prefill with default local ip
	cout << "127.0.0.1";
	inputIndex = 9;

	// Get Ip from user
	while (ConsoleInput(inputIndex, inputBuffer) == false);
	cout << endl;

	// Try to connect
	SOCKADDR_IN sin;
	sin.sin_addr.s_addr = inet_addr(inputBuffer);
	sin.sin_family = AF_INET;
	sin.sin_port = htons(7500);

	int connectionResult = connect(sock, (SOCKADDR*)&sin, sizeof(sin));
	if (connectionResult == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK)
	{
		return 1;
	}

	// Return connection status
	return CheckConnection(sock);
}

int main()
{
	SOCKET sock = Initialize();
	if (sock == INVALID_SOCKET)
	{
		return 1;
	}

	while (TryToConnect(sock) == false);

	cout << "Enter Username: ";

	string name;
	cin >> name;

	// Send Username to server to initialize client on server side
	send(sock, name.c_str(), (int)name.size() + 1, 0);
	cout << name << ": ";

	int inputIndex = 0;
	char inputBuffer[4096];

	while (true)
	{
		if (ConsoleInput(inputIndex, inputBuffer) == true)
		{
			// Send message to server
			inputBuffer[inputIndex] = '\0';
			send(sock, inputBuffer, (int)strlen(inputBuffer) + 1, 0);
			inputIndex = 0;

			putchar('\n');
			cout << name << ": ";
		}

		RecvFromServer(sock, inputIndex, inputBuffer, name);
	}

	closesocket(sock);
	return 0;
}
