"""
Created on 10/14/2026

This is a proposed implementation of the GridLAB-D™ API in Python with the
purpose of showing GridLAB-D™ developers how a Python user might want to use
GridLAB-D™. The fully realized version of this library will include calls to a
compiled C library; these details are not included here, just the signatures
and the docstrings describing what the method does.

This borrows from the implementation and experience of the Transactive
Systems Program and its implemented GLMModifier API for working with 
GridLAB-D™ models.

Modifying a model consists entirely of manipulating the checkpoint dictionary.


TODO - Probably need to figure out if I should use the term "model" and 
"checkpoint" interchangably or can be more consistent.

@author: Trevor Hardy
trevor.hardy
"""

import json
import typing

class GLDSimMessages():
    """Data structure for holding messages from GridLAB-D™ after advancing
    simulation time
    """

    def __init__(self) -> None:
        self.warnings = []
        self.errors = []
        self.info = []
        self.profile = {}

class GLDCheckpoint():
    """Dictionary representing a specific comprehensive GridLAB-D™ model state.

    The raw checkpoint dictionary is provided as an attribute and convenience
    attributes are created from that 
    """

    def __init__(self, cp) -> None:
        self._cp = cp
        self.clock = cp["clock"]
        # only one climate object allowed
        self.climate = cp["objects"]["climate"]["instances"][0] 

        self.houses = {}
        for idx, house_dict in enumerate(cp["objects"]["house"]["instances"]):
            self.houses[house_dict["name"]] = cp["objects"]["house"]["instances"][idx]

        self.recorders = {}
        for idx, recorder_dict in enumerate(cp["objects"]["recorder"]["instances"]):
            self.recorders[recorder_dict["name"]] = cp["objects"]["recorder"]["instances"][idx]

        self.triplex_meters = {}
        for idx, triplex_meter_dict in enumerate(cp["objects"]["triplex_meter"]["instances"]):
            self.triplex_meters[triplex_meter_dict["name"]] = cp["objects"]["triplex_meter"]["instances"][idx]

        self.substations = {}
        for idx, substations_dict in enumerate(cp["objects"]["substations"]["instances"]):
            self.substations[substations_dict["name"]] = cp["objects"]["substations"]["instances"][idx]
        
        # etc; I'm not going to do these all until we're sure this is the right
        # way to do things

        def save_to_file(file_path: str) -> None:
            """Saves current checkpoint object to file.

            Args:
                file_path (str): path to save checkpoint file
            """
            pass


    @property 
    def comments(self) -> list:
        """Checkpoint preamble comments assigned as a read-only property

        Returns:
            list: comments in checkpoint preamble
        """
        return self._cp["__preamble"]["comments"]


class GLD():
    """Library for using GridLAB-D™ programmatically. 

    This library allows a user to interact with the GridLAB-D™ engine, 
    providing support for such activites as loading a model, modifying
    or extending a model, running the model, and retrieving information
    from the model.
    """

    def __init__(self) -> None:
        pass

    def open_model_file(self, model_file: TextIOWrapper) -> GLDCheckpoint:
        """Loads a .glm or JSON model from disk into the GridLAB-D™ core
        and returns a representation as a Python dictionary in the
        GridLAB-D™ checkpoint format.

        Args:
            model_file (TextIOWrapper): File-like object for the GridLAB-D
            model to be loaded in memory

        Returns:
            GLDCheckpoint: GridLAB-D™ checkpoint dictionary
        """
        pass
    
    def load_model(self, glm: GLDCheckpoint) -> GLDSimMessages:
        """ Takes an existing in-memory GLDCheckpoint object and loads it into
        the GridLAB-D™ core, effectively changing the model that will be used
        during the simulation. This method ensures that the gldcore is ready
        to start running the simulation from the time specified in the loaded
        model; that is, the simulation time is effectively zero.

        It is possible that modifications made to the model are invalid 
        (assinging an object attribute value as a string when it should be a
        float) and the return dictionary will contain error messages from the
        gldcore (if any).

        Args:
            glm (GLDCheckpoint): model state being loaded into the gldcore

        Returns:
            GLDSimMessages: Return message information
        """
        pass

    def make_checkpoint() -> GLDCheckpoint:
        """Makes a GridLAB-D™ checkpoint.

        This can be saved to file as a JSON using the `save_to_file()` method
        of the GLDCheckpoint object.

        Returns:
            GLDCheckpoint: Checkpoint for current model state
        """
        pass

    def get_parameter(self, obj_name: str, parameter_name: str) -> typing.Any:
        """Query the current value of a of given parameter in a given object

        Args:
            obj_name (str): name of object containing parameter being queried
            parameter_name (str): parameter name whose value is being queried
        
        Returns:
            typing.Any: value of parameter being queried
        """
        pass

    def set_parameter(self, obj_name: str, parameter_name: str, value: typing.Any) -> None:
        """Set parameter value for the specified parameter in the specified object

        Args:
            obj_name (str): name of object containing parameter being set
            parameter_name (str): parameter name whose value is being set
            value (typing.Any): new parameter value
        """
        pass

    def get_names(self, class_name: str) -> list:
        """Gets the names of all the objects in the model of the specified 
        GridLAB-D™ class name 

        Args:
            class_name (str): GridLAB-D™ class name 

        Returns:
            list: names as strings of the objects in the model
        """
        pass

    def sim_register_time(self, step_size: float) -> GLDSimMessages:
        """Registers a simulation time step size with the gldcore to ensure
        that GridLAB-D™ will stop the simulation no later than the
        simulation step size indicated. GridLAB-D™ may stop earlier than 
        the given step size depending on the other objects in the model.

        Args:
            time (float): simulation time 

        Returns:
            GLDSimMessages: Error/warning messages from running the model
        """

    def sim_run(self) -> GLDSimMessages:
        """Runs the model currently in the gldcore.

        Returns:
            GLDSimMessages: Error/warning messages from running the model
        """
        pass

    def sim_run_async(self) -> GLDSimMessages:
        """Runs the model currently in the gldcore as an asynchronous command.

        This call will immediately return, allowing the caller to execute
        other commands while the gldcore simulates the next simulated time.
        It is the responsibility of the caller to also call 
        `sim_run_async_complete()` to check when the gldcore has completed
        execution of the next simulated time.

        Returns:
            GLDSimMessages: Error/warning messages from running the model
        """
        pass

    def sim_run_async_complete(self) -> GLDSimMessages:
        """Checks to see if the previous call to `sim_run_async()` is
        complete and the gldcore is ready to move forward in simulated time.
        This is a blocking call until gldcore has finished the simulation.

        Returns:
            tuple: 
                - GLDSimMessages: Error/warning messages from running the model
        """
        pass

    def sim_step(self, step_size: float) -> tuple:
        """Advance the simulation time

        Args:
            step_size (float): simulated step size to advance in seconds

        Returns:
            tuple: 
                - GLDSimMessages: Error/warning messages from running the model
                - float: next time to simulate as determined by GridLAB-D™ 
        """
        pass

    def sim_step_async(self, step_size: float) -> GLDSimMessages:
        """Advance the simulation time as an asynchronous command.

        This call will immediately return, allowing the caller to execute
        other commands while the gldcore simulates the next simulated time.
        It is the responsibility of the caller to also call 
        `sim_step_async_complete()` to check when the gldcore has completed
        execution of the next simulated time.

        Args:
            step_size (float): simulated step size to advance in seconds

        Returns:
            tuple: 
                - GLDSimMessages: Error/warning messages from running the model
                - float: next time to simulate as determined by GridLAB-D™ 
        """
        pass

    def sim_step_async_complete(self) -> tuple:
        """Checks to see if the previous call to `sim_step_async()` is
        complete and the gldcore is ready to move forward in simulated time.
        This is a blocking call until gldcore has finished the simulation.

        Returns:
            tuple: 
                - GLDSimMessages: Error/warning messages from running the model
                - float: next time to simulate as determined by GridLAB-D™ 
        """
        pass

    def set_prestep_callback(self):
        """Sets method that is called prior to advancing simulation time
        """
        pass

    def set_poststep_callback(self):
        """Sets method that is called after advancing simulation time
        """
        pass

    def get_current_simtime(self) -> float:
        """Gets the simulation time of the model loaded in the gldcore

        Returns:
            float: TODO is this the right data type? e-time?
        """
        pass

    def set_current_simtime(self, float) -> GLDSimMessages:
        """Sets the simulation time for the current model loaded in the gldcore

        Args:
            float (_type_): TODO is this the right data type? e-time?

        Returns:
            GLDSimMessages: Messages from gldcore after changing the current
            simulation time
        """
        pass

    def exit_gld(self) -> GLDSimMessages:
        """Closes connection to and execution of the gldcore

        Returns:
            GLDSimMessages: Messages from gldcore on ending execution
        """
        pass