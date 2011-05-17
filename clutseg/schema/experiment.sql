
create table experiment (
    id integer primary key autoincrement,
    paramset_id integer not null references paramset(id),
    response_id integer default null references response(id),
    sample_size integer not null,
    train_set varchar(255) not null,
    test_set varchar(255) not null,
    time datetime,
    vcs_commit varchar(255) 
);

