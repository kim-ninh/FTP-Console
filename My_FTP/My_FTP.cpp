// My_FTP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "My_FTP.h"
#include "FTPConnection.h"

#ifdef _DEBUG
#define _new DEBUG_NEW
#endif

#define MAX_LINE_BUF 1024

// The one and only application object

CWinApp theApp;

using namespace std;

bool isEnoughArg(int nArg, const char *cmd){
	if (!strcmp(cmd, "open") 
		|| !strcmp(cmd,"user")
		|| !strcmp(cmd,"cd")
		|| !strcmp(cmd,"get")
		|| !strcmp(cmd,"put")
		|| !strcmp(cmd,"mkdir")
		|| !strcmp(cmd,"rmdir")
		|| !strcmp(cmd,"delete")
		) {
		if (nArg == 1)
			return false;
		if (nArg == 2)
			return true;
	}
	return false;
}

int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // initialize MFC and print and error on failure
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: change error code to suit your needs
            wprintf(L"Fatal Error: MFC initialization failed\n");
            nRetCode = 1;
        }
		else
		{
			// TODO: code your application's behavior here.
			FTPConnection myFTP;
			queue<CString> msg;
			
			char line[MAX_LINE_BUF];
			char cmd[MAX_LINE_BUF];
			char arg[MAX_LINE_BUF];
			int nArg;
			bool isConnected, isLogin;

			while (TRUE){
				cout << "My ftp> ";
				cin.getline(line, MAX_LINE_BUF);

				if (nArg = sscanf_s(line,"%s %s",cmd, (unsigned)_countof(cmd),arg, (unsigned)_countof(arg))){

					if (!strcmp(cmd, "open")) {
						if (!isEnoughArg(nArg,cmd)){
							cout << "To: ";
							cin.getline(arg, MAX_LINE_BUF);
						}
						isConnected = myFTP.OpenConnection(arg);

						myFTP.GetOutputControlMsg(msg);
						while (!msg.empty())
						{
							cout << msg.front() << '\n';
							msg.pop();
						}

						//if (!isConnected)
						//	continue;
						
						//Login
					}
					else if (!strcmp(cmd, "user")) {
						if (!isEnoughArg(nArg, cmd)) {
							cout << "Username: ";
							cin >> arg;
						}

						
					}
					else if (!strcmp(cmd, "quit")) {
						myFTP.Close();
						myFTP.GetOutputControlMsg(msg);
						while (!msg.empty())
						{
							cout << msg.front();
							msg.pop();
						}
						break;
					}
				}
			}
        }
    }
    else
    {
        // TODO: change error code to suit your needs
        wprintf(L"Fatal Error: GetModuleHandle failed\n");
        nRetCode = 1;
    }

    return nRetCode;
}
