//
// Created by ab-flies on 28/11/24.
//

#ifndef PROTOCOLS_H
#define PROTOCOLS_H

#include <cppkafka/cppkafka.h>
#include <uuid/uuid.h>
#include "Logs.h"

#include <expected>

/// @namesapce prot
/// @brief Contains the data structures used in the communication between the different components of the system.
namespace prot {
    ///////////////////////////////////////////////////////////////////////////
    /// ENUMS
    ///////////////////////////////////////////////////////////////////////////

    enum class SUBJ_GUI {
        WRITE_BOTTOM_WINDOW,
        WRITE_TOP_WINDOW,
        MAP_UPDATE,
    };

    enum class SUBJ_REQUEST {
        // Don't Change the values, the order is important as it's hardcoded in the front end
        ORDER_TAXI_GO_TO = 0,
        ORDER_TAXI_STOP,
        ORDER_TAXI_CONTINUE,
        ORDER_TAXI_RETURN_TO_BASE,
        ORDER_TAXI_DISCONNECT,
        ORDER_CUSTOMER_DISCONNECT,
        ORDER_CUSTOMER_PRIORITY,
        ORDER_LOCATION_MOVE,

        NEW_TAXI,
        TAXI_RECONNECTED,
        TAXI_READY,
        TAXI_NOT_READY, // Taxi is not ready to move and waitTime is undefined
        TAXI_UPDATE_WAIT_TIME, // Taxi is not ready to move and waitTime is sent in the data
        TAXI_MOVE,
        TAXI_DESTINATION_REACHED,
        TAXI_PING,

        NEW_CUSTOMER,
        CUSTOMER_PING,
        CUSTOMER_SERVICE_REQUEST,
        CUSTOMER_UPDATE_NEXT_REQUEST,
    };

    enum class SUBJ_RESPONSE {
        TAXI_GO_TO,
        TAXI_STOP,
        TAXI_CONTINUE,
        TAXI_RETURN_TO_BASE,
        TAXI_DISCONNECT,

        CUSTOMER_DISCONNECT,
        CUSTOMER_PRIORITY,
        CUSTOMER_PICKED_UP,
        CUSTOMER_DROPPED_OFF,
        CUSTOMER_ENQUEUED,
        CUSTOMER_SERVICE_COMPLETED,
        CUSTOMER_SERVICE_ACCEPTED,
        CUSTOMER_SERVICE_DENIED,

        MAP_UPDATE,
        DENY,
        ACCEPT,
    };

    using SUBJECT = variant<SUBJ_GUI, SUBJ_REQUEST, SUBJ_RESPONSE>;

    enum class CONTROL_CHAR : uint8_t {
        ENQ = 0x05,
        ACK = 0x06,
        NACK = 0x15,
        EOT = 0x04,
        STX = 0x02,
        ETX = 0x03,
        NONE = 0x00,

        // Similar ot STX, the message doesn't contain data but a single byte (a control char). LRC and ETX aren't used.
        STC = 0x01,
    };

    ///////////////////////////////////////////////////////////////////////////
    /// CLASSES USED AS TEMPLATES IN COMMUNICATION
    ///////////////////////////////////////////////////////////////////////////
    class ISerializable {
    public:
        virtual ~ISerializable() = default;
        virtual vector<byte> serialize() const = 0;

        virtual void deserialize(const span<const byte> &buffer) = 0;

        void deserialize(const cppkafka::Buffer &obj) {
            const span buffer(reinterpret_cast<const byte *>(obj.begin()), obj.get_size());
            this->deserialize(buffer);
        }
    };

    template<typename T>
    concept Serializable = is_base_of_v<ISerializable, T>;

    // Topic: logs
    class Log final : ISerializable {
        bool printBottom; // For central GUI to know where to print the log
        string timestamp;
        code_logs::LogType code;
        string message;

    public:
        Log();
        Log(code_logs::LogType code, const string &message, bool printBottom = false);

        string getTimestampStr() const { return timestamp; }
        code_logs::LogType getCode() const { return code; }
        string getMessage() const { return message; }
        bool getPrintAtBottom() const { return printBottom; }

        Log &setTimestamp(const string &timestamp) {
            this->timestamp = timestamp;
            return *this;
        }
        Log &setCode(const code_logs::LogType code) {
            this->code = code;
            return *this;
        }
        Log &setMessage(const string &message) {
            this->message = message;
            return *this;
        }
        Log &setPrintAtBottom(const bool printBottom) {
            this->printBottom = printBottom;
            return *this;
        }

        vector<byte> serialize() const override;
        void deserialize(const span<const byte> &buffer) override;
    };

    struct Coordinate {
        int x, y;

        string to_string() { return format("[{}, {}]", this->x, this->y); }

        bool operator==(const Coordinate &other) const { return x == other.x && y == other.y; }
        bool operator!=(const Coordinate &other) const { return !(*this == other); }

        string toString() const { return format("[{:02}, {:02}]", x, y); }
    };

    struct Location {
        char id;
        Coordinate coord;
    };

    struct Customer {
        char id;
        Coordinate coord;
        char destination;
        bool onboard;
        bool inQueue;
        int nextRequest;
    };

    struct Taxi {
        short id;
        Coordinate coord;
        char customer;
        Coordinate dest;
        bool connected;
        bool ready;
        bool stopped;
        int waitTime;
    };

    // Topic: map
    class Map final : ISerializable {
        vector<Location> locations;
        vector<Taxi> taxis;
        vector<Customer> customers;

    public:
        Map() = default;
        vector<Location> &getLocations() { return locations; }
        vector<Customer> &getCustomers() { return customers; }
        vector<Taxi> &getTaxis() { return taxis; }

        void addLocation(const Location &location) { locations.push_back(location); }
        void addCustomer(const Customer &customer) { customers.push_back(customer); }
        void addTaxi(const Taxi &taxi) { taxis.push_back(taxi); }

        vector<byte> serialize() const override;
        void deserialize(const span<const byte> &buffer) override;
    };

    // Topic: requests/responses
    class Message final : ISerializable {
        static constexpr size_t MAX_DATA_SIZE = 40;

        SUBJECT subject;
        u_char id;
        short taxiId;
        Coordinate coord;
        string session;
        array<byte, MAX_DATA_SIZE> data;

    public:
        Message();

        // clang-format off
        Message &setSubject(const SUBJECT &subject) { this->subject = subject; return *this; }

        Message &setId(const char id) { this->id = id; return *this; }

        Message &setTaxiId(const short taxiId) { this->taxiId = taxiId; return *this; }

        Message &setCoord(const Coordinate &coord) { this->coord = coord; return *this; }

        Message &setSession(const string &session) { this->session = session; return *this; }
        //clang-format on

        template <typename T>
        Message &setData(const T &obj, const int offset = 0) {
            const size_t maxCopy = MAX_DATA_SIZE - offset;
            if (sizeof(T) <= maxCopy) {
                memcpy(this->data.data() + offset, &obj, sizeof(T));
            }
            return *this;
        }

        SUBJECT getSubject() const { return subject; }

        char getId() const { return id; }

        short getTaxiId() const { return taxiId; }

        Coordinate getCoord() const { return coord; }

        string getSession() const { return session; }

        bool checkSession(const string &session) const {
            return this->session == session;
        }

        template<typename T>
        T getFromData(const int offset = 0, const size_t size = 0) const {
            T obj;
            memcpy(&obj, data.data() + offset, size == 0 ? sizeof(obj) : size);
            return obj;
        }

        array<byte, MAX_DATA_SIZE>& getData() { return data; }


        vector<byte> serialize() const override;

        void deserialize(const span<const byte>& buffer) override;
    };

    ///////////////////////////////////////////////////////////////////////////
    /// EXTENSION OF CPP_KAFKA CLASSES
    ///////////////////////////////////////////////////////////////////////////
    using namespace cppkafka;

    Configuration getConfig(bool consumer, string id = "");

    class ProducerWrapper final : public Producer {
    public:
        explicit ProducerWrapper(const string &id = "") : Producer(getConfig(false, id)) {}

        template <Serializable T>
        void produce(const string &topic, const T &value) {
            vector buffer = value.serialize();
            Producer::produce(MessageBuilder(topic).payload(Buffer(buffer.begin(), buffer.end())));
        }

        ~ProducerWrapper() override { flush(); Producer::~Producer(); }
    };

    class ConsumerWrapper final : public Consumer {
        Error error;
    public:
        explicit ConsumerWrapper(const string &id = "") : Consumer(getConfig(true, id)) {}

        Error get_error() const { return error; }

        template <Serializable T>
        optional<T> consume(const chrono::milliseconds timeout = -1ms) {
            const auto msg = timeout == -1ms ? poll() : poll(timeout);
            error = Error();

            if (!msg || msg.get_error()) {
                if (msg && !msg.is_eof()) {
                    error = msg.get_error();
                }
                return nullopt;
            }

            T value;
            auto& buffer = msg.get_payload();
            const span _buffer(reinterpret_cast<const byte*>(buffer.begin()), buffer.get_size());
            // value.deserialize(span(buffer.begin(), buffer.get_size()));
            value.deserialize(_buffer);
            return value;
        }
    };
} // namespace prot

#endif // PROTOCOLS_H
