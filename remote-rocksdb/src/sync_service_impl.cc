#include "remoterocksdb/sync_service_impl.h"

using namespace remoterocksdb;

RemoteRocksDBServiceImpl::RemoteRocksDBServiceImpl(rocksdb::DB *db)
    : db_(db)
{
    std::cout << "Start RemoteRocksDBService..." << std::endl;
}

RemoteRocksDBServiceImpl::~RemoteRocksDBServiceImpl()
{
    std::cout << "Shutdown RemoteRocksDBService..." << std::endl;
    delete db_;
}

/**
 * 一个 Op 包含 n 个 SingleOp，相当于一个 Op Batch；
 * 一个 OpReply 包含 n 个 SingleOpReply；
 * 一个 Op 对应一个 OpReply，一个 SingleOp 对应一个 SingleOpReply；
 * 一个 SingleOp 有指向所属 Op 和所属 Op 对应的 OpReply 的指针，以方便更新 OpReply
 */

Status RemoteRocksDBServiceImpl::DoOp(ServerContext *context, ServerReaderWriter<OpReply, Op> *stream)
{
    Op tmp_op;
    Status s;
    std::vector<OpReply*> op_reps;

    // 读取请求流
    while (stream->Read(&tmp_op))
    {
        Op *op_req = new Op(tmp_op);
        OpReply *op_rep = new OpReply();
        op_rep->set_time(op_req->time());

        s = this->HandleOp(op_req, op_rep);
        // 有操作未正确执行，退出
        if (!s.ok())
        {
            std::cerr << "Handle Op failed" << std::endl;
            exit(EXIT_FAILURE);
        }

        stream->Write(*op_rep);
        
        op_reps.emplace_back(op_rep);
    }

    // // 写入响应流
    // #ifdef DEBUG
    // std::cout << "SingleOpReply num in an OpReply: " 
    //             << (op_reps.size() ? op_reps[0]->replies_size() : 0) << std::endl;
    // std::cout << "OpReply num to return: " << op_reps.size() << std::endl;
    // std::cout << "==========" << std::endl;
    // #endif
    // for (size_t i = 0; i < op_reps.size(); i++)
    //     stream->Write(*(op_reps[i]));
    
    return s;
}

Status RemoteRocksDBServiceImpl::HandleOp(Op *op, OpReply *op_reply)
{
    assert(op->ops_size() > 0);
    assert(op->ops_size() <= BATCH_SIZE);
    assert(op_reply->replies_size() == 0);

    Status s;

    batch_counter_.fetch_add(1);
    size_t ops_size = op->ops_size();
    
    #ifdef DEBUG
    std::cout << "SingleOp num in an Op: " << ops_size << std::endl;
    std::cout << "==========" << std::endl;
    #endif

    for (size_t i = 0; i < ops_size; i++)
    {
        SingleOp *single_op = op->mutable_ops(i);

        if (i == op->ops_size() - 1)
        {
            single_op->set_op_ptr((uint64_t)op);
            assert((uint64_t)op == single_op->op_ptr());
        }
        else
            single_op->set_op_ptr((uint64_t) nullptr);
        // 设置 OpReply 指针，以方便 SingleOp 处理后更新 OpReply
        single_op->set_reply_ptr((uint64_t)op_reply);
        assert((uint64_t)op_reply == single_op->reply_ptr());

        s = this->HandleSingleOp(single_op);
        if (!s.ok())
        {
            std::cerr << "Handle SingleOp failed" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    return s;
}

Status RemoteRocksDBServiceImpl::HandleSingleOp(SingleOp *single_op)
{
    rocksdb::Status r_s;
    int iterations = 0;
    int record_cnt = 0;
    rocksdb::Iterator *iter;

    // SingleOp 对应的 Reply
    SingleOpReply *single_op_rep;
    // Op Batch 对应的 Reply，其中包含 n 个 SingleOpReply
    OpReply *op_rep = (OpReply *)single_op->reply_ptr();

    rocksdb::WriteOptions wop = rocksdb::WriteOptions();
    // 开启同步写
    wop.sync = true;
    rocksdb::ReadOptions rop = rocksdb::ReadOptions();

    std::string value;

    #ifdef DEBUG
    std::cout << "SingleOp info: " << std::endl
                // << "    key: " << single_op->key() << std::endl
                // << "    value: " << single_op->value() << std::endl
                << "    type: " << single_op->type() << std::endl;
    std::cout << "==========" << std::endl;
    #endif

    switch (single_op->type())
    {
    case remoterocksdb::GET:
        r_s = this->db_->Get(rop, single_op->key(), &value);
        // set single op reply
        single_op_rep = op_rep->add_replies();
        single_op_rep->set_key(single_op->key());
        single_op_rep->set_type(remoterocksdb::GET);
        single_op_rep->set_status(r_s.ToString());
        single_op_rep->set_ok(r_s.ok());
        if (r_s.ok())
            single_op_rep->set_value(value);
        break;

    // TODO
    // 修改为 WriteBatch？结构造成影响
    case remoterocksdb::PUT:
        r_s = this->db_->Put(wop, single_op->key(), single_op->value());
        // set single op reply
        single_op_rep = op_rep->add_replies();
        single_op_rep->set_key(single_op->key());
        single_op_rep->set_type(remoterocksdb::PUT);
        single_op_rep->set_status(r_s.ToString());
        single_op_rep->set_keynum(single_op->keynum());
        single_op_rep->set_ok(r_s.ok());
        break;

    case remoterocksdb::DELETE:
        r_s = this->db_->Delete(wop, single_op->key());
        assert(r_s.ok());

        // set single op reply
        single_op_rep = op_rep->add_replies();
        single_op_rep->set_key(single_op->key());
        single_op_rep->set_type(remoterocksdb::DELETE);
        single_op_rep->set_status(r_s.ToString());
        single_op_rep->set_ok(r_s.ok());
        break;

    case remoterocksdb::UPDATE:
        // 先读取 current value，然后写入 new value
        r_s = this->db_->Get(rop, single_op->key(), &value);
        assert(r_s.ok());

        r_s = this->db_->Put(wop, single_op->key(), single_op->value());
        // set single op reply
        single_op_rep = op_rep->add_replies();
        single_op_rep->set_key(single_op->key());
        single_op_rep->set_type(remoterocksdb::UPDATE);
        single_op_rep->set_status(r_s.ToString());
        single_op_rep->set_ok(r_s.ok());
        break;

    case remoterocksdb::SCAN:
        record_cnt = single_op->record_cnt();
        iter = db_->NewIterator(rop);
        assert(iter != nullptr);

        single_op_rep = op_rep->add_replies();
        single_op_rep->set_key(single_op->key());
        single_op_rep->set_type(remoterocksdb::SCAN);
        single_op_rep->set_ok(true);

        for (iter->Seek(rocksdb::Slice(single_op->key())); iter->Valid() && iterations < record_cnt; iter->Next())
        {
            single_op_rep->add_scanned_values(iter->value().data());
            iterations++;
        }
        delete iter;

        break;

    default:
        std::cerr << "Unsupported Operation" << std::endl;
        break;
    }

    #ifdef DEBUG
    std::cout << "SingleOpReply info: " << std::endl
                << "    ok: " << single_op_rep->ok() << std::endl
                // << "    key: " << single_op_rep->key() << std::endl
                // << "    value: " << single_op_rep->value() << std::endl
                << "    status: " << single_op_rep->status() << std::endl;
    std::cout << "Current SingleOpReply num in an OpReply: " 
                << op_rep->replies_size() << std::endl
                << "=====================" << std::endl;
    #endif

    return (r_s.ok() ? Status::OK : Status::CANCELLED);
}
