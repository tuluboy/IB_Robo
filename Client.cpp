/* Copyright (C) 2013 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#include "StdAfx.h"

#include "Client.h"

#include "EClientSocket.h"
#include "EPosixClientSocketPlatform.h"

#include "Contract.h"
#include "Order.h"
#include "OrderState.h"
#include "Execution.h"
#include "CommissionReport.h"
#include "ContractSamples.h"
#include "OrderSamples.h"
#include "ScannerSubscription.h"
#include "ScannerSubscriptionSamples.h"
#include "executioncondition.h"
#include "PriceCondition.h"
#include "MarginCondition.h"
#include "PercentChangeCondition.h"
#include "TimeCondition.h"
#include "VolumeCondition.h"
#include "AvailableAlgoParams.h"
#include "FAMethodSamples.h"
#include "CommonDefs.h"
#include "AccountSummaryTags.h"

#include <stdio.h>
//#include <iostream>
#include <thread>
#include <ctime>
#include <queue>
#include "defs.h"

const int PING_DEADLINE = 2; // seconds
const int SLEEP_BETWEEN_PINGS = 3; // seconds

HANDLE g_evt;
///////////////////////////////////////////////////////////
// member funcs
//! [socket_init]
Client::Client() :
m_osSignal(2000)//2-seconds timeout
, m_pClient(new EClientSocket(this, &m_osSignal))
, m_state(ST_CONNECT)
, m_sleepDeadline(0)
, m_orderId(0)
, m_pReader(0)
, m_extraAuth(false)
, m_reqid(0)
, queryFeedBack(nullptr)
{
}
//! [socket_init]
Client::~Client()
{
	if (m_pReader)
		delete m_pReader;

	delete m_pClient;
}

bool Client::connect(const char *host, unsigned int port, int clientId)
{
	// trying to connect
	printf("Connecting to %s:%d clientId:%d\n", !(host && *host) ? "127.0.0.1" : host, port, clientId);

	bool bRes = m_pClient->eConnect(host, port, clientId, m_extraAuth);

	if (bRes) {
		printf("Connected to %s:%d clientId:%d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);

		m_pReader = new EReader(m_pClient, &m_osSignal);

		m_pReader->start();
	}
	else
		printf("Cannot connect to %s:%d clientId:%d\n", m_pClient->host().c_str(), m_pClient->port(), clientId);

	return bRes;
}

void Client::disconnect() const
{
	m_pClient->eDisconnect();
	printf("Disconnecting.......\n");
	//sleep(5);
}

bool Client::isConnected() const
{
	return m_pClient->isConnected();
}

void Client::setConnectOptions(const std::string& connectOptions)
{
	m_pClient->setConnectOptions(connectOptions);
}

void Client::processMessages() {
	fd_set readSet, writeSet, errorSet;

	struct timeval tval;
	tval.tv_usec = 0;
	tval.tv_sec = 0;

	time_t now = time(NULL);
	//printf("........................m_state: %d\n", m_state);
	/*****************************************************************/
	/* Below are few quick-to-test examples on the IB API functions grouped by functionality. Uncomment the relevant methods. */
	/*****************************************************************/
	switch (m_state) {
	case ST_TICKDATAOPERATION:
		tickDataOperation();
		break;
	case ST_TICKDATAOPERATION_ACK:
		break;
	case ST_MARKETDEPTHOPERATION:
		marketDepthOperations();
		break;
	case ST_MARKETDEPTHOPERATION_ACK:
		break;
	case ST_REALTIMEBARS:
		realTimeBars();
		break;
	case ST_REALTIMEBARS_ACK:
		break;
	case ST_MARKETDATATYPE:
		marketDataType();
		break;
	case ST_MARKETDATATYPE_ACK:
		break;
	case ST_HISTORICALDATAREQUESTS:
		// historicalDataRequests();
		break;
	case ST_HISTORICALDATAREQUESTS_ACK:
		break;
	case ST_OPTIONSOPERATIONS:
		optionsOperations();
		break;
	case ST_OPTIONSOPERATIONS_ACK:
		break;
	case ST_CONTRACTOPERATION:
		contractOperations();
		break;
	case ST_CONTRACTOPERATION_ACK:
		break;
	case ST_MARKETSCANNERS:
		marketScanners();
		break;
	case ST_MARKETSCANNERS_ACK:
		break;
	case ST_REUTERSFUNDAMENTALS:
		reutersFundamentals();
		break;
	case ST_REUTERSFUNDAMENTALS_ACK:
		break;
	case ST_BULLETINS:
		bulletins();
		break;
	case ST_BULLETINS_ACK:
		break;
	case ST_ACCOUNTOPERATIONS:
		accountOperations();
		break;
	case ST_ACCOUNTOPERATIONS_ACK:
		break;
	case ST_ORDEROPERATIONS:
		orderOperations();
		break;
	case ST_ORDEROPERATIONS_ACK:
		break;
	case ST_OCASAMPLES:
		ocaSamples();
		break;
	case ST_OCASAMPLES_ACK:
		break;
	case ST_CONDITIONSAMPLES:
		conditionSamples();
		break;
	case ST_CONDITIONSAMPLES_ACK:
		break;
	case ST_BRACKETSAMPLES:
		bracketSample();
		break;
	case ST_BRACKETSAMPLES_ACK:
		break;
	case ST_HEDGESAMPLES:
		hedgeSample();
		break;
	case ST_HEDGESAMPLES_ACK:
		break;
	case ST_TESTALGOSAMPLES:
		testAlgoSamples();
		break;
	case ST_TESTALGOSAMPLES_ACK:
		break;
	case ST_FAORDERSAMPLES:
		financialAdvisorOrderSamples();
		break;
	case ST_FAORDERSAMPLES_ACK:
		break;
	case ST_FAOPERATIONS:
		financialAdvisorOperations();
		break;
	case ST_FAOPERATIONS_ACK:
		break;
	case ST_DISPLAYGROUPS:
		testDisplayGroups();
		break;
	case ST_DISPLAYGROUPS_ACK:
		break;
	case ST_MISCELANEOUS:
		miscelaneous();
		break;
	case ST_MISCELANEOUS_ACK:
		break;
	case ST_PING:
		reqCurrentTime();
		break;
	case ST_PING_ACK:
		if (m_sleepDeadline < now) {
			disconnect();
		}
		break;
	case ST_IDLE:
		printf("\nrecieve ST_IDLE......\n");
		if (m_sleepDeadline < now) {
			m_state = ST_PING;
			return;
		}
		break;
	case ST_QUIT:
		disconnect();
		pCmdThread->join();
		delete pCmdThread;
		pCmdThread = nullptr;
		return;
	}

	if (m_sleepDeadline > 0) {
		// initialize timeout with m_sleepDeadline - now
		tval.tv_sec = m_sleepDeadline - now;
	}

	m_pReader->checkClient();
	m_osSignal.waitForSignal();
	m_pReader->processMsgs();

}

//////////////////////////////////////////////////////////////////
// methods
//! [connectack]
void Client::connectAck() {
	if (!m_extraAuth && m_pClient->asyncEConnect())
		m_pClient->startApi();
}
//! [connectack]

void Client::reqCurrentTime()
{
	printf("Requesting Current Time\n");

	// set ping deadline to "now + n seconds"
	m_sleepDeadline = time(NULL) + PING_DEADLINE;

	m_state = ST_PING_ACK;

	m_pClient->reqCurrentTime();
}

void Client::tickDataOperation()
{
	/*** Requesting real time market data ***/
	sleep(1);
	//! [reqmktdata]
	std::queue<int> usedReqid;

	m_pClient->reqMktData(m_reqid, ContractSamples::StockComboContract(), "", false, TagValueListSPtr());
	usedReqid.push(m_reqid++);

	m_pClient->reqMktData(m_reqid, ContractSamples::OptionWithLoacalSymbol(), "", false, TagValueListSPtr());
	usedReqid.push(m_reqid++);
	//! [reqmktdata]
	//! [reqmktdata_snapshot]
	m_pClient->reqMktData(m_reqid, ContractSamples::FutureComboContract(), "", true, TagValueListSPtr());
	usedReqid.push(m_reqid++);
	//! [reqmktdata_snapshot]

	//! [reqmktdata_genticks]
	//Requesting RTVolume (Time & Sales), shortable and Fundamental Ratios generic ticks
	m_pClient->reqMktData(m_reqid, ContractSamples::USStock(), "233,236,258", false, TagValueListSPtr());
	usedReqid.push(m_reqid++);
	//! [reqmktdata_genticks]

	//! [reqmktdata_contractnews]
	m_pClient->reqMktData(m_reqid++, ContractSamples::USStock(), "mdoff,292:BZ", false, TagValueListSPtr());
	m_pClient->reqMktData(m_reqid++, ContractSamples::USStock(), "mdoff,292:BT", false, TagValueListSPtr());
	m_pClient->reqMktData(m_reqid++, ContractSamples::USStock(), "mdoff,292:FLY", false, TagValueListSPtr());
	m_pClient->reqMktData(m_reqid++, ContractSamples::USStock(), "mdoff,292:MT", false, TagValueListSPtr());
	//! [reqmktdata_contractnews]
	//! [reqmktdata_broadtapenews]
	m_pClient->reqMktData(m_reqid++, ContractSamples::BTbroadtapeNewsFeed(), "mdoff,292", false, TagValueListSPtr());
	m_pClient->reqMktData(m_reqid++, ContractSamples::BZbroadtapeNewsFeed(), "mdoff,292", false, TagValueListSPtr());
	m_pClient->reqMktData(m_reqid++, ContractSamples::FLYbroadtapeNewsFeed(), "mdoff,292", false, TagValueListSPtr());
	m_pClient->reqMktData(m_reqid++, ContractSamples::MTbroadtapeNewsFeed(), "mdoff,292", false, TagValueListSPtr());
	//! [reqmktdata_broadtapenews]

	//! [reqoptiondatagenticks]
	//Requesting data for an option contract will return the greek values
	m_pClient->reqMktData(m_reqid++, ContractSamples::USOptionContract(), "", false, TagValueListSPtr());
	//! [reqoptiondatagenticks]

	sleep(1);
	/*** Canceling the market data subscription ***/
	//! [cancelmktdata]
	m_pClient->cancelMktData(usedReqid.front());
	usedReqid.pop();
	m_pClient->cancelMktData(usedReqid.front());
	usedReqid.pop();
	m_pClient->cancelMktData(usedReqid.front());
	usedReqid.pop();
	//! [cancelmktdata]

	m_state = ST_TICKDATAOPERATION_ACK;
}

void Client::marketDepthOperations()
{
	/*** Requesting the Deep Book ***/
	//! [reqmarketdepth]
	m_pClient->reqMktDepth(2001, ContractSamples::EurGbpFx(), 5, TagValueListSPtr());
	//! [reqmarketdepth]
	sleep(2);
	/*** Canceling the Deep Book request ***/
	//! [cancelmktdepth]
	m_pClient->cancelMktDepth(2001);
	//! [cancelmktdepth]

	m_state = ST_MARKETDEPTHOPERATION_ACK;
}

void Client::realTimeBars()
{
	/*** Requesting real time bars ***/
	//! [reqrealtimebars]
	m_pClient->reqRealTimeBars(3001, ContractSamples::EurGbpFx(), 5, "MIDPOINT", true, TagValueListSPtr());
	//! [reqrealtimebars]
	sleep(2);
	/*** Canceling real time bars ***/
	//! [cancelrealtimebars]
	m_pClient->cancelRealTimeBars(3001);
	//! [cancelrealtimebars]

	m_state = ST_REALTIMEBARS_ACK;
}

void Client::marketDataType()
{
	//! [reqmarketdatatype]
	/*** Switch to live (1) frozen (2) delayed (3) or delayed frozen (4)***/
	m_pClient->reqMarketDataType(3);
	//! [reqmarketdatatype]

	//m_state = ST_MARKETDATATYPE_ACK;
}

bool Client::getSecDefOptParams(std::string& underlying, int conid, QryFBSeqDefOptParam& qryfb)
{
	DWORD dwRet = 0;
	ResetEvent(g_evt);
	setQryFdBk((void*)(&qryfb));
	m_pClient->reqSecDefOptParams(m_reqid++, underlying, "", "STK", conid); // callback - securityDefinitionOptionalParameter
	// 加上SMART后没有收到信息，抑或为空

	dwRet = ::WaitForSingleObject(g_evt, 5000);
	if (WAIT_OBJECT_0 != dwRet)
	{
		printf("get %s 's option chain failed! please retry sometime ago...\n", underlying.c_str());
		resetQryFdBk();
		return false;
	}
	return true;
};

// 查询成功：有数据正常返回 true
// 查询失败：
bool Client::historicalDataRequests(Contract& cont, const char* queryTime, const std::string& durationStr, const std::string&  barSizeSetting, QryFBHistData& qryfb)
{
#if 0
	/*** Requesting historical data ***/
	//! [reqhistoricaldata]
	std::time_t rawtime;

	char queryTime [80];
	std::tm nowTime;
	std::time(&rawtime);
	localtime_s(&nowTime, &rawtime);
	std::strftime(queryTime, 80, "%Y%m%d %H:%M:%S", &nowTime);

	sprintf_s(queryTime, "%s", "20170608 21:50:00");
	// m_pClient->reqHistoricalData(4011, ContractSamples::EurGbpFx(), queryTime, "1 D", "1 min", "MIDPOINT", 1, 1, TagValueListSPtr()); ok
	//m_pClient->reqHistoricalData(4011, ContractSamples::EurGbpFx(), queryTime, "1 D", "1 min", "MIDPOINT", 1, 1, TagValueListSPtr());
	m_pClient->reqHistoricalData(4012, ContractSamples::USOptionContract(), queryTime, "3600 S", "1 min", "MIDPOINT", 1, 1, TagValueListSPtr());
	//! [reqhistoricaldata]
	sleep(5);
	/*** Canceling historical data requests ***/
	//m_pClient->cancelHistoricalData(4011);
	m_pClient->cancelHistoricalData(4012);
	//m_state = ST_HISTORICALDATAREQUESTS_ACK;
#endif
	// 确定查询历史数据的合约集合
	//ContractSamples::USOptionContract();
	DWORD dwRet = 0;
	int waitTime = 50000;
	if ("OPT" == cont.secType)
	{
		waitTime = 240000;
	}
	ResetEvent(g_evt);
	setQryFdBk((void*)(&qryfb));
	
	m_pClient->reqHistoricalData(m_reqid, ContractSamples::XGPCommodity(), queryTime, durationStr, barSizeSetting, "MIDPOINT", 1, 1, TagValueListSPtr());
	//m_pClient->reqHistoricalData(m_reqid, ContractSamples::EurGbpFx(), queryTime, durationStr, barSizeSetting, "MIDPOINT", 1, 1, TagValueListSPtr());
	// m_pClient->reqHistoricalData(m_reqid++, cont, queryTime, "3600 S", "1 min", "MIDPOINT", 1, 1, TagValueListSPtr());
	
	//m_pClient->reqHistoricalData(m_reqid, cont, queryTime, durationStr, barSizeSetting, "TRADES", 1, 1, TagValueListSPtr());
	//m_pClient->reqHistoricalData(m_reqid, cont, queryTime, durationStr, barSizeSetting, "MIDPOINT", 1, 1, TagValueListSPtr());
	dwRet = ::WaitForSingleObject(g_evt, waitTime);
	if (WAIT_OBJECT_0 != dwRet)
	{
		printf("get %s 's histdata failed! please retry sometime ago...\n", cont.symbol.c_str());
		//sleep(1);
		resetQryFdBk();
		m_reqid++;
		return false;
	}
	m_pClient->cancelHistoricalData(m_reqid++);
	return true;
}

void Client::optionsOperations()
{
	//! [reqsecdefoptparams]
	m_pClient->reqSecDefOptParams(0, "IBM", "", "STK", 8314);
	//! [reqsecdefoptparams]

	//! [calculateimpliedvolatility]
	m_pClient->calculateImpliedVolatility(5001, ContractSamples::NormalOption(), 5, 85);
	//! [calculateimpliedvolatility]

	//** Canceling implied volatility ***
	m_pClient->cancelCalculateImpliedVolatility(5001);

	//! [calculateoptionprice]
	m_pClient->calculateOptionPrice(5002, ContractSamples::NormalOption(), 0.22, 85);
	//! [calculateoptionprice]

	//** Canceling option's price calculation ***
	m_pClient->cancelCalculateOptionPrice(5002);

	//! [exercise_options]
	//** Exercising options ***
	m_pClient->exerciseOptions(5003, ContractSamples::OptionWithTradingClass(), 1, 1, "", 1);
	//! [exercise_options]

	m_state = ST_OPTIONSOPERATIONS_ACK;
}

void Client::contractOperations()
{
	m_pClient->reqContractDetails(209, ContractSamples::EurGbpFx());
	sleep(2);
	//! [reqcontractdetails]
	m_pClient->reqContractDetails(210, ContractSamples::OptionForQuery());
	//! [reqcontractdetails]

	//! [reqcontractdetailsnews]
	m_pClient->reqContractDetails(211, ContractSamples::NewsFeedForQuery());
	//! [reqcontractdetailsnews]

	m_state = ST_CONTRACTOPERATION_ACK;
}

void Client::marketScanners()
{
	/*** Requesting all available parameters which can be used to build a scanner request ***/
	//! [reqscannerparameters]
	m_pClient->reqScannerParameters();
	//! [reqscannerparameters]
	sleep(2);

	/*** Triggering a scanner subscription ***/
	//! [reqscannersubscription]
	m_pClient->reqScannerSubscription(7001, ScannerSubscriptionSamples::HotUSStkByVolume(), TagValueListSPtr());
	//! [reqscannersubscription]

	sleep(2);
	/*** Canceling the scanner subscription ***/
	//! [cancelscannersubscription]
	m_pClient->cancelScannerSubscription(7001);
	//! [cancelscannersubscription]

	m_state = ST_MARKETSCANNERS_ACK;
}

void Client::reutersFundamentals()
{
	/*** Requesting Fundamentals ***/
	//! [reqfundamentaldata]
	m_pClient->reqFundamentalData(8001, ContractSamples::USStock(), "ReportsFinSummary");
	//! [reqfundamentaldata]
	sleep(2);

	/*** Canceling fundamentals request ***/
	//! [cancelfundamentaldata]
	m_pClient->cancelFundamentalData(8001);
	//! [cancelfundamentaldata]

	m_state = ST_REUTERSFUNDAMENTALS_ACK;
}

void Client::bulletins()
{
	/*** Requesting Interactive Broker's news bulletins */
	//! [reqnewsbulletins]
	m_pClient->reqNewsBulletins(true);
	//! [reqnewsbulletins]
	sleep(2);
	/*** Canceling IB's news bulletins ***/
	//! [cancelnewsbulletins]
	m_pClient->cancelNewsBulletins();
	//! [cancelnewsbulletins]

	m_state = ST_BULLETINS_ACK;
}

void Client::accountOperations()
{
	/*** Requesting managed accounts***/
	//! [reqmanagedaccts]
	m_pClient->reqManagedAccts();
	//! [reqmanagedaccts]
	sleep(2);
	/*** Requesting accounts' summary ***/
	//! [reqaaccountsummary]
	m_pClient->reqAccountSummary(9001, "All", AccountSummaryTags::getAllTags());
	//! [reqaaccountsummary]
	sleep(2);
	//! [reqaaccountsummaryledger]
	m_pClient->reqAccountSummary(9002, "All", "$LEDGER");
	//! [reqaaccountsummaryledger]
	sleep(2);
	//! [reqaaccountsummaryledgercurrency]
	m_pClient->reqAccountSummary(9003, "All", "$LEDGER:EUR");
	//! [reqaaccountsummaryledgercurrency]
	sleep(2);
	//! [reqaaccountsummaryledgerall]
	m_pClient->reqAccountSummary(9004, "All", "$LEDGER:ALL");
	//! [reqaaccountsummaryledgerall]
	sleep(2);

	//! [cancelaaccountsummary]
	m_pClient->cancelAccountSummary(9001);
	m_pClient->cancelAccountSummary(9002);
	m_pClient->cancelAccountSummary(9003);
	m_pClient->cancelAccountSummary(9004);
	//! [cancelaaccountsummary]
	sleep(2);
	/*** Subscribing to an account's information. Only one at a time! ***/
	//! [reqaaccountupdates]
	m_pClient->reqAccountUpdates(true, "U150462");
	//! [reqaaccountupdates]
	sleep(2);
	//! [cancelaaccountupdates]
	m_pClient->reqAccountUpdates(false, "U150462");
	//! [cancelaaccountupdates]
	sleep(2);

	//! [reqaaccountupdatesmulti]
	m_pClient->reqAccountUpdatessMulti(9002, "U150462", "EUstocks", true);
	//! [reqaaccountupdatesmulti]
	sleep(2);

	/*** Requesting all accounts' positions. ***/
	//! [reqpositions]
	printf("......reqPositions:......\n");
	m_pClient->reqPositions();
	//! [reqpositions]
	sleep(2);
	//! [cancelpositions]
	m_pClient->cancelPositions();
	//! [cancelpositions]

	//! [reqpositionsmulti]
	m_pClient->reqPositionsMulti(9003, "U150462", "EUstocks");
	//! [reqpositionsmulti]

	m_state = ST_ACCOUNTOPERATIONS_ACK;
}

void Client::orderOperations()
{
	printf("orderOperations called.................................\n");
	/*** Requesting the next valid id ***/
	//! [reqids]
	//The parameter is always ignored.
	m_pClient->reqIds(-1);
	//! [reqids]
	//! [reqallopenorders]
	printf("reqAllOpenOrders......\n");
	sleep(1);
	m_pClient->reqAllOpenOrders();
	//! [reqallopenorders]
	//! [reqautoopenorders]
	printf("reqAutoOpenOrders......\n");
	sleep(1);

	m_pClient->reqAutoOpenOrders(true);
	//! [reqautoopenorders]
	//! [reqopenorders]
	printf("reqOpenOrders......\n");
	sleep(1);
	m_pClient->reqOpenOrders();
	//! [reqopenorders]

	/*** Placing/modifying an order - remember to ALWAYS increment the nextValidId after placing an order so it can be used for the next one! ***/
	//! [order_submission]
	sleep(1);
	printf("place order ........ \n");
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::LimitOrder("SELL", 1, 50));
	sleep(10);
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::LimitOrder("BUY", 1, 100));
	sleep(10);
	//! [order_submission]

	//m_pClient->placeOrder(m_orderId++, ContractSamples::OptionAtBox(), OrderSamples::Block("BUY", 50, 20));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::OptionAtBox(), OrderSamples::BoxTop("SELL", 10));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::FutureComboContract(), OrderSamples::ComboLimitOrder("SELL", 1, 1, false));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::StockComboContract(), OrderSamples::ComboMarketOrder("BUY", 1, false));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::OptionComboContract(), OrderSamples::ComboMarketOrder("BUY", 1, true));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::StockComboContract(), OrderSamples::LimitOrderForComboWithLegPrices("BUY", 1, std::vector<double>(10, 5), true));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::Discretionary("SELL", 1, 45, 0.5));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::OptionAtBox(), OrderSamples::LimitIfTouched("BUY", 1, 30, 34));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::LimitOnClose("SELL", 1, 34));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::LimitOnOpen("BUY", 1, 35));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketIfTouched("BUY", 1, 35));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketOnClose("SELL", 1));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketOnOpen("BUY", 1));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketOrder("SELL", 1));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::MarketToLimit("BUY", 1));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::OptionAtIse(), OrderSamples::MidpointMatch("BUY", 1));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::Stop("SELL", 1, 34.4));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::StopLimit("BUY", 1, 35, 33));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::StopWithProtection("SELL", 1, 45));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::SweepToFill("BUY", 1, 35));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::TrailingStop("SELL", 1, 0.5, 30));
	//m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), OrderSamples::TrailingStopLimit("BUY", 1, 50, 5, 30));

	/*** Cancel all orders for all accounts ***/
	m_pClient->reqGlobalCancel();

	/*** Request the day's executions ***/
	//! [reqexecutions]
	m_pClient->reqExecutions(10001, ExecutionFilter());
	//! [reqexecutions]
	sleep(1);
	m_state = ST_ORDEROPERATIONS_ACK;

	printf(".................................orderOperations called end\n");
}

void Client::ocaSamples()
{
	//OCA ORDER
	//! [ocasubmit]
	std::vector<Order> ocaOrders;
	ocaOrders.push_back(OrderSamples::LimitOrder("BUY", 1, 10));
	ocaOrders.push_back(OrderSamples::LimitOrder("BUY", 1, 11));
	ocaOrders.push_back(OrderSamples::LimitOrder("BUY", 1, 12));
	for (unsigned int i = 0; i < ocaOrders.size(); i++){
		OrderSamples::OneCancelsAll("TestOca", ocaOrders[i], 2);
		m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), ocaOrders[i]);
	}
	//! [ocasubmit]

	m_state = ST_OCASAMPLES_ACK;
}

void Client::conditionSamples()
{
	//! [order_conditioning_activate]
	Order lmt = OrderSamples::LimitOrder("BUY", 100, 10);
	//Order will become active if conditioning criteria is met
	PriceCondition* priceCondition = dynamic_cast<PriceCondition *>(OrderSamples::Price_Condition(208813720, "SMART", 600, false, false));
	ExecutionCondition* execCondition = dynamic_cast<ExecutionCondition *>(OrderSamples::Execution_Condition("EUR.USD", "CASH", "IDEALPRO", true));
	MarginCondition* marginCondition = dynamic_cast<MarginCondition *>(OrderSamples::Margin_Condition(30, true, false));
	PercentChangeCondition* pctChangeCondition = dynamic_cast<PercentChangeCondition *>(OrderSamples::Percent_Change_Condition(15.0, 208813720, "SMART", true, true));
	TimeCondition* timeCondition = dynamic_cast<TimeCondition *>(OrderSamples::Time_Condition("20160118 23:59:59", true, false));
	VolumeCondition* volumeCondition = dynamic_cast<VolumeCondition *>(OrderSamples::Volume_Condition(208813720, "SMART", false, 100, true));

	lmt.conditions.push_back(ibapi::shared_ptr<PriceCondition>(priceCondition));
	lmt.conditions.push_back(ibapi::shared_ptr<ExecutionCondition>(execCondition));
	lmt.conditions.push_back(ibapi::shared_ptr<MarginCondition>(marginCondition));
	lmt.conditions.push_back(ibapi::shared_ptr<PercentChangeCondition>(pctChangeCondition));
	lmt.conditions.push_back(ibapi::shared_ptr<TimeCondition>(timeCondition));
	lmt.conditions.push_back(ibapi::shared_ptr<VolumeCondition>(volumeCondition));
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), lmt);
	//! [order_conditioning_activate]

	//Conditions can make the order active or cancel it. Only LMT orders can be conditionally canceled.
	//! [order_conditioning_cancel]
	Order lmt2 = OrderSamples::LimitOrder("BUY", 100, 20);
	//The active order will be cancelled if conditioning criteria is met
	lmt2.conditionsCancelOrder = true;
	PriceCondition* priceCondition2 = dynamic_cast<PriceCondition *>(OrderSamples::Price_Condition(208813720, "SMART", 600, false, false));
	lmt2.conditions.push_back(ibapi::shared_ptr<PriceCondition>(priceCondition2));
	m_pClient->placeOrder(m_orderId++, ContractSamples::EuropeanStock(), lmt2);
	//! [order_conditioning_cancel]

	m_state = ST_CONDITIONSAMPLES_ACK;
}

void Client::bracketSample(){
	Order parent;
	Order takeProfit;
	Order stopLoss;
	//! [bracketsubmit]
	OrderSamples::BracketOrder(m_orderId++, parent, takeProfit, stopLoss, "BUY", 100, 30, 40, 20);
	m_pClient->placeOrder(parent.orderId, ContractSamples::EuropeanStock(), parent);
	m_pClient->placeOrder(takeProfit.orderId, ContractSamples::EuropeanStock(), takeProfit);
	m_pClient->placeOrder(stopLoss.orderId, ContractSamples::EuropeanStock(), stopLoss);
	//! [bracketsubmit]

	m_state = ST_BRACKETSAMPLES_ACK;
}

void Client::hedgeSample(){
	//F Hedge order
	//! [hedgesubmit]
	//Parent order on a contract which currency differs from your base currency
	Order parent = OrderSamples::LimitOrder("BUY", 100, 10);
	parent.orderId = m_orderId++;
	//Hedge on the currency conversion
	Order hedge = OrderSamples::MarketFHedge(parent.orderId, "BUY");
	//Place the parent first...
	m_pClient->placeOrder(parent.orderId, ContractSamples::EuropeanStock(), parent);
	//Then the hedge order
	m_pClient->placeOrder(m_orderId++, ContractSamples::EurGbpFx(), hedge);
	//! [hedgesubmit]

	m_state = ST_HEDGESAMPLES_ACK;
}

void Client::testAlgoSamples(){
	//! [algo_base_order]
	Order baseOrder = OrderSamples::LimitOrder("BUY", 1000, 1);
	//! [algo_base_order]

	//! [arrivalpx]
	AvailableAlgoParams::FillArrivalPriceParams(baseOrder, 0.1, "Aggressive", "09:00:00 CET", "16:00:00 CET", true, true);
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [arrivalpx]

	//! [darkice]
	AvailableAlgoParams::FillDarkIceParams(baseOrder, 10, "09:00:00 CET", "16:00:00 CET", true);
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [darkice]

	//! [ad]
	AvailableAlgoParams::FillAccumulateDistributeParams(baseOrder, 10, 60, true, true, 1, true, true, "09:00:00 CET", "16:00:00 CET");
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [ad]

	//! [twap]
	AvailableAlgoParams::FillTwapParams(baseOrder, "Marketable", "09:00:00 CET", "16:00:00 CET", true);
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [twap]

	//! [vwap]
	AvailableAlgoParams::FillBalanceImpactRiskParams(baseOrder, 0.1, "Aggressive", true);
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [vwap]

	//! [balanceimpactrisk]
	AvailableAlgoParams::FillBalanceImpactRiskParams(baseOrder, 0.1, "Aggressive", true);
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [balanceimpactrisk]

	//! [minimpact]
	AvailableAlgoParams::FillMinImpactParams(baseOrder, 0.3);
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [minimpact]

	//! [adaptive]
	AvailableAlgoParams::FillAdaptiveParams(baseOrder, "Normal");
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStockAtSmart(), baseOrder);
	//! [adaptive]

	m_state = ST_TESTALGOSAMPLES_ACK;
}

void Client::financialAdvisorOrderSamples()
{
	//! [faorderoneaccount]
	Order faOrderOneAccount = OrderSamples::MarketOrder("BUY", 100);
	// Specify the Account Number directly
	faOrderOneAccount.account = "DU119915";
	m_pClient->placeOrder(m_orderId++, ContractSamples::USStock(), faOrderOneAccount);
	//! [faorderoneaccount]
	sleep(1);

	//! [faordergroupequalquantity]
	Order faOrderGroupEQ = OrderSamples::LimitOrder("SELL", 200, 2000);
	faOrderGroupEQ.faGroup = "Group_Equal_Quantity";
	faOrderGroupEQ.faMethod = "EqualQuantity";
	m_pClient->placeOrder(m_orderId++, ContractSamples::SimpleFuture(), faOrderGroupEQ);
	//! [faordergroupequalquantity]
	sleep(1);

	//! [faordergrouppctchange]
	Order faOrderGroupPC;
	faOrderGroupPC.action = "BUY";
	faOrderGroupPC.orderType = "MKT";
	// You should not specify any order quantity for PctChange allocation method
	faOrderGroupPC.faGroup = "Pct_Change";
	faOrderGroupPC.faMethod = "PctChange";
	faOrderGroupPC.faPercentage = "100";
	m_pClient->placeOrder(m_orderId++, ContractSamples::EurGbpFx(), faOrderGroupPC);
	//! [faordergrouppctchange]
	sleep(1);

	//! [faorderprofile]
	Order faOrderProfile = OrderSamples::LimitOrder("BUY", 200, 100);
	faOrderProfile.faProfile = "Percent_60_40";
	m_pClient->placeOrder(m_orderId++, ContractSamples::EuropeanStock(), faOrderProfile);
	//! [faorderprofile]

	m_state = ST_FAORDERSAMPLES_ACK;
}

void Client::financialAdvisorOperations()
{
	/*** Requesting FA information ***/
	//! [requestfaaliases]
	m_pClient->requestFA(faDataType::ALIASES);
	//! [requestfaaliases]

	//! [requestfagroups]
	m_pClient->requestFA(faDataType::GROUPS);
	//! [requestfagroups]

	//! [requestfaprofiles]
	m_pClient->requestFA(faDataType::PROFILES);
	//! [requestfaprofiles]

	/*** Replacing FA information - Fill in with the appropriate XML string. ***/
	//! [replacefaonegroup]
	m_pClient->replaceFA(faDataType::GROUPS, FAMethodSamples::FAOneGroup());
	//! [replacefaonegroup]

	//! [replacefatwogroups]
	m_pClient->replaceFA(faDataType::GROUPS, FAMethodSamples::FATwoGroups());
	//! [replacefatwogroups]

	//! [replacefaoneprofile]
	m_pClient->replaceFA(faDataType::PROFILES, FAMethodSamples::FAOneProfile());
	//! [replacefaoneprofile]

	//! [replacefatwoprofiles]
	m_pClient->replaceFA(faDataType::PROFILES, FAMethodSamples::FATwoProfiles());
	//! [replacefatwoprofiles]

	m_state = ST_FAOPERATIONS_ACK;
}

void Client::testDisplayGroups(){
	//! [querydisplaygroups]
	m_pClient->queryDisplayGroups(9001);
	//! [querydisplaygroups]

	sleep(1);

	//! [subscribetogroupevents]
	m_pClient->subscribeToGroupEvents(9002, 1);
	//! [subscribetogroupevents]

	sleep(1);

	//! [updatedisplaygroup]
	m_pClient->updateDisplayGroup(9002, "8314@SMART");
	//! [updatedisplaygroup]

	sleep(1);

	//! [subscribefromgroupevents]
	m_pClient->unsubscribeFromGroupEvents(9002);
	//! [subscribefromgroupevents]

	m_state = ST_TICKDATAOPERATION_ACK;
}

void Client::miscelaneous()
{
	/*** Request TWS' current time ***/
	m_pClient->reqCurrentTime();
	/*** Setting TWS logging level  ***/
	m_pClient->setServerLogLevel(5);

	m_state = ST_MISCELANEOUS_ACK;
}

//! [nextvalidid]
void Client::nextValidId(OrderId orderId)
{
	//printf(" Next Valid Id: %ld\n", orderId);
	m_orderId = orderId;
	if (!pCmdThread)
	{
		pCmdThread = new std::thread(cmdProcess, this);
		if (pCmdThread)
		{
			g_evt = CreateEvent(NULL, TRUE, FALSE, NULL);
			printf("Init UIThread ok...\n");
		}
	}
	//! [nextvalidid]
	//m_state = ST_TICKDATAOPERATION;
	//m_state = ST_MARKETDEPTHOPERATION;
	//m_state = ST_REALTIMEBARS;
	//m_state = ST_MARKETDATATYPE;
	//m_state = ST_HISTORICALDATAREQUESTS;
	//m_state = ST_CONTRACTOPERATION;
	//m_state = ST_MARKETSCANNERS;
	//m_state = ST_REUTERSFUNDAMENTALS;
	//m_state = ST_BULLETINS;
	//m_state = ST_ACCOUNTOPERATIONS;
	//if(m_state != ST_ORDEROPERATIONS_ACK) m_state = ST_ORDEROPERATIONS;
	//m_state = ST_OCASAMPLES;
	//m_state = ST_CONDITIONSAMPLES;
	//m_state = ST_BRACKETSAMPLES;
	//m_state = ST_HEDGESAMPLES;
	//m_state = ST_TESTALGOSAMPLES;
	//m_state = ST_FAORDERSAMPLES;
	//m_state = ST_FAOPERATIONS;
	//m_state = ST_DISPLAYGROUPS;
	//m_state = ST_MISCELANEOUS;
	//m_state = ST_PING;
}

void Client::currentTime(long time)
{
	if (m_state == ST_PING_ACK) {
		time_t t = (time_t)time;
		struct tm timeinfo;
		localtime_s(&timeinfo, &t);
		char timestr[100];
		asctime_s<100>(timestr, &timeinfo);
		printf("The current date/time is: %s", timestr);

		time_t now = ::time(NULL);
		m_sleepDeadline = now + SLEEP_BETWEEN_PINGS;

		//m_state = ST_PING_ACK;
	}
}

//! [error]
void Client::error(const int id, const int errorCode, const std::string errorString)
{
	//-----printf( "Error. Id: %d, Code: %d, Msg: %s\n", id, errorCode, errorString.c_str());
}
//! [error]

//! [tickprice]
void Client::tickPrice(TickerId tickerId, TickType field, double price, int canAutoExecute) {
	//printf("Tick Price. Ticker Id: %ld, Field: %d, Price: %g, CanAutoExecute: %d\n", tickerId, (int)field, price, canAutoExecute);
	if (TickType::DELAYED_ASK == field)
	{
		m_curAsk = price;
		printf("ask: %f\t", price);
	}
	if (TickType::DELAYED_BID == field)
	{
		printf("bid: %f\t", price);
		m_curBid = price;
	}
	if (TickType::DELAYED_CLOSE == field)
	{
		printf("last: %f\n", price);
		m_curLast = price;
	}

	if (m_curLast > -1)SetEvent(g_evt);
}
//! [tickprice]

//! [ticksize]
void Client::tickSize(TickerId tickerId, TickType field, int size) {
	//printf( "Tick Size. Ticker Id: %ld, Field: %d, Size: %d\n", tickerId, (int)field, size);
}
//! [ticksize]

//! [tickoptioncomputation]
void Client::tickOptionComputation(TickerId tickerId, TickType tickType, double impliedVol, double delta,
	double optPrice, double pvDividend,
	double gamma, double vega, double theta, double undPrice) {
	//printf( "TickOptionComputation. Ticker Id: %ld, Type: %d, ImpliedVolatility: %g, Delta: %g, OptionPrice: %g, pvDividend: %g, Gamma: %g, Vega: %g, Theta: %g, Underlying Price: %g\n", tickerId, (int)tickType, impliedVol, delta, optPrice, pvDividend, gamma, vega, theta, undPrice);
}
//! [tickoptioncomputation]

//! [tickgeneric]
void Client::tickGeneric(TickerId tickerId, TickType tickType, double value) {
	printf("Tick Generic. Ticker Id: %ld, Type: %d, Value: %g\n", tickerId, (int)tickType, value);
}
//! [tickgeneric]

//! [tickstring]
void Client::tickString(TickerId tickerId, TickType tickType, const std::string& value) {
	printf("Tick String. Ticker Id: %ld, Type: %d, Value: %s\n", tickerId, (int)tickType, value.c_str());
}
//! [tickstring]

void Client::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const std::string& formattedBasisPoints,
	double totalDividends, int holdDays, const std::string& futureLastTradeDate, double dividendImpact, double dividendsToLastTradeDate) {
	printf("TickEFP. %ld, Type: %d, BasisPoints: %g, FormattedBasisPoints: %s, Total Dividends: %g, HoldDays: %d, Future Last Trade Date: %s, Dividend Impact: %g, Dividends To Last Trade Date: %g\n", tickerId, (int)tickType, basisPoints, formattedBasisPoints.c_str(), totalDividends, holdDays, futureLastTradeDate.c_str(), dividendImpact, dividendsToLastTradeDate);
}

//! [orderstatus]
void Client::orderStatus(OrderId orderId, const std::string& status, double filled,
	double remaining, double avgFillPrice, int permId, int parentId,
	double lastFillPrice, int clientId, const std::string& whyHeld){
	printf("\nOrderStatus. \nId: %ld, Status: %s, Filled: %g, Remaining: %g, AvgFillPrice: %g, PermId: %d, LastFillPrice: %g, ClientId: %d, WhyHeld: %s\n", orderId, status.c_str(), filled, remaining, avgFillPrice, permId, lastFillPrice, clientId, whyHeld.c_str());
}
//! [orderstatus]

//! [openorder]
void Client::openOrder(OrderId orderId, const Contract& contract, const Order& order, const OrderState& ostate) {
	printf("\nfunc called: OpenOrder. \nID: %ld, %s, %s @ %s: %s, %s, %g, %f\n", orderId, contract.symbol.c_str(), contract.secType.c_str(), contract.exchange.c_str(), order.action.c_str(), order.orderType.c_str(), order.totalQuantity, order.lmtPrice);
}
//! [openorder]

//! [openorderend]
void Client::openOrderEnd() {
	printf("OpenOrderEnd\n");
}
//! [openorderend]

void Client::winError(const std::string& str, int lastError) {}
void Client::connectionClosed() {
	printf("Connection Closed\n");
}

//! [updateaccountvalue]
void Client::updateAccountValue(const std::string& key, const std::string& val,
	const std::string& currency, const std::string& accountName) {
	printf("UpdateAccountValue. Key: %s, Value: %s, Currency: %s, Account Name: %s\n", key.c_str(), val.c_str(), currency.c_str(), accountName.c_str());
}
//! [updateaccountvalue]

//! [updateportfolio]
void Client::updatePortfolio(const Contract& contract, double position,
	double marketPrice, double marketValue, double averageCost,
	double unrealizedPNL, double realizedPNL, const std::string& accountName){
	printf("UpdatePortfolio. %s, %s @ %s: Position: %g, MarketPrice: %g, MarketValue: %g, AverageCost: %g, UnrealisedPNL: %g, RealisedPNL: %g, AccountName: %s\n", (contract.symbol).c_str(), (contract.secType).c_str(), (contract.primaryExchange).c_str(), position, marketPrice, marketValue, averageCost, unrealizedPNL, realizedPNL, accountName.c_str());
}
//! [updateportfolio]

//! [updateaccounttime]
void Client::updateAccountTime(const std::string& timeStamp) {
	printf("UpdateAccountTime. Time: %s\n", timeStamp.c_str());
}
//! [updateaccounttime]

//! [accountdownloadend]
void Client::accountDownloadEnd(const std::string& accountName) {
	printf("Account download finished: %s\n", accountName.c_str());
}
//! [accountdownloadend]

//! [contractdetails]
void Client::contractDetails(int reqId, const ContractDetails& contractDetails)
{
	//printf( "ConDetails -Strike: %lf, %s, %s, ConId: %ld, conmonth: %s, right: %s\n", contractDetails.summary.strike, contractDetails.summary.symbol.c_str(), contractDetails.summary.secType.c_str(), contractDetails.summary.conId, contractDetails.contractMonth.c_str(),contractDetails.summary.right.c_str());
	double strk = contractDetails.summary.strike;
	auto itfound = std::find_if(m_optchain.begin(), m_optchain.end(), [&strk](OptionChainItem& a)->bool{return eq(a.strike, strk); });
	if (m_optchain.end() == itfound)
	{
		OptionChainItem newitem;
		newitem.strike = strk;
		newitem.ul_symbol = contractDetails.summary.symbol;

		if (contractDetails.summary.right == "P")
		{
			newitem.conid_put = contractDetails.summary.conId;
			newitem.putSymbol = contractDetails.summary.localSymbol;
		}
		if (contractDetails.summary.right == "C")
		{
			newitem.conid_call = contractDetails.summary.conId;
			newitem.callSymbol = contractDetails.summary.localSymbol;
		}
		newitem.multiplier = contractDetails.summary.multiplier;
		m_optchain.push_back(newitem);
	}
	else
	{
		if (contractDetails.summary.right == "P")
		{
			itfound->conid_put = contractDetails.summary.conId;
			itfound->putSymbol = contractDetails.summary.localSymbol;
		}
		if (contractDetails.summary.right == "C")
		{
			itfound->conid_call = contractDetails.summary.conId;
			itfound->callSymbol = contractDetails.summary.localSymbol;
		}
	}
	if (queryFeedBack)static_cast<Contract*>(queryFeedBack)->conId = contractDetails.summary.conId;
}
//! [contractdetails]

void Client::bondContractDetails(int reqId, const ContractDetails& contractDetails) {
	printf("Bond. ReqId: %d, Symbol: %s, Security Type: %s, Currency: %s, Trading Hours: %s, Liquidation Hours: %s\n", reqId, contractDetails.summary.symbol.c_str(), contractDetails.summary.secType.c_str(), contractDetails.summary.currency.c_str(), contractDetails.tradingHours.c_str(), contractDetails.liquidHours.c_str());
}

//! [contractdetailsend]
void Client::contractDetailsEnd(int reqId) {
	printf("ContractDetailsEnd. %d\n", reqId);
	resetQryFdBk();
	SetEvent(g_evt);
}
//! [contractdetailsend]

//! [execdetails]
void Client::execDetails(int reqId, const Contract& contract, const Execution& execution) {
	//printf("ExecDetails. ReqId: %d\n", reqId);
	printf("%l\t", execution.orderId);
	printf("%s\t", contract.secType.c_str());
	printf("%s\t", contract.symbol.c_str());
	printf("%s\t", execution.side.c_str());
	printf("%f\t", execution.price);
	printf("%0.0f\t", execution.shares);
	printf("%f\t", execution.avgPrice);
	printf("%d\t", execution.cumQty);
	printf("%s\t", contract.currency.c_str());
	printf("%s\t\n", execution.execId.c_str());
}
//! [execdetails]

//! [execdetailsend]
void Client::execDetailsEnd(int reqId) {
	//printf( "ExecDetailsEnd. %d\n", reqId);
}
//! [execdetailsend]

//! [updatemktdepth]
void Client::updateMktDepth(TickerId id, int position, int operation, int side,
	double price, int size) {
	printf("UpdateMarketDepth. %ld - Position: %d, Operation: %d, Side: %d, Price: %g, Size: %d\n", id, position, operation, side, price, size);
}
//! [updatemktdepth]

void Client::updateMktDepthL2(TickerId id, int position, std::string marketMaker, int operation,
	int side, double price, int size) {
	printf("UpdateMarketDepthL2. %ld - Position: %d, Operation: %d, Side: %d, Price: %g, Size: %d\n", id, position, operation, side, price, size);
}

//! [updatenewsbulletin]
void Client::updateNewsBulletin(int msgId, int msgType, const std::string& newsMessage, const std::string& originExch) {
	printf("News Bulletins. %d - Type: %d, Message: %s, Exchange of Origin: %s\n", msgId, msgType, newsMessage.c_str(), originExch.c_str());
}
//! [updatenewsbulletin]

//! [managedaccounts]
void Client::managedAccounts(const std::string& accountsList) {
	//printf( "Account List: %s\n", accountsList.c_str());
	acctDispItem.userId = accountsList;
}
//! [managedaccounts]

//! [receivefa]
void Client::receiveFA(faDataType pFaDataType, const std::string& cxml) {
	//std::cout << "Receiving FA: " << (int)pFaDataType << std::endl << cxml << std::endl;
}
//! [receivefa]

//! [historicaldata]
void Client::historicalData(TickerId reqId, const std::string& datetime, double open, double high,
	double low, double close, int volume, int barCount, double WAP, int hasGaps)
{
	//printf("HistoricalData. ReqId: %ld - Date: %s, Open: %g, High: %g, Low: %g, Close: %g, Volume: %d, Count: %d, WAP: %g, HasGaps: %d\n", reqId, datetime.c_str(), open, high, low, close, volume, barCount, WAP, hasGaps);
	if (open < -0.5 && close < -0.5) // 最后一条
	{
		SetEvent(g_evt);
		resetQryFdBk();
		return;
	}
	if (!queryFeedBack)
	{
		printf("HistoricalData. ReqId: %ld - Date: %s, Open: %g, High: %g, Low: %g, Close: %g, Volume: %d, Count: %d, WAP: %g, HasGaps: %d\n", reqId, datetime.c_str(), open, high, low, close, volume, barCount, WAP, hasGaps);
		printf("queryFeedBack NULL...!\n");
		return;
	}
	QryFBHistData& qryfb = *(static_cast<QryFBHistData*>(queryFeedBack));
	KItem kt;
	std::string date, time;
	util::sep_date_time(datetime, date, time);
	sprintf_s<9>(kt.date, "%s", date.c_str());
	kt.date[9] = '\0';
	sprintf_s<9>(kt.time, "%s", time.c_str());
	kt.open = open;	kt.high = high;	kt.low = low; kt.close = close; kt.vol = volume; kt.amount = kt.close*kt.vol;
	qryfb.data.push_back(kt);
	if (qryfb.qrydatetime.length() < 1)qryfb.qrydatetime = datetime;
}
//! [historicaldata]

//! [scannerparameters]
void Client::scannerParameters(const std::string& xml) {
	printf("ScannerParameters. %s\n", xml.c_str());
}
//! [scannerparameters]

//! [scannerdata]
void Client::scannerData(int reqId, int rank, const ContractDetails& contractDetails,
	const std::string& distance, const std::string& benchmark, const std::string& projection,
	const std::string& legsStr) {
	printf("ScannerData. %d - Rank: %d, Symbol: %s, SecType: %s, Currency: %s, Distance: %s, Benchmark: %s, Projection: %s, Legs String: %s\n", reqId, rank, contractDetails.summary.symbol.c_str(), contractDetails.summary.secType.c_str(), contractDetails.summary.currency.c_str(), distance.c_str(), benchmark.c_str(), projection.c_str(), legsStr.c_str());
}
//! [scannerdata]

//! [scannerdataend]
void Client::scannerDataEnd(int reqId) {
	printf("ScannerDataEnd. %d\n", reqId);
}
//! [scannerdataend]

//! [realtimebar]
void Client::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
	long volume, double wap, int count) {
	printf("RealTimeBars. %ld - Time: %ld, Open: %g, High: %g, Low: %g, Close: %g, Volume: %ld, Count: %d, WAP: %g\n", reqId, time, open, high, low, close, volume, count, wap);
}
//! [realtimebar]

//! [fundamentaldata]
void Client::fundamentalData(TickerId reqId, const std::string& data) {
	printf("FundamentalData. ReqId: %ld, %s\n", reqId, data.c_str());
}
//! [fundamentaldata]

void Client::deltaNeutralValidation(int reqId, const UnderComp& underComp) {
	printf("DeltaNeutralValidation. %d, ConId: %ld, Delta: %g, Price: %g\n", reqId, underComp.conId, underComp.delta, underComp.price);
}

//! [ticksnapshotend]
void Client::tickSnapshotEnd(int reqId) {
	//printf( "TickSnapshotEnd: %d\n", reqId);
}
//! [ticksnapshotend]

//! [marketdatatype]
void Client::marketDataType(TickerId reqId, int marketDataType) {
	printf("MarketDataType. ReqId: %ld, Type: %d\n", reqId, marketDataType);
}
//! [marketdatatype]

//! [commissionreport]
void Client::commissionReport(const CommissionReport& commissionReport) {
	//printf( "CommissionReport. %s - %g %s RPNL %g\n", commissionReport.execId.c_str(), commissionReport.commission, commissionReport.currency.c_str(), commissionReport.realizedPNL);
}
//! [commissionreport]

//! [position]
void Client::position(const std::string& account, const Contract& contract, double position, double avgCost) {
	printf("Position. %s - Symbol: %s, SecType: %s, Currency: %s, Position: %g, Avg Cost: %g\n", account.c_str(), contract.symbol.c_str(), contract.secType.c_str(), contract.currency.c_str(), position, avgCost);
}
//! [position]

//! [positionend]
void Client::positionEnd() {
	printf("PositionEnd\n");
}
//! [positionend]

//! [accountsummary]
void Client::accountSummary(int reqId, const std::string& account, const std::string& tag, const std::string& value, const std::string& currency) {
	//printf( "Acct Summary. ReqId: %d, Account: %s, Tag: %s, Value: %s, Currency: %s\n", reqId, account.c_str(), tag.c_str(), value.c_str(), currency.c_str());
	if (account != acctDispItem.userId) return;
	acctDispItem.Currency = currency;
	if (AccountSummaryTags::AccountType == tag) acctDispItem.AccountType = value;
	if (AccountSummaryTags::TotalCashValue == tag) acctDispItem.TotalCashValue = atof(value.c_str());
	if (AccountSummaryTags::GrossPositionValue == tag) acctDispItem.GrossPositionValue = atof(value.c_str());
	if (AccountSummaryTags::NetLiquidation == tag) acctDispItem.NetLiquidation = atof(value.c_str());
	if (AccountSummaryTags::SettledCash == tag) acctDispItem.SettledCash = atof(value.c_str());
}
//! [accountsummary]

//! [accountsummaryend]
void Client::accountSummaryEnd(int reqId) {
	//printf( "AccountSummaryEnd. Req Id: %d\n", reqId);
	SetEvent(g_evt);
}
//! [accountsummaryend]

void Client::verifyMessageAPI(const std::string& apiData) {
	printf("verifyMessageAPI: %s\b", apiData.c_str());
}

void Client::verifyCompleted(bool isSuccessful, const std::string& errorText) {
	printf("verifyCompleted. IsSuccessfule: %d - Error: %s\n", isSuccessful, errorText.c_str());
}

void Client::verifyAndAuthMessageAPI(const std::string& apiDatai, const std::string& xyzChallenge) {
	printf("verifyAndAuthMessageAPI: %s %s\n", apiDatai.c_str(), xyzChallenge.c_str());
}

void Client::verifyAndAuthCompleted(bool isSuccessful, const std::string& errorText) {
	printf("verifyAndAuthCompleted. IsSuccessful: %d - Error: %s\n", isSuccessful, errorText.c_str());
	if (isSuccessful)
		m_pClient->startApi();
}

//! [displaygrouplist]
void Client::displayGroupList(int reqId, const std::string& groups) {
	printf("Display Group List. ReqId: %d, Groups: %s\n", reqId, groups.c_str());
}
//! [displaygrouplist]

//! [displaygroupupdated]
void Client::displayGroupUpdated(int reqId, const std::string& contractInfo) {
	printf("Display Group Updated. ReqId: %d, Contract Info: %s\n", reqId, contractInfo);
}
//! [displaygroupupdated]

//! [positionmulti]
void Client::positionMulti(int reqId, const std::string& account, const std::string& modelCode, const Contract& contract, double pos, double avgCost) {
	printf("Position Multi. Request: %d, Account: %s, ModelCode: %s, Symbol: %s, SecType: %s, Currency: %s, Position: %g, Avg Cost: %g\n", reqId, account.c_str(), modelCode.c_str(), contract.symbol.c_str(), contract.secType.c_str(), contract.currency.c_str(), pos, avgCost);
}
//! [positionmulti]

//! [positionmultiend]
void Client::positionMultiEnd(int reqId) {
	printf("Position Multi End. Request: %d\n", reqId);
}
//! [positionmultiend]

//! [accountupdatemulti]
void Client::accountUpdateMulti(int reqId, const std::string& account, const std::string& modelCode, const std::string& key, const std::string& value, const std::string& currency) {
	printf("AccountUpdate Multi. Request: %d, Account: %s, ModelCode: %s, Key, %s, Value: %s, Currency: %s\n", reqId, account.c_str(), modelCode.c_str(), key.c_str(), value.c_str(), currency.c_str());
}
//! [accountupdatemulti]

//! [accountupdatemultiend]
void Client::accountUpdateMultiEnd(int reqId) {
	printf("Account Update Multi End. Request: %d\n", reqId);
}
//! [accountupdatemultiend]

//! [securityDefinitionOptionParameter]
void Client::securityDefinitionOptionalParameter(int reqId, const std::string& exchange, int underlyingConId, const std::string& tradingClass, const std::string& multiplier, std::set<std::string> expirations, std::set<double> strikes)
{
	if (!queryFeedBack)
	{
		return;
	}

	QryFBSeqDefOptParam* qryfb = ((QryFBSeqDefOptParam*)queryFeedBack);
	qryfb->Exchanges.push_back(exchange);
	if (qryfb->Expirations.size() == 0)qryfb->Expirations.assign(expirations.begin(), expirations.end());
	if (qryfb->Strikes.size() == 0)qryfb->Strikes.assign(strikes.begin(), strikes.end());
	qryfb->multiplier = multiplier;
	printf("Security Definition Optional Parameter. Exchange : %s\n", exchange.c_str());
	printf("Request: %d, Trading Class : %s, Multiplier : %s, UnderlyingConId : %d\n", reqId, tradingClass.c_str(), multiplier.c_str(), underlyingConId);
	return;
	int size = strikes.size();
	printf("strikes size: %d\n", size);
	size = expirations.size();

	for (auto its = strikes.begin(); its != strikes.end(); ++its)
	{
		printf("strikes: %f\t", *its);
	}
	printf("\n");

	printf("expirations size: %d\n", size);
	for (auto ite = expirations.begin(); ite != expirations.end(); ++ite)
	{
		printf("expirations: %s\t", ite->c_str());
	}
	printf("\n");
}
//! [securityDefinitionOptionParameter]

//! [securityDefinitionOptionParameterEnd]
void Client::securityDefinitionOptionalParameterEnd(int reqId) {
	printf("Security Definition Optional Parameter End. Request: %d\n", reqId);
	resetQryFdBk();
	SetEvent(g_evt);
}
//! [securityDefinitionOptionParameterEnd]

//! [softDollarTiers]
void Client::softDollarTiers(int reqId, const std::vector<SoftDollarTier> &tiers) {
	printf("Soft dollar tiers (%d):", tiers.size());

	for (int i = 0; i < tiers.size(); i++) {
		printf("%s\n", tiers[0].displayName());
	}
}
//! [softDollarTiers]

//! [My Test cmd 1]
void Client::queryLatestTrades()
{
	ExecutionFilter execFilter;
	execFilter.m_clientId = 0;
	//execFilter.m_secType = "STK";
	//execFilter.m_exchange = "SMART";
	printf("\n\nall executed orders: \n");
	m_pClient->reqExecutions(m_reqid++, execFilter);
	printf("orderId\tsecType\tsymbol\tside\tprice\texec shares\tavg price\tcumqty\tcurrency\texecid: \t\n");
	sleep(2);

	// cancel all sended test orders
	//m_pClient->reqGlobalCancel();
}
//! [My Test cmd 1]

//! [My Test cmd 2]
void Client::queryAccountInfo()
{
	//printf("My test operation: queryAccountInfo......\n");
	m_pClient->reqManagedAccts();
	sleep(1);
	std::string tags = AccountSummaryTags::AccountType;
	tags += (",");
	tags += (AccountSummaryTags::TotalCashValue);
	tags += (",");
	tags += (AccountSummaryTags::GrossPositionValue);
	tags += (",");
	tags += (AccountSummaryTags::NetLiquidation);
	tags += (",");
	tags += (AccountSummaryTags::Leverage);
	tags += (",");
	tags += (AccountSummaryTags::SettledCash);
	ResetEvent(g_evt);
	m_pClient->reqAccountSummary(m_reqid, "All", tags);
	DWORD dwRet = ::WaitForSingleObject(g_evt, 5000);
	if (WAIT_OBJECT_0 != dwRet)
	{
		printf("Query account info overtime...\n");
	}
	acctDispItem.disp();
	m_pClient->cancelAccountSummary(m_reqid++);
}
//! [My Test cmd 2]

//! [My Test cmd 3]
void Client::getOptionPrice(const std::string& StkId, const std::string& expiredate, const double strike, const std::string& OpType)
{
	//tickDataOperation();
	//return;
	////////////////////
	Contract contract;
	contract.symbol = StkId;
	contract.secType = "OPT";
	contract.exchange = "SMART";
	contract.currency = "USD";
	contract.lastTradeDateOrContractMonth = expiredate;
	contract.strike = strike;
	contract.right = OpType;
	//contract.multiplier = "100";
	m_curAsk = 0; m_curBid = 0; m_curLast = 0;
	ResetEvent(g_evt);
	m_pClient->reqMktData(m_reqid, contract, "", true, TagValueListSPtr());
	DWORD dwRet = ::WaitForSingleObject(g_evt, 5000);
	if (WAIT_OBJECT_0 != dwRet)
	{
		printf("get tikc price failed!\n");
	}
	printf("mid price: %f\n", (m_curAsk + m_curBid) / 2);
	m_pClient->cancelMktData(m_reqid++);
}
//! [My Test cmd 3]

//! [My Test cmd 1]
/*
给定标的SPY，给定到期日和一个V值，查出该到期日所有期权中相同行权价Put和Call价格相加
大于等于V的所有期权合约和当前价格
*/
void Client::getOptionsByCond0(const std::string& stkid, const long conid, const std::string& expiredate, const double theV)
{
	//contract.secType = "OPT";
	//m_pClient->reqSecDefOptParams(110, stkid, "", "STK", conid); // callback - securityDefinitionOptionalParameter
	Contract contract;
	contract.symbol = stkid;
	contract.secType = "OPT";
	contract.exchange = "SMART";
	contract.currency = "USD";
	contract.lastTradeDateOrContractMonth = expiredate;
	contract.multiplier = "";
	contract.right = "";
	contract.strike = 0;
	if (m_optchain.size() > 0)m_optchain.clear();
	//ResetEvent(g_evt);
	m_pClient->reqContractDetails(m_reqid++, contract);
	//::WaitForSingleObject(g_evt, INFINITE);
	sleep(3);

	DWORD dwRet = 0;
	contract.symbol = "";
	for (auto it = m_optchain.begin(); it != m_optchain.end(); ++it)
	{
		contract.strike = it->strike;
		contract.right = "P";
		contract.conId = it->conid_put;
		contract.multiplier = it->multiplier;
		m_curAsk = 0; m_curBid = 0; m_curLast = -1;
		ResetEvent(g_evt);
		m_pClient->reqMktData(m_reqid, contract, "", true, TagValueListSPtr());
		dwRet = ::WaitForSingleObject(g_evt, 5000);
		if (WAIT_OBJECT_0 != dwRet)
		{
			printf("get tikc price failed!\n");
			return;
		}
		m_pClient->cancelMktData(m_reqid++);
		it->put_price = (m_curAsk > 0 ? (m_curAsk + m_curBid) / 2 : m_curLast);

		contract.right = "C";
		contract.conId = it->conid_call;
		m_curAsk = 0; m_curBid = 0; m_curLast = -1;
		ResetEvent(g_evt);
		m_pClient->reqMktData(m_reqid, contract, "", true, TagValueListSPtr());
		dwRet = ::WaitForSingleObject(g_evt, 5000);
		if (WAIT_OBJECT_0 != dwRet)
		{
			printf("get tikc price failed!\n");
			return;
		}
		m_pClient->cancelMktData(m_reqid++);
		it->call_price = (m_curAsk > 0 ? (m_curAsk + m_curBid) / 2 : m_curLast);

		it->sum = it->put_price + it->call_price;
	}

	// sum
	for (auto it = m_optchain.begin(); it != m_optchain.end();)
	{
		if (it->sum < theV)
		{
			it = m_optchain.erase(it);
		}
		else
		{
			++it;
		}
	}

	// disp
	printf("options that put price + call price > %f:\n", theV);
	for (auto it = m_optchain.begin(); it != m_optchain.end(); ++it)
	{
		printf("%s  strike: %lf call: %lf  %s  put: %lf  put+call:%lf\n", it->callSymbol.c_str(), it->strike, it->call_price, it->putSymbol.c_str(), it->put_price, it->call_price + it->put_price);
	}
}
//! [My Test cmd 4]

void Client::quit()
{
	m_state = ST_QUIT;
}

long Client::getConId(const std::string& StkId, const std::string& sectype)
{
	Contract contract; // for details req
	contract.strike = 0;
	contract.multiplier = "";
	contract.conId = 0;
	contract.symbol = StkId;
	contract.secType = sectype;
	contract.exchange = "SMART";
	contract.currency = "USD";
	ResetEvent(g_evt);
	setQryFdBk((void*)(&contract));
	m_pClient->reqContractDetails(m_reqid++, contract);
	DWORD dwRet = ::WaitForSingleObject(g_evt, 5000);
	if (WAIT_OBJECT_0 != dwRet)
	{
		printf("getConId failed!\n");
		resetQryFdBk();
		return -1;
	}
	return contract.conId;
}

void Client::GetContractsByExpiration(const std::string& udly, const std::string& exp)
{
	Contract contract;
	contract.symbol = udly;
	contract.secType = "OPT";
	contract.exchange = "SMART";
	contract.currency = "USD";
	contract.lastTradeDateOrContractMonth = exp;
	contract.multiplier = "";
	contract.right = "";
	contract.strike = 0;
	if (m_optchain.size() > 0)m_optchain.clear();
	ResetEvent(g_evt);
	m_pClient->reqContractDetails(m_reqid++, contract); // 存入
	DWORD dwRet = ::WaitForSingleObject(g_evt, 5000);
	if (WAIT_OBJECT_0 != dwRet)
	{
		printf("getConId failed!\n");
		resetQryFdBk();
		return;
	}
}
