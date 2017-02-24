name = tracer										# name of the simulator
time_delta = 500ms
broker = tcp://localhost:5570
# list of things this simulator subscribes to
values                      						# list of subscriptions
    GLD1/fncs_output                   						# lookup key
        topic = GLD1/fncs_output			# topic (a regex), name of simulator 
        default = {}      			    # default value
        type = JSON          						# data type (currently unused)
        list = false        						# whether incoming values queue up
