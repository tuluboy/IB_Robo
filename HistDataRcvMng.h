#pragma once
// 管理期权各个合约的下载和存储入库
#include <vector>
#include <string>
#include "defs.h"
class Client;
class HistDataRcvMng
{
public:
	static bool PauseCheck(const std::string& today, const std::string& lastQDate); // 检查是否该停止下载数据

	HistDataRcvMng(const std::string& date, std::string& underlying, std::vector<std::string>& exp, std::vector<double>& strk, Client* pcl, std::string& mult); // opt
	HistDataRcvMng(std::string& date, std::string& time, std::string& stksymbol, std::string& barsize, std::string& duration, Client* pcl);
	HistDataRcvMng(){};
	~HistDataRcvMng();
	void SetInitDate(std::string& initdate){ initDateBegin = initdate; };
	void SetInitTime(std::string& inittime){ initDateTime = inittime; };
	std::string multiplier;
	std::string& getHistDataInitDate(){ return initDateBegin; };
	std::vector<std::string> Expirations;
	std::vector<double> Strikes;
	std::string underlying;
	std::vector<HistDataUpdateInfo> UpdateInfo;
	Client* pClient;
	void qryOpt();
	void qryOptOneByOne(); // 下载指定合约的所有历史数据
	void qryStk();
	void qryStkAllHist();
private:
	std::string initDateBegin; // 初始从哪天开始下载
	std::string initDateTime;
	std::string lastQryDate;
	std::string barSize;
};

