// stdafx.h : ��׼ϵͳ�����ļ��İ����ļ���
// ���Ǿ���ʹ�õ��������ĵ�
// �ض�����Ŀ�İ����ļ�
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


// TODO:  �ڴ˴����ó�����Ҫ������ͷ�ļ�
