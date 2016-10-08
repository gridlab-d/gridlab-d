# $Id: create_database.sql 5047 2015-01-25 00:47:07Z dchassin $
#
# Create transactive system database
#

create database if not exists transactive;
use transactive;

# static data
create table if not exists interconnection (

	system_id integer auto_increment,
	name varchar(64),
	owner_id integer default 0,
	group_id integer default 0,
	cdate timestamp default now(),
	mdate timestamp default 0,
	comments varchar(256),

	primary key i_systemid (system_id),
	key i_ownerid (owner_id),
	key i_groupid (group_id)
);

create table if not exists controlarea (
	area_id integer auto_increment,
	system_id integer,
	name varchar(64),

	primary key i_areaid (area_id),
	foreign key (system_id) references interconnection(system_id) 
);

create table if not exists intertie (
	line_id integer auto_increment,
	name varchar(64),
	from_area_id integer,
	to_area_id integer,
	capacity real,

	primary key i_lineid (line_id),
	foreign key (from_area_id ) references controlarea(area_id),
	foreign key (to_area_id ) references controlarea(area_id)
);

create table if not exists supply (
	supply_id integer auto_increment,
	name varchar(64),
	area_id integer,
	capacity real,
	inertia real,

	primary key i_supplyid (supply_id),
	foreign key (area_id) references controlarea(area_id)
);

create table if not exists demand (
	demand_id integer auto_increment,
	name varchar(64),
	area_id integer,
	
	primary key i_demandid (demand_id),
	foreign key (demand_id) references controlarea(area_id)
);

# dynamic data
create table if not exists interconnection_data (
	system_id integer,
	t timestamp,
	frequency real,
	inertia real,
	damping real,
	supply real,
	demand real,

	unique key i_systemid_t (system_id, t),
	foreign key (system_id) references interconnection(system_id)
);

create table if not exists controlarea_data (
	area_id integer,
	t timestamp,
	inertia real,
	capacity real,
	supply real,
	demand real,
	schedule real,
	actual real,
	ace real,
	bias real,

	unique key i_areaid_t (area_id, t),
	foreign key (area_id) references controlarea(area_id)
);

create table if not exists intertie_data (
	line_id integer,
	t timestamp,
	capacity real,
	status integer,
	flow real,
	loss real,

	unique key i_lineid_t (line_id, t),
	foreign key (line_id) references intertie(line_id)
);

create table if not exists supply_data (
	supply_id integer,
	t timestamp,
	schedule real,
	actual real,

	unique key i_supplyid_t (supply_id, t),
	foreign key (supply_id) references supply(supply_id)
);

create table if not exists demand_data (
	demand_id integer,
	t timestamp,
	schedule real,
	actual real,

	unique key i_demandid_t (demand_id, t),
	foreign key (demand_id) references demand(demand_id)
);
