CREATE TABLE Device
(
    id bigint NOT NULL GENERATED ALWAYS AS IDENTITY ( INCREMENT 1 START 1 ),
    uuid uuid NOT NULL,
    area varchar NOT NULL,
    description varchar NOT NULL,
    CONSTRAINT id PRIMARY KEY (id)
        INCLUDE(id),
    CONSTRAINT uuid UNIQUE (uuid)
        INCLUDE(uuid)
);

CREATE TABLE Detection
(
    id bigint NOT NULL GENERATED ALWAYS AS IDENTITY ( INCREMENT 1 START 1 ),
    device_id bigint NOT NULL,
    source text NOT NULL,
    timestamp timestamp with time zone NOT NULL,
    PRIMARY KEY (id),
    FOREIGN KEY (device_id)
        REFERENCES device (id) MATCH SIMPLE
        ON UPDATE NO ACTION
        ON DELETE NO ACTION
        NOT VALID
);
