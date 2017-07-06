#ifndef __defs_h_adei9496g_4khmmnljpuo_13xxd_dkj46nb__
#define __defs_h_adei9496g_4khmmnljpuo_13xxd_dkj46nb__
//#include <boost/filesystem.hpp> 
#include <vector>
#include <string>

#include "Contract.h"

struct QryFBHistData;

struct QryFBSeqDefOptParam
{
	int conId;
	std::string symbol; // underlying
	std::vector<std::string> Exchanges; // �������б�
	std::vector<std::string> Expirations; // �������б�
	std::vector<double> Strikes;
	std::string multiplier;
};

struct HistDataUpdateInfo
{
	std::string lastQryDate;
	std::string lastQryTime; // �ϴβ�ѯʱ��
	Contract contract;
	//boost::filesystem::path filename;
	std::string filepath;
	std::string queryTime;
	std::string queryDateTime; // ��β�ѯʱ��
	std::string duration;
	std::string barsize;
	void SetContract(std::string& symbol, std::string& exch, std::string& expir, double strike, std::string mult, std::string rt)
	{
		contract.symbol = symbol; contract.exchange = exch; contract.lastTradeDateOrContractMonth = expir; contract.multiplier = mult; contract.currency = "USD"; contract.strike = strike; contract.secType = "OPT"; contract.right = rt;
	};
	void SetContract(std::string& symbol, std::string secType)
	{
		contract.symbol = symbol; contract.secType = secType; contract.currency = "USD";contract.exchange = "ISLAND";
	};
	void toCsv(QryFBHistData& in);
	void PrepareUpdateInfo(); // ��鵱ǰ�����ļ����������ʱ�䣬�Դ�Ϊ��ʼ��������
	void BuildDataFilePath();
	void BuildDataFilePath1(); // ȡһ����ĵ����У�����ͬһ���ļ�
	void SetDuration();
	void UpdateQryTime();
	void UpdateQryInfo(); // ���β�ѯ�ɹ��󣬸��²�ѯʱ��
};

struct KItem
{
	std::string SecId;
	char date[9];
	char time[9];
	float open;
	float high;
	float low;
	float close;
	int vol;
	float amount;
	std::string& toString(std::string& in);
	void disp();
};

struct QryFBHistData
{
	std::string qrydatetime;
	std::vector<KItem> data;
};
#endif