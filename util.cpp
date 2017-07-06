#include "stdafx.h"
#include <time.h>
#include "util.h"
#include <assert.h>
#include "Contract.h"

time_t USMktTime::t_open;
time_t USMktTime::t_close;
struct tm USMktTime::tm_open;
struct tm USMktTime::tm_close;
const char* USMktTime::open = "21:30:00";
const char* USMktTime::close = "04:15:00";
char util::timestr[10] = { '\0' };
int util::curDate = 0;
time_t util::curTime = 0;
int util::timenum = 0;
struct tm util::tminfo;
char util::datetimestr[100];
std::map<std::string, std::string> util::simplist;
util::util()
{
}

util::~util()
{
}

bool util::isNearMarketOpenTime()
{
	return curTime < USMktTime::Open() && curTime > USMktTime::preOpen(120);
}

bool util::isOutMarketCloseTime()
{
	return curTime > USMktTime::postClose(120);
}

void util::UpdateTime()
{
	time(&curTime);
	localtime_s(&tminfo, &curTime);
	strftime(datetimestr, 100, "%Y%m%d %H:%M:%S", &tminfo);
	//asctime_s<100>(datetimestr, &tminfo);
	sprintf_s(timestr, 10, "%s", datetimestr + 9);
	curDate = atoi(std::string(datetimestr).substr(0, 8).c_str());
	timenum = (timestr[0]-48)*100000+(timestr[1]-48)*10000+(timestr[3]-48)*1000+(timestr[4]-48)*100+(timestr[6]-48)*10+timestr[7] - 48;
	//printf("The current datetime is: %s\n", datetimestr);
	//printf("The current time is: %s\n", timestr);
}

// 每天更新，获取当天的各种时间
void USMktTime::initEveryDay(int nDate)
{
	util::UpdateTime();
	assert(0);
}

time_t USMktTime::preOpen(int nSeconds)
{
	return t_open - nSeconds;
}

time_t USMktTime::Open()
{
	return t_open;
}

time_t USMktTime::Close()
{
	return t_close;
}

time_t USMktTime::postClose(int nSeconds)
{
	return t_close + nSeconds;
}

int util::GetCurDate()
{
	return curDate;
}

int util::GetCurTime()
{
	return timenum;
}

const char* util::GetCurTimeStr()
{
	return timestr;
}

// datetime: "20170601 21:05:32" => date: "20170601"  time: "21:32:23"
void util::sep_date_time(const std::string& datetime, std::string& date, std::string& time) // 将datetime字符串分割为date和time子串
{
	date = datetime.substr(0, 8);
	time = datetime.substr(10, 8);
}

// usoption的strike格式: 00240500对应240.5
std::string util::BuildStrikeStr(double in)
{
	// 240.5 => 0024500
	char tmp[32];
	sprintf_s<32>(tmp, "%08.0f", in); // 前面补0
	return tmp;
}

// 返回空表示没有
std::string util::Simple(std::string& barsize)
{
	// 将ib的historydata barsize 对应到
	/*	1 secs 	5 secs 	10 secs 	15 secs 	30 secs
	1 min 	2 mins 	3 mins 	5 mins 	10 mins 	15 mins 	20 mins 	30 mins
	1 hour 	2 hours 	3 hours 	4 hours 	8 hours
	1 day
	1 week
	1 month
	*/
	if (simplist.size() == 0)
	{
		simplist.insert(std::pair<std::string, std::string>("1 min", "60s"));
		simplist.insert(std::pair<std::string, std::string>("2 mins", "120s"));
		simplist.insert(std::pair<std::string, std::string>("3 mins", "180s"));
		simplist.insert(std::pair<std::string, std::string>("5 mins", "300s"));
		simplist.insert(std::pair<std::string, std::string>("10 mins", "600s"));
		simplist.insert(std::pair<std::string, std::string>("15 mins", "900s"));
	}

	auto itf = simplist.find(barsize);
	if (itf != simplist.end())return itf->second;
	return "";
}

int util::NextTradingDate(int date, int direction) // direction: 1 返回date的后一个交易日， -1返回前一个
{
	return 20170622;
}

std::string util::BuildOptionSymbol(Contract& contract)
{
	std::string tmp;
	tmp.append("SPY_").append(contract.lastTradeDateOrContractMonth.substr(2, 6)).append(contract.right).append(util::BuildStrikeStr(contract.strike * 1000));
	return tmp;
}