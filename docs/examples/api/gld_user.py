"""
This is an early prototype of what a user-facing GridLAB-D™ class might look
like. The core functionality it provides is the ability to interact with a
loaded and running GridLAB-D™ model as if it were just a normal Python 
dictionary. 

TODO - If this ends up being the direction we want to go, we should talk with
Mitch Pelton to understand what GLMModifier is doing and what of that we can 
leverage.


Created on 12/31/2025
@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""

import gridlabd
from pathlib import Path
import os
import logging

logger = logging.getLogger(__name__)

class _GLDObj:
    """
    Private diciontary-like class that calls the GLD API for changing an 
    object's parameter value when any value is changed (e.g. 
    house1["floor_area"] = 1000. This allows users to interact with something
    that acts like a dictionary to update the model (during runtime) rather
    than having to remember the GLD API calls.
    """
    def __init__(self, input_dict, gld):
        self._data = input_dict

    def __setitem__(self, key, value):
        self._data[key] = value
        gld.set_property(self._data["__name__"], key, str(value))

    def __getitem__(self, key):
        return self._data[key]

    def __repr__(self):
        return f"{self._data!r}"

class GLDModel:
    """
    GLDModel is the data structure for holding the GridLAB-D™ model in a form
    that is easy for users to interact with. Its only method is a private one
    that takes the output from GridLAB-D's `get_model()` function and converts
    it into this format.

    `get_model()` output format:

    {"house": [{obj1}, {obj2}], 
    "node": [{obj1}, {obj2}], ...}

    TODO - Currently users access this model in the following form:
    `glm.house["house1"]["Rwall"]` where all the house objects are held by a 
    "house" attribute of this class. It may be easier to structure the model
    without a per-class attribute: `glm.["house"]["house1"]["Rwall"]`

    TODO - It would be nice to allow users to use the same instance of the
    model to do modifications before loading it into GridLAB-D™ (when additions
    to the model are permitted) and after running the model begins. This will 
    require changes in the GridLAB-D™ API to allow the model to load (and thus
    be parsed and structures so it can be pulled with `.get_model()`) and also
    to be re-loaded prior to the simulation beginning after the user has made
    changes. If those changes existed we could do some fancy stuff that would 
    allow _GLDObj to support things like `pop()`, `clear()`, and `__delitem()`
    (normal dictionary operations) before the simulation has started but not
    after.
    """
    def __init__(self, input_model, gld):
        # Doing this the stupid way for now. We need a programmatic way 
        # Besides, AI models did the grunt work for me
        self.player = {}
        self.shaper = {}
        self.recorder = {}
        self.multi_recorder = {}
        self.collector = {}
        self.histogram = {}
        self.group_recorder = {}
        self.violation_recorder = {}
        self.metrics_collector = {}
        self.metrics_collector_writer = {}
        self.climate = {}
        self.weather = {}
        self.csv_reader = {}
        self.diesel_dg = {}
        self.windturb_dg = {}
        self.inverter = {}
        self.rectifier = {}
        self.battery = {}
        self.solar = {}
        self.central_dg_control = {}
        self.controller_dg = {}
        self.inverter_dyn = {}
        self.sync_ctrl = {}
        self.energy_storage = {}
        self.sec_control = {}
        self.powerflow_object = {}
        self.powerflow_library = {}
        self.node = {}
        self.link = {}
        self.capacitor = {}
        self.fuse = {}
        self.meter = {}
        self.line = {}
        self.line_spacing = {}
        self.overhead_line = {}
        self.underground_line = {}
        self.overhead_line_conductor = {}
        self.underground_line_conductor = {}
        self.line_configuration = {}
        self.transformer_configuration = {}
        self.transformer = {}
        self.load = {}
        self.regulator_configuration = {}
        self.regulator = {}
        self.triplex_node = {}
        self.triplex_meter = {}
        self.triplex_line = {}
        self.triplex_line_configuration = {}
        self.triplex_line_conductor = {}
        self.switch = {}
        self.substation = {}
        self.pqload = {}
        self.voltdump = {}
        self.series_reactor = {}
        self.restoration = {}
        self.volt_var_control = {}
        self.fault_check = {}
        self.motor = {}
        self.billdump = {}
        self.power_metrics = {}
        self.currdump = {}
        self.recloser = {}
        self.sectionalizer = {}
        self.emissions = {}
        self.load_tracker = {}
        self.triplex_load = {}
        self.impedance_dump = {}
        self.vfd = {}
        self.jsondump = {}
        self.series_compensator = {}
        self.performance_motor = {}
        self.sync_check = {}
        self.residential_enduse = {}
        self.appliance = {}
        self.house = {}
        self.waterheater = {}
        self.lights = {}
        self.refrigerator = {}
        self.clotheswasher = {}
        self.dishwasher = {}
        self.occupantload = {}
        self.plugload = {}
        self.microwave = {}
        self.range = {}
        self.freezer = {}
        self.dryer = {}
        self.evcharger = {}
        self.ZIPload = {}
        self.thermal_storage = {}
        self.evcharger_det = {}
        self._load_dicts(input_model, gld)

    def _load_dicts(self, input_model, gld):
        """
        Takes input model and from the GridLAB-D™ API and loads it into
        the data structure defined for this class
        """
        for class_name in input_model.keys():
            for obj_dict in input_model[class_name]:
                attr_dict = getattr(self, class_name, None)
                if attr_dict is None:
                    raise AttributeError(f"No attribute in GLDModel named '{class_name}'; "
                                        "this probably isn't a valid GridLAB-D™ class.")
                else:
                    try:
                        obj_name = obj_dict["__name__"]
                    except:
                        try:
                            obj_name = obj_dict["__id__"]
                        except:
                            raise ValueError("No '__name__' or '__id__' "
                                            " parameter present; dictionary"
                                            " cannot be named.")
                    attr_dict[obj_name] = _GLDObj(obj_dict, gld)

class GLD:
    """
    GLD is intended to be the user-facing class for the GridLAB-D™ API. It
    largely wraps the existing APIs and in some cases, adds some nice support
    functionality to make said APIs easier to use. 

    It also holds the GLDModel object and the GridLAB-D™ object. The later is 
    the object created with the GridLAB-D™ API and is effectively one instance
    of the GridLAB-D™ engine and its corresponding model. When using this 
    class, the model being used by the engine is accessible through the `glm`
    attribute.
    """
    def __init__(self, *args, **kwargs):
        self.glm = None
        self.gld = gridlabd.GridLabD()
        self.wd = None
        
    def set_working_directory(self, path: str):
        self.wd = path
        self.gld.set_working_directory(self.wd)
        
    
    def load_glm(self, model_path: str) -> GLDModel:
        if not self.wd:
            self.gld.set_working_directory(model_path)
        self.gld.load_glm(model_path)    
        gld.get_model()
    
    def set_time_step(self, step_size: float):
        if step_size.is_integer:
            self.gld.set_time_step(int(step_size))
        else:
            self.gld.set_time_step(step_size)

    def get_model(self):
        model = self.gld.get_model()
        if self.glm:
            self.glm._load_dicts(model, self.gld)
        else:
            self.glm = GLDModel(model, self.gld)

    def step(self):
        self.gld.step()

    def get_executable_path(self) -> str:
        return self.gld.get_executable_path()

    def get_object_properties(self, obj_name: str) -> dict:
        return self.gld.get_object_properties(obj_name)
    
    def set_property(self, obj_name: str, property_name: str, value: str) -> int: 
        return self.gld.set_property(obj_name, property_name, value)
        

# Load and initialize model
gld = GLD()
print(f"GLD executable path: {gld.get_executable_path()}")
script_path = Path(os.path.dirname(os.path.abspath(__file__)))
model_path = script_path.joinpath("house_with_solar")
gld.set_working_directory(str(model_path))
gld.load_glm(["gridlabd", "./houses.glm"])
step_size = 900
gld.set_time_step(step_size)
gld.step()

# Change model and see if it works
print(f'Old Rwall: {gld.glm.house["house1"]["Rwall"]}') # Read it like a dict
gld.glm.house["house1"]["Rwall"] = "30"                 # Modify it like a dict
house1_dict = gld.get_object_properties("house1")       # Call the GLD API to get the new from the model
print(f'New Rwall: {house1_dict["Rwall"]}')             # New value is in GLD!

# Change whole dictionary
dummy = 0

# Get current state of GLD model and push into dictionary