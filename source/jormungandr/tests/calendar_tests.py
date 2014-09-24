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


def check_valid_calendar(cal):
    get_not_null(cal, "id")
    get_not_null(cal, "name")
    pattern = get_not_null(cal, "week_pattern")
    is_valid_bool(get_not_null(pattern, "monday"))  # check one field in pattern

    validity_pattern = get_not_null(cal, "validity_pattern")
    is_valid_validity_pattern(validity_pattern)

    exceptions = get_not_null(cal, "exceptions")
    assert len(exceptions) > 0

    active_periods = get_not_null(cal, "active_periods")
    assert len(active_periods) > 0

    beg = get_not_null(active_periods[0], "begin")
    assert is_valid_date(beg)

    end = get_not_null(active_periods[0], "end")
    assert is_valid_date(end)

    #check links


@dataset(["main_ptref_test"])
class TestCalendar(AbstractTestFixture):

    def test_calendars(self):
        json_response = self.query_region("calendars/")

        calendars = get_not_null(json_response, "calendars")

        #we need at least one calendar
        assert calendars

        cal = calendars[0]
        check_valid_calendar(cal)

    def test_calendars_lines(self):
        json_response = self.query_region("calendars/monday/lines")

        lines = get_not_null(json_response, "lines")

        assert lines

    def test_lines_calendars(self):
        json_response = self.query_region("calendars/monday/lines/line:A/calendars")

        calendars = get_not_null(json_response, "calendars")

        assert calendars
        check_valid_calendar(calendars[0])


