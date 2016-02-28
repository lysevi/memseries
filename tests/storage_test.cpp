#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE Main
#include <boost/test/unit_test.hpp>

#include <meas.h>
#include <storage.h>
#include <string>
#include <thread>

const size_t copies_count = 100;
void checkAll(memseries::Meas::MeasList res, std::string msg, memseries::Time from,memseries::Time to,memseries::Time  step) {
    for (auto i = from; i < to; i += step) {
		size_t count = 0;
        for(auto &m:res) {
            if ((m.id == i) && (m.flag == i) && (m.time == i)) {
				count++;
            }
           
        }
		if (count!=copies_count) {
			BOOST_CHECK_EQUAL(copies_count, count);
		}
    }
}

void storage_test_check(memseries::storage::AbstractStorage *as,memseries::Time from,memseries::Time to,memseries::Time  step){
    auto m = memseries::Meas::empty();
    size_t total_count = 0;
	
    for (auto i = from; i < to; i += step) {
        m.id = i;
        m.flag = i;
        m.time = i;
		m.value= 0;
		for (size_t j = 0; j < copies_count; j++) {
			BOOST_CHECK(as->append(m).writed == 1);
			total_count++;
			m.value=j;
		}
    }

    memseries::Meas::MeasList all{};
    as->readInterval(from,to)->readAll(&all);
    BOOST_CHECK_EQUAL(all.size(), total_count);

    checkAll(all, "readAll error: ",from,to,step);

    memseries::IdArray ids{};
    all.clear();
    as->readInterval(ids,0,from,to)->readAll(&all);
    BOOST_CHECK_EQUAL(all.size(), total_count);

    checkAll(all, "read error: ",from,to,step);

    ids.push_back(from+step);
    memseries::Meas::MeasList fltr_res{};
    as->readInterval(ids,0,from,to)->readAll(&fltr_res);

    BOOST_CHECK_EQUAL(fltr_res.size(), copies_count);

    BOOST_CHECK_EQUAL(fltr_res.front().id,ids[0]);

    fltr_res.clear();
    as->readInterval(ids,to+1,from,to)->readAll(&fltr_res);
    BOOST_CHECK_EQUAL(fltr_res.size(),size_t(0));

    all.clear();
    as->readInTimePoint(to)->readAll(&all);
    //TODO ==(to-from)/step
    BOOST_CHECK_EQUAL(all.size(),total_count);

    checkAll(all, "TimePoint error: ",from,to,step);


    memseries::IdArray emptyIDs{};
    fltr_res.clear();
    as->readInTimePoint(to)->readAll(&fltr_res);
    BOOST_CHECK_EQUAL(fltr_res.size(),total_count);

    checkAll(all, "TimePointFltr error: ",from,to,step);

    auto magicFlag = memseries::Flag(from + step);
    fltr_res.clear();
    as->readInTimePoint(emptyIDs,magicFlag,to)->readAll(&fltr_res);
    BOOST_CHECK_EQUAL(fltr_res.size(), copies_count);

    BOOST_CHECK_EQUAL(fltr_res.front().flag,magicFlag);
}


BOOST_AUTO_TEST_CASE(inFilter) {
	{
		BOOST_CHECK(memseries::in_filter(0, 100));
		BOOST_CHECK(!memseries::in_filter(1, 100));
	}
}

BOOST_AUTO_TEST_CASE(MemoryStorage) {
	{
        auto ms = new memseries::storage::MemoryStorage{ 500 };
		const memseries::Time from = 0;
        const memseries::Time to = 100;
        const memseries::Time step = 2;
		storage_test_check(ms, from, to, step);
        //BOOST_CHECK(ms->chinks_size(), (to - from) / step); // id per chunk.
		delete ms;
	}
}

void thread_writer(memseries::Id id, memseries::Time from,memseries::Time to, memseries::Time step, memseries::storage::MemoryStorage *ms)
{
    auto m = memseries::Meas::empty();
    for (auto i = from; i < to; i += step) {
        m.id = id;
        m.flag = i;
        m.time = i;
        m.value= 0;
        for (size_t j = 0; j < copies_count; j++) {
            ms->append(m);
            m.value=j;
        }
    }
}

BOOST_AUTO_TEST_CASE(MultiThread)
{
    auto ms = new memseries::storage::MemoryStorage{ 500 };
    std::thread t1(thread_writer,0,0,100,2,ms);
    std::thread t2(thread_writer,1,0,100,2,ms);
    std::thread t3(thread_writer,2,0,100,2,ms);
    std::thread t4(thread_writer,3,0,100,2,ms);

    t1.join();
    t2.join();
    t3.join();
    t4.join();
    delete ms;
}
