create table Device (
    id integer not null,
    uuid uuid not null,
    area varchar default null,
    description varchar default null,
    PRIMARY KEY (id)
);

create table Alert (
    id integer not null,
    device_id integer not null,
    source text not null,
    timestamp timestamp not null,
    PRIMARY KEY (id)
);
