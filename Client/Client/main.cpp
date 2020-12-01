
#include <iostream>
#include <vector>

#include <winsock2.h>
#include <string>

#include <Windows.h>
#include <conio.h>

using namespace std;

#pragma comment(lib,"Ws2_32.lib")

int main()
{
	cout << "Entrez votre nom d'utilisateur: ";

	std::string name;
	cin >> name;

	cout << "Entrez l'adresse Ip du Serveur : ";
	std::string ip;
	cin >> ip;

	if (ip.empty() == true)
	{
		ip = "127.0.0.1";
	}

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
	sin.sin_addr.s_addr = inet_addr(ip.c_str());
	sin.sin_family = AF_INET;
	sin.sin_port = htons(7500);

	//result = bind(sock, (SOCKADDR*)&sin, sizeof(sin));
	/*
	if (result != NO_ERROR)
	{
		result = WSAGetLastError();

		closesocket(sock);
		WSACleanup();
		return 1;
	}
	*/

	int connectionResult = connect(sock, (SOCKADDR*)&sin, sizeof(sin));

	Sleep(250);

	send(sock, name.c_str(), (int)name.size() + 1, 0);
	cout << name << ": ";

	int sendIndex = 0;
	char sendBuffer[4096];

	while (true)
	{
		if (_kbhit() != 0)
		{
			char c = _getch();

			if (c == '\b')
			{
				if (sendIndex > 0)
				{
					std::cout << "\b \b";
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

			if (c == '\r' || c == '\n')
			{
				sendBuffer[sendIndex] = '\0';
				send(sock, sendBuffer, (int)strlen(sendBuffer) + 1, 0);
				sendIndex = 0;

				putchar('\n');
				cout << name << ": ";
			}
		}

		FD_SET writeSet;
		FD_SET readSet;

		FD_ZERO(&writeSet);
		FD_ZERO(&readSet);

		FD_SET(sock, &readSet);
		FD_SET(sock, &writeSet);

		if (select(0, &readSet, &writeSet, nullptr, nullptr) == SOCKET_ERROR)
		{
			return 1;
		}

		if (FD_ISSET(sock, &readSet))
		{
			char buffer[4096];
			int recvLen = recv(sock, buffer, sizeof(buffer), 0);
			if (recvLen > 0)
			{
				for (size_t i = 0; i < sendIndex + name.size() + 2; ++i)
				{
					std::cout << "\b \b";
				}

				if (recvLen > 4096 - 1)
				{
					recvLen = 4096 - 1;
				}
				buffer[recvLen] = '\0';

				std::cout << buffer << endl;

				sendBuffer[sendIndex] = '\0';
				std::cout << name << ": " << sendBuffer;
			}
		}
	}

	closesocket(sock);
	return 0;
}
