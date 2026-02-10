"""
Created on 02/10/2026

This example tests the ability for users to control when GLD prints messages
to the console. 

https://github.com/gridlab-d/gridlab-d/issues/1557


@author: Trevor Hardy
trevor.hardy@pnnl.gov
"""


import gridlabd
from pathlib import Path

def print_verbose_setting(verbose_flag):
    print(f"Setting verbose to {verbose_flag}")
    if verbose_flag:
       print("GLD console messages will be interleaved with user messages")
    else:
        print("Only user messages will appear on the console.")
        print("GLD messages accessed through `.get_messages()`.")


def run_GLD_model(verbose_flag):
    gld = gridlabd.GridLabD(verbose=verbose_flag)
    print_verbose_setting(verbose_flag)

    # Load model
    model_path = Path("house_with_solar")
    gld.set_working_directory(str(model_path))
    loaded_model = gld.load("houses.glm")
    gld.set_time_step(900)
    print("USER: Model loaded and ready to take first `.step()`")
    gld.step()
    print("USER: Took first `.step()` and ready to do another")
    gld.step()
    print("USER: Took second `.step()` and wrapping up")
    gld.stop()
    gld.exit_gld()

# Verify version
print(f"GLD Python library location: {gridlabd.__file__}")



# Make GLD instances
verbose_flag = False
run_GLD_model(verbose_flag)
verbose_flag = True
run_GLD_model(verbose_flag)
 