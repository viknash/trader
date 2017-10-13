#pragma once

#include "api.h"
#include "krakenapi.h"
#include "krakendatabase.h"

namespace trader {

    namespace KrakenApi {
        class EndPoints;
    };

    namespace KrakenDatabase {
        class Tables;
    };

	class Kraken : public Api
	{
	public:
        Kraken(Poco::AutoPtr<trader::App> _app);

		~Kraken();

		Poco::Dynamic::Var invoke(const std::string& httpMethod, Poco::URI& uri);

		void run();
		void execute(Poco::Timer& timer);

        KrakenApi::EndPoints api;

	protected:
        Kraken(const Kraken&);
        Kraken& operator = (const Kraken&);

		std::string _uri;

	private:
		Poco::Timer executeTimer;
		Poco::AutoPtr<KrakenDatabase::Tables> dataBase;
		Poco::AutoPtr<trader::App> app;
		std::vector<std::function<void(Poco::Timer&)>> serialExecutionList;
	};

}
