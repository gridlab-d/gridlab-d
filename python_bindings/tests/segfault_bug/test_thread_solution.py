#!/usr/bin/env python3
"""
Test threading solution for multiple GridLabD instances.
This creates two threads, each with its own GridLabD instance.
If threading isolates the global state properly, this should work.
"""

import threading
import gridlabd
import time

def create_and_use_instance(instance_name):
    """Create and use a GridLabD instance in a thread."""
    print(f"{instance_name}: Creating GridLabD instance...")
    gld = gridlabd.GridLabD()
    print(f"{instance_name}: Instance created: {gld}")
    
    print(f"{instance_name}: Trying to set global 'clock'...")
    try:
        gld.set_global("clock", "0")
        print(f"{instance_name}: set_global worked!")
    except Exception as e:
        print(f"{instance_name}: set_global failed: {e}")
    
    # Keep thread alive for a bit
    time.sleep(1)
    print(f"{instance_name}: Thread finishing")

def test_threaded_instances():
    """Test creating GridLabD instances in separate threads."""
    print("Starting threaded test...\n")
    
    # Create two threads
    thread1 = threading.Thread(target=create_and_use_instance, args=("Thread-1",))
    thread2 = threading.Thread(target=create_and_use_instance, args=("Thread-2",))
    
    # Start threads
    print("Starting Thread-1...")
    thread1.start()
    time.sleep(0.5)  # Small delay before starting second thread
    
    print("Starting Thread-2...\n")
    thread2.start()
    
    # Wait for both threads to complete
    thread1.join()
    thread2.join()
    
    print("\nBoth threads completed!")

if __name__ == "__main__":
    test_threaded_instances()
