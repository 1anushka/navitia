# Copyright (c) 2001-2014, Canal TP and/or its affiliates. All rights reserved.
#
# This file is part of Navitia,
#     the software to build cool stuff with public transport.
#
# Hope you'll enjoy and contribute to this project,
#     powered by Canal TP (www.canaltp.fr).
# Help us simplify mobility and open public transport:
#     a non ending quest to the responsive locomotion way of traveling!
#
# LICENCE: This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Stay tuned using
# twitter @navitia
# IRC #navitia on freenode
# https://groups.google.com/d/forum/navitia
# www.navitia.io
from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *


def get_impacts(response):
    return {d['id']: d for d in response['disruptions']}

# for the tests we need custom datetime to display the disruptions
default_date_filter = '_current_datetime=20140101T000000'


@dataset(["main_routing_test"])
class TestDisruptions(AbstractTestFixture):
    """
    Test the structure of the disruption api
    """

    def test_disruption_end_point(self):
        """
        simple disruption call

        we'll get everything, so:
        one impacted line
        one impacted network
        one impacted stop_area
        """

        response = self.query_region('traffic_reports?' + default_date_filter)

        traffic_report = get_not_null(response, 'traffic_reports')

        # the disruptions are grouped by network and we have only one network
        eq_(len(traffic_report), 1)

        impacted_lines = get_not_null(traffic_report[0], 'lines')
        eq_(len(impacted_lines), 1)
        is_valid_line(impacted_lines[0], depth_check=0)
        eq_(impacted_lines[0]['id'], 'A')

        lines_disrupt = get_disruptions(impacted_lines[0], response)
        eq_(len(lines_disrupt), 2)
        for d in lines_disrupt:
            is_valid_disruption(d)
        eq_(lines_disrupt[0]['disruption_id'], 'disruption_on_line_A')
        eq_(lines_disrupt[0]['uri'], 'too_bad_again')
        eq_(lines_disrupt[0]['severity']['name'], 'bad severity')

        eq_(lines_disrupt[1]['uri'], 'later_impact')

        impacted_network = get_not_null(traffic_report[0], 'network')
        is_valid_network(impacted_network, depth_check=0)
        eq_(impacted_network['id'], 'base_network')
        network_disrupt = get_disruptions(impacted_network, response)
        eq_(len(network_disrupt), 2)
        for d in network_disrupt:
            is_valid_disruption(d)
        eq_(network_disrupt[0]['disruption_id'], 'disruption_on_line_A')
        eq_(network_disrupt[0]['uri'], 'too_bad_again')

        eq_(network_disrupt[1]['disruption_id'], 'disruption_on_line_A_but_later')
        eq_(network_disrupt[1]['uri'], 'later_impact')

        impacted_stop_areas = get_not_null(traffic_report[0], 'stop_areas')
        eq_(len(impacted_stop_areas), 1)
        is_valid_stop_area(impacted_stop_areas[0], depth_check=0)
        eq_(impacted_stop_areas[0]['id'], 'stopA')
        stop_disrupt = get_disruptions(impacted_stop_areas[0], response)
        eq_(len(stop_disrupt), 1)
        for d in stop_disrupt:
            is_valid_disruption(d)
        eq_(stop_disrupt[0]['disruption_id'], 'disruption_on_stop_A')
        eq_(stop_disrupt[0]['uri'], 'too_bad')

        """
        by querying directly the impacted object, we only find active disruptions
        """
        networks = self.query_region('networks/base_network?' + default_date_filter)
        network = get_not_null(networks, 'networks')[0]
        is_valid_network(network)
        network_disrupt = get_disruptions(network, response)
        eq_(len(network_disrupt), 1)
        for d in network_disrupt:
            is_valid_disruption(d)
        eq_(network_disrupt[0]['disruption_id'], 'disruption_on_line_A')
        eq_(network_disrupt[0]['uri'], 'too_bad_again')

        lines = self.query_region('lines/A?' + default_date_filter)
        line = get_not_null(lines, 'lines')[0]
        is_valid_line(line)
        lines_disrupt = get_disruptions(impacted_lines[0], response)
        eq_(len(lines_disrupt), 2)
        for d in lines_disrupt:
            is_valid_disruption(d)
        eq_(lines_disrupt[0]['disruption_id'], 'disruption_on_line_A')
        eq_(lines_disrupt[0]['uri'], 'too_bad_again')
        eq_(lines_disrupt[0]['severity']['name'], 'bad severity')

        eq_(lines_disrupt[1]['disruption_id'], 'disruption_on_line_A_but_later')
        eq_(lines_disrupt[1]['uri'], 'later_impact')
        eq_(lines_disrupt[1]['severity']['name'], 'information severity')

        stops = self.query_region('stop_areas/stopA?' + default_date_filter)
        stop = get_not_null(stops, 'stop_areas')[0]
        is_valid_stop_area(stop)
        stop_disrupt = get_disruptions(impacted_stop_areas[0], response)
        eq_(len(stop_disrupt), 1)
        for d in stop_disrupt:
            is_valid_disruption(d)
        eq_(stop_disrupt[0]['disruption_id'], 'disruption_on_stop_A')
        eq_(stop_disrupt[0]['uri'], 'too_bad')

    def test_disruption_with_stop_areas(self):
        """
        when calling the pt object stopA, we should get its disruptions
        """

        response = self.query_region('stop_areas/stopA?' + default_date_filter)

        stops = get_not_null(response, 'stop_areas')
        eq_(len(stops), 1)
        stop = stops[0]

        disruptions = get_disruptions(stop, response)
        eq_(len(disruptions), 1)
        is_valid_disruption(disruptions[0])

    def test_disruption_with_departures(self):
        """
        on a departure call on stopB, we should get its disruptions
        """

        response = self.query_region('stop_points/stop_point:stopB/departures?from_datetime=20120614T080000&_current_datetime=20120614T080000')

        departures = get_not_null(response, 'departures')
        eq_(len(departures), 2)

        departure = departures[0]
        disruptions = get_all_disruptions(departure, response)
        assert disruptions
        eq_(len(disruptions), 1)
        assert 'too_bad_all_lines' in disruptions
        is_valid_disruption(disruptions['too_bad_all_lines'][0][0])

        departure = departures[1]
        disruptions = get_all_disruptions(departure, response)
        assert not disruptions

    def test_disruption_with_arrival(self):
        """
        on a arrivals call on stopA, we should get its disruptions
        """

        response = self.query_region('stop_points/stop_point:stopA/arrivals?'
                                     'from_datetime=20120614T070000&_current_datetime=20120614T080000')

        arrivals = get_not_null(response, 'arrivals')
        eq_(len(arrivals), 2)

        arrival = arrivals[0]
        disruptions = get_all_disruptions(arrival, response)
        assert disruptions
        eq_(len(disruptions), 1)
        assert 'too_bad_all_lines' in disruptions
        is_valid_disruption(disruptions['too_bad_all_lines'][0][0])

        arrival = arrivals[1]
        disruptions = get_all_disruptions(arrival, response)
        assert not disruptions

    def test_direct_disruption_call(self):
        """
        when calling the disruptions on the pt object stopB,
        we should get the disruption summary filtered on the stopB

        so
        we get the impacts on the line A (it goes through stopB),
        and the impacts on the network
        """

        response = self.query_region('stop_areas/stopB/traffic_reports?' + default_date_filter)

        traffic_report = get_not_null(response, 'traffic_reports')
        eq_(len(traffic_report), 1)

        impacted_lines = get_not_null(traffic_report[0], 'lines')
        eq_(len(impacted_lines), 1)
        is_valid_line(impacted_lines[0], depth_check=0)
        eq_(impacted_lines[0]['id'], 'A')

        lines_disrupt = get_disruptions(impacted_lines[0], response)
        eq_(len(lines_disrupt), 2)
        eq_(lines_disrupt[0]['disruption_id'], 'disruption_on_line_A')
        eq_(lines_disrupt[0]['uri'], 'too_bad_again')

        impacted_network = get_not_null(traffic_report[0], 'network')

        is_valid_network(impacted_network, depth_check=0)
        eq_(impacted_network['id'], 'base_network')
        network_disrupt = get_disruptions(impacted_network, response)
        eq_(len(network_disrupt), 2)
        eq_(network_disrupt[0]['disruption_id'], 'disruption_on_line_A')
        eq_(network_disrupt[0]['uri'], 'too_bad_again')

        # but we should not have disruption on stop area, B is not disrupted
        assert 'stop_areas' not in traffic_report[0]

    def test_no_disruption(self):
        """
        when calling the pt object stopB, we should get no disruptions
        """

        response = self.query_region('stop_areas/stopB')

        stops = get_not_null(response, 'stop_areas')
        eq_(len(stops), 1)
        stop = stops[0]

        assert 'disruptions' not in stop

    def test_disruption_publication_date_filter(self):
        """
        test the publication date filter

        'disruption_on_line_A_but_publish_later' is published from the 28th of january at 10

        so at 9 it is not in the list, at 11, we get it
        """
        response = self.query_region('traffic_reports?_current_datetime=20140128T090000')

        impacts = get_impacts(response)
        eq_(len(impacts), 3)
        assert 'impact_published_later' not in impacts

        response = self.query_region('traffic_reports?_current_datetime=20140128T130000')

        impacts = get_impacts(response)
        eq_(len(impacts), 4)
        assert 'impact_published_later' in impacts

    def test_disruption_datefilter_limits(self):
        """
        the _current_datetime is by default in UTC, we test that this is correctly taken into account
        disruption_on_line_A is applied from 20140101T000000 to 20140201T115959
        """
        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20140101T000000Z')

        #we should have disruptions at this date
        assert len(response['disruptions']) > 0

        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20131231T235959Z')

        #before the start of the period => no disruptions
        assert len(response['disruptions']) == 0

        #%2B is the code for '+'
        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20140101T005959%2B0100')

        #UTC + 1, so in UTC it's 23h59 the day before => no disruption
        assert len(response['disruptions']) == 0

        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20140101T000000-0100')

        #UTC - 1, so in UTC it's 01h => disruptions
        assert len(response['disruptions']) > 0

        response = self.query_region('stop_areas/stopB/traffic_reports?_current_datetime=20131231T235959-0100')

        #UTC - 1, so in UTC it's 00h59 => disruptions
        assert len(response['disruptions']) > 0

    def test_disruption_on_route(self):
        """
        check that disruption on route are displayed on the corresponding line
        """
        response = self.query_region('traffic_reports?_current_datetime=20150325T090000')

        impacts = get_impacts(response)
        eq_(len(impacts), 0)

        response = self.query_region('traffic_reports?_current_datetime=20150327T090000')

        impacts = get_impacts(response)
        eq_(len(impacts), 1)
        assert 'too_bad_route_A:0' in impacts

        traffic_report = get_not_null(response, 'traffic_reports')
        eq_(len(traffic_report), 1)

        impacted_lines = get_not_null(traffic_report[0], 'lines')
        eq_(len(impacted_lines), 1)
        is_valid_line(impacted_lines[0], depth_check=0)
        eq_(impacted_lines[0]['id'], 'A')

        lines_disrupt = get_disruptions(impacted_lines[0], response)
        eq_(len(lines_disrupt), 1)
        for d in lines_disrupt:
            is_valid_disruption(d)
        eq_(lines_disrupt[0]['disruption_id'], 'disruption_route_A:0')
        eq_(lines_disrupt[0]['uri'], 'too_bad_route_A:0')

        #Check message, channel and types
        disruption_message = get_not_null(response, 'disruptions')
        eq_(len(disruption_message), 1)
        message = get_not_null(disruption_message[0], 'messages')
        eq_(message[0]['text'], 'no luck')
        channel = get_not_null(message[0], 'channel')
        eq_(channel['id'], 'sms')
        eq_(channel['name'], 'sms')
        channel_types = channel['types']
        eq_(len(channel_types), 2)
        eq_(channel_types[0], 'web')
        eq_(channel_types[1], 'sms')


        eq_(message[1]['text'], 'try again')
        channel = get_not_null(message[1], 'channel')
        eq_(channel['id'], 'email')
        eq_(channel['name'], 'email')
        channel_types = channel['types']
        eq_(len(channel_types), 2)
        eq_(channel_types[0], 'web')
        eq_(channel_types[1], 'email')


    def test_disruption_on_route_and_line(self):
        """
        and we check the sort order of the lines
        """
        response = self.query_region('traffic_reports?_current_datetime=20150125T090000')

        impacts = get_impacts(response)
        eq_(len(impacts), 0)

        response = self.query_region('traffic_reports?_current_datetime=20150127T090000')

        impacts = get_impacts(response)
        eq_(len(impacts), 3)
        assert 'too_bad_route_A:0_and_line' in impacts

        traffic_report = get_not_null(response, 'traffic_reports')
        eq_(len(traffic_report), 1)

        impacted_lines = get_not_null(traffic_report[0], 'lines')
        eq_(len(impacted_lines), 3)
        is_valid_line(impacted_lines[0], depth_check=0)
        eq_(impacted_lines[0]['id'], 'B')
        eq_(impacted_lines[1]['id'], 'A')
        eq_(impacted_lines[2]['id'], 'C')

        lines_disrupt = get_disruptions(impacted_lines[1], response)
        eq_(len(lines_disrupt), 1)
        for d in lines_disrupt:
            is_valid_disruption(d)
        eq_(lines_disrupt[0]['disruption_id'], 'disruption_route_A:0_and_line')
        eq_(lines_disrupt[0]['uri'], 'too_bad_route_A:0_and_line')

        #Verify message and channel without any channel type
        message = get_not_null(lines_disrupt[0], 'messages')
        eq_(len(message), 2)
        eq_(message[0]['text'], 'no luck')
        channel = get_not_null(message[0], 'channel')
        eq_(channel['id'], 'sms')
        eq_(channel['name'], 'sms')
        eq_(len(channel['types']), 0)

        eq_(message[1]['text'], 'try again')
        channel = get_not_null(message[1], 'channel')
        eq_(channel['id'], 'sms')
        eq_(channel['name'], 'sms')
        eq_(len(channel['types']), 0)
