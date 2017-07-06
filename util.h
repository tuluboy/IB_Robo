#pragma once
#include <stdio.h>
#include <thread>
#include <string>
#include <vector>
#include <map>

#define eq(x,y) (abs((x)-(y))<0.00001)
class Contract;
struct OptionChainItem
{
	std::string ul_symbol; // undlying sec symbol
	std::string putSymbol;
	std::string callSymbol;
	double strike;
	std::string multiplier;
	long conid_put;
	long conid_call;
	double put_price;
	double call_price;
	double sum;
};

struct AccountDisp
{
	std::string userId;
	std::string AccountType;
	std::string Currency;
	double TotalCashValue;
	double GrossPositionValue;
	double NetLiquidation;
	double SettledCash;
	void disp()
	{
		printf("\nAccount ID: %s\n", userId.c_str());
		printf("\nAccountType: %s\n", AccountType.c_str());
		printf("\nAccount Currency: %s\n", Currency.c_str());
		printf("\nTotal Cash Value: %f\n", TotalCashValue);
		printf("\nGross Position Value: %f\n", GrossPositionValue);
		printf("\nNetLiquidation: %f\n", NetLiquidation);
		printf("\nSettledCash: %f\n", SettledCash);
	}
};


class Client;
extern std::thread* pCmdThread;
extern void cmdProcess(Client*);

class USMktTime
{
public:
	static time_t preOpen(int nSeconds);
	static time_t Open();
	static time_t Close();
	static time_t postClose(int nSeconds); // 盘后某个时间
	static void initEveryDay(int date);
public:
	USMktTime(){};
	~USMktTime(){};
private:
	static time_t t_open;
	static time_t t_close;
	static tm tm_open;
	static tm tm_close;
	static const char* open;
	static const char* close;
};

class util
{
public:
	static bool isNearMarketOpenTime();
	static bool isOutMarketCloseTime();
	static void UpdateTime();
	static int GetCurDate();
	static int GetCurTime();
	static const char* GetCurTimeStr();
	static void sep_date_time(const std::string& datetime, std::string& date, std::string& time); // 将datetime字符串分割为date和time子串
	static std::string BuildStrikeStr(double in);
	static std::string Simple(std::string& barsize);
	static int NextTradingDate(int date, int direction); // direction: 1 返回date的后一个交易日， -1返回前一个
	static std::string BuildOptionSymbol(Contract& in);
public:
	util();
	~util();
private:
	static int timenum;
	static time_t curTime;
	static tm tminfo;
	static int curDate;
	static char timestr[10];
	static char datetimestr[100]; // yyyymmdd hh:nn:ss
	static std::map<std::string, std::string> simplist;
};