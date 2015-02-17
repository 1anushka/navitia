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

#include "disruption.h"
#include "ptreferential/ptreferential.h"
#include "type/data.h"

namespace navitia { namespace disruption {

type::idx_t Disruption::find_or_create(const type::Network* network){
    auto find_predicate = [&](disrupt network_disrupt) {
        return network->idx == network_disrupt.network_idx;
    };
    auto it = std::find_if(this->disrupts.begin(),
                           this->disrupts.end(),
                           find_predicate);
    if(it == this->disrupts.end()){
        disrupt dist;
        dist.network_idx = network->idx;
        dist.idx = this->disrupts.size();
        this->disrupts.push_back(dist);
        return dist.idx;
    }
    return it->idx;
}

void Disruption::add_stop_areas(const std::vector<type::idx_t>& network_idx,
                      const std::string& filter,
                      const std::vector<std::string>& forbidden_uris,
                      const type::Data &d,
                      const boost::posix_time::ptime now){

    for(auto idx : network_idx){
        const auto* network = d.pt_data->networks[idx];
        std::string new_filter = "network.uri=" + network->uri;
        if(!filter.empty()){
            new_filter  += " and " + filter;
        }
        std::vector<type::idx_t> line_list;

       try{
            line_list = ptref::make_query(type::Type_e::StopArea, new_filter,
                    forbidden_uris, d);
        } catch(const ptref::parsing_error &parse_error) {
            LOG4CPLUS_WARN(logger, "Disruption::add_stop_areas : Unable to parse filter "
                                + parse_error.more);
        } catch(const ptref::ptref_error &ptref_error){
            LOG4CPLUS_WARN(logger, "Disruption::add_stop_areas : ptref : "  + ptref_error.more);
        }
        for(auto stop_area_idx : line_list){
            const auto* stop_area = d.pt_data->stop_areas[stop_area_idx];
            if (stop_area->has_publishable_message(now)){
                disrupt& dist = this->disrupts[this->find_or_create(network)];
                auto find_predicate = [&](type::idx_t idx ) {
                    return stop_area->idx == idx;
                };
                auto it = std::find_if(dist.stop_area_idx.begin(),
                                       dist.stop_area_idx.end(),
                                       find_predicate);
                if(it == dist.stop_area_idx.end()){
                    dist.stop_area_idx.push_back(stop_area->idx);
                }
            }
        }
    }
}

void Disruption::add_networks(const std::vector<type::idx_t>& network_idx,
                      const type::Data &d,
                      const boost::posix_time::ptime now){

    for(auto idx : network_idx){
        const auto* network = d.pt_data->networks[idx];
        if (network->has_publishable_message(now)){
            this->disrupts[this->find_or_create(network)];
        }
    }
}

void Disruption::add_lines(const std::string& filter,
                      const std::vector<std::string>& forbidden_uris,
                      const type::Data &d,
                      const boost::posix_time::ptime now){

    std::vector<type::idx_t> line_list;
    try{
        line_list  = ptref::make_query(type::Type_e::Line, filter,
                forbidden_uris, d);
    } catch(const ptref::parsing_error &parse_error) {
        LOG4CPLUS_WARN(logger, "Disruption::add_lines : Unable to parse filter " + parse_error.more);
    } catch(const ptref::ptref_error &ptref_error){
        LOG4CPLUS_WARN(logger, "Disruption::add_lines : ptref : "  + ptref_error.more);
    }
    for(auto idx : line_list){
        const auto* line = d.pt_data->lines[idx];
        if (line->has_publishable_message(now)){
            disrupt& dist = this->disrupts[this->find_or_create(line->network)];
            auto find_predicate = [&](type::idx_t idx ) {
                return line->idx == idx;
            };
            auto it = std::find_if(dist.line_idx.begin(),
                                   dist.line_idx.end(),
                                   find_predicate);
            if(it == dist.line_idx.end()){
                dist.line_idx.push_back(line->idx);
            }
        }
    }
}

void Disruption::sort_disruptions(const type::Data &d){

    auto sort_disruption = [&](disrupt d1, disrupt d2){
        const auto & n1 = *(d.pt_data->networks[d1.network_idx]);
        const auto & n2 = *(d.pt_data->networks[d2.network_idx]);
            return n1 < n2;
    };

    auto sort_lines = [&](type::idx_t l1_, type::idx_t l2_) {
        const auto & l1 = *(d.pt_data->lines[l1_]);
        const auto & l2 = *(d.pt_data->lines[l2_]);
        return l1 < l2;
    };

    std::sort(this->disrupts.begin(), this->disrupts.end(), sort_disruption);
    for(auto disrupt : this->disrupts){
        std::sort(disrupt.line_idx.begin(), disrupt.line_idx.end(), sort_lines);
    }

}

void Disruption::disruptions_list(const std::string& filter,
                        const std::vector<std::string>& forbidden_uris,
                        const type::Data &d,
                        const boost::posix_time::ptime now){

    std::vector<type::idx_t> network_idx = ptref::make_query(type::Type_e::Network, filter,
                                                                                      forbidden_uris, d);
    add_networks(network_idx, d, now);
    add_lines(filter, forbidden_uris, d, now);
    add_stop_areas(network_idx, filter, forbidden_uris, d, now);
    sort_disruptions(d);
}
const std::vector<disrupt>& Disruption::get_disrupts() const{
    return this->disrupts;
}

size_t Disruption::get_disrupts_size(){
    return this->disrupts.size();
}

}}
