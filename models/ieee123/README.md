# Using the IEEE-123 model

This model is designed to operated either as a real-time simulation or as a normal simulation with a start and stop time. The options are controlled by the file `config/local.glm`, which you may edit manually if you wish, provided you are not using the web-server interface.

If you wish to run in real-time, you should comment out the STARTTIME and STOPTIME definitions in `config/local.glm`. 

If you are saving data to a MySQL data, the you enable MySQL by uncommenting `MYSQL_ENABLE=on`.

To run the simulation, do the following:

~~~
  localhost% cd ieee123
  localhost% gridlabd model/ieee123.glm
~~~

