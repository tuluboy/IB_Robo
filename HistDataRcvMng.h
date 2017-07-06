#pragma once
// ������Ȩ������Լ�����غʹ洢���
#include <vector>
#include <string>
#include "defs.h"
class Client;
class HistDataRcvMng
{
public:
	static bool PauseCheck(const std::string& today, const std::string& lastQDate); // ����Ƿ��ֹͣ��������

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
	void qryOptOneByOne(); // ����ָ����Լ��������ʷ����
	void qryStk();
	void qryStkAllHist();
private:
	std::string initDateBegin; // ��ʼ�����쿪ʼ����
	std::string initDateTime;
	std::string lastQryDate;
	std::string barSize;
};

