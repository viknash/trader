///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// <copyright file="fyb.cpp" company="FinSec Systems">
// Copyright (c) 2018 finsec.systems. All rights reserved.
// </copyright>
// <author>Viknash</author>
// <date>12/5/2018</date>
// <summary>FYB Api connection implementation</summary>
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "fyb.h"
#include "api.h"
#include "fybconfig.h"
#include "fybdatabase.h"
#include "helper.h"

namespace trader
{

    using namespace FybApi;
    using namespace FybDatabase;

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	/// <summary> Sets the parameters. </summary>
	///
	/// <exception cref="Poco::NotFoundException"> Thrown when the requested element is not present. </exception>
	///
	/// <param name="paramString"> The parameter string. </param>
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	void Fyb::setParams(const std::string& paramString)
	{
		for (UInt32 idx = 0; idx < api.config.data.size(); ++idx)
		{
			if (paramString.compare(api.config.data[idx].name) == 0)
			{
				configurationIdx = idx;
				return;
			}
		}
		throw Poco::NotFoundException("Fyb Error: Params not found in config.json", paramString);
	}

    /// <summary> Initializes a new instance of the Fyb class. </summary>
    Fyb::Fyb()
        : api(AppManager::instance.getApp(), this)
        , executeTimer(0, 1000)
        , dataBase(new FybDatabase::Tables(DbManager::instance.getDb()->getDbSession()))
    {
    }

    /// <summary> Runs this object. </summary>
    void Fyb::run()
    {
        dataBase->init();
        // dataBase->clear();

        serialExecutionList.push_back(std::bind(&Fyb::executeTradeHistory, this, _1));
        serialExecutionList.push_back(std::bind(&Fyb::executeOrderBook, this, _1));
        serialExecutionList.push_back(std::bind(&Fyb::executePendingOrders, this, _1));
        serialExecutionList.push_back(std::bind(&Fyb::executeOrderHistory, this, _1));
        executeTimer.start(TimerCallback< Fyb >(*this, &Fyb::execute));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary> Executes the given timer. </summary>
    ///
    /// <param name="timer"> [in,out] The timer. </param>
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Fyb::execute(Timer &timer)
    {
        static std::atomic< std::int32_t > idx;
        idx = 0;
        serialExecutionList[idx % serialExecutionList.size()](timer);
        ++idx;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary> Executes the trade history operation. </summary>
    ///
    /// <param name="timer"> [in,out] The timer. </param>
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Fyb::executeTradeHistory(Timer &timer)
    {
        (void)timer;

        Trade_History::RecordWithId currentRec;
        dataBase->trade_HistoryTable->getLatest(currentRec);

        AutoPtr< TradesParams > tradesParams = new TradesParams();
        if (currentRec.isSetTid())
        {
            tradesParams->dataObject.SetSince(currentRec.tid);
        }

        AutoPtr< Trades > trades = api.GetTrades(tradesParams);

        std::vector< Trade_History::Record > tradeHistoryRecord;
        for (auto &trade : trades->data)
        {
            Trade_History::Record rec;
            rec.amt = trade.amount;
            rec.date = (Int32)trade.date;
            rec.price = trade.price;
            rec.tid = trade.tid;
            tradeHistoryRecord.push_back(rec);
        }
        dataBase->trade_HistoryTable->insertMultipleUnique(tradeHistoryRecord);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary> Searches for the first missing. </summary>
    ///
    /// <typeparam name="RECORDTYPE"> Type of the recordtype. </typeparam>
    /// <param name="orders">		  [in,out] The orders. </param>
    /// <param name="records">		  [in,out] The records. </param>
    /// <param name="missingRecords"> [in,out] The missing records. </param>
    /// <param name="ascending">	  (Optional) True to ascending. </param>
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    template < typename RECORDTYPE >
    void findMissing(std::vector< RECORDTYPE > &orders, std::vector< RECORDTYPE > &records,
                     std::vector< RECORDTYPE > &missingRecords, bool ascending = true)
    {
        Int32 lastDatabaseSearchIdx = 0;
        for (auto &order : orders)
        {
            bool add = true;
            for (Int32 i = lastDatabaseSearchIdx; i < records.size(); i++)
            {
                auto &record = records[i];
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

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary> Executes the order book operation. </summary>
    ///
    /// <param name="timer"> [in,out] The timer. </param>
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Fyb::executeOrderBook(Timer &timer)
    {
        (void)timer;
        AutoPtr< OrderBook > orderBook = api.GetOrderBook();
        time_t currentDateTime = std::time(nullptr);

        if (orderBook->dataObject.asks.empty())
            return;

        std::vector< Order_Book_Asks::RecordWithId > latestAsks;
        dataBase->order_Book_AsksTable->getAll(latestAsks, "DateRemoved = 0");
        if (!latestAsks.size() || !latestAsks[0].isSetPrice() || !latestAsks[0].isSetVolume() ||
            !equal< double >(orderBook->dataObject.asks[0][0], latestAsks[0].price) ||
            !equal< double >(orderBook->dataObject.asks[0][1], latestAsks[0].volume))
        {
            std::vector< Order_Book_Asks::RecordWithId > newAsks;
            for (auto &ask : orderBook->dataObject.asks)
            {
                Order_Book_Asks::RecordWithId askRecord;
                askRecord.price = ask[0];
                askRecord.volume = ask[1];
                newAsks.push_back(askRecord);
            }

            // Find ask prices in exchange that is not available in records and add it
            std::vector< Order_Book_Asks::RecordWithId > missingRecords;
            findMissing< Order_Book_Asks::RecordWithId >(newAsks, latestAsks, missingRecords);
            for (auto &record : missingRecords)
            {
                record.dateCreated = (Int32)currentDateTime;
                record.dateRemoved = 0;
            }
            dataBase->order_Book_AsksTable->insertMultiple(missingRecords);

            missingRecords.clear();
            findMissing< Order_Book_Asks::RecordWithId >(latestAsks, newAsks, missingRecords);
            dataBase->order_Book_AsksTable->deleteMultiple(missingRecords);
            for (auto &record : missingRecords)
            {
                record.dateRemoved = (Int32)currentDateTime;
            }
            dataBase->order_Book_AsksTable->insertMultiple(missingRecords);
        }

        if (orderBook->dataObject.bids.empty())
            return;

        std::vector< Order_Book_Bids::RecordWithId > latestBids;
        dataBase->order_Book_BidsTable->getAll(latestBids, "DateRemoved = 0");
        if (!latestBids.size() || !latestBids[0].isSetPrice() || !latestBids[0].isSetVolume() ||
            !equal< double >(orderBook->dataObject.bids[0][0], latestBids[0].price) ||
            !equal< double >(orderBook->dataObject.bids[0][1], latestBids[0].volume))
        {
            std::vector< Order_Book_Bids::RecordWithId > newBids;
            for (auto &bid : orderBook->dataObject.bids)
            {
                Order_Book_Bids::RecordWithId bidRecord;
                bidRecord.price = bid[0];
                bidRecord.volume = bid[1];
                newBids.push_back(bidRecord);
            }

            // Find ask prices in exchange that is not available in records and add it
            std::vector< Order_Book_Bids::RecordWithId > missingRecords;
            findMissing< Order_Book_Bids::RecordWithId >(newBids, latestBids, missingRecords, false);
            for (auto &record : missingRecords)
            {
                record.dateCreated = (Int32)currentDateTime;
                record.dateRemoved = 0;
            }
            dataBase->order_Book_BidsTable->insertMultiple(missingRecords);

            missingRecords.clear();
            findMissing< Order_Book_Bids::RecordWithId >(latestBids, newBids, missingRecords, false);
            dataBase->order_Book_BidsTable->deleteMultiple(missingRecords);
            for (auto &record : missingRecords)
            {
                record.dateRemoved = (Int32)currentDateTime;
            }
            dataBase->order_Book_BidsTable->insertMultiple(missingRecords);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary> Executes the pending orders operation. </summary>
    ///
    /// <param name="timer"> [in,out] The timer. </param>
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Fyb::executePendingOrders(Timer &timer)
    {
        (void)timer;
        AutoPtr< PendingOrders > orderBook = api.GetPendingOrders();

        if (orderBook->dataObject.isSetError() && orderBook->dataObject.error != 0)
        {
            return;
        }

        std::vector< My_Pending_Buy_Orders::Record > buyRecords;
        std::vector< My_Pending_Sell_Orders::Record > sellRecords;
        for (auto &order : orderBook->dataObject.orders)
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

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary> Executes the order history operation. </summary>
    ///
    /// <param name="timer"> [in,out] The timer. </param>
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    void Fyb::executeOrderHistory(Timer &timer)
    {
        (void)timer;
        AutoPtr< OrderHistoryParams > orderHistoryParams = new OrderHistoryParams();
        orderHistoryParams->dataObject.SetLimit(100);
        AutoPtr< OrderHistory > orderHistory = api.GetOrderHistory(orderHistoryParams);

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

        std::vector< My_Trade_History::Record > records;
        for (auto &order : orderHistory->dataObject.orders)
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

    /// <summary> Finalizes an instance of the Fyb class. </summary>
    Fyb::~Fyb() {}

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    /// <summary> Executes the given operation on a different thread, and waits for the result. </summary>
    ///
    /// <exception cref="TimeoutException">	    Thrown when a Timeout error condition occurs. </exception>
    /// <exception cref="ApplicationException"> Thrown when an Application error condition occurs. </exception>
    ///
    /// <param name="httpMethod"> The HTTP method. </param>
    /// <param name="uri">		  [in,out] URI of the document. </param>
    ///
    /// <returns> A Dynamic::Var. </returns>
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    Dynamic::Var Fyb::invoke(const std::string &httpMethod, URI &uri)
    {
        static FastMutex lock;
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

        // Add timestamp
        ostringstream timeString;
        timeString << currentTimeStamp;
        uri.addQueryParameter(string("timestamp"), timeString.str());

        // Create the request URI.
        HTTPSClientSession session(uri.getHost(), uri.getPort());
        HTTPRequest req(HTTPRequest::HTTP_GET, uri.getPathAndQuery(), HTTPMessage::HTTP_1_1);
        req.add("Accept-Encoding", "gzip");

        // Sign request
        if (httpMethod == HTTPRequest::HTTP_POST)
        {
            // Convert to POST
            HTMLForm form(req);
            req.setMethod(HTTPRequest::HTTP_POST);
            req.setURI(uri.getPath());
            form.setEncoding(HTMLForm::ENCODING_URL);

            // Key
            req.set("key", api.config.data[configurationIdx].consumer_key);

            // Sign
            ostringstream paramStream;
            form.write(paramStream);
            string signatureBase = paramStream.str();
            string signingKey = api.config.data[configurationIdx].consumer_secret;
            HMACEngine< SHA1Engine > hmacEngine(signingKey);
            hmacEngine.update(signatureBase);
            DigestEngine::Digest digest = hmacEngine.digest();
            std::ostringstream digestHexBin;
            HexBinaryEncoder hexBinEncoder(digestHexBin);
            hexBinEncoder.write(reinterpret_cast< char * >(&digest[0]), digest.size());
            hexBinEncoder.close();
            req.set("sig", digestHexBin.str());

            // Submit
            form.prepareSubmit(req);
            ostream &ostr = session.sendRequest(req);
            form.write(ostr);
        }
        else
        {
            session.sendRequest(req);
        }

        Logger::get("Logs").information("Send Request: %s", uri.toString());

        // Receive the response.
        HTTPResponse res;
        istream &rs = session.receiveResponse(res);

        InflatingInputStream inflater(rs, InflatingStreamBuf::STREAM_GZIP);
        // Parse the JSON
        JSON::Parser parser;
        parser.parse(inflater);
        Dynamic::Var result = parser.result();
        string resultString = Dynamic::Var::toString(result);
        Logger::get("Logs").information("Received Response: %s", resultString);

        // If everything went fine, return the JSON document.
        // Otherwise throw an exception.
        if (res.getStatus() == HTTPResponse::HTTP_OK)
        {
            return result;
        }
        else
        {
            throw ApplicationException("Fyb Error", "");
        }
    }
} // namespace trader
