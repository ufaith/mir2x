mysql = require "luasql.mysql"

local env  = assert(mysql.mysql())
local conn = assert(env:connect('mir2x', 'root', '123456', "localhost", 3306))
print(env, conn)

conn:execute("set names utf8")

status, errmsg = conn:execute [[
    create database if not exists mir2x character set utf8
]]

-- create table for user account info
status, errmsg = conn:execute [[
    create table if not exists tbl_account
    (
        fld_id       int unsigned not null auto_increment primary key,
        fld_account  char(32) not null,
        fld_password char(32) not null
    )
]]

if errmsg then print(status, errmsg) end

conn:execute [[use mir2x]]

-- print all tables in current db
-- cursor, errmsg = conn:execute [[show tables]]
-- row = cursor:fetch({}, "a")
--
-- while row do
--     for key, value in pairs(row) do print(value) end
--     row = cursor:fetch(row, "a")
-- end

-- try to add an account
status, errmsg = conn:execute [[
    insert tbl_account (fld_account, fld_password) values("test", "123456")
]]

if errmsg then print(status, errmsg) end

-- create table for guid
status, errmsg = conn:execute [[
    create table if not exists tbl_guid
    (
        fld_guid      int unsigned not null auto_increment primary key,
        fld_id        int unsigned not null,
        fld_mapid     int unsigned not null,
        fld_mapx      int unsigned not null,
        fld_mapy      int unsigned not null,
        fld_level     int unsigned not null,
        fld_jobid     int unsigned not null,
        fld_direction int unsigned not null,
        fld_name      char(32) not null
    )
]]

if errmsg then print(status, errmsg) end

-- try to add guid
status, errmsg = conn:execute [[
    insert tbl_guid (fld_id, fld_mapid, fld_mapx, fld_mapy, fld_level, fld_jobid, fld_direction, fld_name) values
        (1, 1, 12, 21, 1, 1, 1, "mark"),
        (2, 1, 14, 19, 1, 1, 1, "john")
]]

if errmsg then print(status, errmsg) end


-- create table for guid
status, errmsg = conn:execute [[
    create table if not exists tbl_monster
    (
        fld_index           int unsigned not null auto_increment primary key,
        fld_name            char(64)     not null,
        fld_race            int unsigned not null,
        fld_lid             int unsigned not null,
        fld_undead          int unsigned not null,
        fld_level           int unsigned not null,
        fld_hp              int unsigned not null,
        fld_mp              int unsigned not null,
        fld_ac              int unsigned not null,
        fld_mac             int unsigned not null,
        fld_dc              int unsigned not null,
        fld_attackspeed     int unsigned not null,
        fld_walkspeed       int unsigned not null,
        fld_speed           int unsigned not null,
        fld_hit             int unsigned not null,
        fld_viewrange       int unsigned not null,
        fld_raceindex       int unsigned not null,
        fld_exp             int unsigned not null,
        fld_escape          int unsigned not null,
        fld_water           int unsigned not null,
        fld_fire            int unsigned not null,
        fld_wind            int unsigned not null,
        fld_light           int unsigned not null,
        fld_earth           int unsigned not null
    )
]]


-- try to add guid
status, errmsg = conn:execute [[
    insert tbl_monster (
        fld_name,
        fld_race,
        fld_lid,
        fld_undead,
        fld_level,
        fld_hp,
        fld_mp,
        fld_ac,
        fld_mac,
        fld_dc,
        fld_attackspeed,
        fld_walkspeed,
        fld_speed,
        fld_hit,
        fld_viewrange,
        fld_raceindex,
        fld_exp,
        fld_escape,
        fld_water,
        fld_fire,
        fld_wind,
        fld_light,
        fld_earth) values
            -- seems there is problem with utf-8
            ("阿龙怪" , 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
            ("阿龙怪2", 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
            ("hello"  , 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1),
            ("world"  , 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1)
]]

-- print(status, errmsg)


-- create table for guid
status, errmsg = conn:execute [[
    create table if not exists tbl_monsteritem
    (
        fld_index   int unsigned not null auto_increment primary key,
        fld_monster int unsigned not null,
        fld_type    int unsigned not null,
        fld_chance  int unsigned not null,
        fld_count   int unsigned not null
    )
]]

-- try to add guid
conn:execute [[
    insert tbl_monsteritem (fld_monster, fld_type, fld_chance, fld_count) values
        (1, 2, 1, 2),
        (1, 3, 1, 2),
        (1, 5, 1, 2)
]]

