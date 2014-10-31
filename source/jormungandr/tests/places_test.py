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
import logging

from tests_mechanism import AbstractTestFixture, dataset
from check_utils import *


@dataset(["main_routing_test"])
class TestPlaces(AbstractTestFixture):
    """
    Test places responses
    """

    def test_places_by_id(self):
        """can we get the complete address from coords"""

        # we transform x,y to lon,lat using N_M_TO_DEG constant
        lon = 10. / 111319.9
        lat = 100. / 111319.9
        response = self.query_region("places/{};{}".format(lon, lat))

        assert(len(response['places']) == 1)
        is_valid_places(response['places'])
        assert(response['places'][0]['name'] == "42 rue kb (Condom)")

    def test_places_do_not_loose_precision(self):
        """do we have a good precision given back in the id"""

        # it should work for any id with 15 digits max on each coords
        # that returns a result
        id = "8.9831195195e-05;0.000898311281954"
        response = self.query_region("places/{}".format(id))

        assert(len(response['places']) == 1)
        is_valid_places(response['places'])
        assert(response['places'][0]['id'] == id)
        assert(response['places'][0]['address']['id'] == id)
