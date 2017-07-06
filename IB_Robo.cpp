// IB_Robo.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <thread>

const unsigned MAX_ATTEMPTS = 2;
const unsigned SLEEP_TIME = 10;

#include <stdio.h>
#include <stdlib.h>

#include "Client.h"

extern std::thread* pCmdThread;

bool running = false;
int lastDate = 0;
int curDate = 0;
void timefunc()
{
	int test = 0;
	for (;;)
	{
		util::UpdateTime();
		curDate = util::GetCurDate();
		if (lastDate > 0 && lastDate < curDate)
		{
			lastDate = curDate;
			std::cout << "into annother date...\n";
		}
		if (true || util::isNearMarketOpenTime())
		{
			running = true;
		}

		if (false && util::isOutMarketCloseTime())
		{
			running = false;
		}
		sleep(2);
		//if (5 < test)running = true;
		//if (100 < test)running = false;
		//if (105 < test++)test = 0;
	}
}

#include <conio.h>
/* IMPORTANT: always use your paper trading account. The code below will submit orders as part of the demonstration. */
/* IB will not be responsible for accidental executions on your live account. */
/* Any stock or option symbols displayed are for illustrative purposes only and are not intended to portray a recommendation. */
/* Before contacting our API support team please refer to the available documentation. */
int main(int argc, char** argv)
{
#if 0
	std::string a("20170506");
	std::string b("20170801");
	if (a.compare(b) < 0)
	{
		std::cout << a << "<" << b << std::endl;
	}
	else
	{
		std::cout << a << ">=" << b << std::endl;
	}
	_getch();
	return 0;
#endif

	const char* host = argc > 1 ? argv[1] : "";
	unsigned int port = argc > 2 ? atoi(argv[2]) : 0;
	if (port <= 0)
		port = 7497;
	const char* connectOptions = argc > 3 ? argv[3] : "";
	int clientId = 0;

	std::thread timeguard(timefunc);
	sleep(2);

	DWORD dwEc = 0;
	while (true)
	{
		if (!running)
		{
			sleep(0.1);
			continue;
		}

		printf("Start TWS API Test...\n");
		Client client;

		if (connectOptions) {
			client.setConnectOptions(connectOptions);
		}
		//! [connect]
		printf("connectting......\n");
		client.connect(host, port, clientId);
		//! [connect]

		//! [ereader]
		//Unlike the C# and Java clients, there is no need to explicitely create an EReader object nor a thread
		while (running && client.isConnected()) {
			client.processMessages();
		}

		// if (nullptr == (void*)pCmdThread) return 0; // 主动退出
		if (client.isquit())
		{
			sleep(5);
			printf("End TWS and exit!\n");
			return 0;
		}

		if (0 == TerminateThread(pCmdThread->native_handle(), dwEc))
		{
			printf("kill UI thread error!\n");
			continue;
		}
		pCmdThread->join();
		delete pCmdThread;
		pCmdThread = nullptr;

		//! [ereader]
		sleep(2);

		printf("End of TWS API Test\n");
	}
}

