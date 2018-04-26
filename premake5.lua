pocoPackageVS2015 = "finsec.poco-windows-v140"
pocoPackageVS2017 = "finsec.poco-windows-v141"
pocoPackageVersion = "1.8.0.1"
gTestPackage = "gtest-vc140-static-64"
gTestPackageVersion = "1.1.0"

pocoPathVS2015	= "packages/%{pocoPackageVS2015}.%{pocoPackageVersion}/"
pocoPathVS2017	= "packages/%{pocoPackageVS2017}.%{pocoPackageVersion}/"
gtestPath		= "packages/%{gTestPackage}.%{gTestPackageVersion}/"
intelSEAPIPath  = "packages/IntelSEAPI-Windows/"

-------------------------------------------------------------------------------

-- Repo wide settings
editorintegration "On"

platforms {
		"Win64",
		"Win64-Clang",
		"Win64-MSClang",
		"Linux64-gcc",
		"Linux64-Clang"
	}

objdir "tmp"

--- Clean Unnecessary platforms
filter { "system:linux" }
	removeplatforms {                
		"Win64",
		"Win64-Clang",
		"Win64-MSClang"
		}

filter { "system:windows" }
	removeplatforms {                
		"Linux64-Clang",
		"Linux64-gcc"
		}

--- Debug Only
filter "configurations:debug*"
	defines     "_DEBUG"
	symbols 	"On"

--- Release Only
filter "configurations:release*"
	defines     "NDEBUG"
	optimize    "Full"

--- Static Libs Only
filter { "configurations:*static" }
	defines "USE_SHARED_LIBS"

--- Dynamic Libs Only
filter { "configurations:*shared" }
	defines "USE_DYNAMIC_LIBS"

-------------------------------------------------------------------------------

-- Linux Only
filter { "system:linux", "platforms:Linux64*"  }
	buildoptions {
		"-frtti",
		"-fexceptions",
		"-g",
		"-std=c++11",
		"-shared-libgcc",
		"-Wno-unknown-pragmas"
	}
	linkoptions {
		"-g"
	}
	includedirs {
		"deps/poco/Foundation/include",
		"deps/poco/Util/include",
		"deps/poco/JSON/include",
		"deps/poco/XML/include",
		"deps/poco/Net/include",
		"deps/poco/Crypto/include",
		"deps/poco/Foundation/include",
		"deps/poco/Util/include",
		"deps/poco/openssl/include",
		"deps/poco/JSON/include",
		"deps/poco/Data/include",
		"deps/poco/Data/SQLite/include",
		"deps/poco/NetSSL_OpenSSL/include"
	}
	libdirs {
		"deps/poco/lib/Linux/x86_64"
	}

--- Linux, GCC Only
filter { "platforms:Linux64-gcc", "system:linux" }
	toolset "gcc"
	buildoptions {
		"-fpermissive",
		"-Wl,--no-as-needed"
	}
	linkoptions {
		"-Wl,--no-as-needed"
	}

--- Linux, Clang Only
filter { "system:linux", "platforms:Linux64-clang" }
	toolset "clang"

--- Linux, Clang, Debug Only
filter { "system:linux", "platforms:Linux64-clang", "configurations:debug*" }
	buildoptions {
		"-Wall"
	}

-------------------------------------------------------------------------------

--- Windows Only
filter { "system:windows", "platforms:Win64" }
	system "Windows"
	architecture "x64"
	flags { 
		"MultiProcessorCompile"
		}
	buildoptions {
		-- "/GR-"  //Keep rtti enabled because Poco uses dynamic cast
		}
	warnings "Extra"
	disablewarnings { "4251" }
	includedirs {
		"%{gtestPath}lib/native/include",
		"%{intelSEAPIPath}/include"
	}
	libdirs {
		"%{intelSEAPIPath}lib"
	}
	debugenvs {
		"PATH=$(SolutionDir)%{intelSEAPIPath:gsub('/', '\\')}bin"
	}
	prebuildcommands {
		"PATH=$(SolutionDir)%{intelSEAPIPath:gsub('/', '\\')}bin"
	}
	rebuildcommands {
		"PATH=$(SolutionDir)%{intelSEAPIPath:gsub('/', '\\')}bin"
	}

--- Windows, Debug Only
filter { "system:windows", "platforms:Win64", "configurations:debug*" }
	libdirs { 
		"%{gtestPath}lib/native/libs/x64/static/Debug/"
	}
-- 	links { 
-- 		"MSVCRTD.LIB"
-- 	}
-- 	linkoptions {
-- 		"/NODEFAULTLIB:msvcrt"
-- 	}

--- Windows, Release Only
filter { "system:windows", "platforms:Win64", "configurations:release*" }
	libdirs { 
		"%{gtestPath}lib/native/libs/x64/static/Release/"
	}
	flags       {
        "NoBufferSecurityCheck",
        "NoRuntimeChecks",
        "NoIncrementalLink",
        "LinkTimeOptimization"
    }

--- Windows, Static Libs Only
filter {  "system:windows", "platforms:Win64", "configurations:*static" }
	flags { 
		"StaticRuntime"
    }

--- Visual Studio Only
filter "action:vs*"
	defines     { 
        "_CRT_SECURE_NO_DEPRECATE",
        "_CRT_SECURE_NO_WARNINGS",
        "_CRT_NONSTDC_NO_WARNINGS",
		"_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING"
        }

--- Visual Studio 2015 Only
filter { "system:windows", "platforms:Win64", "action:vs2015" }
	toolset "v140"
	libdirs {
		"%{pocoPathVS2015}lib/native/lib64",
	}	
	debugenvs {
		"PATH=$(SolutionDir)%{pocoPathVS2015:gsub('/', '\\')}build\\native\\x64;%PATH%"
	}
	includedirs {
		"%{pocoPathVS2015}lib/native/include"
	}
	prebuildcommands {
		"PATH=$(SolutionDir)%{pocoPathVS2015:gsub('/', '\\')}build\\native\\x64;%PATH%"
	}
	rebuildcommands {
		"PATH=$(SolutionDir)%{pocoPathVS2015:gsub('/', '\\')}build\\native\\x64;%PATH%"
	}

--- Visual Studio 2017 Only
filter { "system:windows", "platforms:Win64", "action:vs2017" }
	toolset "v141"
	libdirs {
		"%{pocoPathVS2017}lib/native/lib64"
	}
	debugenvs {
		"PATH=$(SolutionDir)%{pocoPathVS2017:gsub('/', '\\')}build\\native\\x64;%PATH%"
	}
	includedirs {
		"%{pocoPathVS2017}lib/native/include",
	}
	prebuildcommands {
		"PATH=$(SolutionDir)%{pocoPathVS2017:gsub('/', '\\')}build\\native\\x64;%PATH%"
	}
	rebuildcommands {
		"PATH=$(SolutionDir)%{pocoPathVS2017:gsub('/', '\\')}build\\native\\x64;%PATH%"
	}

--- Windows Clang Only
filter { "system:windows", "platforms:Win64-Clang"  }
	system "Windows"
	architecture "x64"
	toolset "msc-llvm-vs2014"

--- Windows Microsoft Clang Only
filter { "system:windows", "platforms:Win64-MSClang" }
	system "Windows"
	architecture "x64"
	toolset "v140_clang_c2"
	buildoptions {
		"-frtti",
		"-fms-compatibility"
	}

--- Windows Microsoft Clang Debug Only
filter { "system:windows", "platforms:Win64-MSClang", "configurations:debug*", }
	buildoptions {
		"-g2",
		"-Wall"
	}
	
-------------------------------------------------------------------------------			

--- File custom builds
filter "files:premake5.lua"
    -- A message to display while this build step is running (optional)
    buildmessage 'Generating Projects'

    -- One or more commands to run (required)
    buildcommands {
        '$(SolutionDir)build\\genproj.cmd'
    }

    -- One or more outputs resulting from the build (required)
    buildoutputs { '$(SolutionDir)trader.sln' }

-------------------------------------------------------------------------------

--- Workspaces
workspace "generators"
	configurations {
			"debug",
			"release",
		}

	group "Tools"
		project "codegen"
		project "gencode"

	group "Generators"
		project "apis"
		project "configs"
		project "databases"
		project "interface"

	group ""

	include "premake/codegen.lua"
	include "premake/interface.lua"
	include "premake/apis.lua"
	include "premake/databases.lua"
	include "premake/configs.lua"
	include "premake/gencode.lua"	

-------------------------------------------------------------------------------

workspace "trader"
	configurations {
			"debug-static",
			"debug-shared",
			"release-static",
			"release-shared"
		}

	group "Tools"
		project "genproj"

	group "Modules"
		project "dataconnector"

	group "Samples"
		project "trader"

	group "Apps"

	group "Tests"
		project "bittrex_api_test"
		project "sample_app_test"
		project "connection_interface_test"
	
	group ""

	include "premake/genproj"
	include "premake/dataconnector.lua"
	include "premake/trader.lua"
	include "premake/bittrex_api_test.lua"
	include "premake/sample_app_test.lua"
	include "premake/connection_interface_test.lua"
