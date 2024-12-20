-- Insert example values into the locations table
CALL Reset_DB();
INSERT INTO session VALUES ('462b0d2d-2d1b-4a27-bd5d-142fc28c1c27');

INSERT INTO locations (id, x, y)
VALUES ('A', 5, 10),
       ('B', 15, 5),
       ('C', 10, 10),
       ('D', 2, 8),
       ('E', 18, 14),
       ('F', 3, 19),
       ('G', 7, 7),
       ('H', 12, 2),
       ('I', 8, 16),
       ('J', 0, 0);

-- Insert example values into the customers table
INSERT INTO customers (id, x, y, destination, next_request, onboard, in_queue)
VALUES ('a', 1, 3, 'A', 5, FALSE, NULL),
       ('b', 6, 14, 'B', 10, FALSE, NULL),
       ('c', 12, 12, 'C', 15, FALSE, NULL),
       ('d', 3, 8, 'D', 20, FALSE, NULL),
       ('e', 17, 15, 'E', 25, FALSE, NULL),
       ('f', 4, 19, 'F', 30, FALSE, NULL),
       ('g', 8, 6, 'G', 35, FALSE, NULL),
       ('h', 13, 3, 'H', 40, FALSE, NULL),
       ('i', 9, 17, 'I', 45, FALSE, NULL),
       ('j', 0, 1, 'J', 50, FALSE, NULL);

-- Insert example values into the taxis table
INSERT INTO taxis (id, x, y, dest_x, dest_y, customer_id, connected, ready, stopped, wait_time)
VALUES (0, 0, 0, NULL, NULL, NULL, TRUE, FALSE, FALSE, 0),
       (1, 5, 5, 10, 10, 'a', TRUE, TRUE, FALSE, 2),
       (2, 15, 15, 18, 18, 'b', TRUE, FALSE, FALSE, 3),
       (3, 2, 8, 5, 5, 'c', TRUE, TRUE, FALSE, 4),
       (4, 3, 19, 7, 7, 'd', TRUE, FALSE, TRUE, 5),
       (5, 12, 2, 12, 12, 'e', TRUE, TRUE, FALSE, 6),
       (6, 8, 16, 15, 15, 'f', TRUE, FALSE, TRUE, 0),
       (7, 7, 7, 0, 0, 'g', TRUE, TRUE, FALSE, 0),
       (8, 18, 14, 3, 3, 'h', TRUE, FALSE, TRUE, 1),
       (9, 0, 0, 19, 19, 'i', TRUE, TRUE, FALSE, 2);

select * from taxis;
select * from customers;