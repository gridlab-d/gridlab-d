<!-- default_config.php
     Copyright (C) 2016, Stanford University
     Author: dchassin@slac.stanford.edu
-->
<?php
	
	// gridlabd
	$gridlabd_prefix = "/usr/local";
	$gridlabd_bin = "$gridlabd_prefix/bin";
	$gridlabd_lib = "$gridlabd_prefix/lib/gridlabd";
	$gridlabd_share = "$gridlabd_prefix/share/gridlabd";
	$gridlabd_server = gethostname();
	$gridlabd_port = "6267";
	
	// web server
	$www_modelpath = "model";
	
	// location
	$glm_weather = "CA-Bakersfield.tmy3";
	$glm_timezone = "US/CA/Los Angeles";
	$glm_modelname = "ieee123.glm";
	
	// model features
	$glm_loads = 0;
	$glm_solar = 0;
	$glm_storage = 0;
	$glm_demand = 0;
	$glm_hvac = "none";
	$glm_waterheater = "none";
	$glm_dishwasher = "none";
	$glm_washer = "none";
	$glm_dryer = "none";
	
	// MySQL configuration
	$mysql = 0;
	$my_server = gethostname();
	$my_user = "gridlabd_ro";
	$my_pwd = "gridlabd";
	$my_name = "ieee123";
	$my_scada = 0;
	$my_ami = 0;
	$my_model = 0;
	$my_graph = 0;
?>
