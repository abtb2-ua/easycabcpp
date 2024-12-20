DROP DATABASE IF EXISTS db;
CREATE DATABASE db;
USE db;

CREATE TABLE session
(
    id VARCHAR(40) PRIMARY KEY
);

CREATE TABLE locations
(
    id CHAR PRIMARY KEY,
    x  INT,
    y  INT
);

CREATE TABLE customers
(
    id           CHAR PRIMARY KEY,
    x            INT NOT NULL,
    y            INT NOT NULL,
    destination  CHAR      DEFAULT NULL,
    next_request INT       DEFAULT 0,
    onboard      BOOLEAN   DEFAULT FALSE,
    in_queue     TIMESTAMP DEFAULT NULL, # Time when the customer entered the queue, NULL if not in queue

    last_update  TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    CONSTRAINT fk_destination
        FOREIGN KEY (destination)
            REFERENCES locations (id)
);

CREATE TABLE taxis
(
    id          INT PRIMARY KEY,
    x           INT NOT NULL DEFAULT 0,
    y           INT NOT NULL DEFAULT 0,
    dest_x      INT          DEFAULT NULL,
    dest_y      INT          DEFAULT NULL,
    customer_id CHAR         DEFAULT NULL,  # The customer that the taxi is carrying or NULL if it's empty
    service     CHAR         DEFAULT NULL,  # The service that the taxi is providing or NULL if it's not providing any
    connected   BOOLEAN      DEFAULT TRUE,
    ready       BOOLEAN      DEFAULT FALSE, # Indicates whether the taxi is stopped by an internal cause (e.g. the sensor indicated so)
    stopped     BOOLEAN      DEFAULT FALSE, # Indicates whether the taxi is stopped by an external cause (e.g. the user ordered to stop)
    wait_time   INT          DEFAULT 0,     # The time the taxi has to wait before being ready again

    last_update TIMESTAMP    DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    CONSTRAINT fk_customer_id
        FOREIGN KEY (customer_id)
            REFERENCES customers (id),

    CONSTRAINT fk_service
        FOREIGN KEY (service)
            REFERENCES locations (id)
);


DELIMITER !!

CREATE PROCEDURE Reset_DB()
BEGIN
    DELETE FROM taxis WHERE 1;
    DELETE FROM customers WHERE 1;
    DELETE FROM locations WHERE 1;
    DELETE FROM session WHERE 1;
END !!

DELIMITER ;

select * from taxis;
SELECT l.x, l.y FROM customers c JOIN locations l ON c.destination = l.id WHERE c.id = '1';
UPDATE taxis SET x = 8, customer_id = NULL WHERE id = 0;