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

#include "thermometer.h"
#include "ptreferential/ptreferential.h"
#include "time.h"

namespace navitia { namespace timetables {


static uint32_t get_lower_bound(std::vector<vector_size>& pre_computed_lb, type::idx_t max_sp) {
    uint32_t result = 0;
    for(unsigned int sp = 0; sp < (max_sp +1); ++sp) {
        result += std::max(pre_computed_lb[0][sp], pre_computed_lb[1][sp]);
    }
    return result;
}

static std::vector<vector_size>
pre_compute_lower_bound(const std::vector<vector_idx>& journey_patterns, type::idx_t max_sp) {
    std::vector<vector_size> lower_bounds;
    for(auto journey_pattern : journey_patterns) {
        lower_bounds.push_back(vector_size());
        lower_bounds.back().insert(lower_bounds.back().begin(), max_sp+1, 0);
        for(auto sp : journey_pattern) {
            ++lower_bounds.back()[sp];
        }
    }
    return lower_bounds;
}

std::pair<vector_idx, bool> Thermometer::recc(std::vector<vector_idx> &journey_patterns, std::vector<vector_size> &pre_computed_lb, const uint32_t lower_bound_, type::idx_t max_sp, const uint32_t upper_bound_, int depth) {
    ++depth;
    ++nb_branches;
    uint32_t lower_bound = lower_bound_, upper_bound = upper_bound_;
    std::vector<type::idx_t> result;


    std::vector<type::idx_t> possibilities = generate_possibilities(journey_patterns, pre_computed_lb);
    bool res_bool = possibilities.empty();
    for(auto poss_spidx : possibilities) {
        if(nb_branches > 5000 && !result.empty())
            break;
        int temp1 = std::max(pre_computed_lb[0][poss_spidx], pre_computed_lb[1][poss_spidx]);
        std::vector<uint32_t> to_retail = untail(journey_patterns, poss_spidx, pre_computed_lb);
        lower_bound = lower_bound - temp1 + std::max(pre_computed_lb[0][poss_spidx], pre_computed_lb[1][poss_spidx]);

        uint32_t u = upper_bound;
        if(u != std::numeric_limits<uint32_t>::max())
            --u;

        if(lower_bound >= u) {
            int temp = std::max(pre_computed_lb[0][poss_spidx], pre_computed_lb[1][poss_spidx]);
            retail(journey_patterns, poss_spidx, to_retail, pre_computed_lb);
            lower_bound = lower_bound - temp + std::max(pre_computed_lb[0][poss_spidx], pre_computed_lb[1][poss_spidx]);
        } else {
            std::pair<vector_idx, bool> tmp = recc(journey_patterns, pre_computed_lb, lower_bound, max_sp, u, depth);
            int temp = std::max(pre_computed_lb[0][poss_spidx], pre_computed_lb[1][poss_spidx]);
            retail(journey_patterns, poss_spidx, to_retail, pre_computed_lb);
            lower_bound = lower_bound - temp + std::max(pre_computed_lb[0][poss_spidx], pre_computed_lb[1][poss_spidx]);
            if(tmp.second){
                tmp.first.push_back(poss_spidx);
                if(!res_bool|| (tmp.second && tmp.first.size() < upper_bound)) {
                    result = tmp.first;
                    res_bool = true;
                    upper_bound = result.size();
                }

                if(upper_bound == lower_bound) {
                    break;
                }
            }
        }
    }

    return std::make_pair(result, res_bool);
}

static uint32_t get_max_sp(const std::vector<vector_idx>& journey_patterns) {

    uint32_t max_sp = std::numeric_limits<uint32_t>::min();

    for(auto journey_pattern : journey_patterns) {
        for(auto i : journey_pattern) {
            if(i > max_sp)
                max_sp = i;
        }
    }

    return max_sp;
}

void Thermometer::generate_thermometer(const type::Route* route) {
    std::set<vector_idx> stop_point_lists;
    route->for_each_vehicle_journey([&](const type::VehicleJourney& vj) {
            vector_idx stop_point_list;
            for (const auto& st: vj.stop_time_list) {
                stop_point_list.push_back(st.stop_point->idx);
            }
            stop_point_lists.insert(std::move(stop_point_list));
            return true;
        });
    generate_thermometer(std::vector<vector_idx>(stop_point_lists.begin(), stop_point_lists.end()));
}

void Thermometer::generate_thermometer(const std::vector<vector_idx> &stop_point_lists) {

    uint32_t max_sp = get_max_sp(stop_point_lists);
    nb_branches = 0;
    std::vector<vector_idx> req;
    if(stop_point_lists.size() > 1) {
        for(auto v : stop_point_lists) {
            if(req.empty())
                req.push_back(v);
            else {
                nb_branches = 0;
                req.push_back(v);
                auto plb = pre_compute_lower_bound(req, max_sp);
                uint32_t lowerbound = get_lower_bound(plb, max_sp);
                auto tp = recc(req, plb, lowerbound, max_sp).first;
                req.clear();

                req.push_back(tp);
            }
        }
        thermometer = req.back();
    } else if(stop_point_lists.size() == 1){
        thermometer = stop_point_lists.back();
    }

}


vector_idx Thermometer::get_thermometer() const {
    return thermometer;
}



std::vector<uint32_t> Thermometer::stop_times_order(const type::VehicleJourney& vj) const {
    std::vector<type::idx_t> tmp;
    for(const auto& st: vj.stop_time_list)
        tmp.push_back(st.stop_point->idx);
    
    return stop_times_order_helper(tmp);
}

std::vector<uint32_t> Thermometer::stop_times_order_helper(const vector_idx &stop_point_list) const {
    std::vector<uint32_t> result;
    auto it = thermometer.begin();
    for(type::idx_t spidx : stop_point_list) {
        it = std::find(it, thermometer.end(),  spidx);
        if(it==thermometer.end())
            throw cant_match(spidx);
        else {
            result.push_back( distance(thermometer.begin(), it));
        }
        ++it;
    }
    return result;

}

vector_idx Thermometer::generate_possibilities(const std::vector<vector_idx> &journey_patterns, std::vector<vector_size> &pre_computed_lb) {

    //C'est qu'il n'y a pas de possibilités possibles
    if(journey_patterns[0].empty() && journey_patterns[1].empty())
        return {};


    //Si la journey_pattern une est vide, ou bien si le dernier de la journey_pattern n'est pas présent dans la journey_pattern 0, on renvoie la tete de la journey_pattern 1
    if(journey_patterns[0].empty() || (!journey_patterns[1].empty() && pre_computed_lb[0][journey_patterns[1].back()] == 0) ) {
        return {journey_patterns[1].back()};
    } else if(journey_patterns[1].empty() ||(!journey_patterns[0].empty() && pre_computed_lb[1][journey_patterns[0].back()] == 0)) { //Même chose mais avec la journey_pattern 0
        return {journey_patterns[0].back()};
    }


    auto count1 = std::count(journey_patterns[0].begin(), journey_patterns[0].end(), journey_patterns[1].back());
    auto count2 = std::count(journey_patterns[1].begin(), journey_patterns[1].end(), journey_patterns[0].back());

    if(count1 > count2) {
        return {journey_patterns[0].back(), journey_patterns[1].back()};
    } else if(count1 < count2) {
        return {journey_patterns[1].back(), journey_patterns[0].back()};
    } else if(journey_patterns[0].size() < journey_patterns[1].size()) {
        return {journey_patterns[0].back(), journey_patterns[1].back()};
    } else {
        return {journey_patterns[1].back(), journey_patterns[0].back()};
    }
}

std::vector<uint32_t> Thermometer::untail(std::vector<vector_idx> &journey_patterns, type::idx_t spidx, std::vector<vector_size> &pre_computed_lb) {
    std::vector<uint32_t> result;
    if(spidx != type::invalid_idx) {
        if((journey_patterns[0].size() > 0) && (journey_patterns[0].back() == spidx)) {
            journey_patterns[0].pop_back();
            result.push_back(0);
            --pre_computed_lb[0][spidx];
        }
        if((journey_patterns[1].size() > 0) && (journey_patterns[1].back() == spidx)) {
            journey_patterns[1].pop_back();
            result.push_back(1);
            --pre_computed_lb[1][spidx];
        }
    }
    return result;
}

void Thermometer::retail(std::vector<vector_idx> &journey_patterns, type::idx_t spidx, const std::vector<uint32_t> &to_retail, std::vector<vector_size> &pre_computed_lb) {
    for(auto i : to_retail) {
        journey_patterns[i].push_back(spidx);
        ++pre_computed_lb[i][spidx];
    }
}

}}
