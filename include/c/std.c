
int createlistensocket(short port)
{
	struct sockaddr_in serverAddr;

	int sock = socket(AF_INET, SOCK_STREAM, 0);

	if(sock < 0)
	{
		return 0;
	}

	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = INADDR_ANY;
	serverAddr.sin_port = htons(port);

	if(bind(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
	{
		return 0;
	}
	return sock;
}

int acceptsocket(int sock)
{
        struct sockaddr_in clientAddr;
        long clientLen = sizeof(clientAddr);
	return accept(sock, &clientAddr, &clientLen); 
}

