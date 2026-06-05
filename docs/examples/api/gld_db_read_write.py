# CREATE ROLE gld_user WITH LOGIN PASSWORD 'gld_user';
# CREATE DATABASE "gld_db" WITH OWNER "gld_user";

import psycopg
from psycopg import sql
from pprint import pprint
import gridlabd
from pathlib import Path
import os
from datetime import datetime, timedelta, timezone
from zoneinfo import ZoneInfo, available_timezones
import numpy as np
import re
import matplotlib.pyplot as plt

db_user = "gld_user"
db_password = "gld_user"
db_name = "gld_db"
house_list = ["house1", "house3", "house5", "house7", "house9"]
step_size = 900


def test_postgres_connection(db_name, db_user, db_password): 
    try:
        # Replace placeholders with actual values
        connection = psycopg.connect(
            dbname=db_name,
            user=db_user,
            host="0.250.250.254",
            port=5432,
            password=db_password,
            connect_timeout=5
        )
        print("Connected to Postgres")
        return connection
    except psycopg.OperationalError as e:
        print(f"Error: Unable to connect to the database. Details: {e}")
        exit(1)


def change_start_stop_time(gld):
    """
    Change the start and stop time of the GridLAB-D simulation.
    In this case, we're hard-coding it to stop one day after the start time.
    
    gld (gridlabd.GridLABD): The GridLAB-D simulation object.

    Returns:
    tuple: A tuple containing the new start and stop times as datetime objects.
    """
    # Read in current start and stop time
    starttime = datetime.fromisoformat(gld.get_starttime())
    stoptime = datetime.fromisoformat(gld.get_stoptime())
    print(f"Start time in model: {starttime}")
    print(f"Stop time in model: {stoptime}")

    # Calculate and set new start and stop times
    calc_stoptime = datetime.isoformat(starttime + timedelta(days=1))
    gld.set_stoptime(calc_stoptime)

    # Confirm changes to start and stop times
    new_starttime = datetime.fromisoformat(gld.get_starttime())
    new_stoptime = datetime.fromisoformat(gld.get_stoptime())
    print(f"New start time: {new_starttime}")
    print(f"New stop time: {new_stoptime}")
    return new_starttime, new_stoptime


def make_setpoint_schedule(house_list, start_time):
    setpoint_dict = {}
    setpoint_times = [start_time + timedelta(hours=6),
                      start_time + timedelta(hours=12),
                      start_time + timedelta(hours=18)]
    setpoint_temps = [65.0, 60.0, 55.0]
    for house in house_list:
        setpoint_dict[house] = []
        for idx, time in enumerate(setpoint_times) :
            timestamp_str = time.strftime("%Y-%m-%d %H:%M:%S")
            setpoint_dict[house].append({
                "time": timestamp_str,
                "setpoint": setpoint_temps[idx]
            })
    return setpoint_dict


def offset_to_iana(dt: datetime):
    # Ensure dt is timezone-aware
    if dt.tzinfo is None:
        raise ValueError("datetime must be timezone-aware")
    # convert to UTC and get offset at that instant
    offset = dt.utcoffset()
    if offset is None:
        raise ValueError("could not determine offset")
    candidates = []
    for zone_name in sorted(available_timezones()):
        try:
            z = ZoneInfo(zone_name)
            # get offset of zone at this instant
            zoned = dt.astimezone(z)
            if zoned.utcoffset() == offset:
                candidates.append(zone_name)
        except Exception:
            continue
    if "Pacific/Honolulu" in candidates:
        return "Pacific/Honolulu"
    elif "America/Anchorage" in candidates:
        return "America/Anchorage"
    elif "America/Vancouver" in candidates:
        return "America/Vancouver"
    elif "America/Denver" in candidates:
        return "America/Denver"
    elif "America/Chicago" in candidates:
        return "America/Chicago"
    elif "America/New_York" in candidates:
        return "America/New_York"
    else:
        raise ValueError("No matching IANA timezone found")


def set_timezone(db, db_name, starttime):
    cursor = db.cursor()
    tzstring = offset_to_iana(starttime)
    cursor.execute(
        sql.SQL("ALTER DATABASE {} SET TimeZone TO {}")
        .format(sql.Identifier(db_name), sql.Literal(tzstring))
    )
    db.commit()
    cursor.close()


def load_data_into_postgres(db, setpoint_schedule):
    cursor = db.cursor()

    cursor.execute("SELECT to_regclass('public.gld_input_data');")
    table_name = cursor.fetchone()[0]
    if table_name is not None:
        cursor.execute("DROP TABLE gld_input_data;")
        print("Deleted existing table 'gld_input_data'")

    create_table_str = '''
        CREATE TABLE gld_input_data (
        timestamp TIMESTAMP WITH TIME ZONE,
        house CHAR(6),
        setpoint DOUBLE PRECISION
        );
    '''
    cursor.execute(create_table_str)

    for house in setpoint_schedule.keys():
        for setpoint_dict in setpoint_schedule[house]:
            timestamp_str = setpoint_dict["time"]
            setpoint = setpoint_dict["setpoint"]
            cursor.execute(
                '''
                INSERT INTO gld_input_data
                (timestamp, house, setpoint)
                VALUES (%s, %s, %s);
                ''',
                (timestamp_str, house, setpoint),
            )
    print("Data loaded into Postgres table 'gld_input_data'")

    db.commit()
    cursor.close()


def apply_setpoints_for_sim_time(gld, db, sim_time):
    cursor = db.cursor()
    sim_time_str = sim_time.strftime("%Y-%m-%d %H:%M:%S")
    cursor.execute(
        '''
        SELECT house, setpoint
        FROM gld_input_data
        WHERE timestamp = %s;
        ''',
        (sim_time_str,),
    )
    matching_rows = cursor.fetchall()
    cursor.close()

    for house, setpoint in matching_rows:
        gld.set_property(house.strip(), "cooling_setpoint", str(setpoint))
        print(f"Applied setpoint {setpoint} to {house.strip()} at simulation time {sim_time_str}")


def initialize_output_table(db):
    cursor = db.cursor()
    cursor.execute("SELECT to_regclass('public.gld_output_data');")
    table_name = cursor.fetchone()[0]
    if table_name is not None:
        cursor.execute("DROP TABLE gld_output_data;")
        print("Deleted existing table 'gld_output_data'")

    cursor.execute(
        '''
        CREATE TABLE gld_output_data (
        timestamp TIMESTAMP WITH TIME ZONE,
        house CHAR(6),
        indoor_air_temperature DOUBLE PRECISION
        );
        '''
    )
    db.commit()
    cursor.close()


def write_output_data_for_sim_time(gld, db, sim_time):
    houses = gld.get_all_objects("house")
    sim_time_str = sim_time.strftime("%Y-%m-%d %H:%M:%S")
    cursor = db.cursor()

    for house in houses:
        house_name = house.get("name")
        indoor_air_temperature = house.get("air_temperature")
        cursor.execute(
            '''
            INSERT INTO gld_output_data
            (timestamp, house, indoor_air_temperature)
            VALUES (%s, %s, %s);
            ''',
            (sim_time_str, house_name, indoor_air_temperature),
        )

    db.commit()
    cursor.close()


def plot_output_data(db):
    cursor = db.cursor()
    cursor.execute(
        '''
        SELECT timestamp, house, indoor_air_temperature
        FROM gld_output_data
        ORDER BY timestamp, house;
        '''
    )
    rows = cursor.fetchall()
    cursor.close()

    if not rows:
        print("No data found in 'gld_output_data' to plot")
        return

    plot_data = {}
    for timestamp, house, indoor_air_temperature in rows:
        house_name = house.strip()
        if house_name not in plot_data:
            plot_data[house_name] = {"timestamps": [], "temperatures": []}
        plot_data[house_name]["timestamps"].append(timestamp)
        plot_data[house_name]["temperatures"].append(indoor_air_temperature)

    plt.figure(figsize=(12, 6))
    for house_name, series in plot_data.items():
        plt.plot(series["timestamps"], series["temperatures"], label=house_name)

    plt.xlabel("Timestamp")
    plt.ylabel("Indoor Air Temperature")
    plt.title("GridLAB-D Indoor Air Temperature by House")
    plt.legend()
    plt.xticks(rotation=45)
    plt.tight_layout()
    plt.show()







def main():
    # Ensure we're running from the correct directory.
    script_path = os.path.abspath(__file__)
    script_dir = os.path.dirname(script_path)
    os.chdir(script_dir)

    # Instantiate GridLAB-D and load model.
    gld = gridlabd.GridLabD()
    model_path = os.path.join(os.path.dirname(script_dir), "house_with_solar")
    gld.set_working_directory(str(model_path))
    load_code = gld.load("houses.glm")
    if load_code != 0:
        raise RuntimeError(f"Failed to load model with error code {load_code}.")

    starttime, stoptime = change_start_stop_time(gld)
    db = test_postgres_connection(db_name, db_user, db_password)
    set_timezone(db, db_name, starttime)
    thermostat_setpoints = make_setpoint_schedule(house_list, starttime)
    load_data_into_postgres(db, thermostat_setpoints)
    initialize_output_table(db)

    gld.set_time_step(step_size)
    sim_time = starttime
    while sim_time < stoptime:
        time_code, sim_time_str = gld.step()
        if time_code != 0:
            raise RuntimeError(f"Simulation step failed at {sim_time} with error code {time_code}.")
        sim_time = datetime.fromisoformat(sim_time_str)

        # Check for errors
        messages = gld.get_messages()
        filtered_messages = [
            message for message in messages
            if message.get("type") in {"ERROR"}
        ]
        if filtered_messages:
            pprint(filtered_messages)
        gld.clear_messages()

        apply_setpoints_for_sim_time(gld, db, sim_time)
        write_output_data_for_sim_time(gld, db, sim_time)

    gld.stop()
    gld.exit_gld()
    plot_output_data(db)

    

if __name__ == "__main__":
    main()