#!/usr/bin/env bash

# setup remote-rocksdb on local machine

remote_rocksdb_path=""

function get_repo()
{
    git clone https://github.com/CS0522/coroutine_kvdb.git
    pushd coroutine_kvdb/remote-rocksdb
    
    remote_rocksdb_path=`pwd`
    
    popd
    # return to user home dir
}

function get_rocksdb_static_lib()
{
    wget https://github.com/CS0522/CSBlog/releases/download/coroutine_kvdb_files/rocksdb-6.14.6.zip
    unzip rocksdb-6.14.6.zip
    pushd rocksdb
    local proc_num=`nproc`
    make static_lib -j${proc_num}
    cp ./librocksdb.a ${remote_rocksdb_path}/lib/
    popd
    # return to user home dir
}

function build_remote_rocksdb()
{
    cd ${remote_rocksdb_path}
    mkdir build
    cmake -DDEBUG=ON ..
    local proc_num=`nproc`
    make -j${proc_num}
}

function setup_remote_rocksdb_fn() 
{
    get_repo
    get_rocksdb_static_lib
    build_remote_rocksdb
}
