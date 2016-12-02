#include <libdariadb/interfaces/icallbacks.h>

using namespace dariadb;
using namespace dariadb::storage;

IReaderClb::IReaderClb() {
	is_end_called = false;
	is_cancel = false;
}

IReaderClb::~IReaderClb() {
}

void IReaderClb::is_end() {
	is_end_called = true;
	_cond.notify_all();
}

void IReaderClb::wait() {
	std::mutex mtx;
	std::unique_lock<std::mutex> locker(mtx);
	_cond.wait(locker, [this]() { return this->is_end_called; });
	//mtx.unlock();
}

void IReaderClb::cancel() {
  is_cancel = true;
}

bool IReaderClb::is_canceled()const {
	return is_cancel;
}