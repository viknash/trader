#include "stdafx.h"

#include "config.h"
#include "fileoutputstream.h"
#include "endpoint.h"
#include "helpers.h"
#include "fixspec.h"

using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;
using Poco::Util::AbstractConfiguration;
using Poco::Util::OptionCallback;
using Poco::DirectoryIterator;
using Poco::File;
using Poco::Path;
using Poco::XML::DOMParser;
using Poco::XML::InputSource;
using Poco::XML::Document;
using Poco::XML::NodeIterator;
using Poco::XML::NodeFilter;
using Poco::XML::Node;
using Poco::XML::NodeList;
using Poco::XML::AutoPtr;
using Poco::Exception;
using namespace Poco;
using namespace std;
using namespace Poco::XML;

namespace trader {
	FixSpec FixSpec::instance;

	const char* FixSpec::getCppType(const string& dbType)
	{
		if (dbType.compare("STRING") == 0
            || dbType.compare("CHAR") == 0
            || dbType.compare("CURRENCY") == 0
            || dbType.compare("EXCHANGE") == 0
            || dbType.compare("MULTIPLECHARVALUE") == 0
            || dbType.compare("MULTIPLESTRINGVALUE") == 0
            || dbType.compare("COUNTRY") == 0
            || dbType.compare("DATA") == 0)
		{
			return "std::string";
		}
		else if (dbType.compare("PRICE") == 0
            || dbType.compare("QTY") == 0
            || dbType.compare("AMT") == 0
            || dbType.compare("PRICEOFFSET") == 0
            )
		{
			return "double";
		}
        else if (dbType.compare("FLOAT") == 0
            || dbType.compare("PERCENTAGE") == 0
            )
        {
            return "float";
        }
		else if (dbType.compare("SEQNUM") == 0
			|| dbType.compare("INT") == 0
			|| dbType.compare("UTCTIMESTAMP") == 0
			|| dbType.compare("LENGTH") == 0
            || dbType.compare("LOCALMKTDATE") == 0
            || dbType.compare("MONTHYEAR") == 0
            || dbType.compare("NUMINGROUP") == 0
            )
		{
			return "POCO::Int32";
		}
		else if (dbType.compare("BOOLEAN") == 0)
		{
			return "bool";
		}
		return dbType.c_str();
	}

	void FixSpec::process(const string& namespacename, const string& filename, const string& outputdirectory)
	{
		InputSource src(filename);
		DOMParser parser;
		AutoPtr<Document> pDoc = parser.parse(&src);

		Element* root = (Element*)pDoc->firstChild();

		Config config;
		config.outputDir = outputdirectory;
		config.nameSpace = namespacename;
		config.headerFileName = outputdirectory + Path::separator() + "interface.h";
		config.cppFileName = outputdirectory + Path::separator() + "interface.cpp";
	

		ApiFileOutputStream header(config.headerFileName);
		ApiFileOutputStream cpp(config.cppFileName);

        startHeader(header, 0);
        {
            ScopedNamespace scopedNamespaceTrader(header, config.nameSpace);
            {
                ScopedNamespace scopedNamespaceInterface(header, std::string("Interface"));

                Element* fieldsNode = root->getChildElement("fields");
                NodeList* fieldNodeList = fieldsNode->childNodes();

                for (UInt32 i = 0; i < fieldNodeList->length(); i++)
                {
                    Node* child = fieldNodeList->item(i);
                    if (child->nodeType() == Node::ELEMENT_NODE)
                    {
                        Element* fieldNode = (Element*)child;
                        string name = fieldNode->getAttribute("name");
                        if (fieldNode->hasChildNodes())
                        {
                            header << "enum " << name << " ";
                            {
                                ScopedStream<ApiFileOutputStream> enumScope(header);
                                NodeList* enumNodeList = fieldNode->childNodes();
                                for (UInt32 j = 0; j < enumNodeList->length(); j++)
                                {
                                    Node* valueNode = enumNodeList->item(j);
                                    if (valueNode->nodeType() == Node::ELEMENT_NODE)
                                    {
                                        Element* value = (Element*)valueNode;
                                        string description = value->getAttribute("description");
                                        header << description << "," << endl;
                                    }
                                }
                                header << "NUM_" << name << endl;
                            }
                            header << cendl;
                        }
                        else
                        {
                            string type = fieldNode->getAttribute("type");
                            string cppType = getCppType(type);
                            header << "typename " << cppType << " " << name << cendl;
                        }
                        header << endl;
                    }
                }

                Element* componentsNode = root->getChildElement("components");
                NodeList* componentsNodeList = componentsNode->childNodes();

                for (UInt32 i = 0; i < componentsNodeList->length(); i++)
                {
                    Node* componentNode = componentsNodeList->item(i);
                    if (componentNode->nodeType() == Node::ELEMENT_NODE)
                    {
                        Element* component = (Element*)componentNode;
                        string name = component->getAttribute("name");
                        ScopedStruct<0, ApiFileOutputStream> scopedStruct(header, name.c_str());
                        NodeList* componentChildNodeList = componentNode->childNodes();
                        for (UInt32 j = 0; j < componentChildNodeList->length(); j++)
                        {
                            Node* componentChildNode = componentChildNodeList->item(j);
                            if (componentChildNode->nodeType() == Node::ELEMENT_NODE)
                            {
                                Element* groupOrField = (Element*)componentChildNode;
                                std::string tagName = groupOrField->tagName();
                                string name = groupOrField->getAttribute("name");
                                if (tagName.compare("group") == 0)
                                {
                                    {
                                        ScopedStruct<0, ApiFileOutputStream> scopedStruct(header, name.c_str());
                                        NodeList* fieldNodeList = componentChildNode->childNodes();
                                        {
                                            for (UInt32 k = 0; k < fieldNodeList->length(); k++)
                                            {
                                                Node* fieldChildNode = fieldNodeList->item(k);
                                                if (fieldChildNode->nodeType() == Node::ELEMENT_NODE)
                                                {
                                                    Element* field = (Element*)fieldChildNode;
                                                    string name = field->getAttribute("name");
                                                    header << name << " " << var_name(name) << cendl;
                                                }
                                            }
                                        }
                                    }
                                    header << "std::vector<" << name << "> " << var_name(name) << cendl;
                                }
                                else
                                {
                                    header << name << " " << var_name(name) << cendl;
                                }
                            }
                        }
                    }
                }

                {
                    ScopedClass<1> scopedClass(header, "IMessageData", "Poco::RefCountedObject");
                    header << "virtual ProcessMessage(Poco::AutoPtr<IMessageData>> _messageData) = 0" << cendl;
                }

                {
                    ScopedClass<1> scopedClass(header, "Connection", "Poco::RefCountedObject");

                    Element* messagesNode = root->getChildElement("messages");
                    NodeList* messagesNodelist = messagesNode->childNodes();

                    header << "enum MESSAGES ";
                    {
                        ScopedStream<ApiFileOutputStream> enumScope(header);
                        for (UInt32 i = 0; i < messagesNodelist->length(); i++)
                        {
                            Node* messageNode = messagesNodelist->item(i);
                            if (messageNode->nodeType() == Node::ELEMENT_NODE)
                            {
                                Element* message = (Element*)messageNode;
                                string name = message->getAttribute("name");
                                header << name;
                            }
                        }
                        header << "NUM_MESSAGES" << cendl;
                    }
                    header << cendl;

                    for (UInt32 i = 0; i < messagesNodelist->length(); i++)
                    {
                        Node* messageNode = messagesNodelist->item(i);
                        if (messageNode->nodeType() == Node::ELEMENT_NODE)
                        {
                            Element* message = (Element*)messageNode;
                            string name = message->getAttribute("name");
                            ostringstream className;
                            className << name << "Data";
                            ScopedStruct<0, ApiFileOutputStream> scopedStruct(header, className.str().c_str());
                            NodeList* messageChildNodeList = messageNode->childNodes();
                            for (UInt32 j = 0; j < messageChildNodeList->length(); j++)
                            {
                                Node* messageChildNode = messageChildNodeList->item(j);
                                if (messageChildNode->nodeType() == Node::ELEMENT_NODE)
                                {
                                    Element* groupOrField = (Element*)messageChildNode;
                                    std::string tagName = groupOrField->tagName();
                                    string name = groupOrField->getAttribute("name");
                                    if (tagName.compare("group") == 0)
                                    {
                                        {
                                            ScopedStruct<0, ApiFileOutputStream> scopedStruct(header, name.c_str());
                                            NodeList* fieldNodeList = messageChildNode->childNodes();
                                            {
                                                for (UInt32 k = 0; k < fieldNodeList->length(); k++)
                                                {
                                                    Node* fieldChildNode = fieldNodeList->item(k);
                                                    if (fieldChildNode->nodeType() == Node::ELEMENT_NODE)
                                                    {
                                                        Element* field = (Element*)fieldChildNode;
                                                        string name = field->getAttribute("name");
                                                        header << name << " " << var_name(name) << cendl;
                                                    }
                                                }
                                            }
                                        }
                                        header << "std::vector<" << name << "> " << var_name(name) << cendl;
                                    }
                                    else
                                    {
                                        header << name << " " << var_name(name) << cendl;
                                    }
                                }
                            }
                        }
                    }

                    for (UInt32 i = 0; i < messagesNodelist->length(); i++)
                    {
                        Node* messageNode = messagesNodelist->item(i);
                        if (messageNode->nodeType() == Node::ELEMENT_NODE)
                        {
                            Element* message = (Element*)messageNode;
                            string name = message->getAttribute("name");
                            header << "virtual " << name << "(Poco::AutoPtr<" << name << "Data>> " << var_name(name) << "Data)";
                            {
                                ScopedStream<ApiFileOutputStream> funcScope(header);
                                header << "poco_bugcheck_msg(\"" << name << " not supported.\")" << cendl;
                            }
                        }
                    }

                    header << "vitual void SetReceivingConnection(Connection* _connection)";
                    {
                        ScopedStream<ApiFileOutputStream> enumScope(header);
                        header << "connection = _connection" << cendl;
                    }
                    header << "Connection* connection" << endl;

                }

            }
        }


		//Write


		header << endl;

		cpp << endl;
	}

}