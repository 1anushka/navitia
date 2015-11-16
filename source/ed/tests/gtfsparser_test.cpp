/* Copyright © 2001-2014, Canal TP and/or its affiliates. All rights reserved.

This file is part of Navitia,
    the software to build cool stuff with public transport.

Hope you'll enjoy and contribute to this project,
    powered by Canal TP (www.canaltp.fr).
Help us simplify mobility and open public transport:
    a non ending quest to the responsive locomotion way of traveling!

LICENCE: This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.

Stay tuned using
twitter @navitia
IRC #navitia on freenode
https://groups.google.com/d/forum/navitia
www.navitia.io
*/

#include "ed/data.h"
#include "ed/connectors/gtfs_parser.h"
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ed
#include <boost/test/unit_test.hpp>
#include <string>
#include "conf.h"
#include "ed/build_helper.h"
#include "utils/csv.h"

struct logger_initialized {
    logger_initialized()   { init_logger(); }
};
BOOST_GLOBAL_FIXTURE( logger_initialized )

const std::string gtfs_path = "/ed/gtfs";

BOOST_AUTO_TEST_CASE(required_files) {
    std::vector<std::string> files = {"agency", "routes", "stop_times", "trips"};
    for(auto file : files)
    {
        ed::Data data;
        ed::connectors::GtfsParser parser(
            std::string(navitia::config::fixtures_dir) +  gtfs_path + "_sans_" + file);
        BOOST_REQUIRE_THROW(parser.fill(data, "20110105"), ed::connectors::FileNotFoundException);
    }
}

BOOST_AUTO_TEST_CASE(parse_agencies) {
    std::vector<std::string> fields={"agency_id", "agency_name", "agency_url",
        "agency_timezone", "agency_lang", "agency_phone", "agency_fare_url"},
    required_fields = {"agency_name", "agency_url", "agency_timezone"};

    using file_parser = ed::connectors::FileParser<ed::connectors::AgencyGtfsHandler>;
    //Check mandatory fields
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path);
        BOOST_REQUIRE_THROW(file_parser(parser.gtfs_data, sstream).fill(data), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path);
        BOOST_REQUIRE_NO_THROW(file_parser(parser.gtfs_data, sstream).fill(data));
    }

    //Check that the networks are correctly filled
    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",") << "\n";
        sstream << "ratp, RATP,,Europe/Paris,,,\n";
        sstream << ", ACME,,America/New_York,,,";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path);
        file_parser(parser.gtfs_data, sstream).fill(data);

        BOOST_REQUIRE_EQUAL(data.networks.size(), 2);
        BOOST_REQUIRE_EQUAL(data.networks.front()->uri, "ratp");
        BOOST_REQUIRE_EQUAL(data.networks.front()->name, "RATP");
        BOOST_REQUIRE_EQUAL(data.networks[1]->uri, "");
        BOOST_REQUIRE_EQUAL(data.networks[1]->name, "ACME");
    }
}

BOOST_AUTO_TEST_CASE(parse_stops) {
    std::vector<std::string> fields={"stop_id","stop_code", "stop_name", "stop_desc", "stop_lat",
                                     "stop_lon", "zone_id", "stop_url", "location_type", "parent_station",
                                     "stop_timezone", "wheelchair_boarding"},
            required_fields = {"stop_id", "stop_name", "stop_lat", "stop_lon"};

    using file_parser = ed::connectors::FileParser<ed::connectors::StopsGtfsHandler>;
    //Check mandatory fields
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path);
        BOOST_REQUIRE_THROW(file_parser(parser.gtfs_data, sstream).fill(data), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path);
        BOOST_REQUIRE_NO_THROW(file_parser(parser.gtfs_data, sstream).fill(data));
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(required_fields, ",") << "\n";
        sstream << "\"a\", \"A\",\"bad_lon\",\"bad_lat\"";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path);
        BOOST_REQUIRE_NO_THROW(file_parser(parser.gtfs_data, sstream).fill(data));
    }
}

BOOST_AUTO_TEST_CASE(parse_transfers) {

    std::vector<std::string> fields={"from_stop_id","to_stop_id","transfer_type",
                                     "min_transfer_type"},
            required_fields = {"from_stop_id","to_stop_id"};

    using file_parser = ed::connectors::FileParser<ed::connectors::TransfersGtfsHandler>;
    //Check mandatory fields
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path);
        BOOST_REQUIRE_THROW(file_parser(parser.gtfs_data, sstream).fill(data), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path);
        BOOST_REQUIRE_NO_THROW(file_parser(parser.gtfs_data, sstream).fill(data));
    }
}

BOOST_AUTO_TEST_CASE(parse_lines) {
    std::vector<std::string> fields={"route_id","agency_id","route_short_name",
                                     "route_long_name", "route_desc",
                                     "route_type", "route_url", "route_color",
                                     "route_text_color"},
            required_fields = {"route_id", "route_short_name", "route_long_name",
                               "route_type"};

    using file_parser = ed::connectors::FileParser<ed::connectors::RouteGtfsHandler>;
    //Check mandatory fields
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path);
        BOOST_REQUIRE_THROW(file_parser(parser.gtfs_data, sstream).fill(data), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path);
        BOOST_REQUIRE_NO_THROW(file_parser(parser.gtfs_data, sstream).fill(data));
    }
}


BOOST_AUTO_TEST_CASE(parse_trips) {
    std::vector<std::string> fields={"route_id","service_id","trip_id",
                "trip_headsign", "trip_short_name", "direction_id", "block_id",
                "shape_id", "wheelchair_accessible"},
            required_fields = {"route_id", "service_id", "trip_id"};

    using file_parser = ed::connectors::FileParser<ed::connectors::TripsGtfsHandler>;
    //Check mandatory fields
    for(auto required_field : required_fields) {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join_if(fields, "," ,[&](std::string s1) {return s1 == required_field;});
        sstream << "\n";
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path);
        BOOST_REQUIRE_THROW(file_parser(parser.gtfs_data, sstream).fill(data), ed::connectors::InvalidHeaders);
    }

    {
        std::stringstream sstream(std::stringstream::in | std::stringstream::out);
        sstream << boost::algorithm::join(fields, ",");
        ed::Data data;
        ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path);
        BOOST_REQUIRE_NO_THROW(file_parser(parser.gtfs_data, sstream).fill(data));
    }
}

BOOST_AUTO_TEST_CASE(parse_gtfs_no_dst){
    /*
     * use import the google gtfs example file with one difference, the time zone "America/Los_Angeles" of the dataset
     * has been changed to "Africa/Abidjan" because "Africa/Abidjan" has no dst and no utc offset
     */
    ed::Data data;
    ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path + "_google_example_no_dst");
    parser.fill(data);

    //Agency
    BOOST_REQUIRE_EQUAL(data.networks.size(), 1);
    BOOST_CHECK_EQUAL(data.networks[0]->name, "Demo Transit Authority");
    BOOST_CHECK_EQUAL(data.networks[0]->uri, "DTA");

    //=> no stop area in the file, so one area has been created for each stop point
    //Stop areas
    BOOST_REQUIRE_EQUAL(data.stop_areas.size(), 9);
    BOOST_CHECK_EQUAL(data.stop_areas[0]->uri, "FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(data.stop_areas[0]->name, "Furnace Creek Resort (Demo)");
    BOOST_CHECK_CLOSE(data.stop_areas[0]->coord.lat(), 36.425288, 0.1);
    BOOST_CHECK_CLOSE(data.stop_areas[0]->coord.lon(), -117.133162, 0.1);

    BOOST_CHECK_EQUAL(data.stop_areas[8]->uri, "AMV");
    BOOST_CHECK_EQUAL(data.stop_areas[8]->name, "Amargosa Valley (Demo)");
    BOOST_CHECK_CLOSE(data.stop_areas[8]->coord.lat(), 36.641496, 0.1);
    BOOST_CHECK_CLOSE(data.stop_areas[8]->coord.lon(), -116.40094, 0.1);
    //timzeone check
    //no timezone is given for the stop area in this dataset, to the agency time zone (the default one) is taken
    for (auto sa: data.stop_areas) {
//        BOOST_CHECK_EQUAL(sa->time_zone_with_name.first, "America/Los_Angeles");
        BOOST_CHECK_EQUAL(sa->time_zone_with_name.first, "Africa/Abidjan");
    }

    //Stop points
    BOOST_REQUIRE_EQUAL(data.stop_points.size(), 9);
    BOOST_CHECK_EQUAL(data.stop_points[0]->uri, "FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(data.stop_points[0]->name, "Furnace Creek Resort (Demo)");
    BOOST_CHECK_CLOSE(data.stop_points[0]->coord.lat(), 36.425288, 0.1);
    BOOST_CHECK_CLOSE(data.stop_points[0]->coord.lon(), -117.133162, 0.1);

    BOOST_CHECK_EQUAL(data.stop_points[8]->uri, "AMV");
    BOOST_CHECK_EQUAL(data.stop_points[8]->name, "Amargosa Valley (Demo)");
    BOOST_CHECK_CLOSE(data.stop_points[8]->coord.lat(), 36.641496, 0.1);
    BOOST_CHECK_CLOSE(data.stop_points[8]->coord.lon(), -116.40094, 0.1);

    //Transfers
    BOOST_REQUIRE_EQUAL(data.stop_point_connections.size(), 0);

    // Lignes
    BOOST_REQUIRE_EQUAL(data.lines.size(), 5);
    BOOST_CHECK_EQUAL(data.lines[0]->uri, "AB");
    BOOST_CHECK_EQUAL(data.lines[0]->name, "Airport - Bullfrog");
    BOOST_REQUIRE(data.lines[0]->network != nullptr);
    BOOST_CHECK_EQUAL(data.lines[0]->network->uri, "DTA");
    BOOST_REQUIRE(data.lines[0]->commercial_mode != nullptr);
    BOOST_CHECK_EQUAL(data.lines[0]->commercial_mode->uri, "3");

    BOOST_CHECK_EQUAL(data.lines[4]->uri, "AAMV");
    BOOST_CHECK_EQUAL(data.lines[4]->name, "Airport - Amargosa Valley");
    BOOST_REQUIRE(data.lines[4]->network != nullptr);
    BOOST_CHECK_EQUAL(data.lines[4]->network->uri, "DTA");
    BOOST_REQUIRE(data.lines[4]->commercial_mode != nullptr);
    BOOST_CHECK_EQUAL(data.lines[4]->commercial_mode->uri, "3");

    //Trips
    BOOST_REQUIRE_EQUAL(data.vehicle_journeys.size(), 11);
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->uri, "AB1");
    BOOST_REQUIRE(data.vehicle_journeys[0]->validity_pattern != nullptr);
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->validity_pattern->uri, "FULLW");
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->name, "to Bullfrog");
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->block_id, "1");
    BOOST_REQUIRE(data.vehicle_journeys[0]->route != nullptr);
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->route->uri, "AB:0");

    BOOST_CHECK_EQUAL(data.vehicle_journeys[10]->uri, "AAMV4");
    BOOST_REQUIRE(data.vehicle_journeys[10]->validity_pattern != nullptr);
    BOOST_CHECK_EQUAL(data.vehicle_journeys[10]->validity_pattern->uri, "WE");
    BOOST_CHECK_EQUAL(data.vehicle_journeys[10]->name, "to Airport");
    BOOST_CHECK_EQUAL(data.vehicle_journeys[10]->block_id, "");
    BOOST_REQUIRE(data.vehicle_journeys[10]->route != nullptr);
    BOOST_CHECK_EQUAL(data.vehicle_journeys[10]->route->uri, "AAMV:1");

    //Calendar
    BOOST_REQUIRE_EQUAL(data.validity_patterns.size(), 2);
    BOOST_CHECK_EQUAL(data.validity_patterns[0]->uri, "FULLW");
    auto vp = data.validity_patterns[0];
    BOOST_CHECK_EQUAL(vp->check((boost::gregorian::date(2010, 12, 31) - vp->beginning_date).days()), true);

    BOOST_CHECK_EQUAL(data.validity_patterns[1]->uri, "WE");

    //Stop time
    BOOST_REQUIRE_EQUAL(data.stops.size(), 28);
    BOOST_REQUIRE(data.stops[0]->vehicle_journey != nullptr);
    BOOST_CHECK_EQUAL(data.stops[0]->vehicle_journey->uri, "STBA");
    BOOST_CHECK_EQUAL(data.stops[0]->arrival_time, 6*3600);
    BOOST_CHECK_EQUAL(data.stops[0]->departure_time, 6*3600);
    BOOST_REQUIRE(data.stops[0]->stop_point != nullptr);
    BOOST_CHECK_EQUAL(data.stops[0]->stop_point->uri, "STAGECOACH");
    BOOST_CHECK_EQUAL(data.stops[0]->order, 0);

    BOOST_REQUIRE(data.stops[1]->vehicle_journey != nullptr);
    BOOST_CHECK_EQUAL(data.stops[1]->vehicle_journey->uri, "STBA");
    BOOST_CHECK_EQUAL(data.stops[1]->arrival_time, 6*3600 + 20*60);
    BOOST_CHECK_EQUAL(data.stops[1]->departure_time, 6*3600 + 20*60);
    BOOST_REQUIRE(data.stops[1]->stop_point != nullptr);
    BOOST_CHECK_EQUAL(data.stops[1]->stop_point->uri, "BEATTY_AIRPORT");
    BOOST_CHECK_EQUAL(data.stops[1]->order, 1);

    data.complete();
}

static void check_gtfs_google_example(const ed::Data& data, const ed::connectors::GtfsParser& parser) {
    //Agency and stop areas should not have changed compared to parse_gtfs_no_dst
    BOOST_REQUIRE_EQUAL(data.networks.size(), 1);
    BOOST_CHECK_EQUAL(data.networks[0]->name, "Demo Transit Authority");
    BOOST_CHECK_EQUAL(data.networks[0]->uri, "DTA");

    BOOST_REQUIRE_EQUAL(data.stop_areas.size(), 9);
    BOOST_CHECK_EQUAL(data.stop_areas[0]->uri, "FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(data.stop_areas[0]->name, "Furnace Creek Resort (Demo)");
    BOOST_CHECK_CLOSE(data.stop_areas[0]->coord.lat(), 36.425288, 0.1);
    BOOST_CHECK_CLOSE(data.stop_areas[0]->coord.lon(), -117.133162, 0.1);

    BOOST_CHECK_EQUAL(data.stop_areas[8]->uri, "AMV");
    BOOST_CHECK_EQUAL(data.stop_areas[8]->name, "Amargosa Valley (Demo)");
    BOOST_CHECK_CLOSE(data.stop_areas[8]->coord.lat(), 36.641496, 0.1);
    BOOST_CHECK_CLOSE(data.stop_areas[8]->coord.lon(), -116.40094, 0.1);
    //timzeone check
    //no timezone is given for the stop area in this dataset, to the agency time zone (the default one) is taken
    for (auto sa: data.stop_areas) {
        BOOST_CHECK_EQUAL(sa->time_zone_with_name.first, "America/Los_Angeles");
    }

    //stop point, and lines shoudl be equals too
    BOOST_REQUIRE_EQUAL(data.stop_points.size(), 9);
    BOOST_CHECK_EQUAL(data.stop_points[0]->uri, "FUR_CREEK_RES");
    BOOST_CHECK_EQUAL(data.stop_points[0]->name, "Furnace Creek Resort (Demo)");
    BOOST_CHECK_CLOSE(data.stop_points[0]->coord.lat(), 36.425288, 0.1);
    BOOST_CHECK_CLOSE(data.stop_points[0]->coord.lon(), -117.133162, 0.1);

    BOOST_CHECK_EQUAL(data.stop_points[8]->uri, "AMV");
    BOOST_CHECK_EQUAL(data.stop_points[8]->name, "Amargosa Valley (Demo)");
    BOOST_CHECK_CLOSE(data.stop_points[8]->coord.lat(), 36.641496, 0.1);
    BOOST_CHECK_CLOSE(data.stop_points[8]->coord.lon(), -116.40094, 0.1);

    BOOST_REQUIRE_EQUAL(data.stop_point_connections.size(), 0);

    BOOST_REQUIRE_EQUAL(data.lines.size(), 5);
    BOOST_CHECK_EQUAL(data.lines[0]->uri, "AB");
    BOOST_CHECK_EQUAL(data.lines[0]->name, "Airport - Bullfrog");
    BOOST_REQUIRE(data.lines[0]->network != nullptr);
    BOOST_CHECK_EQUAL(data.lines[0]->network->uri, "DTA");
    BOOST_REQUIRE(data.lines[0]->commercial_mode != nullptr);
    BOOST_CHECK_EQUAL(data.lines[0]->commercial_mode->uri, "3");

    BOOST_CHECK_EQUAL(data.lines[4]->uri, "AAMV");
    BOOST_CHECK_EQUAL(data.lines[4]->name, "Airport - Amargosa Valley");
    BOOST_REQUIRE(data.lines[4]->network != nullptr);
    BOOST_CHECK_EQUAL(data.lines[4]->network->uri, "DTA");
    BOOST_REQUIRE(data.lines[4]->commercial_mode != nullptr);
    BOOST_CHECK_EQUAL(data.lines[4]->commercial_mode->uri, "3");

    // we need to also check the number of routes created (since they are implicit in GTFS)
    // we create one by line/direction id
    BOOST_REQUIRE_EQUAL(data.routes.size(), 9);
    BOOST_CHECK_EQUAL(data.routes[0]->uri, "AB:0"); //NOTE: order is not important
    BOOST_CHECK_EQUAL(data.routes[1]->uri, "AB:1");
    BOOST_CHECK_EQUAL(data.routes[2]->uri, "STBA:");
    BOOST_CHECK_EQUAL(data.routes[3]->uri, "CITY:0");
    BOOST_CHECK_EQUAL(data.routes[4]->uri, "CITY:1");
    BOOST_CHECK_EQUAL(data.routes[5]->uri, "BFC:0");
    BOOST_CHECK_EQUAL(data.routes[6]->uri, "BFC:1");
    BOOST_CHECK_EQUAL(data.routes[7]->uri, "AAMV:0");
    BOOST_CHECK_EQUAL(data.routes[8]->uri, "AAMV:1");
    for (const auto& r: data.routes) {
        BOOST_CHECK_NE(r->idx, navitia::invalid_idx);
        BOOST_REQUIRE(r->line);
        BOOST_CHECK_EQUAL(r->name, r->line->name);//route's name is it's line's name
    }

    //Calendar, Trips and stop times are another matters
    //we have to split the trip validity period in such a fashion that the period does not overlap a dst

    BOOST_CHECK_EQUAL(parser.gtfs_data.production_date, boost::gregorian::date_period(
                            boost::gregorian::from_undelimited_string("20070101"),
                            boost::gregorian::from_undelimited_string("20080102")));

    //the dst in los angeles is from the second sunday of march to the first sunday of november
    // -> we thus have to split the period in 9
    //          2007                       2008                     2009                    2010
    // |------------------------|------------------------|------------------------|------------------------|
    //        [----------]             [----------]             [----------]             [----------]
    //            DST                       DST                      DST                     DST
    //     1       2            3            4           5            6           7           8        9
    //
    // technically we can handle only one year of data in navitia, so we limit the period to 2007 -> 2008
    // we group each vp by dst, so we split each period in 2:
    // [------)          |------)  VP1
    //        [----------)         VP2

    BOOST_REQUIRE_EQUAL(data.validity_patterns.size(), 2 * 2);
    BOOST_CHECK_EQUAL(data.validity_patterns[0]->uri, "FULLW_1");
    BOOST_CHECK_EQUAL(data.validity_patterns[1]->uri, "FULLW_2");

    BOOST_CHECK_EQUAL(data.validity_patterns[2]->uri, "WE_1");
    BOOST_CHECK_EQUAL(data.validity_patterns[3]->uri, "WE_2");

    // same for the vj, all have been split in 9
    BOOST_REQUIRE_EQUAL(data.vehicle_journeys.size(), 11 * 2);
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->uri, "AB1_dst_1");
    BOOST_REQUIRE(data.vehicle_journeys[0]->validity_pattern != nullptr);
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->validity_pattern->uri, "FULLW_1");
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->name, "to Bullfrog");
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->block_id, "1");
    BOOST_REQUIRE(data.vehicle_journeys[0]->route != nullptr);
    BOOST_CHECK_EQUAL(data.vehicle_journeys[0]->route->uri, "AB:0");

    for (int i = 1; i <= 2; ++i) {
        BOOST_CHECK_EQUAL(data.vehicle_journeys[i-1]->uri, "AB1_dst_" + std::to_string(i));
        BOOST_REQUIRE(data.vehicle_journeys[i-1]->validity_pattern != nullptr);
        BOOST_CHECK_EQUAL(data.vehicle_journeys[i-1]->validity_pattern->uri, "FULLW_" + std::to_string(i));
        BOOST_REQUIRE(data.vehicle_journeys[i-1]->route != nullptr);
    }

    //Stop time
    BOOST_REQUIRE_EQUAL(data.stops.size(), 28 * 2);
    BOOST_REQUIRE(data.stops[0]->vehicle_journey != nullptr);
    BOOST_CHECK_EQUAL(data.stops[0]->vehicle_journey->uri, "STBA_dst_1");
    BOOST_CHECK_EQUAL(data.stops[0]->arrival_time, 6*3600 + 480*60); //first day is on a non dst period, so the utc offset
    BOOST_CHECK_EQUAL(data.stops[0]->departure_time, 6*3600 + 480*60); //for los angeles is -480 minutes
    BOOST_REQUIRE(data.stops[0]->stop_point != nullptr);
    BOOST_CHECK_EQUAL(data.stops[0]->stop_point->uri, "STAGECOACH");
    BOOST_CHECK_EQUAL(data.stops[0]->order, 0);

    BOOST_REQUIRE(data.stops[1]->vehicle_journey != nullptr);
    BOOST_CHECK_EQUAL(data.stops[1]->vehicle_journey->uri, "STBA_dst_2");
    BOOST_CHECK_EQUAL(data.stops[1]->arrival_time, 6*3600 + 420*60); //the second st is on a dst period, so the utc offset
    BOOST_CHECK_EQUAL(data.stops[1]->departure_time, 6*3600 + 420*60); //for los angeles is -420 minutes
    BOOST_REQUIRE(data.stops[1]->stop_point != nullptr);
    BOOST_CHECK_EQUAL(data.stops[1]->stop_point->uri, "STAGECOACH");
    BOOST_CHECK_EQUAL(data.stops[1]->order, 0);

    for (auto st : data.stops) {
        BOOST_CHECK(st->departure_time > 0);
        BOOST_CHECK(st->arrival_time > 0);
    }
}

BOOST_AUTO_TEST_CASE(parse_gtfs){
    /*
     * use import the raw google gtfs example file
     *
     * We check the diff that can occur because the dataset timezone has dst
     */
    ed::Data data;
    ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path + "_google_example");
    parser.fill(data);

    check_gtfs_google_example(data, parser);
}



BOOST_AUTO_TEST_CASE(parse_gtfs_without_calendar) {
    //calendar.txt is not a mandatory file
    //we created the same data set than the standard one but with calendar_date.txt filled
    //instead of calendar.
    //we must have the exact same thing

    ed::Data data;
    ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path + "_google_example_no_calendar");
    parser.fill(data);

    check_gtfs_google_example(data, parser);
}

BOOST_AUTO_TEST_CASE(parse_gtfs_line_without_network){
    /*
     * import the google gtfs example file with one difference, there is one line without a agency (a network for us)
     *
     * this line should thus be linked to a default_network
     */
    ed::Data data;
    ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path + "_line_without_network");
    parser.fill(data);

    //since there is a real network in the dataset, we got 2 network
    BOOST_REQUIRE_EQUAL(data.networks.size(), 2);
    BOOST_CHECK_EQUAL(data.networks[0]->name, "Demo Transit Authority");
    BOOST_CHECK_EQUAL(data.networks[0]->uri, "DTA");
    BOOST_CHECK_EQUAL(data.networks[1]->name, "default network");
    BOOST_CHECK_EQUAL(data.networks[1]->uri, "default_network");

    BOOST_REQUIRE_EQUAL(data.lines.size(), 5);
    BOOST_CHECK_EQUAL(data.lines[0]->uri, "AB");
    BOOST_CHECK_EQUAL(data.lines[0]->name, "Airport - Bullfrog");
    BOOST_REQUIRE(data.lines[0]->network != nullptr);
    BOOST_CHECK_EQUAL(data.lines[0]->network->uri, "default_network");

}


//TODO: work on this, we should be able to parse line with \\ in char
//BOOST_AUTO_TEST_CASE(parse_gtfs_with_slashs) {
//    ed::Data data;
//    ed::connectors::GtfsData gtfs_data;
//    //we need to provide by hand some data before loading the file
//    std::string stop_file = std::string(navitia::config::fixtures_dir) + gtfs_path + "_slashs" + "/stops.txt";
//    ed::connectors::FileParser<ed::connectors::StopsGtfsHandler> parser (gtfs_data, stop_file, true);
//    parser.fill(data);

//    BOOST_REQUIRE_EQUAL(data.stop_points.size(), 1);

//    //TODO tests on the loaded stop point
//    auto stop_area =  data.stop_points.front();
//    BOOST_CHECK_EQUAL(stop_area->uri, "141581");
//    BOOST_CHECK_EQUAL(stop_area->name, "16058601");
//    BOOST_CHECK_EQUAL(stop_area->comment, "Siebengewald, Gochsedijk\\Centrum");
//}

BOOST_AUTO_TEST_CASE(boost_year_iterator) {
    {
        boost::gregorian::date_period validity_period { boost::gregorian::from_undelimited_string("20120101"), boost::gregorian::from_undelimited_string("20150102")};
        std::vector<int> years;
        for (boost::gregorian::year_iterator y_it(validity_period.begin()); boost::gregorian::date((*y_it).year(), 1, 1) < validity_period.end(); ++y_it) {
            years.push_back((*y_it).year());
        }
        BOOST_REQUIRE_EQUAL(years.size(), 4);
    }
    {
        boost::gregorian::date_period validity_period { boost::gregorian::from_undelimited_string("20120101"), boost::gregorian::from_undelimited_string("20120104")};
        std::vector<int> years;
        for (boost::gregorian::year_iterator y_it(validity_period.begin()); boost::gregorian::date((*y_it).year(), 1, 1) < validity_period.end(); ++y_it) {
            years.push_back((*y_it).year());
        }
        BOOST_REQUIRE_EQUAL(years.size(), 1);
    }
}

/*
 * test the get_dst_periods method
 *
 * the returned periods must be a partition of the validity period => all day must be in exactly one period
 */
BOOST_AUTO_TEST_CASE(get_dst_periods) {
    boost::gregorian::date_period validity_period {"20120101"_d, "20150102"_d};
    ed::connectors::GtfsData gtfs_data;
    auto tz_pair = gtfs_data.tz.get_tz("Europe/Paris");

    BOOST_REQUIRE(tz_pair.second);
    BOOST_REQUIRE_EQUAL(tz_pair.first, "Europe/Paris");

    auto res = ed::connectors::get_dst_periods(validity_period, tz_pair.second);

    for (boost::gregorian::day_iterator d(validity_period.begin()); d < validity_period.end(); ++d) {
        //we must find all day in exactly one period
        int nb_found = 0;
        for (auto p: res) {
            if (p.period.contains(*d)) {
                ++nb_found;
            }
        }
        if (nb_found == 0) {
            std::cout << "day " << *d << " found in no period" << std::endl;
        } else if (nb_found > 1) {
            std::cout << "day " << *d << " found in more than one period" << std::endl;
        }
        BOOST_CHECK_EQUAL(nb_found, 1);
    }
}

/*
 * Test that split_over_dst correctly split the sub_period
 *
 * Period is :
 *
 *         01/02/2012----------------------------------05/11/2012
 *                winter dst  |  summer dst      | winter dst
 *
 *    we must have 2 groups:
 *    one on the winter utc shift with 2 periods
 *    one on the summer utc shift with 1 period
 */
BOOST_AUTO_TEST_CASE(split_over_dst) {
    ed::connectors::GtfsData gtfs_data;
    auto tz_pair = gtfs_data.tz.get_tz("Europe/Paris");

    boost::gregorian::date_period vj_validity_period {"20120201"_d, "20121105"_d};

    auto split_periods = ed::connectors::split_over_dst(vj_validity_period, tz_pair.second);

    BOOST_REQUIRE_EQUAL(split_periods.size(), 2);

    const auto winter_paris_utc_shift = 60 * 60;
    const auto& winter_periods = split_periods.at(winter_paris_utc_shift);
    BOOST_REQUIRE_EQUAL(winter_periods.size(), 2);
    BOOST_CHECK_EQUAL(winter_periods.at(0).begin(), "20120201"_d);
    BOOST_CHECK_EQUAL(winter_periods.at(0).last(), "20120324"_d);
    BOOST_CHECK_EQUAL(winter_periods.at(1).begin(), "20121028"_d);
    BOOST_CHECK_EQUAL(winter_periods.at(1).last(), "20121104"_d);

    const auto summer_paris_utc_shift = 2 * 60 * 60;
    const auto& summer_periods = split_periods.at(summer_paris_utc_shift);
    BOOST_REQUIRE_EQUAL(summer_periods.size(), 1);
    BOOST_CHECK_EQUAL(summer_periods.at(0).begin(), "20120325"_d);
    BOOST_CHECK_EQUAL(summer_periods.at(0).last(), "20121027"_d);
}

BOOST_AUTO_TEST_CASE(parse_with_feed_info) {
    ed::Data data;
    ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir) + gtfs_path + "_with_feed_info");
    parser.fill(data);

    BOOST_REQUIRE_EQUAL(data.feed_infos.size(), 5);
    BOOST_CHECK_EQUAL(data.feed_infos["feed_publisher_name"], "ratp");
    BOOST_CHECK_EQUAL(data.feed_infos["feed_publisher_url"], "http://ratp.fr");

}
/*
 Test start_date and en_date in file feed_info, without beginning_date
 */
BOOST_AUTO_TEST_CASE(gtfs_with_feed_start_end_date_1) {
    ed::Data data;
    ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir)
                                      + gtfs_path + "_with_feed_start_end_date");
    parser.fill(data);

    BOOST_REQUIRE_EQUAL(data.feed_infos.size(), 5);
    BOOST_CHECK_EQUAL(data.feed_infos["feed_start_date"], "20100115");
    BOOST_CHECK_EQUAL(data.feed_infos["feed_end_date"], "20101226");

    BOOST_REQUIRE_EQUAL(parser.gtfs_data.production_date,
                        boost::gregorian::date_period(boost::gregorian::date(2010, 01, 15),
                                                      boost::gregorian::date(2010, 12, 27)));
}

/*
 Test start_date and en_date in file feed_info, with beginning_date

beginning_date  20100113

                                                    |--------------------------------------|
                                                start_date(20100115)               end_date(20101226)
production date :                                   |-----------------------------------------|
                                                 start_date                                 end_date + 1 Day

 */
BOOST_AUTO_TEST_CASE(gtfs_with_feed_start_end_date_2) {
    ed::Data data;
    ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir)
                                      + gtfs_path + "_with_feed_start_end_date");
    parser.fill(data, "20100113");

    BOOST_REQUIRE_EQUAL(data.feed_infos.size(), 5);
    BOOST_CHECK_EQUAL(data.feed_infos["feed_start_date"], "20100115");
    BOOST_CHECK_EQUAL(data.feed_infos["feed_end_date"], "20101226");

    BOOST_REQUIRE_EQUAL(parser.gtfs_data.production_date,
                        boost::gregorian::date_period(boost::gregorian::date(2010, 01, 15),
                                                      boost::gregorian::date(2010, 12, 27)));
}


/*
 Test start_date and en_date in file feed_info, with beginning_date

beginning_date                                          20100117
                                                           |

                                                    |--------------------------------------|
                                                start_date(20100115)               end_date(20101226)
production date :                                          |---------------------------------|
                                                        beginning_date                   end_date + 1 Day

 */
BOOST_AUTO_TEST_CASE(gtfs_with_feed_start_end_date_3) {
    ed::Data data;
    ed::connectors::GtfsParser parser(std::string(navitia::config::fixtures_dir)
                                      + gtfs_path + "_with_feed_start_end_date");
    parser.fill(data, "20100117");

    BOOST_REQUIRE_EQUAL(data.feed_infos.size(), 5);
    BOOST_CHECK_EQUAL(data.feed_infos["feed_start_date"], "20100115");
    BOOST_CHECK_EQUAL(data.feed_infos["feed_end_date"], "20101226");
    BOOST_REQUIRE_EQUAL(parser.gtfs_data.production_date,
                        boost::gregorian::date_period(boost::gregorian::date(2010, 01, 17),
                                                      boost::gregorian::date(2010, 12, 27)));
}
