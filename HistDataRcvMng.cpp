#include "stdafx.h"
//#include <sstream>
#include <fstream>
#include <iostream>
#include "HistDataRcvMng.h"
#include "Client.h"
#include <boost/lexical_cast.hpp>
//#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
//#include <boost/filesystem/path.hpp>
std::string& KItem::toString(std::string& in)
{
	char buf[512];
	//sprintf_s<512>(buf, "secId : %s, date : %s, time : %s, open : %0.2f, high : %0.2f, low : %0.2f, close : %0.2f\n", SecId.c_str(), date, time, open, high, low, close);
	sprintf_s<512>(buf, "%s,%s,%0.2f,%0.2f,%0.2f,%0.2f,%d,%0.2f\n", date, time, open, high, low, close, vol, amount);
	in = buf;
	//std::stringstream ss("");
	//ss << "abc";
	//ss << "secId:" << SecId << ",date:" << date << ",time:" << time << ",open:" << open << ",high:" << high << ",low:" << low << ",close:" << close << std::endl;
	//in = ss.str();
	//in = str(boost::format("secId: %1%, date: %2%, time: %3%, open: %4%, high: %5%, low: %6%, close: %7%\n") % SecId%date%time%open%high%low%close);
	return in;
}

void KItem::disp()
{
	std::string tickstr;
	toString(tickstr);
	printf("%s", tickstr.c_str());
}

void HistDataUpdateInfo::toCsv(QryFBHistData& in)
{
	if (in.data.size() < 1)return;

	std::fstream out(filepath, std::ios::out | std::ios::app);
	if (!out)
	{
		// ��ʧ��
		std::cout << "open file:" << filepath << "failed!\n";
		return;
	}

	for (auto ij = in.data.begin(); ij != in.data.end(); ++ij)
	{
		std::string tickstr;
		ij->toString(tickstr);
		//printf("%s", tickstr.c_str());
		out << tickstr;
		out.flush();
	}
	out.close();
}

/*
��ʷ������ȡ�����ģʽ��
ÿ����ȡ����spy��Ȩ��Լ�������ǵ�1����k�����ݴ���
һ����Լһ���ļ� --> 10000���С�ļ���vs ÿ��һ�����ļ�ֱ����⣿
*/
HistDataRcvMng::HistDataRcvMng(const std::string& date, std::string& udly, std::vector<std::string>& exp, std::vector<double>& strk, Client* pcl, std::string& mult) :pClient(pcl), initDateBegin(date)
{
	Expirations.assign(exp.begin(), exp.end());
	Strikes.assign(strk.begin(), strk.end());
	multiplier = mult;
	this->underlying = udly;
}

HistDataRcvMng::HistDataRcvMng(std::string& date, std::string& time, std::string& stkid, std::string& barsize, std::string& duration, Client* pcl) : barSize(barsize), initDateBegin(date), pClient(pcl)
{
	HistDataUpdateInfo hdui;
	hdui.lastQryDate = "";
	hdui.lastQryTime = time; // ��ʱ��֮ǰ������ʷ����
	hdui.queryTime = hdui.lastQryTime;
	hdui.queryDateTime = initDateBegin + " " + hdui.lastQryTime;
	hdui.SetContract(stkid, "STK");
	hdui.barsize = barSize;
	hdui.duration = duration;
	UpdateInfo.push_back(hdui);
}

HistDataRcvMng::~HistDataRcvMng()
{
}

bool HistDataRcvMng::PauseCheck(const std::string& day1, const std::string& day2)
{
	return day1.compare(day2) < 0;
}

void HistDataRcvMng::qryStkAllHist()
{
	for (auto it = UpdateInfo.begin(); it != UpdateInfo.end(); ++it)
	{
		it->BuildDataFilePath1();

		QryFBHistData qryfb;
		qryfb.qrydatetime = "";
		// -for test-
		//it->contract.strike = 240;
		// -for test-
		//std::cout << it->contract.symbol << "query histdata begin......" << std::endl;
		if (pClient->historicalDataRequests(it->contract, it->queryDateTime.c_str(), it->duration, it->barsize, qryfb))
		{
			it->toCsv(qryfb);
			// qry �ɹ�������ϴγɹ���ѯʱ��
			it->UpdateQryInfo();
			it->queryDateTime = qryfb.qrydatetime; // ���鵽������ʱ�������һ�β�ѯʱ��
			std::cout << it->contract.symbol << "query "<<it->duration<<" "<<it->barsize<<" histdata ok......" << std::endl;
			if ("STK" == it->contract.secType)
			{
				sleep(5);
			}
		}
		//return;
	}
}

void HistDataRcvMng::qryStk()
{
	for (auto it = UpdateInfo.begin(); it != UpdateInfo.end(); ++it)
	{
		it->PrepareUpdateInfo();

		QryFBHistData qryfb;
		// -for test-
		//it->contract.strike = 240;
		// -for test-
		std::cout << it->contract.symbol << "query histdata begin......" << std::endl;
		if (pClient->historicalDataRequests(it->contract, it->queryDateTime.c_str(), it->duration, it->barsize, qryfb))
		{
			it->toCsv(qryfb);
			// qry �ɹ�������ϴγɹ���ѯʱ��
			it->UpdateQryInfo();
			sleep(25);
		}
		//return;
	}
}

void HistDataRcvMng::qryOptOneByOne()
{
}

void HistDataRcvMng::qryOpt()
{
	for (auto exp = Expirations.begin(); exp != Expirations.end(); ++exp)
	{
		// �ٸ���ÿһ��expirationȡ��ÿһ��expiration�����к�Լ �����pClient->m_optchain��
		pClient->GetContractsByExpiration(underlying, *exp);

		// �����ѯ�� UpdateInfo
		for (auto its = pClient->m_optchain.begin(); its != pClient->m_optchain.end(); ++its)
		{
			HistDataUpdateInfo hdui;
			hdui.lastQryDate = "";
			hdui.lastQryTime = "06:00:00"; // ��ʱ��֮ǰ������ʷ����
			hdui.queryTime = hdui.lastQryTime;
			hdui.queryDateTime = initDateBegin + " " + hdui.lastQryTime;
			hdui.SetContract(underlying, std::string("SMART"), *exp, its->strike, its->multiplier, std::string("P"));
			hdui.barsize = barSize;
			hdui.duration = "10 D";

			//hdui.contract.secId = util::BuildOptionSymbol(hdui.contract);
			hdui.contract.secId = its->putSymbol;
			
			UpdateInfo.push_back(hdui);

			hdui.contract.right = "C";
			//hdui.contract.secId = util::BuildOptionSymbol(hdui.contract);
			hdui.contract.secId = its->callSymbol;
			UpdateInfo.push_back(hdui);
		}

		for (auto it = UpdateInfo.begin(); it != UpdateInfo.end(); ++it)
		{
			it->PrepareUpdateInfo();

			QryFBHistData qryfb;
			// -for test-
			//it->contract.strike = 240;
			// -for test-
			std::cout <<it->contract.secId<< "query histdata begin......" << std::endl;
			if (pClient->historicalDataRequests(it->contract, it->queryDateTime.c_str(), it->duration, it->barsize, qryfb))
			{
				it->toCsv(qryfb);
				// qry �ɹ�������ϴγɹ���ѯʱ��
				it->UpdateQryInfo();
				sleep(25);
			}
		}
		return;
	}
}

void HistDataUpdateInfo::UpdateQryInfo()
{
	// ��ѯ�ɹ�����������Ϣ
	lastQryDate = queryDateTime.substr(0, 8);
	lastQryTime = queryDateTime.substr(10, 8);
}

void HistDataUpdateInfo::PrepareUpdateInfo()
{
	// ɨ�豾���Ѿ����ص������ļ������ʱ�䣬���챾��Լ�Ĳ�ѯ�����������β�ѯ����ļ���
	//barsize = "1 min";
	//SetDuration();
	UpdateQryTime();
	BuildDataFilePath();
}

void HistDataUpdateInfo::SetDuration()
{
	// �̶�Ϊ1��
	duration = "1 D";
}

// ���±��β�ѯʱ��
void HistDataUpdateInfo::UpdateQryTime()
{
	// ����lastQryTime��duration �Լ�barsize �����±���queryTime
	// ÿ��ȡ1�������
	// queryTime = "20170614 04:15:00";
	// ����ǵ�һ�β�ѯ�ʹ�init

	// Ŀǰ����Ҫ���κ���
}

void HistDataUpdateInfo::BuildDataFilePath()
{
	// �ļ�����ʽ��SPY_170614C00240500_60s.csv
	namespace fs = boost::filesystem;
	fs::path fullpath(fs::initial_path());
	fullpath /= "data";
	//filepath = "data\\";
	fullpath /= (queryDateTime.substr(0, 8));
	if (!fs::exists(fullpath))
	{
		// ���������Ŀ¼
		bool bRet = fs::create_directories(fullpath);
		if (false == bRet)
		{
			return;
		}

	}
	filepath = fullpath.string();

	std::string bs(util::Simple(barsize));
	if ("OPT" == contract.secType)
	{
		filepath.append("\\").append(util::BuildOptionSymbol(contract)).append("_").append(bs).append(".csv");
	}
	else if ("STK" == contract.secType)
	{
		filepath.append("\\").append(contract.symbol).append("_").append(bs).append(".csv");
	}
	
}

void HistDataUpdateInfo::BuildDataFilePath1()
{
	// �ļ�����ʽ��SPY_170614C00240500_60s.csv
	namespace fs = boost::filesystem;
	fs::path fullpath(fs::initial_path());
	fullpath /= "data";
	//filepath = "data\\";
	fullpath /= "cmd4";
	if (!fs::exists(fullpath))
	{
		// ���������Ŀ¼
		bool bRet = fs::create_directories(fullpath);
		if (false == bRet)
		{
			return;
		}
	}
	filepath = fullpath.string();

	std::string bs(util::Simple(barsize));
	if ("OPT" == contract.secType)
	{
		filepath.append("\\").append(util::BuildOptionSymbol(contract)).append("_").append(bs).append(".csv");
	}
	else if ("STK" == contract.secType)
	{
		filepath.append("\\").append(contract.symbol).append("_").append(bs).append(".csv");
	}
	else if ("CMDTY" == contract.secType)
	{
		filepath.append("\\CMDTY_").append(contract.symbol).append("_").append(bs).append(".csv");
	}

}