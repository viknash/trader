# trader

[![Build Status](https://ci.appveyor.com/api/projects/status/kks53akyvugs7j1w?svg=true)](https://ci.appveyor.com/project/Administrator73626/trader-private)
[![CII Best Practices](https://bestpractices.coreinfrastructure.org/projects/1605/badge)](https://bestpractices.coreinfrastructure.org/projects/1605)

This project is a framework to quickly connect with cryptocurrency and traditional exchanges that utillize REST JSON, FIX or Websockets.

## Build & Run Instructions

### Windows

#### Setup Instructions 

- Run build\shell.cmd
- Run build\deploy_tools.cmd to download necessary packages
- Run build\genproj.cmd once to generate Visual Studio 2015 or Visual Studio 2017 projects

## Compilation
- Open and compile codegen.sln
- Open and compile trader.sln

## Project Structure

### generator.sln
- codegen - This is the code generator. It includes several code generators to transform, json, xml to c++ code. This needs to be built before apis, configs, databases, interface
- apis - This runs codegen to transform the hyperschema.json files in data/apis to c++ code. Hyperschema is a proposed format to describe REST/JSON apis as json files ([Hyperschema Format](http://json-schema.org/latest/json-schema-hypermedia.html))
- interfaces - This runs codegen to transform the fix api xml specs to a c++ interface that can be used to standardize the communication interface between the various modules.
- configs - This runs codegen to transform the config.json files in data/configs to c++ code. These are used for the simple key-value pair json configs to setup api keys right now.
- databases - This runs codegen to transform the databaseschema.json files data/databases to c++ code.
- gencode - This re-runs premake to generate the project and solutions structure. You might need to re-run this if a new file is added or generated

### trader.sln
- genproj - This re-runs premake to generate the project and solutions structure. You might need to re-run this if a new file is added or generated
- dataconnector - This is meant to be a library that allows one to connect to multiple exchanges. It servers as an API extraction layer and exposes a standardize fix c++ interface for applications to use.
- trader - This is a sample application to interact with dataconnector
- *tests - Various test projects

## Running
- Copy bin\*.json & bin\*.properties to bin\debug or bin\release and modify the config with the relevant api keys and proxy settings
- Comment of/delete the proxy settings if you do not require them
- Right-click trader project and Set as StartUp Project
- Press F5 to run/debug in Visual Studio

## Optional
- Install [Clang Format](https://marketplace.visualstudio.com/items?itemName=LLVMExtensions.ClangFormat) Visual Studio Extension to format code
- Install [Google Test Adapter](https://marketplace.visualstudio.com/items?itemName=ChristianSoltenborn.GoogleTestAdapter) to run tests in Visual Studio 
- Install [Google Test Adapter](https://marketplace.visualstudio.com/items?itemName=ChristianSoltenborn.GoogleTestAdapter) to run tests in Visual Studio 

### Linux/WSL

#### Setup Instructions
Aptitude users
- Run build\deploy_tools.sh to use apt-get to install necessary packages

- Run build\genproj.sh once to generate makefiles for the project
- Copy bin\*.json & bin\*.properties to bin\debug on bin\release and modify the config with the relevant api keys and proxy settings

## Compilation
- Tested with Ubuntu 16 on WSL
For Help
- Run make -f generators.make help and make -f generators.make help
Otherwise
 - Build Generators
   - make -f generators.make all
 - Build Trader
   - make -f trader.make all

## Running
- Run using script build\run_debug_<compiler>.sh

## Debug
- Run build\debug_<compiler>.sh

### MacOS X
~This has not been tested~
~Will be supported in the near future~

