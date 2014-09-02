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

#include "georef.h"

#include "utils/logger.h"
#include "utils/functions.h"
#include "utils/csv.h"
#include "utils/configuration.h"

#include <unordered_map>
#include <boost/foreach.hpp>
#include <array>
#include <boost/math/constants/constants.hpp>

using navitia::type::idx_t;

namespace navitia{ namespace georef{

/** Ajout d'une adresse dans la liste des adresses d'une rue
  * les adresses avec un numéro pair sont dans la liste "house_number_right"
  * les adresses avec un numéro impair sont dans la liste "house_number_left"
  * Après l'ajout, la liste est trié dans l'ordre croissant des numéros
*/

void Way::add_house_number(const HouseNumber& house_number){
    if (house_number.number % 2 == 0){
            this->house_number_right.push_back(house_number);
            std::sort(this->house_number_right.begin(),this->house_number_right.end());
    } else{
        this->house_number_left.push_back(house_number);
        std::sort(this->house_number_left.begin(),this->house_number_left.end());
    }
}

/** Recherche des coordonnées les plus proches à un un numéro
    * les coordonnées par extrapolation
*/
nt::GeographicalCoord Way::extrapol_geographical_coord(int number){
    HouseNumber hn_upper, hn_lower;
    nt::GeographicalCoord to_return;

    if (number % 2 == 0){ // pair
        for(auto it=this->house_number_right.begin(); it != this->house_number_right.end(); ++it){
            if ((*it).number  < number){
                hn_lower = (*it);
            }else {
                hn_upper = (*it);
                break;
            }
        }
    }else{
        for(auto it=this->house_number_left.begin(); it != this->house_number_left.end(); ++it){
            if ((*it).number  < number){
                hn_lower = (*it);
            }else {
                hn_upper = (*it);
                break;
            }
        }
    }

    // Extrapolation des coordonnées:
    int diff_house_number = hn_upper.number - hn_lower.number;
    int diff_number = number - hn_lower.number;

    double x_step = (hn_upper.coord.lon() - hn_lower.coord.lon()) /diff_house_number;
    to_return.set_lon(hn_lower.coord.lon() + x_step*diff_number);

    double y_step = (hn_upper.coord.lat() - hn_lower.coord.lat()) /diff_house_number;
    to_return.set_lat(hn_lower.coord.lat() + y_step*diff_number);

    return to_return;
}

/**
    * Si le numéro est plus grand que les numéros, on renvoie les coordonées du plus grand de la rue
    * Si le numéro est plus petit que les numéros, on renvoie les coordonées du plus petit de la rue
    * Si le numéro existe, on renvoie ses coordonnées
    * Sinon, les coordonnées par extrapolation
*/

nt::GeographicalCoord Way::get_geographical_coord(const std::vector< HouseNumber>& house_number_list, const int number){
    if (!house_number_list.empty()){

        /// Dans le cas où le numéro recherché est plus grand que tous les numéros de liste
        if (house_number_list.back().number <= number){
            return house_number_list.back().coord;
        }

        /// Dans le cas où le numéro recherché est plus petit que tous les numéros de liste
        if (house_number_list.front().number >= number){
            return house_number_list.front().coord;
        }

        /// Dans le cas où le numéro recherché est dans la liste = à un numéro dans la liste
        for(auto it=house_number_list.begin(); it != house_number_list.end(); ++it){
            if ((*it).number  == number){
                return (*it).coord;
             }
        }

        /// Dans le cas où le numéro recherché est dans la liste et <> à tous les numéros
        return extrapol_geographical_coord(number);
    }
    nt::GeographicalCoord to_return;
    return to_return;
}

/** Recherche des coordonnées les plus proches à un numéro
    * Si la rue n'a pas de numéro, on renvoie son barycentre
*/
nt::GeographicalCoord Way::nearest_coord(const int number, const Graph& graph){
    /// Attention la liste :
    /// "house_number_right" doit contenir les numéros pairs
    /// "house_number_left" doit contenir les numéros impairs
    /// et les deux listes doivent être trier par numéro croissant

    if (( this->house_number_right.empty() && this->house_number_left.empty() )
            || (this->house_number_right.empty() && number % 2 == 0)
            || (this->house_number_left.empty() && number % 2 != 0)
            || number <= 0)
        return barycentre(graph);

    if (number % 2 == 0) // Pair
        return get_geographical_coord(this->house_number_right, number);
    else // Impair
        return get_geographical_coord(this->house_number_left, number);
}

// Calcul du barycentre de la rue
nt::GeographicalCoord Way::barycentre(const Graph& graph){
    std::vector<nt::GeographicalCoord> line;
    nt::GeographicalCoord centroid;

    std::pair<vertex_t, vertex_t> previous(type::invalid_idx, type::invalid_idx);
    for(auto edge : this->edges){
        if(edge.first != previous.second || edge.second != previous.first ){
            line.push_back(graph[edge.first].coord);
            line.push_back(graph[edge.second].coord);
        }
        previous = edge;
    }
    try{
        boost::geometry::centroid(line, centroid);
    }catch(...){
      LOG4CPLUS_WARN(log4cplus::Logger::getInstance("log") ,"Impossible de trouver le barycentre de la rue :  " + this->name);
    }

    return centroid;
}

/** Recherche du némuro le plus proche à des coordonnées
    * On récupère le numéro se trouvant à une distance la plus petite par rapport aux coordonnées passées en paramètre
*/
int Way::nearest_number(const nt::GeographicalCoord& coord){

    int to_return = -1;
    double distance, distance_temp;
    distance = std::numeric_limits<double>::max();
    for(auto house_number : this->house_number_left){
        distance_temp = coord.distance_to(house_number.coord);
        if (distance  > distance_temp){
            to_return = house_number.number;
            distance = distance_temp;
        }
    }
    for(auto house_number : this->house_number_right){
        distance_temp = coord.distance_to(house_number.coord);
        if (distance  > distance_temp){
            to_return = house_number.number;
            distance = distance_temp;
        }
    }
    return to_return;
}


type::Mode_e GeoRef::get_mode(vertex_t vertex) const {
    return static_cast<type::Mode_e>(vertex / nb_vertex_by_mode);
}

PathItem::TransportCaracteristic GeoRef::get_caracteristic(edge_t edge) const {
    auto source_mode = get_mode(boost::source(edge, graph));
    auto target_mode = get_mode(boost::target(edge, graph));

    if (source_mode == target_mode) {
        switch (source_mode) {
        case type::Mode_e::Walking:
            return PathItem::TransportCaracteristic::Walk;
        case type::Mode_e::Bike:
            return PathItem::TransportCaracteristic::Bike;
        case type::Mode_e::Car:
            return PathItem::TransportCaracteristic::Car;
        default:
            throw navitia::exception("unhandled path item caracteristic");
        }
    }
    if (source_mode == type::Mode_e::Walking && target_mode == type::Mode_e::Bike) {
        return PathItem::TransportCaracteristic::BssTake;
    }
    if (source_mode == type::Mode_e::Bike && target_mode == type::Mode_e::Walking) {
        return PathItem::TransportCaracteristic::BssPutBack;
    }

    throw navitia::exception("unhandled path item caracteristic");
}

void GeoRef::add_way(const Way& w){
    Way* to_add = new Way;
    to_add->name = w.name;
    to_add->idx = w.idx;
    to_add->uri = w.uri;
    ways.push_back(to_add);
}

ProjectionData::ProjectionData(const type::GeographicalCoord & coord, const GeoRef & sn, const proximitylist::ProximityList<vertex_t> &prox) {
    edge_t edge;
    found = true;
    try {
        edge = sn.nearest_edge(coord, prox);
    } catch(proximitylist::NotFound) {
        found = false;
        vertices[Direction::Source] = std::numeric_limits<vertex_t>::max();
        vertices[Direction::Target] = std::numeric_limits<vertex_t>::max();
    }

    if(found) {
        init(coord, sn, edge);
    }
}

ProjectionData::ProjectionData(const type::GeographicalCoord & coord, const GeoRef & sn, type::idx_t offset, const proximitylist::ProximityList<vertex_t> &prox){
    edge_t edge;
    found = true;
    try {
        edge = sn.nearest_edge(coord, offset, prox);
    } catch(proximitylist::NotFound) {
        found = false;
        vertices[Direction::Source] = std::numeric_limits<vertex_t>::max();
        vertices[Direction::Target] = std::numeric_limits<vertex_t>::max();
    }

    if(found) {
        init(coord, sn, edge);
    }
}

void ProjectionData::init(const type::GeographicalCoord & coord, const GeoRef & sn, edge_t nearest_edge) {
    // On cherche les coordonnées des extrémités de ce segment
    vertices[Direction::Source] = boost::source(nearest_edge, sn.graph);
    vertices[Direction::Target] = boost::target(nearest_edge, sn.graph);
    const type::GeographicalCoord& vertex1_coord = sn.graph[vertices[Direction::Source]].coord;
    const type::GeographicalCoord& vertex2_coord = sn.graph[vertices[Direction::Target]].coord;
    // On projette le nœud sur le segment
    this->projected = coord.project(vertex1_coord, vertex2_coord).first;
    // On calcule la distance « initiale » déjà parcourue avant d'atteindre ces extrémité d'où on effectue le calcul d'itinéraire
    distances[Direction::Source] = projected.distance_to(vertex1_coord);
    distances[Direction::Target] = projected.distance_to(vertex2_coord);
}

std::vector<navitia::type::idx_t> GeoRef::find_admins(const type::GeographicalCoord &coord){
    std::vector<navitia::type::idx_t> to_return;
    navitia::georef::Rect search_rect(coord);

    std::vector<idx_t> result;
    auto callback = [](idx_t id, void* vec)->bool{reinterpret_cast<std::vector<idx_t>*>(vec)->push_back(id); return true;};
    this->rtree.Search(search_rect.min, search_rect.max, callback, &result);
    for(idx_t admin_idx : result) {
        if (boost::geometry::within(coord, admins[admin_idx]->boundary)){
            to_return.push_back(admin_idx);
        }
    }
    return to_return;
}

/**
 * there are 3 graphs:
 *  - one for the walk
 *  - one for the bike
 *  - one for the car
 *
 *  since some transportation modes mixes the differents graphs (ie for bike sharing you use the walking and biking graph)
 *  there are some edges between the 3 graphs
 *
 *  the Vls has thus not it's own graph and all projections are done on the walking graph (hence its offset is the walking graph offset)
 */
void GeoRef::init() {
    offsets[nt::Mode_e::Walking] = 0;
    offsets[nt::Mode_e::Bss] = 0;

    //each graph has the same number of vertex
    nb_vertex_by_mode = boost::num_vertices(graph);

    //we dupplicate the graph for the bike and the car
    for (nt::Mode_e mode : {nt::Mode_e::Bike, nt::Mode_e::Car}) {
        offsets[mode] = boost::num_vertices(graph);
        for (vertex_t v = 0; v < nb_vertex_by_mode; ++v){
            boost::add_vertex(graph[v], graph);
        }
    }
}

void GeoRef::build_proximity_list(){
    pl.clear();

    //do not build the proximitylist with the edge of other transportation mode than walking (and walking HAS to be the first graph)
    for(vertex_t v = 0; v < nb_vertex_by_mode; ++v){
        pl.add(graph[v].coord, v);
    }

    pl.build();

    poi_proximity_list.clear();

    for(const POI *poi : pois) {
        poi_proximity_list.add(poi->coord, poi->idx);
    }
    poi_proximity_list.build();
}

void GeoRef::build_autocomplete_list(){
    int pos = 0;
    fl_way.clear();
    for(Way* way : ways){
        if (!way->name.empty()) {
            std::string key="";
            for(Admin* admin : way->admin_list){
                //Level Admin 8  : City
                if (admin->level == 8) {
                    key+= " " + admin->name;
                }
                if ((!admin->post_code.empty()) && (admin->level == 8)) {
                    key += " "+ admin->post_code;
                }
            }
            fl_way.add_string(way->way_type +" "+ way->name + " " + key, pos, this->synonyms);
        }
        pos++;
    }
    fl_way.build();

    fl_poi.clear();
    //Autocomplete poi list
    for(const POI* poi : pois){
        if ((!poi->name.empty()) && (poi->visible)) {
            std::string key="";
            for(Admin* admin : poi->admin_list) {
                //Level Admin 8  : City
                if (admin->level == 8) {
                    key += " " + admin->name;
                }
            }
            fl_poi.add_string(poi->name + " " + key, poi->idx , this->synonyms);
        }
    }
    fl_poi.build();

    fl_admin.clear();
    for(Admin* admin : admins){
        std::string key="";

        if (!admin->post_code.empty())
        {
            key = admin->post_code;
        }
        fl_admin.add_string(admin->name + " " + key, admin->idx , this->synonyms);
    }
    fl_admin.build();
}


/** Chargement de la liste poitype_map : mappage entre codes externes et idx des POITypes*/
void GeoRef::build_poitypes_map(){
   this->poitype_map.clear();
   for(const POIType* ptype : poitypes){
       this->poitype_map[ptype->uri] = ptype->idx;
   }
}

/** Chargement de la liste poi_map : mappage entre codes externes et idx des POIs*/
void GeoRef::build_pois_map(){
    this->poi_map.clear();
   for(const POI* poi : pois){
       this->poi_map[poi->uri] = poi->idx;
   }
}

void GeoRef::build_rtree() {
    typedef boost::geometry::model::box<type::GeographicalCoord> box;
    for(const Admin* admin : this->admins){
        auto envelope = boost::geometry::return_envelope<box>(admin->boundary);
        Rect r(envelope.min_corner().lon(), envelope.min_corner().lat(), envelope.max_corner().lon(), envelope.max_corner().lat());
        this->rtree.Insert(r.min, r.max, admin->idx);
    }
}

/** Normalisation des codes externes des rues*/
void GeoRef::normalize_extcode_way(){
    this->way_map.clear();
    for(Way* way : ways){
        way->uri = "address:"+ way->uri;
        this->way_map[way->uri] = way->idx;
    }
}


void GeoRef::build_admin_map(){
    this->admin_map.clear();
    for(Admin* admin : admins){
        this->admin_map[admin->uri] = admin->idx;
    }
}

/**
    * Recherche les voies avec le nom, ce dernier peut contenir : [Numéro de rue] + [Type de la voie ] + [Nom de la voie] + [Nom de la commune]
        * Exemple : 108 rue victor hugo reims
    * Si le numéro est rensigné, on renvoie les coordonnées les plus proches
    * Sinon le barycentre de la rue
*/
std::vector<nf::Autocomplete<nt::idx_t>::fl_quality> GeoRef::find_ways(const std::string & str, const int nbmax, const int search_type, std::function<bool(nt::idx_t)> keep_element) const{
    std::vector<nf::Autocomplete<nt::idx_t>::fl_quality> to_return;
    boost::tokenizer<> tokens(str);

    int search_number = str_to_int(*tokens.begin());
    std::string search_str;

    //Si un numero existe au début de la chaine alors il faut l'exclure.
    if (search_number != -1){
        search_str = "";
        int i = 0;
        for(auto token : tokens){
            if (i != 0){
                search_str = search_str + " " + token;
            }
            ++i;
           }
    }else{
        search_str = str;
    }
    if (search_type == 0){
        to_return = fl_way.find_complete(search_str, this->synonyms, nbmax, keep_element);
    }else{
        to_return = fl_way.find_partial_with_pattern(search_str, this->synonyms, word_weight, nbmax, keep_element);
    }

    /// récupération des coordonnées du numéro recherché pour chaque rue
    for(auto &result_item  : to_return){
       Way * way = this->ways[result_item.idx];
       result_item.coord = way->nearest_coord(search_number, this->graph);
       result_item.house_number = search_number;
    }

    return to_return;
}

void GeoRef::project_stop_points(const std::vector<type::StopPoint*> &stop_points) {
   enum class error {
       matched = 0,
       matched_walking,
       matched_bike,
       matched_car,
       not_initialized,
       not_valid,
       other,
       size
   };
   navitia::flat_enum_map<error, int> messages {{{}}};

   this->projected_stop_points.clear();
   this->projected_stop_points.reserve(stop_points.size());

   for(const type::StopPoint* stop_point : stop_points) {
       std::pair<GeoRef::ProjectionByMode, bool> pair = project_stop_point(stop_point);

       this->projected_stop_points.push_back(pair.first);
       if (pair.second) {
           messages[error::matched] += 1;
       } else {
           //verify if coordinate is not valid:
           if (! stop_point->coord.is_initialized()) {
               messages[error::not_initialized] += 1;
           } else if (! stop_point->coord.is_valid()) {
               messages[error::not_valid] += 1;
           } else {
               messages[error::other] += 1;
           }
       }
       if (pair.first[nt::Mode_e::Walking].found) {
           messages[error::matched_walking] += 1;
       }
       if (pair.first[nt::Mode_e::Bike].found) {
           messages[error::matched_bike] += 1;
       }
       if (pair.first[nt::Mode_e::Car].found) {
           messages[error::matched_car] += 1;
       }
   }

   auto log = log4cplus::Logger::getInstance("kraken::type::Data::project_stop_point");
   LOG4CPLUS_DEBUG(log, "Number of stop point projected on the georef network : "
                   << messages[error::matched] << " (on " << stop_points.size() << ")");


   LOG4CPLUS_DEBUG(log, "Number of stop point projected on the walking georef network : "
                   << messages[error::matched_walking] << " (on " << stop_points.size() << ")");
   LOG4CPLUS_DEBUG(log, "Number of stop point projected on the biking georef network : "
                   << messages[error::matched_bike] << " (on " << stop_points.size() << ")");
   LOG4CPLUS_DEBUG(log, "Number of stop point projected on the car georef network : "
                   << messages[error::matched_car] << " (on " << stop_points.size() << ")");

   if (messages[error::not_initialized]) {
       LOG4CPLUS_DEBUG(log, "Number of stop point rejected (X=0 or Y=0)"
                       << messages[error::not_initialized]);
   }
   if (messages[error::not_valid]) {
       LOG4CPLUS_DEBUG(log, "Number of stop point rejected (not valid)"
                       << messages[error::not_valid]);
   }
   if (messages[error::other]) {
       LOG4CPLUS_DEBUG(log, "Number of stop point rejected (other issues)"
                       << messages[error::other]);
   }
}

void GeoRef::build_admins_stop_points(std::vector<type::StopPoint*> & stop_points){
    auto log = log4cplus::Logger::getInstance("kraken::type::GeoRef::fill_admins_stop_points");
    int cpt_no_projected = 0;
    for(type::StopPoint* stop_point : stop_points) {
        ProjectionData projection = this->projected_stop_points[stop_point->idx][type::Mode_e::Walking];
        if(projection.found){
            const edge_t edge = boost::edge(projection[ProjectionData::Direction::Source],
                                         projection[ProjectionData::Direction::Target],
                                         this->graph).first;
            const georef::Way *way = this->ways[this->graph[edge].way_idx];
            stop_point->admin_list.insert(stop_point->admin_list.end(),
                                          way->admin_list.begin(),
                                          way->admin_list.end());
        }else{
            cpt_no_projected++;
        }
    }
    LOG4CPLUS_DEBUG(log, cpt_no_projected<<"/"<<stop_points.size() << " stop_points are not associated with any admins");
}

void GeoRef::build_admins_pois(){
    auto log = log4cplus::Logger::getInstance("kraken::type::GeoRef::fill_admins_pois");
    int cpt_no_projected = 0;
    int cpt_no_initialized = 0;
    for(POI* poi : this->pois){
        if(poi->coord.is_initialized()){
            try{
                edge_t edge = this->nearest_edge(poi->coord);
                georef::Way *way = this->ways[this->graph[edge].way_idx];
                poi->admin_list.insert(poi->admin_list.end(),
                                       way->admin_list.begin(), way->admin_list.end());
            }catch(proximitylist::NotFound){
                cpt_no_projected++;
            }
        }else{
            cpt_no_initialized++;
        }
    }
    LOG4CPLUS_DEBUG(log, cpt_no_projected<<"/"<<this->pois.size() << " pois are not associated with any admins");
    LOG4CPLUS_DEBUG(log, cpt_no_initialized<<"/"<<this->pois.size() << " pois with coordinates not initialized");
}

std::pair<GeoRef::ProjectionByMode, bool> GeoRef::project_stop_point(const type::StopPoint* stop_point) const {
    bool one_proj_found = false;
    ProjectionByMode projections;

    for (const auto& pair : offsets) {
        type::idx_t offset = pair.second;
        type::Mode_e transportation_mode = pair.first;

        ProjectionData proj(stop_point->coord, *this, offset, this->pl);
        projections[transportation_mode] = proj;
        if(proj.found)
            one_proj_found = true;
    }
    return {projections, one_proj_found};
}

edge_t GeoRef::nearest_edge(const type::GeographicalCoord & coordinates) const {
    return this->nearest_edge(coordinates, this->pl);
}

vertex_t GeoRef::nearest_vertex(const type::GeographicalCoord & coordinates, const proximitylist::ProximityList<vertex_t> &prox) const {
    return prox.find_nearest(coordinates);
}

/// Get the nearest_edge with at least one vertex in the graph corresponding to the offset (walking, bike, ...)
edge_t GeoRef::nearest_edge(const type::GeographicalCoord & coordinates, type::idx_t offset, const proximitylist::ProximityList<vertex_t>& prox) const {
    auto vertexes_within = prox.find_within(coordinates);
    for (const auto pair_coord : vertexes_within) {
        //we increment the index to get the vertex in the other graph
        const auto new_vertex = pair_coord.first + offset;

        try {
            edge_t edge_in_graph = nearest_edge(coordinates, new_vertex);
            return edge_in_graph;
        } catch(proximitylist::NotFound) {}
    }
    throw proximitylist::NotFound();
}



edge_t GeoRef::nearest_edge(const type::GeographicalCoord & coordinates, const vertex_t & u) const{

    type::GeographicalCoord coord_u, coord_v;
    coord_u = this->graph[u].coord;
    float dist = std::numeric_limits<float>::max();
    edge_t best;
    bool found = false;
    BOOST_FOREACH(edge_t e, boost::out_edges(u, this->graph)){
        vertex_t v = boost::target(e, this->graph);
        coord_v = this->graph[v].coord;
        // Petite approximation de la projection : on ne suit pas le tracé de la voirie !
        auto projected = coordinates.project(coord_u, coord_v);
        if(projected.second < dist){
            found = true;
            dist = projected.second;
            best = e;
        }
    }
    if(!found)
        throw proximitylist::NotFound();
    else
        return best;

}
edge_t GeoRef::nearest_edge(const type::GeographicalCoord & coordinates, const proximitylist::ProximityList<vertex_t> &prox) const {
    vertex_t u = nearest_vertex(coordinates, prox);
    return nearest_edge(coordinates, u);
}

GeoRef::~GeoRef() {
    for(POIType* poi_type : poitypes) {
        delete poi_type;
    }
    for(POI* poi: pois) {
        delete poi;
    }
    for(Way* way: ways) {
        delete way;
    }
    for(Admin* admin: admins) {
        delete admin;
    }

}


std::vector<type::idx_t> POI::get(type::Type_e type, const GeoRef &) const {
    switch(type) {
    case type::Type_e::POIType : return {poitype_idx}; break;
    default : return {};
    }
}

std::vector<type::idx_t> POIType::get(type::Type_e type, const GeoRef & data) const {
    std::vector<type::idx_t> result;
    switch(type) {
    case type::Type_e::POI:
        for(const POI* elem : data.pois) {
            if(elem->poitype_idx == idx) {
                result.push_back(elem->idx);
            }
        }
        break;
    default : break;
    }
    return result;
}

}}
