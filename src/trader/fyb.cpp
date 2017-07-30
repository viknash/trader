#include "stdafx.h"
#include "fyb.h"
#include "fybapi.h"
#include "fybconfig.h"
#include "fybdatabase.h"
#include "helper.h"

using namespace Poco::Data::Keywords;
using Poco::Data::Session;
using Poco::Data::Statement;
using Poco::Timer;
using Poco::TimerCallback;
using Poco::Thread;
using Poco::Stopwatch;
using Poco::Data::TypeHandler;
using Poco::Data::AbstractBinder;
using Poco::Data::AbstractExtractor;
using Poco::Data::AbstractPreparator;
using Poco::AutoPtr;
using namespace std;
using namespace Poco;

namespace trader {

	Fyb::Fyb(Poco::AutoPtr<trader::App> _app)
		: fybApi(_app, this)
		, app(_app)
		, dataBase(new FybDatabase(_app->dbSession))
		, executeTimer(0, 1000)
	{
	}

	void Fyb::run()
	{
		dataBase->init();
		//dataBase->clear();

		using std::placeholders::_1;
		serialExecutionList.push_back(std::bind(&Fyb::executeTickerDetailed, this, _1));
		serialExecutionList.push_back(std::bind(&Fyb::executeAccountInfo, this, _1));
		serialExecutionList.push_back(std::bind(&Fyb::executeTradeHistory, this, _1));
		serialExecutionList.push_back(std::bind(&Fyb::executeOrderBook, this, _1));
		serialExecutionList.push_back(std::bind(&Fyb::executePendingOrders, this, _1));
		serialExecutionList.push_back(std::bind(&Fyb::executeOrderHistory, this, _1));
		executeTimer.start(TimerCallback<Fyb>(*this, &Fyb::execute));
	}

	void Fyb::execute(Timer& timer)
	{
		static std::atomic_int32_t idx = 0;
		serialExecutionList[ idx % serialExecutionList.size() ](timer);
		++idx;
	}

	void Fyb::executeTickerDetailed(Timer& timer)
	{
		(void)timer;
		Poco::AutoPtr<TickerDetailed> tickerDetailedData = fybApi.GetTickerDetailed();

		Ticker_Detailed::Record rec;
		rec.timeStamp = (Poco::Int32)std::time(nullptr);
		rec.ask = tickerDetailedData->dataObject.ask;
		rec.bid = tickerDetailedData->dataObject.bid;
		rec.last = tickerDetailedData->dataObject.last;
		rec.vol = tickerDetailedData->dataObject.vol;

		dataBase->ticker_DetailedTable->insertAndDeleteUnchanged(rec);
	}

	void Fyb::executeAccountInfo(Timer& timer)
	{
		(void)timer;
		Poco::AutoPtr<AccountInfo> accountInfo = fybApi.GetAccountInfo();
		
		Account_Balance::Record rec;
		rec.timeStamp = (Poco::Int32)std::time(nullptr);
		rec.sgdBal = accountInfo->dataObject.sgdBal;
		rec.btcBal = accountInfo->dataObject.btcBal;

		dataBase->account_BalanceTable->insertAndDeleteUnchanged(rec);

		std::ostringstream accountInfoStream;
		accountInfoStream << "FYB" << accountInfo->dataObject.accNo;

		Account_Info::Record recInfo;
		recInfo.accNum = accountInfoStream.str();
		recInfo.btcAddress = accountInfo->dataObject.btcDeposit;
		recInfo.email = accountInfo->dataObject.email;

		dataBase->account_InfoTable->insertOnce(recInfo);
	}

	void Fyb::executeTradeHistory(Timer& timer)
	{
		(void)timer;

		Trade_History::RecordWithId currentRec;
		dataBase->trade_HistoryTable->getLatest(currentRec);

		AutoPtr<TradesParams> tradesParams = new TradesParams();
		if (currentRec.isSetTid())
		{
			tradesParams->dataObject.SetSince(currentRec.tid);
		}

		AutoPtr<Trades> trades = fybApi.GetTrades(tradesParams);

		std::vector<Trade_History::Record> tradeHistoryRecord;
		for (auto& trade : trades->data)
		{
			Trade_History::Record rec;
			rec.amt = trade.amount;
			rec.date = (Poco::Int32)trade.date;
			rec.price = trade.price;
			rec.tid = trade.tid;
			tradeHistoryRecord.push_back(rec);
		}
		dataBase->trade_HistoryTable->insertMultipleUnique(tradeHistoryRecord);
	}

	template <typename RECORDTYPE> void findMissing(std::vector<RECORDTYPE>& orders, std::vector<RECORDTYPE>& records, std::vector<RECORDTYPE>& missingRecords, bool ascending = true)
	{
		Int32 lastDatabaseSearchIdx = 0;
		for (auto& order : orders)
		{
			bool add = true;
			for (Int32 i = lastDatabaseSearchIdx; i < records.size(); i++)
			{
				auto& record = records[i];
				if (record.price == order.price && record.volume == order.volume)
				{
					add = false;
					break;
				}
				if (ascending)
				{
					if (record.price > order.price)
						break;
				}
				else
				{
					if (record.price < order.price)
						break;
				}
			}
			if (add)
			{
				RECORDTYPE missingRecord;
				missingRecord.price = order.price;
				missingRecord.volume = order.volume;
				missingRecords.push_back(missingRecord);
			}
		}
	}

	void Fyb::executeOrderBook(Timer& timer)
	{
		(void)timer;
		Poco::AutoPtr<OrderBook> orderBook = fybApi.GetOrderBook();
		time_t currentDateTime = std::time(nullptr);

		if (orderBook->dataObject.asks.empty())
			return;

		std::vector<Order_Book_Asks::RecordWithId> latestAsks;
		dataBase->order_Book_AsksTable->getAll(latestAsks, "DateRemoved = 0");
		if (!latestAsks.size()
			|| !latestAsks[0].isSetPrice()
			|| !latestAsks[0].isSetVolume()
			|| !equal<double>(orderBook->dataObject.asks[0][0], latestAsks[0].price)
			|| !equal<double>(orderBook->dataObject.asks[0][1], latestAsks[0].volume))
		{
			std::vector<Order_Book_Asks::RecordWithId> newAsks;
			for (auto& ask : orderBook->dataObject.asks)
			{
				Order_Book_Asks::RecordWithId askRecord;
				askRecord.price = ask[0];
				askRecord.volume = ask[1];
				newAsks.push_back(askRecord);
			}

			//Find ask prices in exchange that is not available in records and add it
			std::vector<Order_Book_Asks::RecordWithId> missingRecords;
			findMissing<Order_Book_Asks::RecordWithId>(newAsks, latestAsks, missingRecords);
			for (auto& record : missingRecords) { record.dateCreated = (Int32)currentDateTime; record.dateRemoved = 0; }
			dataBase->order_Book_AsksTable->insertMultiple(missingRecords);

			missingRecords.clear();
			findMissing<Order_Book_Asks::RecordWithId>(latestAsks, newAsks, missingRecords);
			dataBase->order_Book_AsksTable->deleteMultiple(missingRecords);
			for (auto& record : missingRecords) { record.dateRemoved = (Int32)currentDateTime; }
			dataBase->order_Book_AsksTable->insertMultiple(missingRecords);
		}

		if (orderBook->dataObject.bids.empty())
			return;

		std::vector<Order_Book_Bids::RecordWithId> latestBids;
		dataBase->order_Book_BidsTable->getAll(latestBids, "DateRemoved = 0");
		if (!latestBids.size()
			|| !latestBids[0].isSetPrice()
			|| !latestBids[0].isSetVolume()
			|| !equal<double>(orderBook->dataObject.bids[0][0], latestBids[0].price)
			|| !equal<double>(orderBook->dataObject.bids[0][1], latestBids[0].volume))
		{
			std::vector<Order_Book_Bids::RecordWithId> newBids;
			for (auto& bid : orderBook->dataObject.bids)
			{
				Order_Book_Bids::RecordWithId bidRecord;
				bidRecord.price = bid[0];
				bidRecord.volume = bid[1];
				newBids.push_back(bidRecord);
			}

			//Find ask prices in exchange that is not available in records and add it
			std::vector<Order_Book_Bids::RecordWithId> missingRecords;
			findMissing<Order_Book_Bids::RecordWithId>(newBids, latestBids, missingRecords, false);
			for (auto& record : missingRecords) { record.dateCreated = (Int32)currentDateTime; record.dateRemoved = 0; }
			dataBase->order_Book_BidsTable->insertMultiple(missingRecords);

			missingRecords.clear();
			findMissing<Order_Book_Bids::RecordWithId>(latestBids, newBids, missingRecords, false);
			dataBase->order_Book_BidsTable->deleteMultiple(missingRecords);
			for (auto& record : missingRecords) { record.dateRemoved = (Int32)currentDateTime; }
			dataBase->order_Book_BidsTable->insertMultiple(missingRecords);
		}
		
	}

	void Fyb::executePendingOrders(Poco::Timer& timer)
	{
		(void)timer;
		Poco::AutoPtr<PendingOrders> orderBook = fybApi.GetPendingOrders();

		if (orderBook->dataObject.isSetError() && orderBook->dataObject.error != 0)
		{
			return;
		}

		std::vector<My_Pending_Buy_Orders::Record> buyRecords;
		std::vector<My_Pending_Sell_Orders::Record> sellRecords;
		for (auto& order : orderBook->dataObject.orders)
		{
			if (order.type == 'B')
			{
				My_Pending_Buy_Orders::Record rec;
				rec.amt = order.qty;
				rec.date = (Int32)order.date;
				rec.price = order.price;
				rec.ticket = order.ticket;
				buyRecords.push_back(rec);
			}
			else if (order.type == 'S')
			{
				My_Pending_Sell_Orders::Record rec;
				rec.amt = order.qty;
				rec.date = (Int32)order.date;
				rec.price = order.price;
				rec.ticket = order.ticket;
				sellRecords.push_back(rec);
			}
		}

		dataBase->my_Pending_Buy_OrdersTable->clear();
		dataBase->my_Pending_Sell_OrdersTable->clear();

		if (!buyRecords.empty())
			dataBase->my_Pending_Buy_OrdersTable->insertMultipleUnique(buyRecords);

		if (!sellRecords.empty())
			dataBase->my_Pending_Sell_OrdersTable->insertMultipleUnique(sellRecords);
	}

	void Fyb::executeOrderHistory(Poco::Timer& timer)
	{
		(void)timer;
		Poco::AutoPtr<OrderHistoryParams> orderHistoryParams = new OrderHistoryParams();
		orderHistoryParams->dataObject.SetLimit(100);
		Poco::AutoPtr<OrderHistory> orderHistory = fybApi.GetOrderHistory(orderHistoryParams);

		if (orderHistory->dataObject.isSetError() && orderHistory->dataObject.error != 0)
		{
			return;
		}

		My_Trade_History::RecordWithId latestRecord;
		dataBase->my_Trade_HistoryTable->getLatest(latestRecord);
		if (!latestRecord.isSetTicket())
		{
			latestRecord.ticket = 0;
		}

		std::vector<My_Trade_History::Record> records;
		for (auto& order : orderHistory->dataObject.orders)
		{
			if (order.ticket > latestRecord.ticket)
			{
				My_Trade_History::Record record;
				record.dateCreated = (Int32)order.date_created;
				record.dateExecuted = (Int32)order.date_executed;
				string priceStr = remove_non_digits(order.price);
				record.price = stod(priceStr);
				string qtyStr = remove_non_digits(order.qty);
				record.qty = stod(qtyStr);
				record.status = order.status;
				record.type = order.type;
				record.ticket = order.ticket;
				records.push_back(record);
			}
		}
		dataBase->my_Trade_HistoryTable->insertMultipleUnique(records);
	}


	Fyb::~Fyb()
	{
	}

	Poco::Dynamic::Var Fyb::invoke(const std::string& httpMethod, Poco::URI& uri)
	{
		static Poco::FastMutex lock;
		static time_t lastTimeStamp = 0;
		time_t currentTimeStamp;
		if (lock.tryLock())
		{
			currentTimeStamp = std::time(nullptr);
			if (lastTimeStamp == currentTimeStamp)
			{
				throw TimeoutException("Fyb Error : API executed too fast", "");
			}
			lastTimeStamp = currentTimeStamp;
			lock.unlock();
		}
		else
		{
			throw TimeoutException("Fyb Error : API executed too fast", "");
		}

		//Add timestamp
		std::ostringstream timeString;
		timeString << currentTimeStamp;
		uri.addQueryParameter(std::string("timestamp"), timeString.str());

		// Create the request URI.
		Poco::Net::HTTPSClientSession session(uri.getHost(), uri.getPort());
		Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, uri.getPathAndQuery(), Poco::Net::HTTPMessage::HTTP_1_1);
		req.add("Accept-Encoding", "gzip");

		// Sign request
		if (httpMethod == Poco::Net::HTTPRequest::HTTP_POST)
		{
			//Convert to POST
			Poco::Net::HTMLForm form(req);
			req.setMethod(Poco::Net::HTTPRequest::HTTP_POST);
			req.setURI(uri.getPath());
			form.setEncoding(Poco::Net::HTMLForm::ENCODING_URL);

			//Key
			req.set("key", fybApi.config.dataObject.consumer_key);

			//Sign
			std::ostringstream paramStream;
			form.write(paramStream);
			std::string signatureBase = paramStream.str();
			std::string signingKey = fybApi.config.dataObject.consumer_secret;
			Poco::HMACEngine<Poco::SHA1Engine> hmacEngine(signingKey);
			hmacEngine.update(signatureBase);
			Poco::DigestEngine::Digest digest = hmacEngine.digest();
			std::ostringstream digestHexBin;
			Poco::HexBinaryEncoder hexBinEncoder(digestHexBin);
			hexBinEncoder.write(reinterpret_cast<char*>(&digest[0]), digest.size());
			hexBinEncoder.close();
			req.set("sig", digestHexBin.str());

			//Submit
			form.prepareSubmit(req);
			std::ostream& ostr = session.sendRequest(req);
			form.write(ostr);
		}
		else
		{
			session.sendRequest(req);
		}

		Poco::Logger::get("Logs").information("Send Request: %s", uri.toString());

		// Receive the response.
		Poco::Net::HTTPResponse res;
		std::istream& rs = session.receiveResponse(res);

		Poco::InflatingInputStream inflater(rs, Poco::InflatingStreamBuf::STREAM_GZIP);
		// Parse the JSON
		Poco::JSON::Parser parser;
		parser.parse(inflater);
		Poco::Dynamic::Var result = parser.result();
		std::string resultString = Poco::Dynamic::Var::toString(result);
		Poco::Logger::get("Logs").information("Received Response: %s", resultString);

		// If everything went fine, return the JSON document.
		// Otherwise throw an exception.
		if (res.getStatus() == Poco::Net::HTTPResponse::HTTP_OK)
		{
			return result;
		}
		else
		{
			throw ApplicationException("Fyb Error", "");
		}
	}
}
