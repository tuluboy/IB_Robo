// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#ifndef TWSAPIDLL
#ifndef TWSAPIDLLEXP
#define TWSAPIDLLEXP __declspec(dllimport)
#endif
#endif

#define assert ASSERT
#define snprintf _snprintf
#include <WinSock2.h>
#include <Windows.h>

#define sleep( seconds) Sleep( seconds * 1000);
#define IB_WIN32

#include <stdio.h>
#include <tchar.h>
#include <string>
#include <deque>
#include <vector>
#include <algorithm>


// TODO:  在此处引用程序需要的其他头文件
