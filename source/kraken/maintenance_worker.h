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

#pragma once

#include <SimpleAmqpClient/SimpleAmqpClient.h>
#include "type/data.h"
#include "kraken/data_manager.h"
#include "kraken/configuration.h"

#include <memory>


namespace navitia {

class MaintenanceWorker{
    private:
        DataManager<type::Data>& data_manager;
        log4cplus::Logger logger;
        const kraken::Configuration conf;

        AmqpClient::Channel::ptr_t channel;
        //nom de la queue créer pour ce worker
        std::string queue_name_task;
        std::string queue_name_rt;

        void init_rabbitmq();
        void listen_rabbitmq();

        void handle_task(AmqpClient::Envelope::ptr_t envelope);
        void handle_rt(AmqpClient::Envelope::ptr_t envelope);
        bool is_initialized = false;

    public:
        MaintenanceWorker(DataManager<type::Data>& data_manager, const kraken::Configuration conf);

        bool load_and_switch();

        void load();

        void operator()();
};

}
