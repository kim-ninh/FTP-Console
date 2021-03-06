// My_FTP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "My_FTP.h"
#include "FTPConnection.h"
#include <map>
#include <vector>
#include <iomanip>

#ifdef _DEBUG
#define _new DEBUG_NEW
#endif

#define MAX_LINE_BUF 1024

// The one and only application object

CWinApp theApp;

using namespace std;

vector<CString> args;

enum CMD {
	m_open,
	m_user,
	m_ls,
	m_dir,
	m_get,
	m_put,
	m_delete,
	m_mkdir,
	m_rmdir,
	m_mget,
	m_mput,
	m_quit,
	m_close,
	m_cd,
	m_pwd,
	m_passive,
	m_lcd,
	m_mdelete,
	m_ascii,
	m_binary,
	m_help
};

class key_cmp
{
private:
	static CString tolower(CString s)
	{
		return s.MakeLower();
	}
public:
	bool operator ()(const CString& a, const CString& b) const
	{
		return tolower(a) < tolower(b);
	}
};

map<CString, CMD, key_cmp> mCMD = {
	{ "open", CMD::m_open },
	{ "user", CMD::m_user },
	{ "ls", CMD::m_ls },
	{ "dir", CMD::m_dir },
	{ "quit", CMD::m_quit },
	{ "get", CMD::m_get },
	{ "mget", CMD::m_mget},
	{ "put", CMD::m_put },
	{ "lcd", CMD::m_lcd },
	{ "delete", CMD::m_delete },
	{ "mdelete", CMD::m_mdelete },
	{ "passive", CMD::m_passive },
	{ "close", CMD::m_close },
	{ "mkdir", CMD::m_mkdir },
	{ "mput", CMD::m_mput },
	{ "ascii", CMD::m_ascii },
	{ "binary", CMD::m_binary },
	{ "cd", CMD::m_cd},
	{ "rmdir", CMD::m_rmdir},
	{ "pwd", CMD::m_pwd},
	{ "!", CMD::m_quit},
	{ "?", CMD::m_help},
	{ "help", CMD::m_help}
};



bool isEnoughArg(int nArg, const char *cmd)
{
	switch (mCMD[cmd])
	{
	case m_pwd:
	case m_passive:
	case m_quit:
	case m_close:
	case m_help:
		return nArg == 0;
	case m_open:
	case m_cd:
	case m_mkdir:
	case m_rmdir:
	case m_delete:
	case m_lcd:
		return nArg == 1;
	case m_user:
	case m_get:
	case m_put:
	case m_ls:
	case m_dir:
		return nArg == 2;
	case m_mput:
	case m_mget:
	case m_mdelete:
		return nArg >= 1;
	default:
		return true;
	}
}

/*
* Tách từng chuỗi con có trong line
* Dấu phân cách giữa các chuỗi con là khoảng trắng
* Nếu chuỗi con có dấu nháy kép bao bọc "Chuoi con"
* -> Tách theo dấu nháy kép
* Đưa vào vector args
*/
void LoadArgIntoVector(char *line)
{
	if (*line == '\0')
		return;

	char *p;
	char delimeter;
	size_t len = strlen(line);
	p = line + 1;
	*p == '\"' ? delimeter = '\"' : delimeter = ' ';
	if (*p == '\"')
		p++;
	while (*p != '\0')
	{
		char *q = p;
		while (*q != delimeter && *q != '\0')
			q++;
		*q = '\0';
		args.push_back(p);
		if (q >= line + len || q + 1 >= line + len || q + 2 >= line + len)
			break;
		delimeter == '\"' ? p = q + 2 : p = q + 1;
		*p == '\"' ? delimeter = '\"' : delimeter = ' ';
		if (*p == '\"')
			p++;
	}
}

///*
//* In các thông báo từ hàng đợi outputControlMsg
//*/
void PrintControlMsg(FTPConnection &ftp, CMD m)
{
	// Các lệnh bình thường thì chỉ cần in toàn bộ hàng đợi outputControlMsg

	if (m != m_ls && m != m_dir && m != m_help)
	{
		while (!ftp.outputControlMsg.empty())
		{
			cout << ftp.outputControlMsg.front();
			ftp.outputControlMsg.pop();
		}
		return;
	}

	if (m == m_help)
	{
		const int N_ROWS = 4;
		const int N_CMDS = ftp.outputMsg.size();
		const int width = max_element(begin(ftp.outputMsg), end(ftp.outputMsg), 
			[](const CString& a, const CString& b) {return a.GetLength() < b.GetLength(); })->GetLength() * 1.61803398875;	// golden ratio
		
		for (int i = 0; i < N_ROWS; ++i)
		{
			for (int j = i; j < N_CMDS; j += N_ROWS)
			{
				cout << left << setw(width) << ftp.outputMsg[j];
			}

			cout << '\n';
		}

		return;
	}

	// Lệnh ls hoặc dir
	// Cú pháp : [remote-directory] [local-file]	->	 args[0] args[1]
	// 
	CString local_file = args[1];

	// mở file xuất (nếu có)
	ofstream file;
	ostream *os;
	if (!local_file.IsEmpty())
	{
		file.open(local_file);
		os = &file;
	}
	else
	{
		os = &cout;
	}

	while (!ftp.outputControlMsg.empty())
	{
		int replyCode;
		cout << ftp.outputControlMsg.front();

		if (sscanf_s(ftp.outputControlMsg.front().GetString(), "%d", &replyCode))
		{
			if (replyCode == 150)			// Khi vừa in xong "150 Opening ..." 
			{								// -> In toàn bộ data nhận được 
				for (auto it : ftp.outputMsg)
					*os << it << endl;
			}
		}
		ftp.outputControlMsg.pop();
	}

	if (file.is_open())
		file.close();
}

/*
* KT có phải là lệnh hợp lệ hay không?
*/
bool isValidCommand(CString cmd)
{
	return mCMD.find(cmd) != mCMD.end();
}

bool isRequireConnected(CString cmd)
{
	switch (mCMD[cmd])
	{
	case m_passive: case m_lcd: case m_open: case m_quit: case m_help:
		return false;
	default:
		return true;
	}
}

/*
* @ KT có đủ 'tham số' cho 'lệnh' hay chưa
* @ Nếu chưa đủ thì tùy vào 'lệnh' gì mà bổ sung cho đủ
*   Hàng đợi args sẽ chứa các tham số
*/
void GetEnoughArg(CString cmd)
{
	char line[MAX_LINE_BUF];

	if (isEnoughArg(args.size(), cmd))
		return;

	switch (mCMD[cmd])
	{
	case m_open:
		cout << "To: ";
		cin.getline(line, MAX_LINE_BUF);
		args.push_back(line);
		break;

	case m_user:
		switch (args.size())
		{
		case 0:
			cout << "User: ";
			cin.getline(line, MAX_LINE_BUF);
			args.push_back(line);
		case 1:
			HANDLE hstdin = GetStdHandle(STD_INPUT_HANDLE);
			DWORD mode = 0;
			GetConsoleMode(hstdin, &mode);
			SetConsoleMode(hstdin, mode & (~ENABLE_ECHO_INPUT));

			cout << "Password: ";
			cin.getline(line, MAX_LINE_BUF); cout << '\n';
			SetConsoleMode(hstdin, mode | ENABLE_ECHO_INPUT);
			args.push_back(line);
		}
		break;

	case m_lcd:
		args.push_back("");
		break;

	case m_ls: case m_dir:
		switch (args.size())
		{
		case 0:
			args.push_back("");
			args.push_back("");
			break;
		case 1:
			args.push_back("");
			break;
		}
		break;

	case m_mkdir:
		cout << "Directory name: ";
		cin.getline(line, MAX_LINE_BUF);
		args.push_back(line);
		break;

	case m_put:
		switch (args.size())
		{
		case 0:
			cout << "Local file: ";
			cin.getline(line, MAX_LINE_BUF);
			args.push_back(line);
			cout << "Remote file: ";
			cin.getline(line, MAX_LINE_BUF);
			args.push_back(line);
			break;
		case 1:
			args.push_back("");
			break;
		}
		break;

	case m_get:
		if (args.size() == 0)
		{
			cout << "Remote file ";
			cin.getline(line, MAX_LINE_BUF);
			args.push_back(line);

			cout << "Local file ";
			cin.getline(line, MAX_LINE_BUF);
			args.push_back(line);
		}
		else if (args.size() == 1)			//  tham số chỉ có Remote-file => Local-file = Remote-file
		{
			if (args[0].Find(_T('/')) == -1)
				args.push_back(args[0]);
			else
				args.push_back(args[0].Right(args[0].GetLength() - args[0].Find(_T('/')) - 1));
		}
		break;

 	case m_mget:
		if (args.empty())
		{
			cout << "Remote file ";
			cin.getline(line + 1, MAX_LINE_BUF);
			line[0] = ' ';
			LoadArgIntoVector(line);
		}
		break;

	case m_cd:
		if (args.empty())
		{
			cout << "Remote directory ";
			cin.getline(line, MAX_LINE_BUF);
			args.push_back(line);
		}
		break;

	case m_delete:
		if (args.empty())
		{
			cout << "Remote file ";
			cin.getline(line, MAX_LINE_BUF);
			args.push_back(line);
		}
		break;
	case m_mdelete:
		if (args.empty())
		{
			cout << "Remote file ";
			cin.getline(line + 1, MAX_LINE_BUF);
			line[0] = ' ';
			LoadArgIntoVector(line);
		}
		break;

	case m_rmdir:
		if (args.empty())
		{
			cout << "Directory name ";
			cin.getline(line, MAX_LINE_BUF);
			args.push_back(line);
		}
		break;

	case m_mput:
		if (args.empty())
		{
			cout << "Local files: ";			
			cin.getline(line + 1, MAX_LINE_BUF);
			line[0] = ' ';
			LoadArgIntoVector(line);				// Có thể nhâp nhiều hơn 1 tham số	
		}
	}
}

int main(int argc, char* argv[])
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
			char line[MAX_LINE_BUF + 1];
			char cmd[MAX_LINE_BUF + 1];
			char *arg;

			BOOL isConnected, isLogin, keepLoop;
			isConnected = FALSE;
			isLogin = FALSE;
			keepLoop = TRUE;
			
			if (argc == 2) {
				args.push_back(argv[1]);
				goto connectToServer;
			}
			
			while (keepLoop) {
				cout << "My ftp> ";											// Nhập 1 'dòng'
				memset(line, 0, sizeof(line));
				cin.getline(line, MAX_LINE_BUF);							// dòng = 'lệnh' + 'các tham số'
			nextCommand:
				if (sscanf_s(line, "%[a-zA-Z?!]", cmd, (unsigned)_countof(cmd)) > 0)		// Thử lấy 'lệnh' từ 1 dòng
				{
					if (!isValidCommand(cmd))								// Nếu 'lệnh' không hợp lệ -> Xuất tb, bỏ qua 1 vòng lặp
					{
						cout << "Invalid command!\n";
						continue;
					}
					if (isRequireConnected(cmd) && !isConnected)
					{
						cout << "Not connected.\n";
						continue;
					}
					arg = line + strlen(cmd);
					args.clear();
					LoadArgIntoVector(arg);									// Đưa 'các tham số' -> hàng đợi args

					GetEnoughArg(cmd);										// Bổ sung cho đủ 'các tham số' phục vụ cho 'lệnh'

					switch (mCMD[cmd])
					{
					case m_open:
					connectToServer:
						if (isConnected)
							continue;
						isConnected = myFTP.OpenConnection(args[0]);		// My ftp> open computer
						PrintControlMsg(myFTP, m_open);
						if (!isConnected)
							continue;
						strcpy_s(line, "user");
						goto nextCommand;

					case m_user:
						isLogin = myFTP.LogIn(args[0], args[1]);			// My ftp> user user-name [password]			
						PrintControlMsg(myFTP, m_user);
						break;

					case m_close:
						isConnected = !myFTP.Close();						// My ftp> close
						PrintControlMsg(myFTP, m_close);
						break;

					case m_quit:
						keepLoop = FALSE;									// My ftp> quit
						if (isConnected)
						{
							strcpy_s(line, "close");
							goto nextCommand;
						}
						break;

					case m_lcd:
						myFTP.LocalChangeDir(args[0]);						// My ftp> lcd [directory]
						PrintControlMsg(myFTP, m_lcd);
						break;

					case m_passive:
						myFTP.SetPassiveMode();								// My ftp> passive
						PrintControlMsg(myFTP, m_passive);
						break;

					case m_ls:
						myFTP.ListAllFile(args[0], args[1]);				// My ftp> ls [remote-directory] [local-file]
						PrintControlMsg(myFTP, m_ls);
						break;

					case m_dir:
						myFTP.ListAllDirectory(args[0], args[1]);			// My ftp> dir [remote-directory] [local-file]
						PrintControlMsg(myFTP, m_dir);
						break;

					case m_mkdir:
						myFTP.CreateDir(args[0]);							// My ftp> mkdir directory
						PrintControlMsg(myFTP, m_mkdir);
						break;

					case m_put:
						myFTP.PutFile(args[0], args[1]);
						PrintControlMsg(myFTP, m_put);
						break;

					case m_get:
						myFTP.GetFile(args[0], args[1]);
						PrintControlMsg(myFTP, m_get);
						break;

					case m_mget:
						myFTP.GetMultipleFiles(args);
						PrintControlMsg(myFTP, m_mget);
						break;

					case m_ascii:
						myFTP.SetMode(FTPConnection::Mode::ASCII);
						PrintControlMsg(myFTP, m_ascii);
						break;

					case m_binary:
						myFTP.SetMode(FTPConnection::Mode::BINARY);
						PrintControlMsg(myFTP, m_binary);
						break;

					case m_cd:
						myFTP.ChangeRemoteWorkingDir(args[0]);
						PrintControlMsg(myFTP, m_cd);
						break;

					case m_delete:
						myFTP.DeleteRemoteFile(args[0]);
						PrintControlMsg(myFTP, m_delete);
						break;

					case m_mdelete:
						myFTP.DeleteRemoteMultipleFiles(args);
						PrintControlMsg(myFTP, m_mdelete);
						break;

					case m_rmdir:
						myFTP.RemoveRemoteDir(args[0]);
						PrintControlMsg(myFTP, m_rmdir);
						break;

					case m_pwd:
						myFTP.PrintRemoteWorkingDir();
						PrintControlMsg(myFTP, m_pwd);
						break;

					case m_mput:
						myFTP.PutMultipleFiles(args);
						PrintControlMsg(myFTP, m_mput);
						break;

					case m_help:
						myFTP.Help();
						PrintControlMsg(myFTP, m_help);
						break;
					default:;
					}
					myFTP.outputMsg.clear();


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
