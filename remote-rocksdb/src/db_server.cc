#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include "remoterocksdb/sync_service_impl.h"

// using namespace ROCKSDB_NAMESPACE;

ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");

/**
 * 用 grpc 远程调用
 * 服务端：
 * 用 Service 封装 rocksdb
 */

/**
 * 指定 rocksdb 存储路径
 * 注意权限问题
 */
const std::string db_path = "/tmp/remote-rocksdb";

/**
 * 获取 db 实例
 */
rocksdb::DB* get_db(const std::string &db_path)
{
    rocksdb::DB *db;
    rocksdb::Options options;
    // optimize rocksdb
    options.IncreaseParallelism();
    options.OptimizeLevelStyleCompaction();
    // create the DB if it's not already present
    options.create_if_missing = true;
    // set direct io and direct reads, same as rubbledb
    options.use_direct_io_for_flush_and_compaction = true;
    options.use_direct_reads = true;

    // open rocksdb
    rocksdb::Status s = rocksdb::DB::Open(options, db_path, &db);
    assert(s.ok());
    
    return db;
}

/**
 * 运行服务器，用 service 封装 db 实例
 */
void run_server(rocksdb::DB* db, const std::string &server_addr)
{
    // 创建 service 对象
    RemoteRocksDBServiceImpl service(db);
    grpc::EnableDefaultHealthCheckService(true);
    grpc::reflection::InitProtoReflectionServerBuilderPlugin();
    ServerBuilder builder;

    builder.AddListeningPort(server_addr, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);
    std::unique_ptr<Server> server(builder.BuildAndStart());

    std::cout << "Server listening on " << server_addr << std::endl;
    
    server->Wait();
}

int main(int argc, char **argv)
{
    if (argc < 1)
    {
        std::cout << "Usage: ./db_server --port=";
        return 0;
    }

    absl::ParseCommandLine(argc, argv);
    uint16_t port = absl::GetFlag(FLAGS_port);
    std::string server_addr = absl::StrFormat("0.0.0.0:%d", port);

    // 获取 db 实例
    rocksdb::DB* db = get_db(db_path);
    // 运行 service
    run_server(db, server_addr);

    return 0;
}