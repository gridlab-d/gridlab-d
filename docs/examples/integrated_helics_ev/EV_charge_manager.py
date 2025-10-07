# -*- coding: utf-8 -*-
"""
Created on 4 July 2024

Simple EV charge management demonstration where GridLAB-D models the EVs and
this Python script implements a simple charge management algorithm to 
regulate the total charging power in the power system.

This implements a silly charge management algorithm where the lower the 
SOC, the greater the charging power allowed.

Co-simulation data exchange diagram:

                 
                     -------  EV SOCs  ----- >
GridLAB-D EV models                            Charge manager 
                     <--- charging powers ----

Install PyHELICS by
$ pip install helics

Test co-simulation by running
$ helics run --path=gld_python_demo.json

OR

$ python EV_charge_manager.py & (or launch in its own shell)
$ gridlabd five_EV_chargers.glm& (or launch in its own shell)
$ helics_broker -f 2  (or launch in its own shell)

@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""

import matplotlib.pyplot as plt
import helics as h
import logging
import numpy as np


logger = logging.getLogger(__name__)
logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.DEBUG)


if __name__ == "__main__":
    # *****  Model parameter definitions and data setup  *****
    # Maximum charging power per EV
    max_charging_power = 10000
    min_ev_charging_power = 3000
    regulated_total_charging_power = 20000
    ev_charging_power_delta = max_charging_power - min_ev_charging_power
    regulated_charging_power = {}

    # Simulation time management variable
    hours = 24 * 7
    final_sim_time = int(60 * 60 * hours)
    sim_time_stepsize_s = None
    init_sim_time = 0
    requested_sim_time = init_sim_time
    granted_sim_time = 0

    

    # *****  HELICS configuration  *****
    # Load in HELICS configuration from JSON 
    fed = h.helicsCreateValueFederateFromConfig("EV_Charge_Manager_config.json")
    fed_name =  h.helicsFederateGetName(fed)
    sub_count = h.helicsFederateGetInputCount(fed)
    logger.debug(f"\tNumber of subscriptions: {sub_count}")
    pub_count = h.helicsFederateGetPublicationCount(fed)
    logger.debug(f"\tNumber of publications: {pub_count}")
    sim_time_stepsize_s = h.helicsFederateGetTimeProperty(fed, h.HELICS_PROPERTY_TIME_PERIOD)


    # Diagnostics to confirm JSON config correctly added the required
    #   publications and subscriptions
    subs = {}
    for i in range(0, sub_count):
        sub_obj = h.helicsFederateGetInputByIndex(fed, i)
        sub_name = h.helicsSubscriptionGetTarget(sub_obj)
        subs[i] = {"sub obj": sub_obj, "sub name": sub_name}
        logger.debug(f"\tRegistered subscription---> {sub_name}")

    pubs = {}
    for i in range(0, pub_count):
        pub_obj = h.helicsFederateGetPublicationByIndex(fed, i)
        pub_name = h.helicsPublicationGetName(pub_obj)
        pubs[i] = {"pub obj": pub_obj, "pub name": pub_name}
        logger.debug(f"\tRegistered publication---> {pub_name}")

    # *****  Data collection setup  *****
    # Each EV produces two subscriptions and one publication in the HELICS
    # configuration
    ev_count = pub_count

    recorded_charging_power = {}
    recorded_ev_SOCs= {}
    recorded_time = []
    ev_names = []
    # Setting up data collection dictionaries with the keys being the 
    # EV name
    for sub in subs.keys():
        ev_name = sub[sub_name] # TODO: update this based on the subscription name
        ev_names.append(ev_name)
        recorded_charging_power[ev_name] = []
        recorded_ev_SOCs[ev_name] = []

    recorded_total_charging_power = []

    # *****  HELICS start co-simulation  *****
    fed.enter_executing_mode()

    # As long as granted time is in the time range to be simulated, 
    # update the model
    while granted_sim_time < final_sim_time:
         # ***** Advance simulation time *****
        requested_sim_time += sim_time_stepsize_s
        logger.debug(f"Requesting time: {requested_sim_time/3600}")
        granted_sim_time = fed.request_time(requested_sim_time)
        sim_time_hr = granted_sim_time / 3600  
        logger.debug(f"Granted sim time (hr): {sim_time_hr:.2f}")   
        recorded_time.append(sim_time_hr)

        total_charging_power = 0
        for i, ev_name in enumerate(ev_names): 
            # Get latest inputs from rest of federation 
            # SOC and corresponding vehicle location subs are separated by 
            # an index delta of sub_count
            logger.debug(f"\t{subs[i]['sub name']}")
            soc_percent = h.helicsInputGetDouble(subs[i]["sub obj"])  
            logger.debug(f"\t\treceived SOC of {soc_percent}")
            logger.debug(f"\t{subs[i + ev_count]['sub name']}")
            vehicle_location = h.helicsInputGetString(subs[i + ev_count]["sub obj"])  
            logger.debug(f"\t\treceived location of {vehicle_location}")

            # *****  Update internal model  ***** 
            # Set charging power inversely propotional to the SOC
            # Higher SOC -> lower charging power (down to min_charging_power)
            # Lower SOC -> higher charging power (up to max_charging_power)
            soc_factor = soc_percent/100
            recorded_ev_SOCs[ev_names[i]].append(soc_factor) 
            soc_remainder = 1 - soc_factor
            if soc_factor == 1:
                regulated_charging_power[i] = 0
            else:
                # Only worry about charging if the vehicle is at home
                if vehicle_location == "HOME":
                    regulated_charging_power[i] = (soc_remainder * ev_charging_power_delta) + min_ev_charging_power
                else: 
                    regulated_charging_power[i] = 0
            total_charging_power += regulated_charging_power[i]

        logger.debug(f"\t\tPre-regulated total charging power: {total_charging_power}")
        if total_charging_power > regulated_total_charging_power:
            scaling_factor = regulated_total_charging_power/total_charging_power
            logger.debug(f"\t\tReducing charging power by factor of {scaling_factor} to stay under total charging limit of {regulated_total_charging_power}.") 
        else:
            scaling_factor = 1
            logger.debug(f"\t\tNo reduction in charging power needed to stay under total charging limit of {regulated_total_charging_power}.")

        total_charging_power = 0
        for i in range(0, ev_count): 
            regulated_charging_power[i] = regulated_charging_power[i] * scaling_factor
            total_charging_power += regulated_charging_power[i]
            h.helicsPublicationPublishDouble(pubs[i]["pub obj"], regulated_charging_power[i])
            logger.debug(f"\t\t{pubs[i]['pub name']} sent charging power of {regulated_charging_power[i]}") 
            recorded_charging_power[ev_name].append(regulated_charging_power[i])
        logger.debug(f"\t\tPost-regulated total charging power: {total_charging_power}")      
        recorded_total_charging_power.append(total_charging_power)
    
    # *****  HELICS end co-simulation  *****
    fed.disconnect()
    h.helicsFederateDestroy(fed)


    # Printing out final results graphs for comparison/diagnostic purposes.
    fig, axs = plt.subplots()
    axs.plot(recorded_time, recorded_total_charging_power)
    axs.set_yticks(np.arange(0, 30000, 5000))
    axs.set_xlabel("Time (hrs)")
    axs.set_ylabel("Total Charging Power")
    # plt.show()


    fig, axs = plt.subplots(5, sharex=True, sharey=True)
    fig.suptitle("Battery Charging Power")

    axs[0].plot(recorded_time, recorded_charging_power["EV 1"], color="tab:blue", linestyle="-")
    axs[0].set_yticks(np.arange(0, 10000, 2500))
    axs[0].set(ylabel="EV 1")
    axs[0].grid(True)

    axs[1].plot(recorded_time, recorded_charging_power["EV 2"], color="tab:blue", linestyle="-")
    axs[1].set(ylabel="EV 2")
    axs[1].grid(True)

    axs[2].plot(recorded_time, recorded_charging_power["EV 3"], color="tab:blue", linestyle="-")
    axs[2].set(ylabel="EV 3")
    axs[2].grid(True)

    axs[3].plot(recorded_time, recorded_charging_power["EV 4"], color="tab:blue", linestyle="-")
    axs[3].set(ylabel="EV 4")
    axs[3].grid(True)

    axs[4].plot(recorded_time, recorded_charging_power["EV 5"], color="tab:blue", linestyle="-")
    axs[4].set(ylabel="EV 5")
    axs[4].grid(True)
    plt.xlabel("time (hr)")
    # plt.show()


    fig, axs = plt.subplots(5, sharex=True, sharey=True)
    fig.suptitle("SOC of each EV Battery")

    axs[0].plot(recorded_time, recorded_ev_SOCs["EV 1"], color="tab:blue", linestyle="-")
    axs[0].set_yticks(np.arange(0, 1.25, 0.25))
    axs[0].set(ylabel="EV 1")
    axs[0].grid(True)

    axs[1].plot(recorded_time, recorded_ev_SOCs["EV 2"], color="tab:blue", linestyle="-")
    axs[1].set(ylabel="EV 2")
    axs[1].grid(True)

    axs[2].plot(recorded_time, recorded_ev_SOCs["EV 3"], color="tab:blue", linestyle="-")
    axs[2].set(ylabel="EV 3")
    axs[2].grid(True)

    axs[3].plot(recorded_time, recorded_ev_SOCs["EV 4"], color="tab:blue", linestyle="-")
    axs[3].set(ylabel="EV 4")
    axs[3].grid(True)

    axs[4].plot(recorded_time, recorded_ev_SOCs["EV 5"], color="tab:blue", linestyle="-")
    axs[4].set(ylabel="EV 5")
    axs[4].grid(True)
    plt.xlabel("time (hr)")
    plt.show()



