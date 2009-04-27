#!/bin/bash

if [ ! -e dependencies ]; then
    mkdir dependencies
fi
cd dependencies

deps_dir=`pwd`


# raknet
if [ -e raknet ]; then
  rm -rf raknet
fi
if [ -e installed-raknet ]; then
  rm -rf installed-raknet
fi

mkdir raknet
cd raknet
wget http://www.jenkinssoftware.com/raknet/downloads/RakNet-3.51.zip
unzip RakNet-3.51.zip
patch -p1 < ../raknet_gcc_4_3.patch
sh bootstrap
./configure --prefix=${deps_dir}/installed-raknet
make
make install
cd ..


# sst
if [ -e sst ]; then
  rm -rf sst
fi
if [ -e installed-sst ]; then
  rm -rf installed-sst
fi

#svn co svn://svn.pdos.csail.mit.edu/uia/trunk/uia/sst
git clone git@ahoy:sst.git
cd sst
git branch stanford origin/stanford
git checkout stanford
misc/setup
./configure --prefix=${deps_dir}/installed-sst
make
make install
