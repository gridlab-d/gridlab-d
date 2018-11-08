To create a docker image run the command
~~~
localhost% docker build -f DockerFile -t gridlabd .
~~~
   
If the build succeeds and passes validation, the container will be created.

To save the docker image use the following command
~~~
localhost% docker save gridlabd -o gridlabd
~~~

To run gridlabd after mapping a local folder to a docker folder, use the following command
~~~
localhost% docker run -it -v <localpath>:<dockerpath> gridlabd gridlabd -W <dockerpath> <your-options>
~~~
   
To run a shell instead of gridlabd, use the following command
~~~
localhost% docker run -it -v <localpath>:<dockerpath> gridlabd bash
~~~
   
If you want to use an alias, try this one:
~~~
localhost% alias gridlabd='docker run -it -v $(pwd):/gridlabd gridlabd gridlabd -W /gridlabd'
~~~
