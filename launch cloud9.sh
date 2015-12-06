#!/bin/bash

function pause(){
   read -n 1 -p "Press any key to continue..."
}

export PYTHONHOME=/home/roger/Python-2.7.8/dist
export PYTHONPATH=/home/roger/Software/ActiveTcl-8.6.1/lib:$PYTHONHOME/lib:$PYTHONHOME/lib/python-2.7:/home/roger/Software/numpy-1.8.1/dist/lib/python2.7/site-packages:/home/roger/Software/h5py-2.3.1/dist/lib/python2.7/site-packages:/home/roger/Software/mercurial-3.1-rc/dist/lib/python2.7/site-packages:/home/roger/Software/ActiveTcl-8.6.1/lib:/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu:/lib64:$PYTHONPATH

#export V8_HOME=/home/roger/NodeProjects/V8new
export V8_HOME=/home/roger/Software/node-v0.12.4/deps/v8
export HDF5_HOME=/home/roger/NodeProjects/hdf5
#/home/roger/Software/ros/dist
#export NODE_HOME=/home/roger/NodeProjects/V8new/node/dist
export NODE_HOME=/home/roger/Software/node-v0.12.4/dist
export PATH=`pwd`/bin:/home/roger/Software/gcc/dist/bin:$PYTHONHOME/bin:$NODE_HOME/bin:$V8_HOME/out/x64.release:$PATH
export LD_LIBRARY_PATH=$PYTHONHOME/lib:$PYTHONHOME/lib/python-2.7:/home/roger/Software/gcc/dist/lib64:/home/roger/NetBeansProjects/zlib/lib:/home/roger/NetBeansProjects/hdf5/lib:/home/roger/NetBeansProjects/openmpi/lib:/home/roger/Software/ActiveTcl-8.6.1/lib:/lib/x86_64-linux-gnu:/usr/lib/x86_64-linux-gnu:/lib64:$HDF5_HOME/lib:$LD_LIBRARY_PATH
#export NODE_PATH=/home/roger/.codebox/packages:/home/roger/NodeProjects/codebox:$NODE_PATH

export DEV=true
export WORKSPACE_ADDONS_DIR=/home/roger/NodeProjects/codebox_addons

export FC=gfortran
export CC=gcc
export CXX=g++
#export LDFLAGS="-fPIC -O4 -L$HDF5_HOME/lib -I$HDF5_HOME/include/oce "
#export CFLAGS="-fPIC -O4 -I/home/roger/NodeProjects/V8new/node/src -I$V8_HOME/include -I$V8_HOME/src -I$HDF5_HOME/include -I$HDF5_HOME/include/oce "
#export CXXFLAGS="-fPIC -O4 -std=c++11 -I/home/roger/NodeProjects/V8new/node/src -I$V8_HOME/include -I$V8_HOME/src -I$HDF5_HOME/include -I$HDF5_HOME/include/oce "
#export CPPFLAGS="-fPIC -O4 -I/home/roger/NodeProjects/V8new/node/src -I$V8_HOME/include -I$V8_HOME/src -I$HDF5_HOME/include -I$HDF5_HOME/include/oce "

#which python
export PYTHON=$PYTHONHOME/bin/python2.7

which g++
g++ --version

#./scripts/install-sdk.sh

node /home/roger/NodeProjects/c9sdk/server.js --help
node /home/roger/NodeProjects/c9sdk/server.js -p 9393 -l 0.0.0.0 -a : -w `pwd`
# --workspacetype c++

#$NODE_HOME/bin/npm outdated
pause
