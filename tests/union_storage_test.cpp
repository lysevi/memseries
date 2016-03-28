#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Main
#include <boost/test/unit_test.hpp>
#include <union_storage.h>
#include <timeutil.h>

BOOST_AUTO_TEST_CASE(UnionStorage) {
	{
		const std::string storage_path = "testStorage";
		const size_t chunk_per_storage = 1024;
		const size_t chunk_size = 512;
		const size_t cap_max_size = 10000;
		const dariadb::Time write_window_deep = 2000;

		dariadb::storage::AbstractStorage_ptr ms{
			new dariadb::storage::UnionStorage(storage_path,
			dariadb::storage::STORAGE_MODE::SINGLE,
				chunk_per_storage,
				chunk_size,
				write_window_deep,
				cap_max_size) };

		const dariadb::Time from = 0;
		const dariadb::Time to = 100;
		const dariadb::Time step = 2;
		auto e = dariadb::Meas::empty();

		//max time always
		dariadb::Time t = dariadb::timeutil::current_time();
		for (size_t i = 0; i < cap_max_size; i++) {
			e.time = t;
			t += 1;
			BOOST_CHECK(ms->append(e).writed==1);
		}
	}
}

