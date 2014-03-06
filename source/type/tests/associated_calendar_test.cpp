#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE associated_calendar_test
#include <boost/test/unit_test.hpp>
#include "type/type.h"
#include "ed/build_helper.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

//helper for lazyness
boost::gregorian::date date(std::string str) {
    return boost::gregorian::from_undelimited_string(str);
}

boost::posix_time::time_period period(std::string beg, std::string end) {
    boost::posix_time::ptime start_date = boost::posix_time::from_iso_string(beg + "T000000");
    boost::posix_time::ptime end_date = boost::posix_time::from_iso_string(end + "T000000"); //end is not in the period
    return {start_date, end_date};
}

struct calendar_fixture {
    calendar_fixture() : b("20140210"),
        cal(new navitia::type::Calendar(b.data.meta.production_date.begin())) {
        cal->uri="cal1";
        boost::posix_time::ptime start = boost::posix_time::from_iso_string("20140210T000010");
        boost::posix_time::ptime end = boost::posix_time::from_iso_string("20140214T000000"); //end is not in the period
        cal->active_periods.push_back({start, end});
        cal->week_pattern = std::bitset<7>("1111111");
        b.data.pt_data.calendars.push_back(cal);
    }
    ed::builder b;
    navitia::type::Calendar* cal;
};

BOOST_FIXTURE_TEST_CASE(build_validity_pattern_test1, calendar_fixture) {
    /*
    Periodes :
           10/02/2014                    13/02/2014
               +-----------------------------+
    */
    cal->build_validity_pattern();
    BOOST_CHECK(cal->validity_pattern.check(date("20140210")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140211")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140212")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140213")));
    BOOST_CHECK(! cal->validity_pattern.check(date("20140214")));
    BOOST_CHECK(! cal->validity_pattern.check(date("20140514")));
}

BOOST_FIXTURE_TEST_CASE(build_validity_pattern_test2, calendar_fixture) {
    /*
    Periodes :
           10/02/2014                   13/02/2014
               +----------------------------+
    */
    navitia::type::ExceptionDate exd;
    exd.date = date("20140212");
    exd.type = navitia::type::ExceptionDate::ExceptionType::sub;
    cal->exceptions.push_back(exd);

    cal->build_validity_pattern();

    BOOST_CHECK(cal->validity_pattern.check(date("20140210")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140211")));
    BOOST_CHECK(! cal->validity_pattern.check(date("20140212")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140213")));
}

BOOST_FIXTURE_TEST_CASE(build_validity_pattern_test3, calendar_fixture) {
    /*
    Periodes :
           10/02/2014                   13/02/2014
               +----------------------------+
    */
    navitia::type::ExceptionDate exd;
    exd.date = date("20140212");
    exd.type = navitia::type::ExceptionDate::ExceptionType::add;
    cal->exceptions.push_back(exd);

    cal->build_validity_pattern();

    BOOST_CHECK(cal->validity_pattern.check(date("20140210")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140211")));
    BOOST_CHECK(cal->validity_pattern.check(date("20140213")));
}

BOOST_AUTO_TEST_CASE(get_differences_test) {
    std::bitset<12> cal ("111000111000");
    std::bitset<12> vj ("101010101010");

    //the dif are the differences between cal and vj restricted on the active days of the calendar
    auto res = navitia::type::get_difference(cal, vj);

    BOOST_CHECK_EQUAL(res, std::bitset<12>("010000010000"));
}

BOOST_AUTO_TEST_CASE(get_differences_test_empty_cal) {
    std::bitset<12> cal ("000000000000");
    std::bitset<12> vj ("101010101010");

    auto res = navitia::type::get_difference(cal, vj);

    BOOST_CHECK_EQUAL(res, std::bitset<12>("000000000000"));
}

BOOST_AUTO_TEST_CASE(get_differences_test_full_cal) {
    std::bitset<12> cal ("111111111111");
    std::bitset<12> vj ("101010101010");

    auto res = navitia::type::get_difference(cal, vj);

    BOOST_CHECK_EQUAL(res, std::bitset<12>("010101010101"));
}

struct associated_cal_fixture {
    associated_cal_fixture() : b("20140101"),
        always_on_cal(new navitia::type::Calendar(b.data.meta.production_date.begin())),
        holidays_cal(new navitia::type::Calendar(b.data.meta.production_date.begin())),
        sunday_cal(new navitia::type::Calendar(b.data.meta.production_date.begin())) {
        {
            always_on_cal->uri="always_on";
            always_on_cal->active_periods.push_back(period("20140101", "20150101"));
            always_on_cal->week_pattern = std::bitset<7>("1111111");
            b.data.pt_data.calendars.push_back(always_on_cal);
            always_on_cal->build_validity_pattern();
        }

        {
            sunday_cal->uri="sunday";
            always_on_cal->active_periods.push_back(period("20140101", "20150101"));
            sunday_cal->week_pattern = std::bitset<7>("0000001");
            b.data.pt_data.calendars.push_back(sunday_cal);
            sunday_cal->build_validity_pattern();
        }

        {
            holidays_cal->uri="holidays";
            always_on_cal->active_periods.push_back(period("20140201", "20140215"));
            always_on_cal->active_periods.push_back(period("20140701", "20140901"));
            always_on_cal->active_periods.push_back(period("20141215", "20150101"));
            holidays_cal->week_pattern = std::bitset<7>("1111100");
            b.data.pt_data.calendars.push_back(holidays_cal);
            holidays_cal->build_validity_pattern();
        }


        b.vj("network:R", "line:A", "1111111", "", true, "vj1")
                ("stop_area:stop1", 10 * 3600 + 15 * 60, 10 * 3600 + 15 * 60)
                ("stop_area:stop2", 11 * 3600 + 10 * 60 ,11 * 3600 + 10 * 60);
        b.lines["line:A"]->calendar_list.push_back(always_on_cal);
        b.lines["line:A"]->calendar_list.push_back(sunday_cal);
        b.lines["line:A"]->calendar_list.push_back(holidays_cal);
    }
    ed::builder b;
    navitia::type::Calendar* always_on_cal;
    navitia::type::Calendar* holidays_cal;
    navitia::type::Calendar* sunday_cal;
};

BOOST_FIXTURE_TEST_CASE(associated_val_test1, associated_cal_fixture) {

    /*
    Periodes :
           10/02/2014                   13/02/2014
               +----------------------------+
    */
    // VehicleJourney à un validitypattern equivalent au validitypattern du calendrier
//    b.data.build_associated_calendar();
//    navitia::type::Calendar* cal = b.data.pt_data.calendars.front();
//    navitia::type::VehicleJourney* vj = b.data.pt_data.vehicle_journeys.front();
//    BOOST_REQUIRE(! vj->associated_calendars.empty());
//    auto it_associated_cal = vj->associated_calendars.find(cal->id);

//    BOOST_REQUIRE(it_associated_cal != vj->associated_calendars.end());

//    auto associated_cal = it_associated_cal->second;
//    BOOST_CHECK((associated_cal->calendar == cal));
//    BOOST_CHECK(associated_cal->calendar->exceptions.size() == 0);
//    BOOST_CHECK(associated_cal->exceptions.size() == 0);

}

BOOST_FIXTURE_TEST_CASE(associated_val_test_threshold, associated_cal_fixture) {

}
