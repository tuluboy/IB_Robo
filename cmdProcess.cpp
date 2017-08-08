#include "StdAfx.h"
#include <typeinfo.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include "Process.h"
#include "Client.h"
#include <functional>
#include "HistDataRcvMng.h"
#include <boost/lexical_cast.hpp>

std::thread* pCmdThread = nullptr;
extern HANDLE g_evt;

std::string& strToUpperInSitu(std::string& str)
{
	int len = str.length();
	for (int i = 0; i < len; i++)	str[i] = toupper(str[i]);
	return str;
}

template<typename T, class CHKAndIniter>
void RecieveInput(const char* prompt, T& out, CHKAndIniter chk)
{
	while (true)
	{
		std::cout << prompt;
		std::cin >> out;
		if (std::cin.fail())
		{
			std::cout << "input wrong,please input again!\n";
			std::cin.ignore();
			std::cin.clear();
			std::cin.sync();
			continue;
		}

		if (!chk(out))
		{
			std::cout << "wrong input,please try again!\n";
			continue;
		}

		std::cin.ignore();
		std::cin.clear();
		std::cin.sync();
		return;
	}
}

void getStkId(std::string& stkid)
{
	//RecieveInput("\nstock id(like nvda or NVDA):  ", stkid, [](std::string& in)->bool{int len = in.length();	for (int i = 0; i < len; i++)if (!isalpha(in[i]))return false; return true; });
	RecieveInput("\nstock id(like nvda or NVDA):  ", stkid, [](std::string& in)->bool{ return true;});
	stkid = strToUpperInSitu(stkid);
}

void getDate(std::string& inputDate)
{
	util::UpdateTime();

	RecieveInput("\nquery date[like:20170602,use today input 0]:  ", inputDate, [](std::string& in)->bool{ \
		int len = in.length(); \
	if (1 == len){ if (in[0] == '0')return true; else return false; } \
	if (len != 8)return false; \
	for (int i = 0; i < len; i++)if (!isdigit(in[i]))return false; return true; });

	if (inputDate == "0")
	{
		inputDate = boost::lexical_cast<std::string>(util::GetCurDate()); // GetCurDate后掉了括号，会导致难以排查的错误
		//char tmp[9];
		//sprintf_s<9>(tmp, "%d", util::GetCurDate());
		//inputDate = tmp;
	}
}

void getTime(std::string& inputTime)
{
	RecieveInput("\nquery time[like:12:34:56,use 05:15:00 input 0]:  ", inputTime, [](std::string& in)->bool{ \
		int len = in.length(); \
	if (1 == len){ if (in[0] == '0')return true; else return false; } \
	if (len != 8)return false;  return true; });
	if (inputTime == "0")inputTime = "05:16:00";
}

void getExpire(std::string& expird)
{
	RecieveInput("\nexpire date[like:20170602,input 0 use default]:  ", expird, [](std::string& in)->bool{ \
		int len = in.length(); \
	if (1 == len){ if (in[0] == '0')return true; else return false; } \
	if (len != 8)return false; \
	for (int i = 0; i < len; i++)if (!isdigit(in[i]))return false; return true; });
	if (expird == "0")expird = "20170602";
}

void getSecType(std::string& secType)
{
	RecieveInput("\nSecurity type:[like:STK,OPT,CMDTY]: ", secType, [](std::string& in)->bool{return true;	});
	secType = strToUpperInSitu(secType);
}

void getRight(std::string& right)
{
	RecieveInput("\nPut Or Call [p/c]:  ", right, [](std::string& in)->bool{if ('p' != in[0] && 'P' != in[0] && 'c' != in[0] && 'C' != in[0])return false; if ('p' == in[0])in[0] = 'P'; if ('c' == in[0])in[0] = 'C'; return true; });
}

void getMultiplier(std::string& mult)
{
	RecieveInput("\nMultiplier [100]:  ", mult, [](std::string& in)->bool{return true;});
}

void getStrike(double& strk)
{
	RecieveInput("\nStrike: ", strk, [](double& in)->bool{return true; });
}

void getTheV(double& v)
{
	RecieveInput("\nthe V:  ", v, [](double& in)->bool{if (in < 0)return false; else return true; });
}

// "1min" => "1 min"
std::string insertSpace(std::string& brs)
{
	std::string out(brs);
	out.append(" ");
	int i;
	for (i = 0; i < brs.length(); i++)
	{
		if (!isdigit(brs[i]))break;
		out[i] = (brs[i]);
	}
	out[i] = ' ';
	for (i++; i < out.length(); i++)
	{
		out[i] = brs[i - 1];
	}
	return out;
}

void getHDBarSize(std::string& brs)
{
	RecieveInput("\nthe BarSize [eg: 1min]:  ", brs, [](std::string& in)->bool{if (isdigit(in[0]))return true; else return false; });
	brs = insertSpace(brs);
}

void getHDDuration(std::string& dr)
{
	RecieveInput("\nthe Duration [eg: 100D]:  ", dr, [](std::string& in)->bool{return true; });
	dr = insertSpace(dr); // "100D" => "100 D"
}

void getCmd(int& cmd)
{
	RecieveInput("\ninput your cmd:  ", cmd, [](int& in)->bool{if (in < 0 || in>99)return false; else return true; });
}

void dispMenu()
{
	std::cout << "\n\n########################################################\n";
	std::cout << "\t            M E N U\n";
	std::cout << "\t1. query option histdata\n";
	std::cout << "\t2. query account summary\n";
	std::cout << "\t3. query stock option price\n";
	std::cout << "\t4. get stk all histdata can get\n";
	std::cout << "\t5. query historical market data\n";
	std::cout << "\t6. query conid\n";
	std::cout << "\t7. get histdata day by day\n";
	std::cout << "\t99. show menu\n";
	std::cout << "\t0. quit\n";
	std::cout << "########################################################\n";
}

void ReadDateList(std::vector<std::string>& datelist)
{
	datelist.push_back("20140101");
}

void execute(Client* pClient, const int cmdnum)
{
	if (cmdnum > 99 || cmdnum < 0)return;
	std::string stkid(""), expiredate(""), optype(""), secType("STK"),multip("100");
	double strike(0);
	double theV(0.5);
	long conid(1);
	pClient->m_curCmd = cmdnum;
	std::string underlying("SPY");
	int conId = 756733;
	QryFBSeqDefOptParam qryfb;
	DWORD dwRet = 0;
	HistDataRcvMng* hdq = nullptr;
	std::string histDataDate; // 起始下载时间
	std::string histDataTime; // 查历史数据的时间
	std::string today;
	std::string lastquerydate("");
	int curtime = 0;
	int curdate = 0;
	std::string barsize(""); // 1 min 2 mins 5 mins...
	std::string duration("");
	std::vector<std::string> datelist;
	switch (cmdnum)
	{
	case 6:
		getStkId(stkid);
		getSecType(secType);
		std::cout << "conid = " << pClient->getConId(stkid, secType) << std::endl;
		break;
	case 5:
		getDate(histDataDate); // 第一次启动
		while (true)
		{
			util::UpdateTime();
			curtime = util::GetCurTime();
			curdate = util::GetCurDate();
			//today = boost::lexical_cast<std::string>(util::GetCurDate());
			// 如果已经取到了今天就停止
			if (atoi(histDataDate.c_str()) >= curdate)
			{
				sleep(10);
				continue;
			}
			std::cout << "query 1m spy history k data  of " << histDataDate << " begin!\n";
			// 先获取到expiration
			if (!pClient->getSecDefOptParams(underlying, conId, qryfb)) break;

			std::cout << "new HistDataRcvMng...\n";
			hdq = new HistDataRcvMng(histDataDate, underlying, qryfb.Expirations, qryfb.Strikes, pClient, qryfb.multiplier);
			hdq->qryOpt();
			std::cout << "query history k data of " << histDataDate << " end!\n";
			histDataDate = util::NextTradingDate(atoi(histDataDate.c_str()), 1);
		}
		delete hdq;
		break;
	case 1:
		getDate(histDataDate); // 第一次启动
		util::UpdateTime();
		curtime = util::GetCurTime();
		curdate = util::GetCurDate();
		std::cout << "query 1m spy history k data  of " << histDataDate << " begin!\n";
		// 先获取到expiration
		if (!pClient->getSecDefOptParams(underlying, conId, qryfb)) break;
		std::cout << "new HistDataRcvMng...\n";
		hdq = new HistDataRcvMng(histDataDate, underlying, qryfb.Expirations, qryfb.Strikes, pClient, qryfb.multiplier);
		hdq->qryOptOneByOne();
		std::cout << "query history k data of " << histDataDate << " end!\n";
		delete hdq;
		break;
	case 2:
		pClient->queryAccountInfo();
		break;
	case 3:
		getStkId(stkid);
		getExpire(expiredate);
		getRight(optype);
		getStrike(strike);
		pClient->getOptionPrice(stkid, expiredate, strike, optype);
		break;
	case 4:
		// 取回一个标的可以取到的所有历史数据
		getSecType(secType);
		getStkId(stkid);
		if ("OPT" == secType)
		{
			getExpire(expiredate);
			getRight(optype);
			getStrike(strike);
			getMultiplier(multip);
		}
		getDate(histDataDate); // 获取从哪一天开始的数据，0表示取最近一天，11111111表示取能取到的最久的数据
		getTime(histDataTime);
		getHDBarSize(barsize);
		getHDDuration(duration);

		if (!hdq)hdq = new HistDataRcvMng(histDataDate, histDataTime, stkid, barsize, duration, pClient);
		if ("OPT" == secType)
		{
			hdq->UpdateInfo[0].SetContract(stkid, std::string("SMART"), expiredate, strike, multip, optype);
		}
		if ("CMDTY" == secType)
		{
			hdq->UpdateInfo[0].contract.secType = secType;
			hdq->UpdateInfo[0].contract.exchange = "GLOBEX";
		}
		// 根据交易日列表，每10天一取，循环取完
		for (;;)
		{
			//std::cout << "query 1m spy history k data  of " << stkid << " begin!\n";
			hdq->qryStkAllHist();
			//std::cout << "query history k data of " << stkid << " end!\n";
		}
		delete hdq;
		break;
	case 7:
		getStkId(stkid);
		getDate(histDataDate); // 获取从哪一天开始的数据，0表示取最近一天，11111111表示取能取到的最久的数据
		getHDBarSize(barsize);
		getHDDuration(duration);

		std::cout << "query 1m spy history k data  of " << histDataDate << " begin!\n";
		hdq = new HistDataRcvMng(histDataDate, histDataTime, stkid, barsize, duration, pClient);
		hdq->qryStk();
		std::cout << "query history k data of " << histDataDate << " end!\n";
		break;
	default:
		return;
	}

	if (hdq)delete hdq;

}

void cmdProcess(Client* pClient)
{
	int cmd = 0;
	sleep(1);
	pClient->marketDataType();
	sleep(1);
	for (;;)
	{
		dispMenu();
		if (999 != cmd)
		{
			getCmd(cmd);
			std::cout << "\n";
		}
		else cmd = 5;

		if (99 == cmd)
		{
			continue;
		}

		if (0 == cmd)
		{
			pClient->quit();
			std::cout << "exiting......\n";
			std::cout << "UIThread ended ok......\n";
			sleep(3);
			return;
		}
		execute(pClient, cmd);
		sleep(3);
	}
}
