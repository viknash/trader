#include "stdafx.h"
#include "Poco/Util/Application.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "Poco/File.h"
#include "Poco/FileStream.h"
#include "Poco/StreamCopier.h"
#include "Poco/DirectoryIterator.h"
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/ParseHandler.h"
#include "Poco/JSON/JSONException.h"
#include "Poco/StringTokenizer.h"
#include "endpoint.h"
#include "fileoutputstream.h"
#pragma once

#include "utils.h"

namespace trader {

	using namespace Poco;
	using namespace std;

	struct ObjectSchemaDefinition
	{
		struct ExpansionPair
		{
			ExpansionPair(const std::string& name, JSON::Object::Ptr obj)
				: _obj(obj)
			{
				static UInt32 count = 0;
				stringstream str;
				str << name;// << ++count;
				_name = str.str();
			}
			JSON::Object::Ptr _obj;
			std::string _name;
		};

		void read(JSON::Object::Ptr obj);
		void writeCpp(ApiFileOutputStream& cpp);
		void writeHeader(ApiFileOutputStream& cpp);
		void headerConstruct(std::vector<ExpansionPair>& toExpand, UInt32 arrayCount, UInt32 objectCount, JSON::Object::Ptr obj, ApiStreamBuffer& stream, string depthName, string anoymousName, bool newObject = false);
		void headerConstructProperties(std::vector<ExpansionPair>& toExpand, UInt32 arrayCount, UInt32 objectCount, JSON::Object::Ptr obj, ApiStreamBuffer& stream, string depthName, string anoymousName);
		void cppConstruct(UInt32 arrayCount, UInt32 objectCount, JSON::Object::Ptr obj, ApiFileOutputStream& stream, string depthName, string anoymousName, UInt32 objIndex, string prefix, bool useTemp);
		void header_construct(JSON::Object::Ptr obj, ApiFileOutputStream& stream, string expandedName, string keyName, bool useAsType);

		string name;
		JSON::Object::Ptr rootObj;
		vector<ExpansionPair> nameToObjectMap;
	};

}


