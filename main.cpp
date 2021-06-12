#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>

#pragma comment (lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#include <zlib.h>

void send_data(const unsigned char* data, size_t len, SOCKET s) {
    gzFile temp_data = gzopen("packet.gz", "wb");
    gzwrite(temp_data, data, len);
    gzclose(temp_data);

    FILE* temp_file = NULL;
    fopen_s(&temp_file, "packet.gz", "rb");

    if (temp_file) {
        size_t compress_len = 0;
        unsigned char buffer[DEFAULT_BUFLEN] = { 0 };
        compress_len = fread(buffer, 1, sizeof(buffer), temp_file);
        fclose(temp_file);

        unsigned char buff_packet[DEFAULT_BUFLEN] = { 0 };
        memcpy(buff_packet + 3, &compress_len, sizeof(compress_len));
        memcpy(buff_packet + 4, buffer, compress_len);

        int iResult = send(s, (const char*)buff_packet, compress_len + 4, 0);
        printf("Bytes Sent: %ld\n", iResult);
    }
}

bool parse_data(unsigned char* buff, int buff_len) {
    unsigned char data[DEFAULT_BUFLEN] = { 0 };

    memcpy(data, buff + 4, buff_len - 4);

    FILE* temp_file = NULL;
    fopen_s(&temp_file, "packet_recv.gz", "wb");

    if (temp_file) {
        fwrite(data, 1, sizeof(data), temp_file);
        fclose(temp_file);
    }

    gzFile temp_data_in = gzopen("packet_recv.gz", "rb");
    unsigned char decompressed_data[DEFAULT_BUFLEN] = { 0 };
    gzread(temp_data_in, decompressed_data, DEFAULT_BUFLEN);
    fwrite(decompressed_data, 1, DEFAULT_BUFLEN, stdout);
    gzclose(temp_data_in);

    return strstr((const char*)decompressed_data, (const char*)"\\wave");
}

int main(void)
{
    WSADATA wsaData;
    int iResult;

    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;
    SOCKET ServerSocket = INVALID_SOCKET;

    struct addrinfo* result = NULL,
        hints;

    int iSendResult;
    unsigned char recvbuf[DEFAULT_BUFLEN];
    int recvbuflen = DEFAULT_BUFLEN;

    DWORD timeout = 1000;

    // Client
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    ClientSocket = accept(ListenSocket, NULL, NULL);
    closesocket(ListenSocket);

    // Server
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    iResult = getaddrinfo("127.0.0.1", "15000", &hints, &result);
    ServerSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    iResult = connect(ServerSocket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);

    setsockopt(ServerSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    do {
        iResult = recv(ClientSocket, (char*)recvbuf, recvbuflen, 0);
        Sleep(100);
        if (iResult > 0) {
            printf("Bytes received: %d\n", iResult);
            if (parse_data(recvbuf, iResult)) {
                const unsigned char message[] = "[message]\nmessage=\"Hello!\"\nroom=\"lobby\"\nsender=\"ChatBot\"\n[/message]";
                send_data(message, sizeof(message), ServerSocket);
                Sleep(100);
            }

            iSendResult = send(ServerSocket, (char*)recvbuf, iResult, 0);
            Sleep(100);
            printf("Bytes sent: %d\n", iSendResult);
            iResult = recv(ServerSocket, (char*)recvbuf, recvbuflen, 0);
            Sleep(100);
            if (iResult != SOCKET_ERROR) {
                iSendResult = send(ClientSocket, (char*)recvbuf, iResult, 0);
                Sleep(100);
            }
        }
        else if (iResult == 0)
            printf("Connection closing...\n");
        else
            printf("recv failed with error: %d\n", WSAGetLastError());

    } while (iResult > 0 || WSAGetLastError() == WSAETIMEDOUT);

    iResult = shutdown(ClientSocket, SD_SEND);

    closesocket(ClientSocket);
    closesocket(ServerSocket);
    WSACleanup();

    return 0;
}
