#define WIN32_LEAN_AND_MEAN
#undef UTF8

#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include <tchar.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define BUFSIZE 512
#define DEFAULT_PORT "51488"
#define EOK

BOOL StopFlag = FALSE;

DWORD WINAPI WriteToPipe(LPDWORD);
DWORD WINAPI ReadFromPipe(LPDWORD);

SOCKET ConnectSocket = INVALID_SOCKET;

DWORD main(int argc, char **argv) {
  WSADATA wsaData;

  struct addrinfo *result = NULL, *ptr = NULL, hints;
  const char *sendbuf = "this is a test";
  DWORD iResult;

  // Validate the parameters
  if (argc != 2) {
    printf("usage: %s <server-address>\n", argv[0]);
    return 1;
  }

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed with error: %d\n", iResult);
    return 1;
  }

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  // Resolve the server address and port
  iResult = getaddrinfo(argv[1], DEFAULT_PORT, &hints, &result);
  if (iResult != 0) {
    printf("getaddrinfo failed with error: %d\n", iResult);
    WSACleanup();
    return 1;
  }

  // Attempt to connect to an address until one succeeds
  for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

    // Create a SOCKET for connecting to server
    ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (ConnectSocket == INVALID_SOCKET) {
      printf("socket failed with error: %ld\n", WSAGetLastError());
      WSACleanup();
      return 1;
    }

    // Connect to server.
    iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
      closesocket(ConnectSocket);
      ConnectSocket = INVALID_SOCKET;
      continue;
    }
    break;
  }

  freeaddrinfo(result);

  if (ConnectSocket == INVALID_SOCKET) {
    printf("Unable to connect to server!\n");
    WSACleanup();
    return 1;
  }

  HANDLE threads[2];
  threads[0] = CreateThread(NULL, 0, WriteToPipe, NULL, 0, NULL);
  threads[1] = CreateThread(NULL, 0, ReadFromPipe, NULL, 0, NULL);

  WaitForMultipleObjects(2, threads, TRUE, INFINITE);

  // shutdown the connection since no more data will be sent
  iResult = shutdown(ConnectSocket, SD_SEND);
  if (iResult == SOCKET_ERROR) {
    printf("shutdown failed with error: %d\n", WSAGetLastError());
    closesocket(ConnectSocket);
    WSACleanup();
    return 1;
  }
  // cleanup
  closesocket(ConnectSocket);
  WSACleanup();

  return EOK;
}

DWORD WINAPI WriteToPipe(LPDWORD dummy) {
  UNREFERENCED_PARAMETER(dummy);
  DWORD iResult, dwRead, bSuccess;

  for (;;) {
    CHAR chBuf[BUFSIZE] = "";
    for (size_t i = 0; i < BUFSIZE; ++i) chBuf[i] = 0;
    bSuccess = ReadFile(GetStdHandle(STD_INPUT_HANDLE), chBuf, BUFSIZE, &dwRead, NULL);
    if (!bSuccess || dwRead == 0) return -1;
    chBuf[dwRead] = '\n';

    iResult = send(ConnectSocket, chBuf, dwRead+1, 0);
    if (iResult == SOCKET_ERROR) {
      printf("Send failed with error: %d\n", WSAGetLastError());
      closesocket(ConnectSocket);
      WSACleanup();
      return -1;
    }
  }
}

DWORD WINAPI ReadFromPipe(LPDWORD dummy) {
  UNREFERENCED_PARAMETER(dummy);
  DWORD iResult, dwRead, bSuccess;

  for (;;) {
    CHAR chBuf[BUFSIZE] = "";
    for (size_t i = 0; i < BUFSIZE; ++i) chBuf[i] = 0;
    chBuf[strlen(chBuf)] = '\n';
    iResult = recv(ConnectSocket, chBuf, BUFSIZE, 0);
    if (iResult > 0) {
        bSuccess = WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), chBuf, strlen(chBuf), &dwRead, NULL);
        if (!bSuccess || dwRead == 0) return -1;
    }
    else return -1;
  }
}