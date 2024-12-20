//
// Created by ab-flies on 28/11/24.
//

#include "KafkaServer.h"

#include <Common.h>

KafkaServer KafkaServer::server;

using namespace code_logs;
using namespace prot;

void KafkaServer::startInternal() {
    // Those for whom the session is not checked
    set greenList = {
            SUBJ_REQUEST::ORDER_TAXI_GO_TO,
            SUBJ_REQUEST::ORDER_TAXI_STOP,
            SUBJ_REQUEST::ORDER_TAXI_CONTINUE,
            SUBJ_REQUEST::ORDER_TAXI_RETURN_TO_BASE,
            SUBJ_REQUEST::ORDER_TAXI_DISCONNECT,
            SUBJ_REQUEST::ORDER_CUSTOMER_DISCONNECT,
            SUBJ_REQUEST::ORDER_CUSTOMER_PRIORITY,
            SUBJ_REQUEST::ORDER_LOCATION_MOVE,
            SUBJ_REQUEST::NEW_CUSTOMER,
    };

    initConsumer();
    initProducer();
    initDb();

    setDefaultLogHandler([this](const LogType &code, const string &message) {
        centralLogHandler(*this->producer, code, message, false);
    });

    createProcess(checkStrays);

    this->refreshMap();

    this->consumer->subscribe({"requests"});


    while (true) {
        auto opt = this->consumer->consume<prot::Message>(1000ms);
        if (opt == nullopt) {
            if (consumer->get_error()) {
                codeLog(WARNING::BAD_READING, "Error consuming message: ", consumer->get_error().to_string());
            }
            continue;
        }

        prot::Message &request = opt.value();

        if (!holds_alternative<SUBJ_REQUEST>(request.getSubject())) {
            codeLog(WARNING::INVALID_SUBJECT, "Invalid subject: ", to_string(request.getSubject().index()));
            continue;
        }

        if (!greenList.contains(get<SUBJ_REQUEST>(request.getSubject())) && !request.checkSession(session)) {
            continue;
        }

        // By default, the map will be refreshed after processing the request, except for some cases
        bool refresh = true;

        switch (get<SUBJ_REQUEST>(request.getSubject())) {
            case SUBJ_REQUEST::TAXI_RECONNECTED:
            case SUBJ_REQUEST::NEW_TAXI: {
                if (get<SUBJ_REQUEST>(request.getSubject()) == SUBJ_REQUEST::NEW_TAXI) {
                    codeLog(MESSAGE::NEW_TAXI, "New taxi with id: ", to_string(request.getTaxiId()));
                    ;
                } else {
                    codeLog(MESSAGE::TAXI_RECONNECTED, "Taxi with id ", to_string(request.getTaxiId()), " reconnected");
                }
                auto customer = checkQueue();
                if (customer.has_value()) {
                    assign(request.getTaxiId(), customer.value());
                }
                break;
            }

            case SUBJ_REQUEST::ORDER_CUSTOMER_DISCONNECT: {
                string query = format("DELETE FROM customers WHERE id = '{}';", request.getId());
                codeLog(MESSAGE::CUSTOMER_DISCONNECT, "Customer disconnected: ", to_string(request.getId()));
                auto res = this->db->query(query);
                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't delete customer: ", res.error());
                }
                response.setId(request.getId());
                response.setSubject(SUBJ_RESPONSE::CUSTOMER_DISCONNECT);
                this->producer->produce("responses", response);
                break;
            }

            case SUBJ_REQUEST::ORDER_TAXI_DISCONNECT: {
                string query = format("UPDATE taxis SET connected = FALSE WHERE id = {};", request.getTaxiId());
                codeLog(MESSAGE::TAXI_DISCONNECT, "Taxi disconnected: ", to_string(request.getTaxiId()));
                auto res = this->db->query(query);
                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't disconnect taxi: ", res.error());
                }
                response.setTaxiId(request.getTaxiId());
                response.setSubject(SUBJ_RESPONSE::TAXI_DISCONNECT);
                this->producer->produce("responses", response);
                break;
            }

            case SUBJ_REQUEST::NEW_CUSTOMER:
                codeLog(MESSAGE::NEW_CUSTOMER, "New customer with id: ", to_string(request.getId()));
                this->newCustomer(request);
                break;

            case SUBJ_REQUEST::TAXI_PING: {
                auto res = this->db->query("UPDATE taxis SET last_update = NOW() WHERE id = " +
                                           to_string(request.getTaxiId()));
                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't update taxi's last update: ", res.error());
                }
                break;
            }

            case SUBJ_REQUEST::TAXI_DESTINATION_REACHED:
            case SUBJ_REQUEST::TAXI_MOVE: {
                codeLog(MESSAGE::TAXI_MOVE, "Taxi with id ", to_string(request.getTaxiId()), " moved to ",
                        request.getCoord().toString());
                string query = format("UPDATE taxis SET x = {}, y = {} WHERE id = {};", request.getCoord().x,
                                      request.getCoord().y, request.getTaxiId());
                auto customer = carryingClient(request.getTaxiId());
                if (customer.has_value()) {
                    query += format("UPDATE customers SET x = {}, y = {} WHERE id = '{}';", request.getCoord().x,
                                    request.getCoord().y, customer.value());
                }
                auto res = this->db->query(query);

                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't update taxi's position: ", res.error());
                }

                if (get<SUBJ_REQUEST>(request.getSubject()) == SUBJ_REQUEST::TAXI_MOVE) {
                    break;
                }

                if (customer.has_value()) {
                    setOnboard(customer.value(), request.getTaxiId(), false);
                    response.setId(customer.value());
                    codeLog(MESSAGE::SERVICE_COMPLETED, "Customer ", to_string(customer.value()), " completed its service with taxi ",
                            to_string(request.getTaxiId()));
                    response.setSubject(SUBJ_RESPONSE::CUSTOMER_SERVICE_COMPLETED);
                    this->producer->produce("responses", response);

                    customer = checkQueue();
                    if (customer.has_value()) {
                        assign(request.getTaxiId(), customer.value());
                    }
                } else {
                    customer = getService(request.getTaxiId());
                    auto coord = getDestination(customer.value());
                    if (!customer.has_value()) {
                        codeLog(WARNING::SHOULD_NOT_HAPPEN, "This shouldn't happen");
                        break;
                    }
                    setOnboard(customer.value(), request.getTaxiId(), true);
                    response.setId(customer.value());
                    if (!coord.has_value()) {
                        response.setSubject(SUBJ_RESPONSE::CUSTOMER_SERVICE_DENIED);
                    } else {
                        codeLog(MESSAGE::CUSTOMER_PICKED_UP, "Customer ", to_string(customer.value()), " picked up by taxi ",
                                to_string(request.getTaxiId()));
                        response.setSubject(SUBJ_RESPONSE::CUSTOMER_PICKED_UP);
                    }
                    this->producer->produce("responses", response);
                    if (!coord.has_value())
                        break;
                    response.setTaxiId(request.getTaxiId());
                    response.setSubject(SUBJ_RESPONSE::TAXI_GO_TO);
                    response.setCoord(coord.value());
                    this->producer->produce("responses", response);
                }

                break;
            }

            case SUBJ_REQUEST::TAXI_READY: {
                auto res = this->db->query(format("UPDATE taxis SET ready = TRUE WHERE id = {};", request.getTaxiId()));
                codeLog(MESSAGE::TAXI_READY, "Taxi with id ", to_string(request.getTaxiId()), " is ready");

                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't update taxi's ready status: ", res.error());
                }
                break;
            }

            case SUBJ_REQUEST::TAXI_NOT_READY: {
                auto res =
                        this->db->query(format("UPDATE taxis SET ready = FALSE WHERE id = {};", request.getTaxiId()));
                codeLog(MESSAGE::TAXI_NOT_READY, "Taxi with id ", to_string(request.getTaxiId()), " is not ready");

                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't update taxi's ready status: ", res.error());
                }
                break;
            }

            case SUBJ_REQUEST::CUSTOMER_PING: {
                auto res = this->db->query(
                        format("UPDATE customers SET last_update = NOW() WHERE id = '{}';", request.getId()));
                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't update taxi's last update: ", res.error());
                }
                break;
            }

            case SUBJ_REQUEST::CUSTOMER_SERVICE_REQUEST: {
                codeLog(MESSAGE::SERVICE_REQUEST, "Customer ", to_string(request.getId()), " requested service to: ",
                        to_string(request.getFromData<char>()));
                auto taxiId = this->checkAvailableTaxis();
                response.setId(request.getId());
                if (!taxiId.has_value()) {
                    this->enqueue(request.getId());
                    response.setSubject(SUBJ_RESPONSE::CUSTOMER_ENQUEUED);
                } else {
                    if (!this->setDestination(request.getId(), request.getFromData<char>())) {
                        response.setSubject(SUBJ_RESPONSE::CUSTOMER_SERVICE_DENIED);
                    } else {
                        this->assign(taxiId.value(), request.getId());
                    }
                }
                this->producer->produce("responses", response);
                break;
            }

            case SUBJ_REQUEST::CUSTOMER_UPDATE_NEXT_REQUEST: {
                auto res = this->db->query(format("UPDATE customers SET next_request = {} WHERE id = '{}';",
                                                  request.getFromData<int>(), request.getId()));
                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't update customer's next request time: ", res.error());
                }
                break;
            }

            case SUBJ_REQUEST::TAXI_UPDATE_WAIT_TIME: {
                auto res = this->db->query(
                        format("UPDATE taxis SET wait_time = {}, ready = {} WHERE id = {};", request.getFromData<int>(),
                               request.getFromData<int>() == 0 ? "TRUE" : "FALSE", request.getTaxiId()));
                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't update customer's next request time: ", res.error());
                }
                break;
            }

            case SUBJ_REQUEST::ORDER_TAXI_RETURN_TO_BASE:
                request.setCoord({0, 0});
                [[fallthrough]];
            case SUBJ_REQUEST::ORDER_TAXI_GO_TO: {
                codeLog(MESSAGE::TAXI_GO_TO, "Taxi with id ", to_string(request.getTaxiId()), " is now heading to ",
                        request.getCoord().toString());
                auto res = this->db->query(
                        format("UPDATE taxis SET dest_x = {}, dest_y = {}, customer_id = NULL WHERE id = {};"
                               "UPDATE customers SET onboard = FALSE WHERE id = '{}';",
                               request.getCoord().x, request.getCoord().y, request.getTaxiId(), request.getTaxiId()));
                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't update taxi's destination: ", res.error());
                }

                response.setTaxiId(request.getTaxiId());
                response.setCoord(Coordinate(request.getCoord().x, request.getCoord().y));
                response.setSubject(SUBJ_RESPONSE::TAXI_GO_TO);
                this->producer->produce("responses", response);
                break;
            }

            case SUBJ_REQUEST::ORDER_TAXI_STOP: {
                codeLog(MESSAGE::TAXI_STOP, "Taxi with id ", to_string(request.getTaxiId()), " stopped");
                auto res =
                        this->db->query(format("UPDATE taxis SET stopped = TRUE WHERE id = {};", request.getTaxiId()));
                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't update taxi's stopped status: ", res.error());
                }
                response.setTaxiId(request.getTaxiId());
                response.setSubject(SUBJ_RESPONSE::TAXI_STOP);
                this->producer->produce("responses", response);
                break;
            }

            case SUBJ_REQUEST::ORDER_TAXI_CONTINUE: {
                codeLog(MESSAGE::TAXI_CONTINUE, "Taxi with id ", to_string(request.getTaxiId()), " continued");
                auto res =
                        this->db->query(format("UPDATE taxis SET stopped = FALSE WHERE id = {};", request.getTaxiId()));
                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't update taxi's stopped status: ", res.error());
                }
                response.setTaxiId(request.getTaxiId());
                response.setSubject(SUBJ_RESPONSE::TAXI_CONTINUE);
                this->producer->produce("responses", response);
                break;
            }

            case SUBJ_REQUEST::ORDER_LOCATION_MOVE: {
                codeLog(MESSAGE::LOCATION_MOVE, "Location with id ", to_string(request.getId()), " moved to ",
                        request.getCoord().toString());
                auto res = this->db->query(format("UPDATE locations SET x = {}, y = {} WHERE id = '{}';",
                                                  request.getCoord().x, request.getCoord().y, request.getId()));
                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't update location's position: ", res.error());
                }
                break;
            }

            case SUBJ_REQUEST::ORDER_CUSTOMER_PRIORITY: {
                codeLog(MESSAGE::CUSTOMER_PRIORITY, "Customer ", to_string(request.getId()), " prioritized");
                auto res = this->db->query(
                        format("UPDATE customers SET in_queue = NULL WHERE id = '{}';", request.getId()));

                if (!res.has_value()) {
                    codeLog(WARNING::QUERY, "Couldn't update customer's priority status: ", res.error());
                }
                response.setId(request.getId());
                response.setSubject(SUBJ_RESPONSE::CUSTOMER_PRIORITY);
                this->producer->produce("responses", response);
                break;
            }

            default:
                refresh = false;
                break;
        }

        if (refresh) {
            this->refreshMap();
        }
    }
}

void KafkaServer::refreshMap() const {
    const string query =
            "SELECT id, x, y FROM locations;"
            "SELECT id, x, y, destination, next_request, onboard, in_queue FROM customers;"
            "SELECT id, x, y, dest_x, dest_y, customer_id, connected, ready, stopped, wait_time FROM taxis;";

    auto _results = this->db->query(query);

    if (!_results.has_value()) {
        codeLog(WARNING::QUERY, "Couldn't run query: ", _results.error());
        return;
    }

    auto &results = _results.value();
    Map map;
    vector<string> row;

    while (!(row = results.getRow()).empty()) {
        Location location;

        location.id = row[0][0];
        location.coord.x = stoi(row[1]);
        location.coord.y = stoi(row[2]);

        map.getLocations().emplace_back(location);
    }

    results.nextResult();

    while (!(row = results.getRow()).empty()) {
        Customer customer;

        customer.id = row[0][0];
        customer.coord.x = stoi(row[1]);
        customer.coord.y = stoi(row[2]);
        customer.destination = row[3][0];
        customer.nextRequest = stoi(row[4]);
        customer.onboard = row[5] == "1";
        customer.inQueue = !row[6].empty();

        map.getCustomers().emplace_back(customer);
    }

    results.nextResult();

    while (!(row = results.getRow()).empty()) {
        Taxi taxi;
        // "SELECT id, x, y, dest_x, dest_y, customer_id, connected, ready, stopped, wait_time FROM taxis;";

        taxi.id = stoi(row[0]);
        taxi.coord.x = stoi(row[1]);
        taxi.coord.y = stoi(row[2]);
        taxi.dest.x = row[3].empty() ? -1 : stoi(row[3]);
        taxi.dest.y = row[4].empty() ? -1 : stoi(row[4]);
        taxi.customer = row[5].empty() ? '-' : row[5][0];
        taxi.connected = row[6] == "1";
        taxi.ready = row[7] == "1";
        taxi.stopped = row[8] == "1";
        taxi.waitTime = stoi(row[9]);

        map.getTaxis().emplace_back(taxi);
    }

    this->producer->produce("map", map);
}

void KafkaServer::checkStrays() {
    DbConnectionHandler db;
    ProducerWrapper producer;
    const string query = "SELECT id FROM taxis WHERE connected = TRUE AND last_update < NOW() - INTERVAL 3 SECOND;"
                         "SELECT id FROM customers WHERE last_update < NOW() - INTERVAL 3 SECOND;";

    setDefaultLogHandler([&producer](const LogType &code, const string &message) {
        centralLogHandler(producer, code, message, false);
    });

    if (!db.connect()) {
        codeLog(ERROR::DATABASE, "Failed to connect to database");
        return;
    }

    while (db.isConnected()) {
        auto results = db.query(query);

        if (!results.has_value()) {
            codeLog(WARNING::QUERY, "Couldn't run query: ", results.error());
            this_thread::sleep_for(1s);
            continue;
        }

        vector<string> row;
        while (!(row = results.value().getRow()).empty()) {
            prot::Message message;
            message.setTaxiId(static_cast<short>(stoi(row[0])));
            message.setSubject(SUBJ_REQUEST::ORDER_TAXI_DISCONNECT);

            producer.produce("requests", message);
        }

        results.value().nextResult();

        while (!(row = results.value().getRow()).empty()) {
            prot::Message message;
            message.setId(row[0][0]);
            message.setSubject(SUBJ_REQUEST::ORDER_CUSTOMER_DISCONNECT);

            producer.produce("requests", message);
        }

        this_thread::sleep_for(1s);
    }
}

void KafkaServer::newCustomer(const prot::Message &request) {
    response.setSession(request.getSession());

    string query = format("SELECT 1 FROM customers WHERE id = '{}'", request.getId());
    auto res = this->db->query(query);
    if (!res.has_value()) {
        codeLog(WARNING::QUERY, "Couldn't run query: ", res.error());
        return;
    }

    if (res.value().getRow().empty()) {
        query = format("INSERT INTO customers (id, x, y) VALUES ('{}', {}, {});", request.getId(), request.getCoord().x,
                       request.getCoord().y);
        res = this->db->query(query);
        if (!res.has_value()) {
            codeLog(WARNING::QUERY, "Couldn't run query: ", res.error());
            return;
        }
        response.setSubject(SUBJ_RESPONSE::ACCEPT);
        memcpy(response.getData().data(), session.data(), 36);
    } else {
        response.setSubject(SUBJ_RESPONSE::DENY);
    }

    producer->produce("responses", response);
}

optional<short> KafkaServer::checkAvailableTaxis() const {
    const string query =
            "SELECT id FROM taxis WHERE connected = TRUE AND wait_time < NOW() - INTERVAL 3 SECOND LIMIT 1";

    auto results = this->db->query(query);
    if (!results.has_value()) {
        codeLog(WARNING::QUERY, "Couldn't run query: ", results.error());
        return nullopt;
    }

    const auto row = results.value().getRow();
    if (row.empty()) {
        return nullopt;
    }

    return static_cast<short>(stoi(row[0]));
}

optional<char> KafkaServer::checkQueue() const {
    const string query = "SELECT id FROM customers WHERE in_queue IS NOT NULL ORDER BY in_queue LIMIT 1";

    auto results = this->db->query(query);
    if (!results.has_value()) {
        codeLog(WARNING::QUERY, "Couldn't run query: ", results.error());
        return nullopt;
    }

    const auto row = results.value().getRow();
    if (row.empty()) {
        return nullopt;
    }

    return row[0][0];
}

void KafkaServer::enqueue(char customerId) const {
    const string query = format("UPDATE customers SET in_queue = NOW() WHERE id = '{}'", customerId);

    auto results = this->db->query(query);
    if (!results.has_value()) {
        codeLog(WARNING::QUERY, "Couldn't run query: ", results.error());
    }
}

void KafkaServer::prioritize(char customerId) const {
    const string query = format("UPDATE customers SET in_queue = FROM_UNIXTIME(3) WHERE id = '{}'", customerId);

    auto results = this->db->query(query);
    if (!results.has_value()) {
        codeLog(WARNING::QUERY, "Couldn't run query: ", results.error());
    }
}

bool KafkaServer::setDestination(char customerId, char service) const {
    const string query = format("UPDATE customers SET destination = '{}' WHERE id = '{}'", service, customerId);

    const auto results = this->db->query(query);
    if (!results.has_value()) {
        return false;
    }
    return true;
}


void KafkaServer::assign(short taxiId, char customerId) {
    // update taxis t join customers c on c.id = 'c' set t.dest_x = c.x, t.dest_y = c.y where t.id = 2;
    const string query = format("UPDATE taxis t JOIN customers c ON c.id = '{0}' "
                                "SET t.dest_x = c.x, t.dest_y = c.y, t.service = '{0}' "
                                "WHERE t.id = {1};"
                                "SELECT dest_x, dest_y FROM taxis WHERE id = {1};",
                                customerId, taxiId);

    auto results = this->db->query(query);
    if (!results.has_value()) {
        codeLog(WARNING::QUERY, "Couldn't run query: ", results.error());
    }

    const auto row = results.value().getRow();
    response.setTaxiId(taxiId);
    response.setId(customerId);
    response.setCoord(Coordinate(stoi(row[0]), stoi(row[1])));
    response.setSubject(SUBJ_RESPONSE::TAXI_GO_TO);
    producer->produce("responses", response);
    response.setSubject(SUBJ_RESPONSE::CUSTOMER_SERVICE_ACCEPTED);
    producer->produce("responses", response);
}


optional<char> KafkaServer::carryingClient(short taxiId) const {
    const string query = format("SELECT customer_id FROM taxis WHERE id = {}", taxiId);

    auto results = this->db->query(query);

    if (!results.has_value()) {
        codeLog(WARNING::QUERY, "Couldn't run query: ", results.error());
        return nullopt;
    }

    auto row = results->getRow();
    if (row.empty() || row[0].empty()) {
        return nullopt;
    }

    return row[0][0];
}

optional<char> KafkaServer::getService(short taxiId) const {
    const string query = format("SELECT service FROM taxis WHERE id = {}", taxiId);

    auto results = this->db->query(query);
    if (!results.has_value()) {
        codeLog(WARNING::QUERY, "Couldn't run query: ", results.error());
        return nullopt;
    }

    auto row = results->getRow();
    if (row.empty() || row[0].empty()) {
        return nullopt;
    }

    return row[0][0];
}

optional<Coordinate> KafkaServer::getDestination(char customerId) const {
    const string query = format(
            "SELECT l.x, l.y FROM customers c JOIN locations l ON c.destination = l.id WHERE c.id = '{}'", customerId);


    auto results = this->db->query(query);
    if (!results.has_value()) {
        codeLog(WARNING::QUERY, "Couldn't run query: ", results.error());
        return nullopt;
    }

    const auto row = results->getRow();
    if (row.empty() || row.size() < 2 || row[0].empty() || row[1].empty()) {
        return nullopt;
    }

    return Coordinate(stoi(row[0]), stoi(row[1]));
}

void KafkaServer::setOnboard(const char customerId, const short taxiId, const bool val) const {
    const string query = format("UPDATE customers SET onboard = {} WHERE id = '{}'; "
                                "UPDATE taxis SET customer_id = {} WHERE id = {};",
                                val ? "TRUE" : "FALSE", customerId, val ? format("'{}'", customerId) : "NULL", taxiId);

    auto results = this->db->query(query);
    if (!results.has_value()) {
        codeLog(WARNING::QUERY, "Couldn't run query: ", results.error());
    }
}
