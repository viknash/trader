#!/bin/bash
pushd $(dirname $0)
#apt-get install -y autoconf gcc clang premake libpoco* jq python-pip
cat ../../deb_dependencies.txt | xargs apt-get install -y
if [ -e ../../tools/bin/premake/premake5 ]
then
	echo "Premake found..."
else
	echo "Installing Premake..."
	wget `curl -s https://api.github.com/repos/premake/premake-core/releases | jq -r ".[0].assets[] | select(.name | test(\"linux\")) | .browser_download_url"` -O ../../tmp/premake.tar.gz
	tar zxf ../../tmp/premake.tar.gz -C ../../tools/bin/premake
fi
pip install -r ../../requirements.txt
conan remote add conan-poco https://api.bintray.com/conan/finsecsystem/finsec.pocoproject
conan install -if ../../tmp/. ../../. -s compiler.libcxx=libstdc++11
popd

