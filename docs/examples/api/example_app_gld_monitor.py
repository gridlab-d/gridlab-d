"""
Created on 12/5/2025

Class created by claude-sonnet-4-20250514-v1 with the following prompt

"You are a Python expert, particularly with Tk. I need a GUI application that 
will allow me to display data coming from a simulation that I launch. The data
is gathered at each time-step of the simulation and I want to update a 
time-series graph with that data."

Revised by Trevor Hardy to incorporate GridLAB-D.

trevor.hardy@pnnl.gov
"""


import tkinter as tk
from tkinter import ttk, messagebox
from functools import partial
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from matplotlib.animation import FuncAnimation
import matplotlib.dates as mdates
import numpy as np
import threading
import queue
import time
from collections import deque
import gridlabd
from datetime import datetime, timedelta
from pathlib import Path
import json
import copy
import logging
import sys

# Setting up logging
logger = logging.getLogger(__name__)


class GLD:
    def __init__(self, working_dir:str, glm_file:str):
        self.gld = gridlabd.GridLabD()
        self.sim_time: datetime
        self.set_working_folder(working_dir)
        self.load_model(glm_file)

    def set_working_folder(self, working_folder):
        logger.debug(f"Setting working directory to {working_folder}")
        model_folder = Path(working_folder)
        self.gld.set_working_directory(str(model_folder))

    def load_model(self, model_file:str):
        logger.debug(f"Loading model {model_file}")
        self.gld.load_glm(["gridlabd", model_file])
        # self._make_dictionaries()

    def set_time_step(self, step_size):
        logger.debug(f"Setting step size to {step_size}")
        self.gld.set_time_step(step_size)
        
    def step(self):
        self.gld.step()
        status, current_time_str = self.gld.get_time()
        self.sim_time = self.parse_gld_time(current_time_str)
        logger.debug(f"Setting current time to {self.sim_time}")
        return status, self.sim_time

    def parse_gld_time(self, time_str):
        """
        Parses a GridLAB-D time string and converts it to a datetime object.

        Args:
            time_str (str): A time string retrieved from GridLAB-D, formatted as '%Y-%m-%d %H:%M:%S'.

        Returns:
            datetime: A Python datetime object.
        """
        parts = time_str.rsplit(' ', 1)
        if len(parts) == 2 and parts[1] in ['PST', 'PDT', 'EST', 'EDT', 'CST', 'CDT', 'MST', 'MDT']:
            time_str = parts[0]
        return datetime.strptime(time_str, '%Y-%m-%d %H:%M:%S')
    
    def _make_dictionaries(self):
        # Making the checkpoint object produces a segmentation fault
        logger.debug("Making checkpoint")
        checkpoint_json = self.gld.get_checkpoint_json()
        logger.debug(f"Checkpoint JSON created")
        checkpoint_dict = json.loads(checkpoint_json)

        logger.debug(f"Getting object types in model")
        object_types = list(checkpoint_dict['objects'])
        for obj_type in object_types:
            obj_dict = {}
            for obj in checkpoint_dict['objects'][obj_type]['instances']:
                obj_dict[obj['name']] = obj 
            logger.debug(f"Made dictionary for {obj_type}")
            setattr(self, obj_type, copy.deepcopy(obj_dict))
        
        logger.debug(f"Class attributes: {vars(self)}")


class SimulationGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("Real-Time Simulation Data Viewer")
        self.root.geometry("800x600")
        
        # Data storage
        self.data_queue = queue.Queue()
        self.time_data = deque(maxlen=1000)  # Store last 1000 points
        self.value_data = deque(maxlen=1000)
        
        # Simulation control
        self.simulation_running = False
        self.simulation_thread = None
        
        # Create GUI elements
        self.create_widgets()
        
        # Setup matplotlib animation
        self.setup_plot()
        
    def create_widgets(self):
        # Control frame
        control_frame = ttk.Frame(self.root)
        control_frame.pack(side=tk.TOP, fill=tk.X, padx=10, pady=5)
        
        # Simulation controls
        self.start_button = ttk.Button(control_frame, text="Start Simulation", 
                                     command=self.start_simulation)
        self.start_button.pack(side=tk.LEFT, padx=5)
        
        self.stop_button = ttk.Button(control_frame, text="Stop Simulation", 
                                    command=self.stop_simulation, state=tk.DISABLED)
        self.stop_button.pack(side=tk.LEFT, padx=5)
        
        self.clear_button = ttk.Button(control_frame, text="Clear Data", 
                                     command=self.clear_data)
        self.clear_button.pack(side=tk.LEFT, padx=5)
        
        # Status label
        self.status_label = ttk.Label(control_frame, text="Status: Ready")
        self.status_label.pack(side=tk.RIGHT, padx=5)
        
        # Settings frame
        settings_frame = ttk.LabelFrame(self.root, text="Animation Settings")
        settings_frame.pack(side=tk.TOP, fill=tk.X, padx=10, pady=5)
        
        # Update interval
        ttk.Label(settings_frame, text="Update Interval (ms):").pack(side=tk.LEFT, padx=5)
        self.interval_var = tk.StringVar(value="50")
        interval_spinbox = ttk.Spinbox(settings_frame, from_=10, to=1000, 
                                     textvariable=self.interval_var, width=10)
        interval_spinbox.pack(side=tk.LEFT, padx=5)
        
        # Max points to display
        ttk.Label(settings_frame, text="Max Points:").pack(side=tk.LEFT, padx=5)
        self.max_points_var = tk.StringVar(value="2000")
        points_spinbox = ttk.Spinbox(settings_frame, from_=100, to=5000, 
                                   textvariable=self.max_points_var, width=10)
        points_spinbox.pack(side=tk.LEFT, padx=5)

        # Date format selection
        ttk.Label(settings_frame, text="Date Format:").pack(side=tk.LEFT, padx=5)
        self.date_format_var = tk.StringVar(value="full")
        date_format_combo = ttk.Combobox(settings_frame, textvariable=self.date_format_var, 
                                       values=["auto", "full", "date_only", "time_only", "compact"], 
                                       width=10, state="readonly")
        date_format_combo.pack(side=tk.LEFT, padx=5)
        
        # Apply settings button
        apply_button = ttk.Button(settings_frame, text="Apply Settings", 
                                command=self.apply_settings)
        apply_button.pack(side=tk.LEFT, padx=10)
        
        # Plot frame
        plot_frame = ttk.Frame(self.root)
        plot_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        # Create matplotlib figure
        self.fig, self.ax = plt.subplots(figsize=(10, 6))
        self.ax.set_title("Real-Time Simulation Data")
        self.ax.set_xlabel("Time Step")
        self.ax.set_ylabel("Value")
        self.ax.grid(True, alpha=0.3)

        # Setup datetime formatting for x-axis
        self.setup_datetime_formatting()
        
        # Create canvas
        self.canvas = FigureCanvasTkAgg(self.fig, plot_frame)
        self.canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        
        # Initialize empty line plot
        self.line, = self.ax.plot([], [], 'b-', linewidth=2)

    def setup_datetime_formatting(self):
        """Configure datetime formatting for the x-axis"""
        # Format the x-axis to show dates nicely
        self.ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
        self.ax.xaxis.set_minor_formatter(mdates.DateFormatter('%H:%M'))
        
        # Rotate labels for better readability
        plt.setp(self.ax.xaxis.get_majorticklabels(), rotation=45, ha='right')
        
        # Enable automatic date formatting
        self.fig.autofmt_xdate()
        
    def update_date_formatting(self, format_type="auto"):
        """Update the date formatting based on user selection"""
        if format_type == "full":
            self.ax.xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m-%d %H:%M:%S'))
        elif format_type == "date_only":
            self.ax.xaxis.set_major_formatter(mdates.DateFormatter('%Y-%m-%d'))
        elif format_type == "time_only":
            self.ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
        elif format_type == "compact":
            self.ax.xaxis.set_major_formatter(mdates.DateFormatter('%m/%d %H:%M'))
        else:  # auto
            # Automatically choose format based on data range
            if len(self.time_data) > 1:
                time_range = max(self.time_data) - min(self.time_data)
                if time_range.days > 1:
                    self.ax.xaxis.set_major_formatter(mdates.DateFormatter('%m/%d %H:%M'))
                elif time_range.seconds > 3600:  # More than 1 hour
                    self.ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
                else:
                    self.ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
            else:
                self.ax.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
        
        # Re-enable automatic date formatting
        self.fig.autofmt_xdate()
        
    def setup_plot(self):
        """Setup the animated plot"""
        self.animation = FuncAnimation(
            self.fig, self.update_plot, interval=50, 
            blit=False, repeat=True
        )
        
    def update_plot(self, frame):
        """Update the plot with new data"""
        # Process all available data from queue
        while not self.data_queue.empty():
            try:
                timestamp, value = self.data_queue.get_nowait()
                # Ensure timestamp is a datetime object
                if not isinstance(timestamp, datetime):
                    if isinstance(timestamp, (int, float)):
                        # Convert numeric timestamp to datetime (assuming Unix timestamp or similar)
                        timestamp = datetime.fromtimestamp(timestamp)
                    else:
                        # Try to parse string timestamp
                        timestamp = datetime.fromisoformat(str(timestamp))
                
                self.time_data.append(timestamp)
                self.value_data.append(value)
            except queue.Empty:
                break
            except Exception as e:
                logger.error(f"Error processing timestamp: {e}")
                continue

            # Update plot if we have data
        if self.time_data and self.value_data:
            # Convert datetime objects to matplotlib date numbers for plotting
            time_nums = mdates.date2num(list(self.time_data))
            self.line.set_data(time_nums, list(self.value_data))
            
            # Auto-scale the plot
            self.ax.relim()
            self.ax.autoscale_view()
            
            # Update date formatting if needed
            self.update_date_formatting(self.date_format_var.get())
        
        return self.line,
    
    def start_simulation(self):
        """Start the simulation in a separate thread"""
        if not self.simulation_running:
            self.simulation_running = True
            self.simulation_thread = threading.Thread(target=self.run_simulation, daemon=True)
            self.simulation_thread.start()
            
            self.start_button.config(state=tk.DISABLED)
            self.stop_button.config(state=tk.NORMAL)
            self.status_label.config(text="Status: Running")
    
    def stop_simulation(self):
        """Stop the simulation"""
        self.simulation_running = False
        self.start_button.config(state=tk.NORMAL)
        self.stop_button.config(state=tk.DISABLED)
        self.status_label.config(text="Status: Stopped")
    
    def clear_data(self):
        """Clear all data"""
        self.time_data.clear()
        self.value_data.clear()
        # Clear the queue
        while not self.data_queue.empty():
            try:
                self.data_queue.get_nowait()
            except queue.Empty:
                break
        self.status_label.config(text="Status: Data Cleared")
    
    def apply_settings(self):
        """Apply new settings"""
        try:
            new_interval = int(self.interval_var.get())
            max_points = int(self.max_points_var.get())
            
            # Update deque max length
            self.time_data = deque(self.time_data, maxlen=max_points)
            self.value_data = deque(self.value_data, maxlen=max_points)
            
            # Update animation interval
            self.animation.event_source.interval = new_interval

            # Update date formatting
            self.update_date_formatting(self.date_format_var.get())
            
            messagebox.showinfo("Settings", "Settings applied successfully!")
            
        except ValueError:
            messagebox.showerror("Error", "Please enter valid numeric values")

    def get_temperature_value(self, temperature:str):
        # temperature = +70.0005 degF
        temperature = temperature[:-4]
        temperature.strip()
        temperature_float = float(temperature)
        logger.debug(f"house1 air_temperature: {temperature_float}")
        return temperature_float
    
    def run_simulation(self):
        
        # Initialize GLD
        gld = GLD("./house_with_solar", "./houses.glm")
        gld.set_time_step(60)
        sim_time_ordinal = 0 
        
        while self.simulation_running:
            # Step simulation time.
            status, sim_time = gld.step()
            house1_properties = gld.gld.get_object_properties("house1")
            house1_air_str = house1_properties["air_temperature"]
            house_1_air_float = self.get_temperature_value(house1_air_str)

            house2_properties = gld.gld.get_object_properties("house2")
            house2_air_str = house2_properties["air_temperature"]
            house_2_air_float = self.get_temperature_value(house2_air_str)

            # Put data into queue for the GUI thread
            # Single data plot 
            #self.data_queue.put((sim_time, house_1_air_float))
            house1_data = {
                "series": "house1",
                "time": sim_time,
                "value": house_1_air_float
            }

            house2_data = {
                "series": "house2",
                "time": sim_time,
                "value": house_2_air_float
            }
            self.data_queue.put(house1_data)
            self.data_queue.put(house2_data)


            sim_time_ordinal += 1
            if sim_time_ordinal % 500 == 0:
                cooling_setpoint_str = house1_properties["air_temperature"]
                cooling_setpoint_float = self.get_temperature_value(cooling_setpoint_str)
                gld.gld.set_property("house1", "cooling_setpoint", str(cooling_setpoint_float + 1))

            # Simulate some processing time
            time.sleep(0.00)  # Adjust this based on your simulation speed
    
    def add_data_point(self, timestamp, value):
        """
        Public method to add data points from external simulation
        Call this method from your simulation to add data points
        """
        self.data_queue.put((timestamp, value))
    
    def on_closing(self):
        """Handle window closing"""
        self.stop_simulation()
        self.root.quit()
        self.root.destroy()

# Enhanced version with multiple data series
class MultiSeriesSimulationGUI(SimulationGUI):
    def __init__(self, root):
        # Data for multiple series
        self.series_data = {}
        self.series_lines = {}
        self.colors = ['b', 'r', 'g', 'c', 'm', 'y', 'k']
        
        super().__init__(root)
    
    def add_data_series(self, series_name, color=None):
        """Add a new data series to track"""
        if series_name not in self.series_data:
            self.series_data[series_name] = {
                'time': deque(maxlen=1000),
                'values': deque(maxlen=1000)
            }
            
            # Choose color
            if color is None:
                color = self.colors[len(self.series_data) % len(self.colors)]
            
            # Create line for this series
            line, = self.ax.plot([], [], color=color, linewidth=2, 
                               label=series_name)
            self.series_lines[series_name] = line
            
            # Update legend
            self.ax.legend()
            
    def add_data_point_series(self, series_name, timestamp, value):
        """Add data point to a specific series
        timestamp should be a datetime object"""
        data = {'series': series_name, 'time': timestamp, 'value': value}
        self.data_queue.put(data)
    
    def update_plot(self, frame):
        """Update plot for multiple series"""
        # Process all available data from queue
        while not self.data_queue.empty():
            try:
                data = self.data_queue.get_nowait()
                logger.debug(f"Data from queue: {}")
                if isinstance(data, dict) and 'series' in data:
                    # Multi-series data
                    logger.debug(f"***** Got valid data: {data} ******")
                    series_name = data['series']
                    timestamp = data['time']
                    value = data['value']
                    
                    # Ensure timestamp is a datetime object
                    if not isinstance(timestamp, datetime):
                        if isinstance(timestamp, (int, float)):
                            timestamp = datetime.fromtimestamp(timestamp)
                        else:
                            timestamp = datetime.fromisoformat(str(timestamp))
                    
                    if series_name in self.series_data:
                        self.series_data[series_name]['time'].append(timestamp)
                        self.series_data[series_name]['values'].append(value)
                else:
                    # Single series data (backward compatibility)
                    timestamp, value = data
                    # Ensure timestamp is a datetime object
                    if not isinstance(timestamp, datetime):
                        if isinstance(timestamp, (int, float)):
                            timestamp = datetime.fromtimestamp(timestamp)
                        else:
                            timestamp = datetime.fromisoformat(str(timestamp))
                    
                    self.time_data.append(timestamp)
                    self.value_data.append(value)
            except queue.Empty:
                break
            except Exception as e:
                logger.error(f"Error processing data: {e}")
                continue
        
        # Update all series
        for series_name, line in self.series_lines.items():
            time_data = list(self.series_data[series_name]['time'])
            value_data = list(self.series_data[series_name]['values'])
            if time_data and value_data:
                # Convert datetime objects to matplotlib date numbers
                time_nums = mdates.date2num(time_data)
                line.set_data(time_nums, value_data)
        
        # Update single series (backward compatibility)
        if self.time_data and self.value_data:
            time_nums = mdates.date2num(list(self.time_data))
            self.line.set_data(time_nums, list(self.value_data))
        
        # Auto-scale the plot
        self.ax.relim()
        self.ax.autoscale_view()
        
        # Update date formatting if needed
        self.update_date_formatting(self.date_format_var.get())
        
        return list(self.series_lines.values()) + [self.line]

def main():
    # This slightly complex mess allows lower importance messages
    # to be sent to the log file and ERROR messages to additionally
    # be sent to the console as well. Thus, when bad things happen
    # the user will get an error message in both places which,
    # hopefully, will aid in trouble-shooting.
    fileHandle = logging.FileHandler("gld_monitor.log",'w')
    fileHandle.setLevel(logging.DEBUG)
    streamHandle = logging.StreamHandler(sys.stdout)
    streamHandle.setLevel(logging.DEBUG)
    logging.basicConfig(level=logging.DEBUG,
                        handlers=[fileHandle, streamHandle])

    # Initializing GUI
    root = tk.Tk()

    
    
    # Choose which version to use
    # app = SimulationGUI(root)  # Single series
    app = MultiSeriesSimulationGUI(root)  # Multiple series
    
    # For multi-series, add some series
    if isinstance(app, MultiSeriesSimulationGUI):
        app.add_data_series("House 1 temperature", 'blue')
        app.add_data_series("House 2 temperature", 'red')
    
    # Handle window closing
    root.protocol("WM_DELETE_WINDOW", app.on_closing)
    
    root.mainloop()

if __name__ == "__main__":
    main()