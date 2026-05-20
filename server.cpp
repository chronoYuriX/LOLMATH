#include <iostream>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
int main() {
    WSADATA wsaData;
    SOCKET serverSocket, clientSocket;
    sockaddr_in serverAddr, clientAddr;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(1145);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, 5);
    
    std::cout << "等待客户端连接..." << std::endl;
    int clientSize = sizeof(clientAddr);
    clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);
    std::cout << "客户端已连接！" << std::endl;
    char buffer[1024] = {0};
    recv(clientSocket, buffer, sizeof(buffer), 0);
    std::cout << "收到消息: " << buffer << std::endl;
    send(clientSocket, "欢迎加入！", strlen("欢迎加入！"), 0);
    ZeroMemory(buffer, 1024);
    
    DWORD ticks = GetTickCount();
    while (1) {
        recv(clientSocket, buffer, sizeof(buffer), 0);
        if (strlen(buffer) > 0) {
        	std::cout << "收到消息: " << buffer << std::endl;
        	ZeroMemory(buffer, 1024);
        	ticks = GetTickCount();
		} else if (ticks + 10000 < GetTickCount()) {
			std::cout << "连接超时" << std::endl;
			break;
		}
    }
   
    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
