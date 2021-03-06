project "genproj"
    kind "Makefile"
	location "%{wks.location}/tmp/projects"
	targetdir "%{wks.location}"
	targetname  "%{wks.name}.sln"
	files
	{
        "%{wks.location}/*.lua",
		"%{wks.location}/*.md",
		"%{wks.location}/*.txt",
		"%{wks.location}/premake/**",
		"%{wks.location}/*.config",
		"%{wks.location}/*.yml",
		"%{wks.location}/build/**.cmd",
		"%{wks.location}/build/**.ps1"
	}

	filter { "system:linux" }
		buildcommands {
			"%{wks.location}/tools/bin/premake/premake5 gmake --file=%{wks.location}/premake5.lua"
		}

		rebuildcommands {
			"{RMDIR} %{wks.location}/Makefile",
			"{RMDIR} %{wks.location}/*.make",
			"%{wks.location}/tools/bin/premake/premake5 gmake --file=%{wks.location}/premake5.lua"
		}
		cleancommands {
			"{RMDIR} %{wks.location}Makefile",
			"{RMDIR} %{wks.location}*.make"
		}

	filter { "action:vs2015", "system:windows" }
		files {
			"%{pocoPathVS2015}lib/native/include/**.h"
		}
		buildcommands {
			"{COPY} %{wks.location}/%{pocoPathVS2015}/build/native/x64/*.dll %{wks.location}/bin/%{cfg.platform}/debug-shared/",
			"{COPY} %{wks.location}/%{pocoPathVS2015}/build/native/x64/*.dll %{wks.location}/bin/%{cfg.platform}/release-shared/",
			"{COPY} %{wks.location}/%{pocoPathVS2015}/build/native/x64/*.pdb %{wks.location}/bin/%{cfg.platform}/debug-shared/",
			"{COPY} %{wks.location}/%{pocoPathVS2015}/build/native/x64/*.pdb %{wks.location}/bin/%{cfg.platform}/release-shared/"
		}

	filter { "action:vs2017", "system:windows" }
		files {
			"%{pocoPathVS2017}lib/native/include/**.h"
		}
		buildcommands {
			"{COPY} %{wks.location}/%{pocoPathVS2017}/build/native/x64/*.dll %{wks.location}/bin/%{cfg.platform}/debug-shared/",
			"{COPY} %{wks.location}/%{pocoPathVS2017}/build/native/x64/*.dll %{wks.location}/bin/%{cfg.platform}/release-shared/",
			"{COPY} %{wks.location}/%{pocoPathVS2017}/build/native/x64/*.pdb %{wks.location}/bin/%{cfg.platform}/debug-shared/",
			"{COPY} %{wks.location}/%{pocoPathVS2017}/build/native/x64/*.pdb %{wks.location}/bin/%{cfg.platform}/release-shared/"
		}

	filter { "system:windows" }
		toolset "v140"
		files {
			"%{gtestPath}lib/native/include/**.h"			
		}
		buildcommands {
			"{MKDIR} %{wks.location}/bin/%{cfg.platform}/debug-static",
			"{MKDIR} %{wks.location}/bin/%{cfg.platform}/debug-shared",
			"{MKDIR} %{wks.location}/bin/%{cfg.platform}/release-static",
			"{MKDIR} %{wks.location}/bin/%{cfg.platform}/release-shared",
			"{COPY} %{wks.location}/%{gtestPathDynamic}/lib/native/v140/windesktop/msvcstl/dyn/rt-dyn/x64/Debug/*.dll %{wks.location}/bin/%{cfg.platform}/debug-shared",
			"{COPY} %{wks.location}/%{gtestPathDynamic}/lib/native/v140/windesktop/msvcstl/dyn/rt-dyn/x64/Release/*.dll %{wks.location}/bin/%{cfg.platform}/release-shared",
			"{COPY} %{wks.location}/%{gtestPathDynamic}/lib/native/v140/windesktop/msvcstl/dyn/rt-dyn/x64/Debug/*.pdb %{wks.location}/bin/%{cfg.platform}/debug-shared",
			"{COPY} %{wks.location}/%{gtestPathDynamic}/lib/native/v140/windesktop/msvcstl/dyn/rt-dyn/x64/Release/*.pdb %{wks.location}/bin/%{cfg.platform}/release-shared",
			"$(SolutionDir)build\\win\\genproj.cmd"
		}
		rebuildcommands {
			"{RMDIR} $(SolutionDir)*.vcxproj*",
			"{RMDIR} $(SolutionDir)*.sln*",
			"$(SolutionDir)build\\win\\genproj.cmd"		}
		cleancommands {
			"{RMDIR} $(SolutionDir)*.vcxproj*",
			"{RMDIR} $(SolutionDir)*.sln*"
		}

	filter { "action:vs2015", "system:windows" }
		buildcommands {
			"$(SolutionDir)tools\\bin\\premake\\premake5.exe --file=$(SolutionDir)premake5.lua vs2015"
		}
		rebuildcommands {
			"$(SolutionDir)tools\\bin\\premake\\premake5.exe --file=$(SolutionDir)premake5.lua vs2015"
		}

	filter { "action:vs2017", "system:windows" }
		buildcommands {
			"$(SolutionDir)tools\\bin\\premake\\premake5.exe --file=$(SolutionDir)premake5.lua vs2017"
		}
		rebuildcommands {
			"$(SolutionDir)tools\\bin\\premake\\premake5.exe --file=$(SolutionDir)premake5.lua vs2017"
		}
