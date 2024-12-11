#include "remoterocksdb/sync_service_impl.h"

using namespace remoterocksdb;

#define BATCH_SIZE 1000

RemoteRocksDBServiceImpl::RemoteRocksDBServiceImpl(rocksdb::DB *db)
    : db_(db)
{
    std::cout << "Construct RemoteRocksDBService..." << std::endl;
}

RemoteRocksDBServiceImpl::~RemoteRocksDBServiceImpl()
{
    std::cout << "Deconstruct RemoteRocksDBService..." << std::endl;
    delete db_;
}

Status RemoteRocksDBServiceImpl::DoOp(ServerContext *context, ServerReaderWriter<OpReply, Op> *stream)
{
    Op tmp_op;
    Status s;

    while (stream->Read(&tmp_op))
    {
        Op *op_req = new Op(tmp_op);
        OpReply *op_rep = new OpReply();
        op_rep->set_time(op_req->time());

        s = this->HandleOp(op_req, op_rep);
        // 有操作未正确执行，退出
        if (!s.ok())
            break;
    }

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
        // update the OpReply after every SingleOp
        single_op->set_reply_ptr((uint64_t)op_reply);
        assert((uint64_t)op_reply == single_op->reply_ptr());

        s = this->HandleSingleOp(single_op);
        if (!s.ok())
            break;
    }

    return s;
}

Status RemoteRocksDBServiceImpl::HandleSingleOp(SingleOp *single_op)
{
    rocksdb::Status r_s;
    int iterations = 0;
    int record_cnt = 0;
    rocksdb::Iterator *iter;

    SingleOpReply *single_op_rep;
    OpReply *op_rep = (OpReply *)single_op->reply_ptr();

    rocksdb::WriteOptions wop = rocksdb::WriteOptions();
    // sync write option
    wop.sync = true;
    rocksdb::ReadOptions rop = rocksdb::ReadOptions();

    std::string value;

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

    return (r_s.ok() ? Status::OK : Status::CANCELLED);
}
