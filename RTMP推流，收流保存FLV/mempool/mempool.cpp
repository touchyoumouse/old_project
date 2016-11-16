// mempool.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "BufPool.h"
#include <stdio.h>

int _tmain(int argc, _TCHAR* argv[])
{
	CBufPool bufpool(1024*1024,1000);
	char* save_name = bufpool.NewBuf();
	strcpy(save_name, "asdfasdfasdfasdf");
	bufpool.FreeBuf(save_name);
	return 0;
}

