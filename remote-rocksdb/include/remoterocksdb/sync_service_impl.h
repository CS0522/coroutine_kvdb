#pragma once

#include <iostream>
#include <cstdio>
#include <string>

#include "rocksdb/db.h"
#include "rocksdb/slice.h"
#include "rocksdb/options.h"

#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <grpcpp/ext/proto_server_reflection_plugin.h>

#include "remote_rocksdb.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReaderWriter;
using grpc::Status;

using remoterocksdb::Empty;
using remoterocksdb::Op;
using remoterocksdb::OpReplies;
using remoterocksdb::OpReply;
using remoterocksdb::OpType;
using remoterocksdb::RemoteRocksDBService;
using remoterocksdb::Reply;
using remoterocksdb::SingleOp;
using remoterocksdb::SingleOpReply;

using std::chrono::high_resolution_clock;
using std::chrono::time_point;

namespace remoterocksdb
{
    class RemoteRocksDBServiceImpl final : public RemoteRocksDBService::Service
    {
    public:
        explicit RemoteRocksDBServiceImpl(rocksdb::DB *db);
        ~RemoteRocksDBServiceImpl();

        /**
         * 同步执行 CRUD
         */
        Status DoOp(ServerContext *context,
                    ServerReaderWriter<OpReply, Op> *stream);

    private:
        /**
         * 实际处理一个 op request
         */
        Status HandleOp(Op *op, OpReply *op_reply);
        Status HandleSingleOp(SingleOp *single_op);

        // db instance
        rocksdb::DB *db_ = nullptr;
        // db status after processing an operation
        rocksdb::Status s_;

        std::atomic<uint64_t> batch_counter_{0};
        time_point<high_resolution_clock> batch_start_time_;
        time_point<high_resolution_clock> batch_end_time_;

        std::shared_ptr<Channel> channel_ = nullptr;
        std::shared_ptr<rocksdb::Logger> logger_ = nullptr;
    };
} // namespace remoterocksdb