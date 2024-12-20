#include <Socket/SocketHandler.h>


#include <dotenv.h>
#include "../EC_Central/AuthServer.h"
#include "Common.h"
#include "Logs.h"
#include "Protocols.h"

using namespace std;
using namespace code_logs;

void handshake();

void readFile();

bool askService(int index);

void ping(string session);

unique_ptr<ProducerWrapper> producer;
unique_ptr<ConsumerWrapper> consumer;

prot::Message request;
prot::Message *response = nullptr;

vector<char> services;

int main(int argc, char *argv[]) {
    dotenv::init(dotenv::Preserve);

    producer = make_unique<ProducerWrapper>();
    consumer = make_unique<ConsumerWrapper>();
    consumer->subscribe({"responses"});
    request.setId(dotenv::getenv("CUSTOMER_ID", "a")[0]);

    handshake();
    readFile();

    thread ping_thread(ping, request.getSession());

    for (size_t i = 0; i < services.size(); i++) {
        if (!askService(i)) {
            return 0;
        }

        request.setSubject(SUBJ_REQUEST::CUSTOMER_UPDATE_NEXT_REQUEST);
        for (int j = 4; j > 0; j--) {
            request.setData(j - 1);
            producer->produce("requests", request);
            this_thread::sleep_for(1s);
        }
    }

    codeLog(MESSAGE::UNDEFINED, "All services completed");
    return 0;
}

void handshake() {
    const string coordStr = dotenv::getenv("CUSTOMER_COORD", "0,0");
    const size_t comma = coordStr.find(',');
    if (comma == string::npos) {
        codeLog(ERROR::UNDEFINED, "Invalid coordinate");
    }

    Coordinate coord;
    coord.x = stoi(coordStr.substr(0, comma));
    coord.y = stoi(coordStr.substr(comma + 1));

    const string uuid = generate_unique_id();
    request.setSubject(SUBJ_REQUEST::NEW_CUSTOMER);
    request.setCoord(coord);
    request.setSession(uuid);

    producer->produce("requests", request);

    for (int i = 0; i < 5; i++) {
        auto opt = consumer->consume<prot::Message>(1s);
        if (opt.has_value()) {
            if (!opt.value().checkSession(uuid)) {
                i--;
                continue;
            }

            const SUBJECT subject = opt.value().getSubject();
            if (holds_alternative<SUBJ_RESPONSE>(subject) && get<SUBJ_RESPONSE>(subject) == SUBJ_RESPONSE::ACCEPT) {
                string session;
                session.resize(36);
                memcpy(session.data(), opt.value().getData().data(), 36);
                request.setSession(session);
                return;
            }
            break;
        }
    }

    codeLog(ERROR::UNDEFINED, "Couldn't connect to the server");
}

void readFile() {
    ifstream file(dotenv::getenv("SERVICES_FILE", "services.csv"));
    if (!file.is_open()) {
        codeLog(ERROR::UNDEFINED, "Couldn't open file");
    }

    string line;
    while (getline(file, line)) {
        if (!line.empty()) {
            services.push_back(line[0]);
        }
    }
}

bool askService(const int index) {
    const char service = services[index];

    request.setSubject(SUBJ_REQUEST::CUSTOMER_SERVICE_REQUEST);
    request.setData(service);
    producer->produce("requests", request);

    while (true) {
        auto opt = consumer->consume<prot::Message>(1s);
        if (opt == nullopt) {
            if (consumer->get_error()) {
                codeLog(WARNING::UNDEFINED, "Error consuming message: ", consumer->get_error().to_string());
            } else {
                codeLog(MESSAGE::DEBUG, "No message consumed");
            }
            continue;
        }

        response = &opt.value();

        if (response->getId() != request.getId() || !holds_alternative<SUBJ_RESPONSE>(response->getSubject())) {
            continue;
        }

        switch (get<SUBJ_RESPONSE>(response->getSubject())) {
            case SUBJ_RESPONSE::CUSTOMER_DISCONNECT:
                codeLog(MESSAGE::UNDEFINED, "Received order to disconnect");
                return false;

            case SUBJ_RESPONSE::CUSTOMER_PRIORITY:
                codeLog(MESSAGE::UNDEFINED, "We've been given maximum priority");
                break;

            case SUBJ_RESPONSE::CUSTOMER_PICKED_UP:
                codeLog(MESSAGE::UNDEFINED, "We've been picked up");
                break;

            case SUBJ_RESPONSE::CUSTOMER_DROPPED_OFF:
                codeLog(MESSAGE::UNDEFINED, "Our taxi suffered an issue and we got dropped off at ",
                        response->getCoord().toString());
                break;

            case SUBJ_RESPONSE::CUSTOMER_SERVICE_COMPLETED:
                codeLog(MESSAGE::UNDEFINED, "We've arrived to ", "" + service);
                return true;

            case SUBJ_RESPONSE::CUSTOMER_SERVICE_DENIED:
                codeLog(WARNING::UNDEFINED, "The location asked for doesn't exist");
                return true;

            case SUBJ_RESPONSE::CUSTOMER_SERVICE_ACCEPTED:
                codeLog(MESSAGE::UNDEFINED, "We've been assigned the taxi ", to_string(response->getTaxiId()));
                break;

            case SUBJ_RESPONSE::CUSTOMER_ENQUEUED:
                codeLog(MESSAGE::UNDEFINED, "There are no available taxis, we've been put in the queue");
                break;

            default:
                codeLog(MESSAGE::DEBUG, "Received invalid message ", to_string(response->getSubject().index()), " ",
                        to_string(static_cast<int>(get<SUBJ_RESPONSE>(response->getSubject()))));
                break;
        }
    }

    return true;
}

void ping(const string session) {
    prot::Message request;
    ProducerWrapper producer;
    request.setSubject(SUBJ_REQUEST::CUSTOMER_PING);
    request.setId(dotenv::getenv("CUSTOMER_ID", "a")[0]);
    request.setSession(session);

    while (true) {
        producer.produce("requests", request);
        this_thread::sleep_for(1s);
    }
}