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
from jormungandr.interfaces.v1.Ptobjects import Ptobjects

from v1 import Index
from v1 import Uri
from v1 import Coverage
from v1 import Journeys
from v1 import Schedules
from v1 import Places
from v1 import Ptobjects
from v1 import Coord
from v1 import Disruptions
from v1 import Calendars
from v1 import converters_collection_type
from v1 import Status
from werkzeug.routing import BaseConverter, FloatConverter, PathConverter,\
    ValidationError
from flask import redirect, current_app


class RegionConverter(BaseConverter):

    """ The region you want to query"""

    def __init__(self, *args, **kwargs):
        BaseConverter.__init__(self, *args, **kwargs)
        self.type_ = "string"
        self.regex = '[^(/;)]+'


class LonConverter(FloatConverter):

    """ The longitude of where the coord you want to query"""

    def __init__(self, *args, **kwargs):
        FloatConverter.__init__(self, *args, **kwargs)
        self.type_ = "float"
        self.regex = '-?\\d+(\\.\\d+)?'


class LatConverter(FloatConverter):

    """ The latitude of where the coord you want to query"""

    def __init__(self, *args, **kwargs):
        FloatConverter.__init__(self, *args, **kwargs)
        self.type_ = "float"
        self.regex = '-?\\d+(\\.\\d+)?'


class UriConverter(PathConverter):

    """First part of the uri"""

    def __init__(self, *args, **kwargs):
        PathConverter.__init__(self, *args, **kwargs)
        self.type_ = "string"


class IdConverter(BaseConverter):

    """Id of the object you want to query"""

    def __init__(self, *args, **kwargs):
        BaseConverter.__init__(self, *args, **kwargs)
        self.type_ = "string"


def v1_routing(api):
    api.app.url_map.converters['region'] = RegionConverter
    api.app.url_map.converters['lon'] = LonConverter
    api.app.url_map.converters['lat'] = LatConverter
    api.app.url_map.converters['uri'] = UriConverter
    api.app.url_map.converters['id'] = IdConverter
    api.app.url_map.strict_slashes = False

    api.add_resource(Index.Index,
                     '/v1/',
                     '/v1',
                     endpoint='v1.index')

    api.add_resource(Index.TechnicalStatus,
                     '/v1/status',
                     endpoint='v1.technical_status')

    coverage = '/v1/coverage/'
    region = coverage + '<region:region>/'
    coord = coverage + '<lon:lon>;<lat:lat>/'

    api.add_resource(Coverage.Coverage,
                     coverage,
                     region,
                     coord,
                     endpoint='v1.coverage')

    api.add_resource(Coord.Coord,
                     '/v1/coord/<lon:lon>;<lat:lat>',
                     endpoint='v1.coord')

    collecs = converters_collection_type.collections_to_resource_type.keys()
    for collection in collecs:
        api.add_resource(getattr(Uri, collection)(True),
                         region + collection,
                         coord + collection,
                         region + '<uri:uri>/' + collection,
                         coord + '<uri:uri>/' + collection,
                         endpoint='v1.' + collection + '.collection')

        api.add_resource(getattr(Uri, collection)(False),
                         region + collection + '/<id:id>',
                         coord + collection + '/<id:id>',
                         region + '<uri:uri>/' + collection + '/<id:id>',
                         coord + '<uri:uri>/' + collection + '/<id:id>',
                         endpoint='v1.' + collection + '.id')
        api.app.add_url_rule('/v1/coverage/' + collection + '/<string:id>',
                             'v1.' + collection + '.redirect',
                             Uri.Redirect)

    collecs = ["routes", "lines", "networks", "stop_areas", "stop_points", "vehicle_journeys"]
    for collection in collecs:
        api.add_resource(getattr(Uri, collection)(True),
                         '/v1/' + collection,
                         endpoint='v1.' + collection + '.external_codes')

    api.add_resource(Places.Places,
                     region + 'places',
                     coord + 'places',
                     endpoint='v1.places'
                     )
    api.add_resource(Ptobjects.Ptobjects,
                     region + 'pt_objects',
                     coord + 'pt_objects',
                     endpoint='v1.pt_objects'
                     )

    api.add_resource(Places.PlaceUri,
                     region + 'places/<id:id>',
                     coord + 'places/<id:id>',
                     endpoint='v1.place_uri'
                     )

    api.add_resource(Places.PlacesNearby,
                     region + 'places_nearby',
                     coord + 'places_nearby',
                     region + '<uri:uri>/places_nearby',
                     coord + '<uri:uri>/places_nearby',
                     endpoint='v1.places_nearby'
                     )

    api.add_resource(Journeys.Journeys,
                     region + '<uri:uri>/journeys',
                     coord + '<uri:uri>/journeys',
                     region + 'journeys',
                     coord + 'journeys',
                     '/v1/journeys',
                     endpoint='v1.journeys'
                     )

    api.add_resource(Schedules.RouteSchedules,
                     region + '<uri:uri>/route_schedules',
                     coord + '<uri:uri>/route_schedules',
                     '/v1/route_schedules',
                     endpoint='v1.route_schedules'
                     )

    api.add_resource(Schedules.NextArrivals,
                     region + '<uri:uri>/arrivals',
                     coord + '<uri:uri>/arrivals',
                     '/v1/arrivals',
                     endpoint='v1.arrivals'
                     )

    api.add_resource(Schedules.NextDepartures,
                     region + '<uri:uri>/departures',
                     coord + '<uri:uri>/departures',
                     '/v1/departures',
                     endpoint='v1.departures'
                     )

    api.add_resource(Schedules.StopSchedules,
                     region + '<uri:uri>/stop_schedules',
                     coord + '<uri:uri>/stop_schedules',
                     '/v1/stop_schedules',
                     endpoint='v1.stop_schedules',
                     )

    api.add_resource(Disruptions.Disruptions,
                     region + 'disruptions',
                     region + '<uri:uri>/disruptions',
                     endpoint='v1.disruptions')

    api.add_resource(Status.Status,
                     region + 'status',
                     endpoint='v1.status')

    api.add_resource(Calendars.Calendars,
                     region + 'calendars',
                     region + '<uri:uri>/calendars',
                     region + "calendars/<id:id>",
                     endpoint="v1.calendars")
