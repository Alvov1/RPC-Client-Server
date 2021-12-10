#pragma comment(lib, "rpcrt4.lib")
#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <stdio.h>
#include "Solution_h.h"
using namespace std;

#define PORT "9000"
const unsigned blockSize = 1048576;

handle_t handle;

int LoginUserRequest(unsigned char* login, unsigned char* password) {
	if (LogonUserA((char*)login, NULL,
		(char*)password, LOGON32_LOGON_INTERACTIVE,
		LOGON32_PROVIDER_WINNT40, &handle) == 0) {
		cout << "    - Failed to establish connection with " << login << endl;
		return 1;
	}
	cout << "    + Connection established with " << login << endl;
	return 0;
}

int gOffset = 0;

void LogoutUserRequest(void) {
	RevertToSelf();
}

unsigned long ServerToHostRequest(const unsigned char* iFilename, unsigned char* buffer, unsigned long bfSize) {

	cout << "    * Sending file to client..." << endl;

	if (ImpersonateLoggedOnUser(handle) == 0) {
		cout << "    - Impersonating failed" << endl;
		return 1;
	}

	unsigned long size = 0;
	FILE* file = fopen((const char*)iFilename, "rb");

	if (file == NULL) {
		cout << "    - Unable to open file" << endl;
		if (errno == EACCES) {
			cout << "    - Access denied!" << endl;
			LogoutUserRequest();
			return 0;
		}
		else {
			cout << "    - Someting went wrong while sending a file. " << endl;
			LogoutUserRequest();
			return 0;
		}
	}

	fseek(file, gOffset, SEEK_SET);
	fread(buffer, sizeof(char), blockSize, file);
	gOffset += blockSize;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	if (size < gOffset) {
		size += blockSize - gOffset;
		gOffset = 0;
	}
	else {
		size = blockSize;
	}
	fclose(file);
	cout << "    + Block of data sent successfuly." << endl;
	LogoutUserRequest();
	return size;
}

FILE* fille;

int HostToServerRequest(const unsigned char* iFilename, unsigned char* buffer, unsigned long bfSize, unsigned long start) {

	cout << "    * Recieving file from client..." << endl;

	if (ImpersonateLoggedOnUser(handle) == 0) {
		cout << "    - Impersonating failed" << endl;
		return 2;
	}
	if (start == 0) {
		fille = fopen((const char*)iFilename, "ab");
	}
	if (fille == NULL) {
		cout << "    - Unable to open file" << endl;
		if (errno == EACCES) {
			cout << "    - Access denied!" << endl;
			LogoutUserRequest();
			return -1;
		}
		else {
			cout << "    - Someting went wrong while receiving a file. " << endl;
			LogoutUserRequest();
			return 1;
		}
	}
	fseek(fille, start, SEEK_SET);
	fwrite(buffer, sizeof(char), bfSize, fille);
	cout << "    + Block of data received successfuly." << endl;
	if (bfSize > 0 && bfSize < blockSize) {
		fclose(fille);
		cout << "    + File copied successfully." << endl;
	}
	LogoutUserRequest();
	return 0;
}

void RemoveFileRequest(const unsigned char* filename) {

	cout << "    * Deletting file '" << filename << "'" << endl;

	if (ImpersonateLoggedOnUser(handle) == 0) {
		cout << "    - Impersonating failed" << endl;
		return;
	}

	if (remove((const char*)filename) == -1) {
		if (errno == EACCES) {
			cout << "    - Access denied!" << endl;
			LogoutUserRequest();
			return;
		}
		else {
			cout << "    - Someting went wrong while deletting a file. " << endl;
			LogoutUserRequest();
			return;
		}

	}
	cout << "    + File deleted succesfully." << endl;
	LogoutUserRequest();
}


int main() {

	RPC_STATUS status = 0;
	status = RpcServerUseProtseqEpA(
		(RPC_CSTR)("ncacn_ip_tcp"),
		RPC_C_PROTSEQ_MAX_REQS_DEFAULT,
		(RPC_CSTR)((const char*)PORT),
		NULL);

	if (status) { exit(status); }

	status = RpcServerRegisterIf2(
		Solution_v1_0_s_ifspec,
		NULL,
		NULL,
		RPC_IF_ALLOW_CALLBACKS_WITH_NO_AUTH,
		RPC_C_LISTEN_MAX_CALLS_DEFAULT,
		(unsigned)-1,
		NULL);

	if (status) { exit(status); }

	cout << "Listening on port: " << PORT << endl;

	status = RpcServerListen(
		1,
		RPC_C_LISTEN_MAX_CALLS_DEFAULT,
		FALSE);

	if (status) { exit(status); }
}

// Memory allocation function for RPC.
void* __RPC_USER midl_user_allocate(size_t size)
{
	return malloc(size);
}

// Memory deallocation function for RPC.
void __RPC_USER midl_user_free(void* p)
{
	free(p);
}
