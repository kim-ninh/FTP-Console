﻿#pragma once
#include <afxsock.h>
#include <queue>
#include <cstdio>

#define MAX_BUFFER 1461

using namespace std;

class FTPConnection
{
private:
	CSocket controlSock;									//Socket chính, kết nối tới port 21 trên Server
	CSocket dataSock;										
	CSocket dataTrans;
	CString clientIPAddr;		
	
	bool isPassive;											//Đang ở mode Active hay Passive
	void InitDataSock(bool isPass, CSocket &);
public:
	FTPConnection();
	~FTPConnection();
	BOOL OpenConnection(const char* IPAddr);				//Thiết lập kết nối tới Server
	BOOL LogIn(const char *userName, const char *userPass);	//Truyền cả 2 tham số, đã sửa lại!
															
	BOOL Close();
	BOOL ListAllFile(char* fileExt);

	queue<CString> outputMsg;								
	queue<CString> outputControlMsg;						//Hàng đợi chứa thông điệp nhận được, lỗi,...
};															//trong quá trình gọi hàm


