project "connection_interface_test"
	location "%{wks.location}/tmp/projects"
	targetname	"connection_interface_test"
	language	"C++"
	kind		"ConsoleApp"
	targetdir	"%{wks.location}/bin/%{cfg.platform}/%{cfg.buildcfg}"
	debugdir	"%{wks.location}/bin/%{cfg.platform}/%{cfg.buildcfg}"
	links {
		"dataconnector"
	}
	includedirs {
		"%{wks.location}",
		"%{wks.location}/src",
		"%{wks.location}/sdk/include",
		"%{wks.location}/samples/utils",
		"%{wks.location}/src/dataconnector",
		"%{wks.location}/tests/connection_interface_test",
		"%{wks.location}/tmp/%{cfg.platform}/codegen",
		"%{wks.location}/deps/tinyfsm/include"
	}
	pchheader	"stdafx.h"
	pchsource	"%{wks.location}/tests/connection_interface_test/stdafx.cpp"
	files {
		"%{wks.location}/sdk/include/**.h",
		"%{wks.location}/tests/connection_interface_test/**.h",
		"%{wks.location}/tests/connection_interface_test/**.cpp",
		"%{wks.location}/samples/utils/*",
		"%{wks.location}/include/**.h",
		"%{wks.location}/bin/**.json",
		"%{wks.location}/bin/**.properties"
	}

	filter "files:%{wks.location}/deps/**.*"
		flags { "ExcludeFromBuild" }

	filter { "platforms:Linux64*", "system:linux" }
		files {
			"%{wks.location}/deps/poco/openssl/include/**.h",
			"%{wks.location}/deps/poco/openssl/src/**.cpp",
			"%{wks.location}/deps/poco/NetSSL_OpenSSL/include/**.h",
			"%{wks.location}/deps/poco/NetSSL_OpenSSL/src/**.cpp"
		}

	filter { "system:windows", "platforms:Win64" }
		links {
			"ittnotify64.lib",
			"Iphlpapi.lib",
			"ws2_32.lib",
			"crypt32.lib",
			"gtest.lib"
		}

	filter { "system:windows", "configurations:*shared" }
		defines "IMPORT_DATACONNECTOR"

	filter { "platforms:Win64", "system:windows", "configurations:debug-static" }
		links { 
			"PocoFoundationmtd.lib",
			"PocoNetmtd.lib",
			"PocoNetSSLWinmtd.lib",
			"PocoUtilmtd.lib",
			"PocoCryptomtd.lib",
			"ssleay64MTd.lib",
			"libeay64MTd.lib",
			"PocoJSONmtd.lib",
			"PocoDatamtd.lib",
			"PocoXMLmtd.lib"
		}

	filter { "platforms:Win64", "system:windows", "configurations:release-static" }
		links { 
			"PocoFoundationmt.lib",
			"PocoNetmt.lib",
			"PocoNetSSLWinmt.lib",
			"PocoUtilmt.lib",
			"PocoCryptomt.lib",
			"ssleay64MT.lib",
			"libeay64MT.lib",
			"PocoJSONmt.lib",
			"PocoDatamt.lib",
			"PocoDataSQLitemt.lib",
			"PocoXMLmt.lib"
		}

	filter { "platforms:Win64", "system:windows", "configurations:debug-shared" }
		links { 
			"PocoFoundationd.lib",
			"PocoNetd.lib",
			"PocoNetSSLWind.lib",
			"PocoUtild.lib",
			"PocoCryptod.lib",
			"ssleay64MTd.lib",
			"libeay64MTd.lib",
			"PocoJSONd.lib",
			"PocoDatad.lib",
			"PocoDataSQLited.lib",
			"PocoXMLd.lib"
		}

	filter { "platforms:Win64", "system:windows", "configurations:release-shared" }
		links { 
			"PocoFoundation.lib",
			"PocoNet.lib",
			"PocoNetSSLWin.lib",
			"PocoUtil.lib",
			"PocoCrypto.lib",
			"ssleay64MT.lib",
			"libeay64MT.lib",
			"PocoJSON.lib",
			"PocoData.lib",
			"PocoDataSQLite.lib",
			"PocoXML.lib"
		}

	filter { "platforms:Linux64*", "system:linux", "configurations:debug*" }
		links { 
			"PocoUtild",
			"PocoJSONd",
			"PocoXMLd",
			"PocoNetd",
			"PocoDatad",
			"PocoDataSQLited",
			"PocoEncodingsd",
			"PocoNetSSLd",
			"PocoCryptod",
			"PocoFoundationd",
		}

	filter { "platforms:Linux64*", "system:linux", "configurations:release*" }
		links { 
			"PocoUtil",
			"PocoJSON",
			"PocoXML",
			"PocoNet",
			"PocoData",
			"PocoDataSQLite",
			"PocoEncodings",
			"PocoNetSSL",
			"PocoCrypto",
			"PocoFoundation",
		}

	filter { "platforms:Linux64*", "system:linux" }
		links {
			"gtest",
			"pthread",
			"ssl",
			"crypto",
			"z",
			"dl"
		}