#!/usr/bin/env bash

# setup grpc on local machine

function install_dependencies()
{
    sudo apt install -y build-essential autoconf libtool \
                pkg-config cmake make git vim
}

function get_grpc()
{
    git clone --recurse-submodules -b v1.56.0 --depth 1 \
          --shallow-submodules https://github.com/grpc/grpc.git
}

function build_grpc()
{
    local proc_num=`nproc`
    cd grpc
    export 'MY_INSTALL_DIR=$HOME/.local'
    mkdir -p '$MY_INSTALL_DIR'
    export 'PATH="$MY_INSTALL_DIR/bin:$PATH"'

    mkdir -p cmake/build
    pushd cmake/build
    cmake -DgRPC_INSTALL=ON \
          -DgRPC_BUILD_TESTS=OFF \
          -DCMAKE_INSTALL_PREFIX='$MY_INSTALL_DIR' \
          ../..
    make -j${proc_num}
    sudo make install
    popd
}

function setup_grpc_fn()
{
    install_dependencies
    get_grpc
    build_grpc
}