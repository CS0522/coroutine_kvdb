#!/usr/bin/env bash
# setup remote rocksdb by SSH

set -x

if [ $# -lt 3 ]; then
    echo "Usage: setup_remote.sh username hostname_client hostname_server"
    exit
fi

username=$1
shift 1
hostnames=("$@")

ssh_arg="-o StrictHostKeyChecking=no"

remote_rocksdb_repo="https://github.com/CS0522/coroutine_kvdb.git"
remote_rocksdb_path="./coroutine_kvdb/remote-rocksdb"

function install_dependencies()
{
    # Ubuntu
    for hostname in ${hostnames[@]}; do
        ssh ${ssh_arg} ${username}@${hostname} << ENDSSH
            sudo apt update
            sudo apt install -y build-essential autoconf libtool \
                            pkg-config cmake make vim dos2unix git libprotobuf-dev
            exit
ENDSSH
    done
}

function get_repo()
{
    for hostname in ${hostnames[@]}; do
        ssh ${ssh_arg} ${username}@${hostname} << ENDSSH
            git clone ${remote_rocksdb_repo}
ENDSSH
    done
}

function install_jdk_mvn()
{
    ssh ${ssh_arg} ${username}@${hostnames[0]} << ENDSSH
        cd ${remote_rocksdb_path}/scripts
        sudo bash ./install_jdk_mvn.sh
        source /etc/profile
        exit
ENDSSH
}

function setup_grpc()
{
    for hostname in ${hostnames[@]}; do
        ssh ${ssh_arg} ${username}@${hostname} << ENDSSH
            cd ${remote_rocksdb_path}/scripts
            bash ./setup_grpc.sh
            exit
ENDSSH
    done
}

function setup_remote_rocksdb()
{
    ssh ${ssh_arg} ${username}@${hostnames[1]} << ENDSSH
        cd ${remote_rocksdb_path}/scripts
        bash ./setup_remote_rocksdb.sh
        exit
ENDSSH
}

function setup_ycsb()
{
    ssh ${ssh_arg} ${username}@${hostnames[0]} << ENDSSH
        cd ${remote_rocksdb_path}/scripts
        bash ./setup_ycsb.sh
        exit
ENDSSH
}

function setup_remote_fn()
{
    install_dependencies
    get_repo
    install_jdk_mvn
    setup_grpc
    setup_remote_rocksdb
    setup_ycsb
}