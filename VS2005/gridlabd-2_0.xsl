<?xml version="1.0" encoding="ISO-8859-1"?>
<!-- output by GridLAB-D -->
<html xsl:version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns="http://www.w3.org/1999/xhtml">
<xsl:for-each select="/gridlabd">
<head>
<title>GridLAB-D <xsl:value-of select="version.major"/>.<xsl:value-of select="version.minor"/> - <xsl:value-of select="modelname"/></title>
<link rel="stylesheet" href="http://gridlabd.pnl.gov/source/VS2005/gridlabd-2_0.css" type="text/css"/>
</head>
<body>
<H1><xsl:value-of select="modelname"/></H1>
<H2>Table of Contents</H2>
<OL TYPE="1">
<LI><A HREF="#global_variables">Global variables</A></LI>
<LI><A HREF="#solver_ranks">Solver ranks</A></LI>
<LI><A HREF="#modules">Modules</A></LI><OL TYPE="a">
<LI><A HREF="#modules_climate">climate</A></LI>
<LI><A HREF="#modules_commercial">commercial</A></LI>
<LI><A HREF="#modules_generators">generators</A></LI>
<LI><A HREF="#modules_market">market</A></LI>
<LI><A HREF="#modules_network">network</A></LI>
<LI><A HREF="#modules_plc">plc</A></LI>
<LI><A HREF="#modules_powerflow">powerflow</A></LI>
<LI><A HREF="#modules_residential">residential</A></LI>
<LI><A HREF="#modules_tape">tape</A></LI>
</OL>
<LI><A HREF="#output">Output</A></LI>
</OL>
<H2><A NAME="global_variables">GridLAB-D system variables</A></H2>
<TABLE BORDER="1">
<TR><TH>version.major</TH><TD><xsl:value-of select="version.major"/></TD></TR>
<TR><TH>version.minor</TH><TD><xsl:value-of select="version.minor"/></TD></TR>
<TR><TH>command_line</TH><TD><xsl:value-of select="command_line"/></TD></TR>
<TR><TH>environment</TH><TD><xsl:value-of select="environment"/></TD></TR>
<TR><TH>quiet</TH><TD><xsl:value-of select="quiet"/></TD></TR>
<TR><TH>warn</TH><TD><xsl:value-of select="warn"/></TD></TR>
<TR><TH>debugger</TH><TD><xsl:value-of select="debugger"/></TD></TR>
<TR><TH>gdb</TH><TD><xsl:value-of select="gdb"/></TD></TR>
<TR><TH>debug</TH><TD><xsl:value-of select="debug"/></TD></TR>
<TR><TH>test</TH><TD><xsl:value-of select="test"/></TD></TR>
<TR><TH>verbose</TH><TD><xsl:value-of select="verbose"/></TD></TR>
<TR><TH>iteration_limit</TH><TD><xsl:value-of select="iteration_limit"/></TD></TR>
<TR><TH>workdir</TH><TD><xsl:value-of select="workdir"/></TD></TR>
<TR><TH>dumpfile</TH><TD><xsl:value-of select="dumpfile"/></TD></TR>
<TR><TH>savefile</TH><TD><xsl:value-of select="savefile"/></TD></TR>
<TR><TH>dumpall</TH><TD><xsl:value-of select="dumpall"/></TD></TR>
<TR><TH>runchecks</TH><TD><xsl:value-of select="runchecks"/></TD></TR>
<TR><TH>threadcount</TH><TD><xsl:value-of select="threadcount"/></TD></TR>
<TR><TH>profiler</TH><TD><xsl:value-of select="profiler"/></TD></TR>
<TR><TH>pauseatexit</TH><TD><xsl:value-of select="pauseatexit"/></TD></TR>
<TR><TH>testoutputfile</TH><TD><xsl:value-of select="testoutputfile"/></TD></TR>
<TR><TH>xml_encoding</TH><TD><xsl:value-of select="xml_encoding"/></TD></TR>
<TR><TH>clock</TH><TD><xsl:value-of select="clock"/></TD></TR>
<TR><TH>starttime</TH><TD><xsl:value-of select="starttime"/></TD></TR>
<TR><TH>stoptime</TH><TD><xsl:value-of select="stoptime"/></TD></TR>
<TR><TH>double_format</TH><TD><xsl:value-of select="double_format"/></TD></TR>
<TR><TH>complex_format</TH><TD><xsl:value-of select="complex_format"/></TD></TR>
<TR><TH>object_format</TH><TD><xsl:value-of select="object_format"/></TD></TR>
<TR><TH>object_scan</TH><TD><xsl:value-of select="object_scan"/></TD></TR>
<TR><TH>object_tree_balance</TH><TD><xsl:value-of select="object_tree_balance"/></TD></TR>
<TR><TH>kmlfile</TH><TD><xsl:value-of select="kmlfile"/></TD></TR>
<TR><TH>modelname</TH><TD><xsl:value-of select="modelname"/></TD></TR>
<TR><TH>execdir</TH><TD><xsl:value-of select="execdir"/></TD></TR>
<TR><TH>strictnames</TH><TD><xsl:value-of select="strictnames"/></TD></TR>
<TR><TH>website</TH><TD><xsl:value-of select="website"/></TD></TR>
<TR><TH>urlbase</TH><TD><xsl:value-of select="urlbase"/></TD></TR>
<TR><TH>randomseed</TH><TD><xsl:value-of select="randomseed"/></TD></TR>
<TR><TH>include</TH><TD><xsl:value-of select="include"/></TD></TR>
<TR><TH>trace</TH><TD><xsl:value-of select="trace"/></TD></TR>
<TR><TH>gdb_window</TH><TD><xsl:value-of select="gdb_window"/></TD></TR>
<TR><TH>tmp</TH><TD><xsl:value-of select="tmp"/></TD></TR>
<TR><TH>force_compile</TH><TD><xsl:value-of select="force_compile"/></TD></TR>
<TR><TH>nolocks</TH><TD><xsl:value-of select="nolocks"/></TD></TR>
<TR><TH>skipsafe</TH><TD><xsl:value-of select="skipsafe"/></TD></TR>
</TABLE>
<H2><A NAME="solver_ranks">Solver ranks</A></H2>
<TABLE BORDER="1">
<TR><xsl:for-each select="sync-order/pass">
<TH>Pass <xsl:value-of select="name"/></TH>
</xsl:for-each>
</TR>
<TR>
<xsl:for-each select="sync-order/pass"><TD><DL>
<xsl:for-each select="rank">
<xsl:sort select="ordinal" data-type="number" order="descending"/>
<DT>Rank <xsl:value-of select="ordinal"/></DT><xsl:for-each select="object">
<DD><a href="#{name}"><xsl:value-of select="name"/></a></DD>
</xsl:for-each>
</xsl:for-each>
</DL></TD></xsl:for-each>
</TR>
</TABLE>
<H2><A NAME="modules">Modules</A></H2>
<H3><A NAME="modules_climate">climate</A></H3><TABLE BORDER="1">
<TR><TH>version.major</TH><TD><xsl:value-of select="climate/version.major"/></TD></TR><TR><TH>version.minor</TH><TD><xsl:value-of select="climate/version.minor"/></TD></TR></TABLE>
<H4>climate objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>city</TH><TH>tmyfile</TH><TH>temperature</TH><TH>humidity</TH><TH>solar_flux</TH><TH>wind_speed</TH><TH>wind_dir</TH><TH>wind_gust</TH></TR>
<xsl:for-each select="climate/climate_list/climate"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="city"/></TD><TD><xsl:value-of select="tmyfile"/></TD><TD><xsl:value-of select="temperature"/></TD><TD><xsl:value-of select="humidity"/></TD><TD><xsl:value-of select="solar_flux"/></TD><TD><xsl:value-of select="wind_speed"/></TD><TD><xsl:value-of select="wind_dir"/></TD><TD><xsl:value-of select="wind_gust"/></TD></TR>
</xsl:for-each></TABLE>
<H3><A NAME="modules_commercial">commercial</A></H3><TABLE BORDER="1">
<TR><TH>version.major</TH><TD><xsl:value-of select="commercial/version.major"/></TD></TR><TR><TH>version.minor</TH><TD><xsl:value-of select="commercial/version.minor"/></TD></TR></TABLE>
<H4>office objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>occupancy</TH><TH>floor_height</TH><TH>exterior_ua</TH><TH>interior_ua</TH><TH>interior_mass</TH><TH>floor_area</TH><TH>glazing</TH><TH>glazing.north</TH><TH>glazing.northeast</TH><TH>glazing.east</TH><TH>glazing.southeast</TH><TH>glazing.south</TH><TH>glazing.southwest</TH><TH>glazing.west</TH><TH>glazing.northwest</TH><TH>glazing.horizontal</TH><TH>glazing.coefficient</TH><TH>occupants</TH><TH>schedule</TH><TH>air_temperature</TH><TH>mass_temperature</TH><TH>temperature_change</TH><TH>Qh</TH><TH>Qs</TH><TH>Qi</TH><TH>Qz</TH><TH>hvac.mode</TH><TH>hvac.cooling.balance_temperature</TH><TH>hvac.cooling.capacity</TH><TH>hvac.cooling.capacity_perF</TH><TH>hvac.cooling.design_temperature</TH><TH>hvac.cooling.efficiency</TH><TH>hvac.cooling.cop</TH><TH>hvac.heating.balance_temperature</TH><TH>hvac.heating.capacity</TH><TH>hvac.heating.capacity_perF</TH><TH>hvac.heating.design_temperature</TH><TH>hvac.heating.efficiency</TH><TH>hvac.heating.cop</TH><TH>lights.capacity</TH><TH>lights.fraction</TH><TH>plugs.capacity</TH><TH>plugs.fraction</TH><TH>demand</TH><TH>power</TH><TH>energy</TH><TH>power_factor</TH><TH>hvac.demand</TH><TH>hvac.power</TH><TH>hvac.energy</TH><TH>hvac.power_factor</TH><TH>lights.demand</TH><TH>lights.power</TH><TH>lights.energy</TH><TH>lights.power_factor</TH><TH>plugs.demand</TH><TH>plugs.power</TH><TH>plugs.energy</TH><TH>plugs.power_factor</TH><TH>control.cooling_setpoint</TH><TH>control.heating_setpoint</TH><TH>control.setpoint_deadband</TH><TH>control.ventilation_fraction</TH><TH>control.lighting_fraction</TH></TR>
<xsl:for-each select="commercial/office_list/office"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="occupancy"/></TD><TD><xsl:value-of select="floor_height"/></TD><TD><xsl:value-of select="exterior_ua"/></TD><TD><xsl:value-of select="interior_ua"/></TD><TD><xsl:value-of select="interior_mass"/></TD><TD><xsl:value-of select="floor_area"/></TD><TD><xsl:value-of select="glazing"/></TD><TD><xsl:value-of select="glazing.north"/></TD><TD><xsl:value-of select="glazing.northeast"/></TD><TD><xsl:value-of select="glazing.east"/></TD><TD><xsl:value-of select="glazing.southeast"/></TD><TD><xsl:value-of select="glazing.south"/></TD><TD><xsl:value-of select="glazing.southwest"/></TD><TD><xsl:value-of select="glazing.west"/></TD><TD><xsl:value-of select="glazing.northwest"/></TD><TD><xsl:value-of select="glazing.horizontal"/></TD><TD><xsl:value-of select="glazing.coefficient"/></TD><TD><xsl:value-of select="occupants"/></TD><TD><xsl:value-of select="schedule"/></TD><TD><xsl:value-of select="air_temperature"/></TD><TD><xsl:value-of select="mass_temperature"/></TD><TD><xsl:value-of select="temperature_change"/></TD><TD><xsl:value-of select="Qh"/></TD><TD><xsl:value-of select="Qs"/></TD><TD><xsl:value-of select="Qi"/></TD><TD><xsl:value-of select="Qz"/></TD><TD><xsl:value-of select="hvac.mode"/></TD><TD><xsl:value-of select="hvac.cooling.balance_temperature"/></TD><TD><xsl:value-of select="hvac.cooling.capacity"/></TD><TD><xsl:value-of select="hvac.cooling.capacity_perF"/></TD><TD><xsl:value-of select="hvac.cooling.design_temperature"/></TD><TD><xsl:value-of select="hvac.cooling.efficiency"/></TD><TD><xsl:value-of select="hvac.cooling.cop"/></TD><TD><xsl:value-of select="hvac.heating.balance_temperature"/></TD><TD><xsl:value-of select="hvac.heating.capacity"/></TD><TD><xsl:value-of select="hvac.heating.capacity_perF"/></TD><TD><xsl:value-of select="hvac.heating.design_temperature"/></TD><TD><xsl:value-of select="hvac.heating.efficiency"/></TD><TD><xsl:value-of select="hvac.heating.cop"/></TD><TD><xsl:value-of select="lights.capacity"/></TD><TD><xsl:value-of select="lights.fraction"/></TD><TD><xsl:value-of select="plugs.capacity"/></TD><TD><xsl:value-of select="plugs.fraction"/></TD><TD><xsl:value-of select="demand"/></TD><TD><xsl:value-of select="power"/></TD><TD><xsl:value-of select="energy"/></TD><TD><xsl:value-of select="power_factor"/></TD><TD><xsl:value-of select="hvac.demand"/></TD><TD><xsl:value-of select="hvac.power"/></TD><TD><xsl:value-of select="hvac.energy"/></TD><TD><xsl:value-of select="hvac.power_factor"/></TD><TD><xsl:value-of select="lights.demand"/></TD><TD><xsl:value-of select="lights.power"/></TD><TD><xsl:value-of select="lights.energy"/></TD><TD><xsl:value-of select="lights.power_factor"/></TD><TD><xsl:value-of select="plugs.demand"/></TD><TD><xsl:value-of select="plugs.power"/></TD><TD><xsl:value-of select="plugs.energy"/></TD><TD><xsl:value-of select="plugs.power_factor"/></TD><TD><xsl:value-of select="control.cooling_setpoint"/></TD><TD><xsl:value-of select="control.heating_setpoint"/></TD><TD><xsl:value-of select="control.setpoint_deadband"/></TD><TD><xsl:value-of select="control.ventilation_fraction"/></TD><TD><xsl:value-of select="control.lighting_fraction"/></TD></TR>
</xsl:for-each></TABLE>
<H4>multizone objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>from</TH><TH>to</TH><TH>ua</TH></TR>
<xsl:for-each select="commercial/multizone_list/multizone"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><a href="#{from}"><xsl:value-of select="from"/></a></TD><TD><a href="#{to}"><xsl:value-of select="to"/></a></TD><TD><xsl:value-of select="ua"/></TD></TR>
</xsl:for-each></TABLE>
<H3><A NAME="modules_generators">generators</A></H3><TABLE BORDER="1">
<TR><TH>version.major</TH><TD><xsl:value-of select="generators/version.major"/></TD></TR><TR><TH>version.minor</TH><TD><xsl:value-of select="generators/version.minor"/></TD></TR></TABLE>
<H4>diesel_dg objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>Gen_mode</TH><TH>Gen_status</TH><TH>Rated_kVA</TH><TH>Rated_kV</TH><TH>Rs</TH><TH>Xs</TH><TH>Rg</TH><TH>Xg</TH><TH>voltage_A</TH><TH>voltage_B</TH><TH>voltage_C</TH><TH>current_A</TH><TH>current_B</TH><TH>current_C</TH><TH>EfA</TH><TH>EfB</TH><TH>EfC</TH><TH>power_A</TH><TH>power_B</TH><TH>power_C</TH><TH>power_A_sch</TH><TH>power_B_sch</TH><TH>power_C_sch</TH><TH>EfA_sch</TH><TH>EfB_sch</TH><TH>EfC_sch</TH><TH>SlackBus</TH><TH>phases</TH></TR>
<xsl:for-each select="generators/diesel_dg_list/diesel_dg"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="Gen_mode"/></TD><TD><xsl:value-of select="Gen_status"/></TD><TD><xsl:value-of select="Rated_kVA"/></TD><TD><xsl:value-of select="Rated_kV"/></TD><TD><xsl:value-of select="Rs"/></TD><TD><xsl:value-of select="Xs"/></TD><TD><xsl:value-of select="Rg"/></TD><TD><xsl:value-of select="Xg"/></TD><TD><xsl:value-of select="voltage_A"/></TD><TD><xsl:value-of select="voltage_B"/></TD><TD><xsl:value-of select="voltage_C"/></TD><TD><xsl:value-of select="current_A"/></TD><TD><xsl:value-of select="current_B"/></TD><TD><xsl:value-of select="current_C"/></TD><TD><xsl:value-of select="EfA"/></TD><TD><xsl:value-of select="EfB"/></TD><TD><xsl:value-of select="EfC"/></TD><TD><xsl:value-of select="power_A"/></TD><TD><xsl:value-of select="power_B"/></TD><TD><xsl:value-of select="power_C"/></TD><TD><xsl:value-of select="power_A_sch"/></TD><TD><xsl:value-of select="power_B_sch"/></TD><TD><xsl:value-of select="power_C_sch"/></TD><TD><xsl:value-of select="EfA_sch"/></TD><TD><xsl:value-of select="EfB_sch"/></TD><TD><xsl:value-of select="EfC_sch"/></TD><TD><xsl:value-of select="SlackBus"/></TD><TD><xsl:value-of select="phases"/></TD></TR>
</xsl:for-each></TABLE>
<H4>windturb_dg objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>Gen_status</TH><TH>Gen_type</TH><TH>Gen_mode</TH><TH>Turbine_Model</TH><TH>Rated_VA</TH><TH>Rated_V</TH><TH>Pconv</TH><TH>WSadj</TH><TH>Wind_Speed</TH><TH>pf</TH><TH>GenElecEff</TH><TH>TotalRealPow</TH><TH>TotalReacPow</TH><TH>voltage_A</TH><TH>voltage_B</TH><TH>voltage_C</TH><TH>current_A</TH><TH>current_B</TH><TH>current_C</TH><TH>EfA</TH><TH>EfB</TH><TH>EfC</TH><TH>Vrotor_A</TH><TH>Vrotor_B</TH><TH>Vrotor_C</TH><TH>Irotor_A</TH><TH>Irotor_B</TH><TH>Irotor_C</TH><TH>phases</TH></TR>
<xsl:for-each select="generators/windturb_dg_list/windturb_dg"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="Gen_status"/></TD><TD><xsl:value-of select="Gen_type"/></TD><TD><xsl:value-of select="Gen_mode"/></TD><TD><xsl:value-of select="Turbine_Model"/></TD><TD><xsl:value-of select="Rated_VA"/></TD><TD><xsl:value-of select="Rated_V"/></TD><TD><xsl:value-of select="Pconv"/></TD><TD><xsl:value-of select="WSadj"/></TD><TD><xsl:value-of select="Wind_Speed"/></TD><TD><xsl:value-of select="pf"/></TD><TD><xsl:value-of select="GenElecEff"/></TD><TD><xsl:value-of select="TotalRealPow"/></TD><TD><xsl:value-of select="TotalReacPow"/></TD><TD><xsl:value-of select="voltage_A"/></TD><TD><xsl:value-of select="voltage_B"/></TD><TD><xsl:value-of select="voltage_C"/></TD><TD><xsl:value-of select="current_A"/></TD><TD><xsl:value-of select="current_B"/></TD><TD><xsl:value-of select="current_C"/></TD><TD><xsl:value-of select="EfA"/></TD><TD><xsl:value-of select="EfB"/></TD><TD><xsl:value-of select="EfC"/></TD><TD><xsl:value-of select="Vrotor_A"/></TD><TD><xsl:value-of select="Vrotor_B"/></TD><TD><xsl:value-of select="Vrotor_C"/></TD><TD><xsl:value-of select="Irotor_A"/></TD><TD><xsl:value-of select="Irotor_B"/></TD><TD><xsl:value-of select="Irotor_C"/></TD><TD><xsl:value-of select="phases"/></TD></TR>
</xsl:for-each></TABLE>
<H4>power_electronics objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>GENERATOR_MODE</TH><TH>GENERATOR_STATUS</TH><TH>CONVERTER_TYPE</TH><TH>SWITCH_TYPE</TH><TH>FILTER_TYPE</TH><TH>FILTER_IMPLEMENTATION</TH><TH>FILTER_FREQUENCY</TH><TH>POWER_TYPE</TH><TH>Rated_kW</TH><TH>Max_P</TH><TH>Min_P</TH><TH>Rated_kVA</TH><TH>Rated_kV</TH><TH>phases</TH></TR>
<xsl:for-each select="generators/power_electronics_list/power_electronics"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="GENERATOR_MODE"/></TD><TD><xsl:value-of select="GENERATOR_STATUS"/></TD><TD><xsl:value-of select="CONVERTER_TYPE"/></TD><TD><xsl:value-of select="SWITCH_TYPE"/></TD><TD><xsl:value-of select="FILTER_TYPE"/></TD><TD><xsl:value-of select="FILTER_IMPLEMENTATION"/></TD><TD><xsl:value-of select="FILTER_FREQUENCY"/></TD><TD><xsl:value-of select="POWER_TYPE"/></TD><TD><xsl:value-of select="Rated_kW"/></TD><TD><xsl:value-of select="Max_P"/></TD><TD><xsl:value-of select="Min_P"/></TD><TD><xsl:value-of select="Rated_kVA"/></TD><TD><xsl:value-of select="Rated_kV"/></TD><TD><xsl:value-of select="phases"/></TD></TR>
</xsl:for-each></TABLE>
<H4>energy_storage objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>GENERATOR_MODE</TH><TH>GENERATOR_STATUS</TH><TH>POWER_TYPE</TH><TH>Rinternal</TH><TH>V_Max</TH><TH>I_Max</TH><TH>E_Max</TH><TH>Energy</TH><TH>efficiency</TH><TH>Rated_kVA</TH><TH>V_Out</TH><TH>I_Out</TH><TH>VA_Out</TH><TH>V_In</TH><TH>I_In</TH><TH>V_Internal</TH><TH>I_Internal</TH><TH>I_Prev</TH><TH>phases</TH></TR>
<xsl:for-each select="generators/energy_storage_list/energy_storage"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="GENERATOR_MODE"/></TD><TD><xsl:value-of select="GENERATOR_STATUS"/></TD><TD><xsl:value-of select="POWER_TYPE"/></TD><TD><xsl:value-of select="Rinternal"/></TD><TD><xsl:value-of select="V_Max"/></TD><TD><xsl:value-of select="I_Max"/></TD><TD><xsl:value-of select="E_Max"/></TD><TD><xsl:value-of select="Energy"/></TD><TD><xsl:value-of select="efficiency"/></TD><TD><xsl:value-of select="Rated_kVA"/></TD><TD><xsl:value-of select="V_Out"/></TD><TD><xsl:value-of select="I_Out"/></TD><TD><xsl:value-of select="VA_Out"/></TD><TD><xsl:value-of select="V_In"/></TD><TD><xsl:value-of select="I_In"/></TD><TD><xsl:value-of select="V_Internal"/></TD><TD><xsl:value-of select="I_Internal"/></TD><TD><xsl:value-of select="I_Prev"/></TD><TD><xsl:value-of select="phases"/></TD></TR>
</xsl:for-each></TABLE>
<H4>inverter objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>INVERTER_TYPE</TH><TH>GENERATOR_MODE</TH><TH>V_In</TH><TH>I_In</TH><TH>VA_In</TH><TH>Vdc</TH><TH>phaseA_V_Out</TH><TH>phaseB_V_Out</TH><TH>phaseC_V_Out</TH><TH>phaseA_I_Out</TH><TH>phaseB_I_Out</TH><TH>phaseC_I_Out</TH><TH>power_A</TH><TH>power_B</TH><TH>power_C</TH><TH>phases</TH></TR>
<xsl:for-each select="generators/inverter_list/inverter"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="INVERTER_TYPE"/></TD><TD><xsl:value-of select="GENERATOR_MODE"/></TD><TD><xsl:value-of select="V_In"/></TD><TD><xsl:value-of select="I_In"/></TD><TD><xsl:value-of select="VA_In"/></TD><TD><xsl:value-of select="Vdc"/></TD><TD><xsl:value-of select="phaseA_V_Out"/></TD><TD><xsl:value-of select="phaseB_V_Out"/></TD><TD><xsl:value-of select="phaseC_V_Out"/></TD><TD><xsl:value-of select="phaseA_I_Out"/></TD><TD><xsl:value-of select="phaseB_I_Out"/></TD><TD><xsl:value-of select="phaseC_I_Out"/></TD><TD><xsl:value-of select="power_A"/></TD><TD><xsl:value-of select="power_B"/></TD><TD><xsl:value-of select="power_C"/></TD><TD><xsl:value-of select="phases"/></TD></TR>
</xsl:for-each></TABLE>
<H4>dc_dc_converter objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>DC_DC_CONVERTER_TYPE</TH><TH>GENERATOR_MODE</TH><TH>V_Out</TH><TH>I_Out</TH><TH>Vdc</TH><TH>VA_Out</TH><TH>P_Out</TH><TH>Q_Out</TH><TH>service_ratio</TH><TH>V_In</TH><TH>I_In</TH><TH>VA_In</TH><TH>phases</TH></TR>
<xsl:for-each select="generators/dc_dc_converter_list/dc_dc_converter"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="DC_DC_CONVERTER_TYPE"/></TD><TD><xsl:value-of select="GENERATOR_MODE"/></TD><TD><xsl:value-of select="V_Out"/></TD><TD><xsl:value-of select="I_Out"/></TD><TD><xsl:value-of select="Vdc"/></TD><TD><xsl:value-of select="VA_Out"/></TD><TD><xsl:value-of select="P_Out"/></TD><TD><xsl:value-of select="Q_Out"/></TD><TD><xsl:value-of select="service_ratio"/></TD><TD><xsl:value-of select="V_In"/></TD><TD><xsl:value-of select="I_In"/></TD><TD><xsl:value-of select="VA_In"/></TD><TD><xsl:value-of select="phases"/></TD></TR>
</xsl:for-each></TABLE>
<H4>rectifier objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>RECTIFIER_TYPE</TH><TH>GENERATOR_MODE</TH><TH>V_Out</TH><TH>I_Out</TH><TH>VA_Out</TH><TH>P_Out</TH><TH>Q_Out</TH><TH>Vdc</TH><TH>phaseA_V_In</TH><TH>phaseB_V_In</TH><TH>phaseC_V_In</TH><TH>phaseA_I_In</TH><TH>phaseB_I_In</TH><TH>phaseC_I_In</TH><TH>power_A_In</TH><TH>power_B_In</TH><TH>power_C_In</TH><TH>phases</TH></TR>
<xsl:for-each select="generators/rectifier_list/rectifier"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="RECTIFIER_TYPE"/></TD><TD><xsl:value-of select="GENERATOR_MODE"/></TD><TD><xsl:value-of select="V_Out"/></TD><TD><xsl:value-of select="I_Out"/></TD><TD><xsl:value-of select="VA_Out"/></TD><TD><xsl:value-of select="P_Out"/></TD><TD><xsl:value-of select="Q_Out"/></TD><TD><xsl:value-of select="Vdc"/></TD><TD><xsl:value-of select="phaseA_V_In"/></TD><TD><xsl:value-of select="phaseB_V_In"/></TD><TD><xsl:value-of select="phaseC_V_In"/></TD><TD><xsl:value-of select="phaseA_I_In"/></TD><TD><xsl:value-of select="phaseB_I_In"/></TD><TD><xsl:value-of select="phaseC_I_In"/></TD><TD><xsl:value-of select="power_A_In"/></TD><TD><xsl:value-of select="power_B_In"/></TD><TD><xsl:value-of select="power_C_In"/></TD><TD><xsl:value-of select="phases"/></TD></TR>
</xsl:for-each></TABLE>
<H4>microturbine objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>GENERATOR_MODE</TH><TH>GENERATOR_STATUS</TH><TH>POWER_TYPE</TH><TH>Rinternal</TH><TH>Rload</TH><TH>V_Max</TH><TH>I_Max</TH><TH>frequency</TH><TH>Max_Frequency</TH><TH>Min_Frequency</TH><TH>Fuel_Used</TH><TH>Heat_Out</TH><TH>KV</TH><TH>Power_Angle</TH><TH>Max_P</TH><TH>Min_P</TH><TH>phaseA_V_Out</TH><TH>phaseB_V_Out</TH><TH>phaseC_V_Out</TH><TH>phaseA_I_Out</TH><TH>phaseB_I_Out</TH><TH>phaseC_I_Out</TH><TH>power_A_Out</TH><TH>power_B_Out</TH><TH>power_C_Out</TH><TH>VA_Out</TH><TH>pf_Out</TH><TH>E_A_Internal</TH><TH>E_B_Internal</TH><TH>E_C_Internal</TH><TH>efficiency</TH><TH>Rated_kVA</TH><TH>VA_Out</TH><TH>phases</TH></TR>
<xsl:for-each select="generators/microturbine_list/microturbine"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="GENERATOR_MODE"/></TD><TD><xsl:value-of select="GENERATOR_STATUS"/></TD><TD><xsl:value-of select="POWER_TYPE"/></TD><TD><xsl:value-of select="Rinternal"/></TD><TD><xsl:value-of select="Rload"/></TD><TD><xsl:value-of select="V_Max"/></TD><TD><xsl:value-of select="I_Max"/></TD><TD><xsl:value-of select="frequency"/></TD><TD><xsl:value-of select="Max_Frequency"/></TD><TD><xsl:value-of select="Min_Frequency"/></TD><TD><xsl:value-of select="Fuel_Used"/></TD><TD><xsl:value-of select="Heat_Out"/></TD><TD><xsl:value-of select="KV"/></TD><TD><xsl:value-of select="Power_Angle"/></TD><TD><xsl:value-of select="Max_P"/></TD><TD><xsl:value-of select="Min_P"/></TD><TD><xsl:value-of select="phaseA_V_Out"/></TD><TD><xsl:value-of select="phaseB_V_Out"/></TD><TD><xsl:value-of select="phaseC_V_Out"/></TD><TD><xsl:value-of select="phaseA_I_Out"/></TD><TD><xsl:value-of select="phaseB_I_Out"/></TD><TD><xsl:value-of select="phaseC_I_Out"/></TD><TD><xsl:value-of select="power_A_Out"/></TD><TD><xsl:value-of select="power_B_Out"/></TD><TD><xsl:value-of select="power_C_Out"/></TD><TD><xsl:value-of select="VA_Out"/></TD><TD><xsl:value-of select="pf_Out"/></TD><TD><xsl:value-of select="E_A_Internal"/></TD><TD><xsl:value-of select="E_B_Internal"/></TD><TD><xsl:value-of select="E_C_Internal"/></TD><TD><xsl:value-of select="efficiency"/></TD><TD><xsl:value-of select="Rated_kVA"/></TD><TD><xsl:value-of select="VA_Out"/></TD><TD><xsl:value-of select="phases"/></TD></TR>
</xsl:for-each></TABLE>
<H4>battery objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>GENERATOR_MODE</TH><TH>GENERATOR_STATUS</TH><TH>RFB_SIZE</TH><TH>POWER_TYPE</TH><TH>Rinternal</TH><TH>V_Max</TH><TH>I_Max</TH><TH>E_Max</TH><TH>Energy</TH><TH>efficiency</TH><TH>base_efficiency</TH><TH>Rated_kVA</TH><TH>V_Out</TH><TH>I_Out</TH><TH>VA_Out</TH><TH>V_In</TH><TH>I_In</TH><TH>V_Internal</TH><TH>I_Internal</TH><TH>I_Prev</TH><TH>phases</TH></TR>
<xsl:for-each select="generators/battery_list/battery"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="GENERATOR_MODE"/></TD><TD><xsl:value-of select="GENERATOR_STATUS"/></TD><TD><xsl:value-of select="RFB_SIZE"/></TD><TD><xsl:value-of select="POWER_TYPE"/></TD><TD><xsl:value-of select="Rinternal"/></TD><TD><xsl:value-of select="V_Max"/></TD><TD><xsl:value-of select="I_Max"/></TD><TD><xsl:value-of select="E_Max"/></TD><TD><xsl:value-of select="Energy"/></TD><TD><xsl:value-of select="efficiency"/></TD><TD><xsl:value-of select="base_efficiency"/></TD><TD><xsl:value-of select="Rated_kVA"/></TD><TD><xsl:value-of select="V_Out"/></TD><TD><xsl:value-of select="I_Out"/></TD><TD><xsl:value-of select="VA_Out"/></TD><TD><xsl:value-of select="V_In"/></TD><TD><xsl:value-of select="I_In"/></TD><TD><xsl:value-of select="V_Internal"/></TD><TD><xsl:value-of select="I_Internal"/></TD><TD><xsl:value-of select="I_Prev"/></TD><TD><xsl:value-of select="phases"/></TD></TR>
</xsl:for-each></TABLE>
<H4>solar objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>GENERATOR_MODE</TH><TH>GENERATOR_STATUS</TH><TH>PANEL_TYPE</TH><TH>POWER_TYPE</TH><TH>NOCT</TH><TH>Tcell</TH><TH>Tambient</TH><TH>Insolation</TH><TH>Rinternal</TH><TH>Rated_Insolation</TH><TH>V_Max</TH><TH>Voc_Max</TH><TH>Voc</TH><TH>efficiency</TH><TH>area</TH><TH>Rated_kVA</TH><TH>V_Out</TH><TH>I_Out</TH><TH>VA_Out</TH><TH>phases</TH></TR>
<xsl:for-each select="generators/solar_list/solar"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="GENERATOR_MODE"/></TD><TD><xsl:value-of select="GENERATOR_STATUS"/></TD><TD><xsl:value-of select="PANEL_TYPE"/></TD><TD><xsl:value-of select="POWER_TYPE"/></TD><TD><xsl:value-of select="NOCT"/></TD><TD><xsl:value-of select="Tcell"/></TD><TD><xsl:value-of select="Tambient"/></TD><TD><xsl:value-of select="Insolation"/></TD><TD><xsl:value-of select="Rinternal"/></TD><TD><xsl:value-of select="Rated_Insolation"/></TD><TD><xsl:value-of select="V_Max"/></TD><TD><xsl:value-of select="Voc_Max"/></TD><TD><xsl:value-of select="Voc"/></TD><TD><xsl:value-of select="efficiency"/></TD><TD><xsl:value-of select="area"/></TD><TD><xsl:value-of select="Rated_kVA"/></TD><TD><xsl:value-of select="V_Out"/></TD><TD><xsl:value-of select="I_Out"/></TD><TD><xsl:value-of select="VA_Out"/></TD><TD><xsl:value-of select="phases"/></TD></TR>
</xsl:for-each></TABLE>
<H3><A NAME="modules_market">market</A></H3><TABLE BORDER="1">
<TR><TH>version.major</TH><TD><xsl:value-of select="market/version.major"/></TD></TR><TR><TH>version.minor</TH><TD><xsl:value-of select="market/version.minor"/></TD></TR></TABLE>
<H4>auction objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>type</TH><TH>unit</TH><TH>period</TH><TH>latency</TH><TH>market_id</TH><TH>last.Q</TH><TH>last.P</TH><TH>next.Q</TH><TH>next.P</TH><TH>network</TH></TR>
<xsl:for-each select="market/auction_list/auction"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="type"/></TD><TD><xsl:value-of select="unit"/></TD><TD><xsl:value-of select="period"/></TD><TD><xsl:value-of select="latency"/></TD><TD><xsl:value-of select="market_id"/></TD><TD><xsl:value-of select="last.Q"/></TD><TD><xsl:value-of select="last.P"/></TD><TD><xsl:value-of select="next.Q"/></TD><TD><xsl:value-of select="next.P"/></TD><TD><a href="#{network}"><xsl:value-of select="network"/></a></TD></TR>
</xsl:for-each></TABLE>
<H3><A NAME="modules_network">network</A></H3><TABLE BORDER="1">
<TR><TH>version.major</TH><TD><xsl:value-of select="network/version.major"/></TD></TR><TR><TH>version.minor</TH><TD><xsl:value-of select="network/version.minor"/></TD></TR><TR><TH>acceleration_factor</TH><TD><xsl:value-of select="network/acceleration_factor"/></TD></TR><TR><TH>convergence_limit</TH><TD><xsl:value-of select="network/convergence_limit"/></TD></TR><TR><TH>mvabase</TH><TD><xsl:value-of select="network/mvabase"/></TD></TR><TR><TH>kvbase</TH><TD><xsl:value-of select="network/kvbase"/></TD></TR><TR><TH>model_year</TH><TD><xsl:value-of select="network/model_year"/></TD></TR><TR><TH>model_case</TH><TD><xsl:value-of select="network/model_case"/></TD></TR><TR><TH>model_name</TH><TD><xsl:value-of select="network/model_name"/></TD></TR></TABLE>
<H4>node objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>V</TH><TH>S</TH><TH>G</TH><TH>B</TH><TH>Qmax_MVAR</TH><TH>Qmin_MVAR</TH><TH>type</TH><TH>flow_area_num</TH><TH>base_kV</TH></TR>
<xsl:for-each select="network/node_list/node"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="V"/></TD><TD><xsl:value-of select="S"/></TD><TD><xsl:value-of select="G"/></TD><TD><xsl:value-of select="B"/></TD><TD><xsl:value-of select="Qmax_MVAR"/></TD><TD><xsl:value-of select="Qmin_MVAR"/></TD><TD><xsl:value-of select="type"/></TD><TD><xsl:value-of select="flow_area_num"/></TD><TD><xsl:value-of select="base_kV"/></TD></TR>
</xsl:for-each></TABLE>
<H4>link objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>Y</TH><TH>I</TH><TH>B</TH><TH>from</TH><TH>to</TH></TR>
<xsl:for-each select="network/link_list/link"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="Y"/></TD><TD><xsl:value-of select="I"/></TD><TD><xsl:value-of select="B"/></TD><TD><a href="#{from}"><xsl:value-of select="from"/></a></TD><TD><a href="#{to}"><xsl:value-of select="to"/></a></TD></TR>
</xsl:for-each></TABLE>
<H4>capbank objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>KVARrated</TH><TH>Vrated</TH><TH>state</TH><TH>CTlink</TH><TH>PTnode</TH><TH>VARopen</TH><TH>VARclose</TH><TH>Vopen</TH><TH>Vclose</TH></TR>
<xsl:for-each select="network/capbank_list/capbank"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="KVARrated"/></TD><TD><xsl:value-of select="Vrated"/></TD><TD><xsl:value-of select="state"/></TD><TD><a href="#{CTlink}"><xsl:value-of select="CTlink"/></a></TD><TD><a href="#{PTnode}"><xsl:value-of select="PTnode"/></a></TD><TD><xsl:value-of select="VARopen"/></TD><TD><xsl:value-of select="VARclose"/></TD><TD><xsl:value-of select="Vopen"/></TD><TD><xsl:value-of select="Vclose"/></TD></TR>
</xsl:for-each></TABLE>
<H4>fuse objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>TimeConstant</TH><TH>SetCurrent</TH><TH>SetBase</TH><TH>SetScale</TH><TH>SetCurve</TH><TH>TresetAvg</TH><TH>TresetStd</TH><TH>State</TH></TR>
<xsl:for-each select="network/fuse_list/fuse"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="TimeConstant"/></TD><TD><xsl:value-of select="SetCurrent"/></TD><TD><xsl:value-of select="SetBase"/></TD><TD><xsl:value-of select="SetScale"/></TD><TD><xsl:value-of select="SetCurve"/></TD><TD><xsl:value-of select="TresetAvg"/></TD><TD><xsl:value-of select="TresetStd"/></TD><TD><xsl:value-of select="State"/></TD></TR>
</xsl:for-each></TABLE>
<H4>relay objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>Curve</TH><TH>TimeDial</TH><TH>SetCurrent</TH><TH>State</TH></TR>
<xsl:for-each select="network/relay_list/relay"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="Curve"/></TD><TD><xsl:value-of select="TimeDial"/></TD><TD><xsl:value-of select="SetCurrent"/></TD><TD><xsl:value-of select="State"/></TD></TR>
</xsl:for-each></TABLE>
<H4>regulator objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>Type</TH><TH>Vmax</TH><TH>Vmin</TH><TH>Vstep</TH><TH>CTlink</TH><TH>PTbus</TH><TH>TimeDelay</TH></TR>
<xsl:for-each select="network/regulator_list/regulator"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="Type"/></TD><TD><xsl:value-of select="Vmax"/></TD><TD><xsl:value-of select="Vmin"/></TD><TD><xsl:value-of select="Vstep"/></TD><TD><a href="#{CTlink}"><xsl:value-of select="CTlink"/></a></TD><TD><a href="#{PTbus}"><xsl:value-of select="PTbus"/></a></TD><TD><xsl:value-of select="TimeDelay"/></TD></TR>
</xsl:for-each></TABLE>
<H4>transformer objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>Type</TH><TH>Sbase</TH><TH>Vbase</TH><TH>Zpu</TH><TH>Vprimary</TH><TH>Vsecondary</TH></TR>
<xsl:for-each select="network/transformer_list/transformer"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="Type"/></TD><TD><xsl:value-of select="Sbase"/></TD><TD><xsl:value-of select="Vbase"/></TD><TD><xsl:value-of select="Zpu"/></TD><TD><xsl:value-of select="Vprimary"/></TD><TD><xsl:value-of select="Vsecondary"/></TD></TR>
</xsl:for-each></TABLE>
<H4>meter objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>type</TH><TH>demand</TH><TH>meter</TH><TH>line1_current</TH><TH>line2_current</TH><TH>line3_current</TH><TH>line1_admittance</TH><TH>line2_admittance</TH><TH>line3_admittance</TH><TH>line1_power</TH><TH>line2_power</TH><TH>line3_power</TH><TH>line1_volts</TH><TH>line2_volts</TH><TH>line3_volts</TH></TR>
<xsl:for-each select="network/meter_list/meter"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="type"/></TD><TD><xsl:value-of select="demand"/></TD><TD><xsl:value-of select="meter"/></TD><TD><xsl:value-of select="line1_current"/></TD><TD><xsl:value-of select="line2_current"/></TD><TD><xsl:value-of select="line3_current"/></TD><TD><xsl:value-of select="line1_admittance"/></TD><TD><xsl:value-of select="line2_admittance"/></TD><TD><xsl:value-of select="line3_admittance"/></TD><TD><xsl:value-of select="line1_power"/></TD><TD><xsl:value-of select="line2_power"/></TD><TD><xsl:value-of select="line3_power"/></TD><TD><xsl:value-of select="line1_volts"/></TD><TD><xsl:value-of select="line2_volts"/></TD><TD><xsl:value-of select="line3_volts"/></TD></TR>
</xsl:for-each></TABLE>
<H4>generator objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>Pdesired_MW</TH><TH>Qdesired_MVAR</TH><TH>Qcontrolled</TH><TH>Pmax_MW</TH><TH>Qmin_MVAR</TH><TH>Qmax_MVAR</TH><TH>QVa</TH><TH>QVb</TH><TH>QVc</TH><TH>state</TH></TR>
<xsl:for-each select="network/generator_list/generator"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="Pdesired_MW"/></TD><TD><xsl:value-of select="Qdesired_MVAR"/></TD><TD><xsl:value-of select="Qcontrolled"/></TD><TD><xsl:value-of select="Pmax_MW"/></TD><TD><xsl:value-of select="Qmin_MVAR"/></TD><TD><xsl:value-of select="Qmax_MVAR"/></TD><TD><xsl:value-of select="QVa"/></TD><TD><xsl:value-of select="QVb"/></TD><TD><xsl:value-of select="QVc"/></TD><TD><xsl:value-of select="state"/></TD></TR>
</xsl:for-each></TABLE>
<H3><A NAME="modules_plc">plc</A></H3><TABLE BORDER="1">
<TR><TH>version.major</TH><TD><xsl:value-of select="plc/version.major"/></TD></TR><TR><TH>version.minor</TH><TD><xsl:value-of select="plc/version.minor"/></TD></TR><TR><TH>libpath</TH><TD><xsl:value-of select="plc/libpath"/></TD></TR><TR><TH>incpath</TH><TD><xsl:value-of select="plc/incpath"/></TD></TR></TABLE>
<H4>plc objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>source</TH><TH>network</TH></TR>
<xsl:for-each select="plc/plc_list/plc"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="source"/></TD><TD><a href="#{network}"><xsl:value-of select="network"/></a></TD></TR>
</xsl:for-each></TABLE>
<H4>comm objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>latency</TH><TH>reliability</TH><TH>bitrate</TH><TH>timeout</TH></TR>
<xsl:for-each select="plc/comm_list/comm"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="latency"/></TD><TD><xsl:value-of select="reliability"/></TD><TD><xsl:value-of select="bitrate"/></TD><TD><xsl:value-of select="timeout"/></TD></TR>
</xsl:for-each></TABLE>
<H3><A NAME="modules_powerflow">powerflow</A></H3><TABLE BORDER="1">
<TR><TH>version.major</TH><TD><xsl:value-of select="powerflow/version.major"/></TD></TR><TR><TH>version.minor</TH><TD><xsl:value-of select="powerflow/version.minor"/></TD></TR><TR><TH>show_matrix_values</TH><TD><xsl:value-of select="powerflow/show_matrix_values"/></TD></TR><TR><TH>primary_voltage_ratio</TH><TD><xsl:value-of select="powerflow/primary_voltage_ratio"/></TD></TR><TR><TH>nominal_frequency</TH><TD><xsl:value-of select="powerflow/nominal_frequency"/></TD></TR><TR><TH>require_voltage_control</TH><TD><xsl:value-of select="powerflow/require_voltage_control"/></TD></TR><TR><TH>geographic_degree</TH><TD><xsl:value-of select="powerflow/geographic_degree"/></TD></TR><TR><TH>fault_impedance</TH><TD><xsl:value-of select="powerflow/fault_impedance"/></TD></TR><TR><TH>warning_underfrequency</TH><TD><xsl:value-of select="powerflow/warning_underfrequency"/></TD></TR><TR><TH>warning_overfrequency</TH><TD><xsl:value-of select="powerflow/warning_overfrequency"/></TD></TR><TR><TH>warning_undervoltage</TH><TD><xsl:value-of select="powerflow/warning_undervoltage"/></TD></TR><TR><TH>warning_overvoltage</TH><TD><xsl:value-of select="powerflow/warning_overvoltage"/></TD></TR><TR><TH>warning_voltageangle</TH><TD><xsl:value-of select="powerflow/warning_voltageangle"/></TD></TR><TR><TH>maximum_voltage_error</TH><TD><xsl:value-of select="powerflow/maximum_voltage_error"/></TD></TR><TR><TH>solver_method</TH><TD><xsl:value-of select="powerflow/solver_method"/></TD></TR></TABLE>
<H4>node objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>bustype</TH><TH>busflags</TH><TH>reference_bus</TH><TH>maximum_voltage_error</TH><TH>voltage_A</TH><TH>voltage_B</TH><TH>voltage_C</TH><TH>voltage_AB</TH><TH>voltage_BC</TH><TH>voltage_CA</TH><TH>current_A</TH><TH>current_B</TH><TH>current_C</TH><TH>power_A</TH><TH>power_B</TH><TH>power_C</TH><TH>shunt_A</TH><TH>shunt_B</TH><TH>shunt_C</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/node_list/node"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="bustype"/></TD><TD><xsl:value-of select="busflags"/></TD><TD><a href="#{reference_bus}"><xsl:value-of select="reference_bus"/></a></TD><TD><xsl:value-of select="maximum_voltage_error"/></TD><TD><xsl:value-of select="voltage_A"/></TD><TD><xsl:value-of select="voltage_B"/></TD><TD><xsl:value-of select="voltage_C"/></TD><TD><xsl:value-of select="voltage_AB"/></TD><TD><xsl:value-of select="voltage_BC"/></TD><TD><xsl:value-of select="voltage_CA"/></TD><TD><xsl:value-of select="current_A"/></TD><TD><xsl:value-of select="current_B"/></TD><TD><xsl:value-of select="current_C"/></TD><TD><xsl:value-of select="power_A"/></TD><TD><xsl:value-of select="power_B"/></TD><TD><xsl:value-of select="power_C"/></TD><TD><xsl:value-of select="shunt_A"/></TD><TD><xsl:value-of select="shunt_B"/></TD><TD><xsl:value-of select="shunt_C"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>link objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>status</TH><TH>from</TH><TH>to</TH><TH>power_in</TH><TH>power_out</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/link_list/link"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="status"/></TD><TD><a href="#{from}"><xsl:value-of select="from"/></a></TD><TD><a href="#{to}"><xsl:value-of select="to"/></a></TD><TD><xsl:value-of select="power_in"/></TD><TD><xsl:value-of select="power_out"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>capacitor objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>pt_phase</TH><TH>switch</TH><TH>control</TH><TH>bustype</TH><TH>busflags</TH><TH>reference_bus</TH><TH>maximum_voltage_error</TH><TH>voltage_A</TH><TH>voltage_B</TH><TH>voltage_C</TH><TH>voltage_AB</TH><TH>voltage_BC</TH><TH>voltage_CA</TH><TH>current_A</TH><TH>current_B</TH><TH>current_C</TH><TH>power_A</TH><TH>power_B</TH><TH>power_C</TH><TH>shunt_A</TH><TH>shunt_B</TH><TH>shunt_C</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/capacitor_list/capacitor"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="pt_phase"/></TD><TD><xsl:value-of select="switch"/></TD><TD><xsl:value-of select="control"/></TD><TD><xsl:value-of select="bustype"/></TD><TD><xsl:value-of select="busflags"/></TD><TD><a href="#{reference_bus}"><xsl:value-of select="reference_bus"/></a></TD><TD><xsl:value-of select="maximum_voltage_error"/></TD><TD><xsl:value-of select="voltage_A"/></TD><TD><xsl:value-of select="voltage_B"/></TD><TD><xsl:value-of select="voltage_C"/></TD><TD><xsl:value-of select="voltage_AB"/></TD><TD><xsl:value-of select="voltage_BC"/></TD><TD><xsl:value-of select="voltage_CA"/></TD><TD><xsl:value-of select="current_A"/></TD><TD><xsl:value-of select="current_B"/></TD><TD><xsl:value-of select="current_C"/></TD><TD><xsl:value-of select="power_A"/></TD><TD><xsl:value-of select="power_B"/></TD><TD><xsl:value-of select="power_C"/></TD><TD><xsl:value-of select="shunt_A"/></TD><TD><xsl:value-of select="shunt_B"/></TD><TD><xsl:value-of select="shunt_C"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>fuse objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>current_limit</TH><TH>time_curve</TH><TH>status</TH><TH>from</TH><TH>to</TH><TH>power_in</TH><TH>power_out</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/fuse_list/fuse"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="current_limit"/></TD><TD><xsl:value-of select="time_curve"/></TD><TD><xsl:value-of select="status"/></TD><TD><a href="#{from}"><xsl:value-of select="from"/></a></TD><TD><a href="#{to}"><xsl:value-of select="to"/></a></TD><TD><xsl:value-of select="power_in"/></TD><TD><xsl:value-of select="power_out"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>meter objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>measured_energy</TH><TH>measured_power</TH><TH>measured_demand</TH><TH>measured_real_power</TH><TH>measured_reactive_power</TH><TH>measured_voltage_A</TH><TH>measured_voltage_B</TH><TH>measured_voltage_C</TH><TH>measured_current_A</TH><TH>measured_current_B</TH><TH>measured_current_C</TH><TH>bustype</TH><TH>busflags</TH><TH>reference_bus</TH><TH>maximum_voltage_error</TH><TH>voltage_A</TH><TH>voltage_B</TH><TH>voltage_C</TH><TH>voltage_AB</TH><TH>voltage_BC</TH><TH>voltage_CA</TH><TH>current_A</TH><TH>current_B</TH><TH>current_C</TH><TH>power_A</TH><TH>power_B</TH><TH>power_C</TH><TH>shunt_A</TH><TH>shunt_B</TH><TH>shunt_C</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/meter_list/meter"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="measured_energy"/></TD><TD><xsl:value-of select="measured_power"/></TD><TD><xsl:value-of select="measured_demand"/></TD><TD><xsl:value-of select="measured_real_power"/></TD><TD><xsl:value-of select="measured_reactive_power"/></TD><TD><xsl:value-of select="measured_voltage_A"/></TD><TD><xsl:value-of select="measured_voltage_B"/></TD><TD><xsl:value-of select="measured_voltage_C"/></TD><TD><xsl:value-of select="measured_current_A"/></TD><TD><xsl:value-of select="measured_current_B"/></TD><TD><xsl:value-of select="measured_current_C"/></TD><TD><xsl:value-of select="bustype"/></TD><TD><xsl:value-of select="busflags"/></TD><TD><a href="#{reference_bus}"><xsl:value-of select="reference_bus"/></a></TD><TD><xsl:value-of select="maximum_voltage_error"/></TD><TD><xsl:value-of select="voltage_A"/></TD><TD><xsl:value-of select="voltage_B"/></TD><TD><xsl:value-of select="voltage_C"/></TD><TD><xsl:value-of select="voltage_AB"/></TD><TD><xsl:value-of select="voltage_BC"/></TD><TD><xsl:value-of select="voltage_CA"/></TD><TD><xsl:value-of select="current_A"/></TD><TD><xsl:value-of select="current_B"/></TD><TD><xsl:value-of select="current_C"/></TD><TD><xsl:value-of select="power_A"/></TD><TD><xsl:value-of select="power_B"/></TD><TD><xsl:value-of select="power_C"/></TD><TD><xsl:value-of select="shunt_A"/></TD><TD><xsl:value-of select="shunt_B"/></TD><TD><xsl:value-of select="shunt_C"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>line objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>configuration</TH><TH>length</TH><TH>status</TH><TH>from</TH><TH>to</TH><TH>power_in</TH><TH>power_out</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/line_list/line"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><a href="#{configuration}"><xsl:value-of select="configuration"/></a></TD><TD><xsl:value-of select="length"/></TD><TD><xsl:value-of select="status"/></TD><TD><a href="#{from}"><xsl:value-of select="from"/></a></TD><TD><a href="#{to}"><xsl:value-of select="to"/></a></TD><TD><xsl:value-of select="power_in"/></TD><TD><xsl:value-of select="power_out"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>line_spacing objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>distance_AB</TH><TH>distance_BC</TH><TH>distance_AC</TH><TH>distance_AN</TH><TH>distance_BN</TH><TH>distance_CN</TH></TR>
<xsl:for-each select="powerflow/line_spacing_list/line_spacing"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="distance_AB"/></TD><TD><xsl:value-of select="distance_BC"/></TD><TD><xsl:value-of select="distance_AC"/></TD><TD><xsl:value-of select="distance_AN"/></TD><TD><xsl:value-of select="distance_BN"/></TD><TD><xsl:value-of select="distance_CN"/></TD></TR>
</xsl:for-each></TABLE>
<H4>overhead_line objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>configuration</TH><TH>length</TH><TH>status</TH><TH>from</TH><TH>to</TH><TH>power_in</TH><TH>power_out</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/overhead_line_list/overhead_line"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><a href="#{configuration}"><xsl:value-of select="configuration"/></a></TD><TD><xsl:value-of select="length"/></TD><TD><xsl:value-of select="status"/></TD><TD><a href="#{from}"><xsl:value-of select="from"/></a></TD><TD><a href="#{to}"><xsl:value-of select="to"/></a></TD><TD><xsl:value-of select="power_in"/></TD><TD><xsl:value-of select="power_out"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>underground_line objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>configuration</TH><TH>length</TH><TH>status</TH><TH>from</TH><TH>to</TH><TH>power_in</TH><TH>power_out</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/underground_line_list/underground_line"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><a href="#{configuration}"><xsl:value-of select="configuration"/></a></TD><TD><xsl:value-of select="length"/></TD><TD><xsl:value-of select="status"/></TD><TD><a href="#{from}"><xsl:value-of select="from"/></a></TD><TD><a href="#{to}"><xsl:value-of select="to"/></a></TD><TD><xsl:value-of select="power_in"/></TD><TD><xsl:value-of select="power_out"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>overhead_line_conductor objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>geometric_mean_radius</TH><TH>resistance</TH><TH>rating.summer.continuous</TH><TH>rating.summer.emergency</TH><TH>rating.winter.continuous</TH><TH>rating.winter.emergency</TH></TR>
<xsl:for-each select="powerflow/overhead_line_conductor_list/overhead_line_conductor"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="geometric_mean_radius"/></TD><TD><xsl:value-of select="resistance"/></TD><TD><xsl:value-of select="rating.summer.continuous"/></TD><TD><xsl:value-of select="rating.summer.emergency"/></TD><TD><xsl:value-of select="rating.winter.continuous"/></TD><TD><xsl:value-of select="rating.winter.emergency"/></TD></TR>
</xsl:for-each></TABLE>
<H4>underground_line_conductor objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>outer_diameter</TH><TH>conductor_gmr</TH><TH>conductor_diameter</TH><TH>conductor_resistance</TH><TH>neutral_gmr</TH><TH>neutral_diameter</TH><TH>neutral_resistance</TH><TH>neutral_strands</TH><TH>shield_gmr</TH><TH>shield_resistance</TH><TH>rating.summer.continuous</TH><TH>rating.summer.emergency</TH><TH>rating.winter.continuous</TH><TH>rating.winter.emergency</TH></TR>
<xsl:for-each select="powerflow/underground_line_conductor_list/underground_line_conductor"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="outer_diameter"/></TD><TD><xsl:value-of select="conductor_gmr"/></TD><TD><xsl:value-of select="conductor_diameter"/></TD><TD><xsl:value-of select="conductor_resistance"/></TD><TD><xsl:value-of select="neutral_gmr"/></TD><TD><xsl:value-of select="neutral_diameter"/></TD><TD><xsl:value-of select="neutral_resistance"/></TD><TD><xsl:value-of select="neutral_strands"/></TD><TD><xsl:value-of select="shield_gmr"/></TD><TD><xsl:value-of select="shield_resistance"/></TD><TD><xsl:value-of select="rating.summer.continuous"/></TD><TD><xsl:value-of select="rating.summer.emergency"/></TD><TD><xsl:value-of select="rating.winter.continuous"/></TD><TD><xsl:value-of select="rating.winter.emergency"/></TD></TR>
</xsl:for-each></TABLE>
<H4>line_configuration objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>conductor_A</TH><TH>conductor_B</TH><TH>conductor_C</TH><TH>conductor_N</TH><TH>spacing</TH></TR>
<xsl:for-each select="powerflow/line_configuration_list/line_configuration"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><a href="#{conductor_A}"><xsl:value-of select="conductor_A"/></a></TD><TD><a href="#{conductor_B}"><xsl:value-of select="conductor_B"/></a></TD><TD><a href="#{conductor_C}"><xsl:value-of select="conductor_C"/></a></TD><TD><a href="#{conductor_N}"><xsl:value-of select="conductor_N"/></a></TD><TD><a href="#{spacing}"><xsl:value-of select="spacing"/></a></TD></TR>
</xsl:for-each></TABLE>
<H4>relay objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>time_to_change</TH><TH>recloser_delay</TH><TH>recloser_tries</TH><TH>status</TH><TH>from</TH><TH>to</TH><TH>power_in</TH><TH>power_out</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/relay_list/relay"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="time_to_change"/></TD><TD><xsl:value-of select="recloser_delay"/></TD><TD><xsl:value-of select="recloser_tries"/></TD><TD><xsl:value-of select="status"/></TD><TD><a href="#{from}"><xsl:value-of select="from"/></a></TD><TD><a href="#{to}"><xsl:value-of select="to"/></a></TD><TD><xsl:value-of select="power_in"/></TD><TD><xsl:value-of select="power_out"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>transformer_configuration objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>connect_type</TH><TH>install_type</TH><TH>primary_voltage</TH><TH>secondary_voltage</TH><TH>power_rating</TH><TH>powerA_rating</TH><TH>powerB_rating</TH><TH>powerC_rating</TH><TH>resistance</TH><TH>reactance</TH><TH>impedance</TH></TR>
<xsl:for-each select="powerflow/transformer_configuration_list/transformer_configuration"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="connect_type"/></TD><TD><xsl:value-of select="install_type"/></TD><TD><xsl:value-of select="primary_voltage"/></TD><TD><xsl:value-of select="secondary_voltage"/></TD><TD><xsl:value-of select="power_rating"/></TD><TD><xsl:value-of select="powerA_rating"/></TD><TD><xsl:value-of select="powerB_rating"/></TD><TD><xsl:value-of select="powerC_rating"/></TD><TD><xsl:value-of select="resistance"/></TD><TD><xsl:value-of select="reactance"/></TD><TD><xsl:value-of select="impedance"/></TD></TR>
</xsl:for-each></TABLE>
<H4>transformer objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>configuration</TH><TH>status</TH><TH>from</TH><TH>to</TH><TH>power_in</TH><TH>power_out</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/transformer_list/transformer"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><a href="#{configuration}"><xsl:value-of select="configuration"/></a></TD><TD><xsl:value-of select="status"/></TD><TD><a href="#{from}"><xsl:value-of select="from"/></a></TD><TD><a href="#{to}"><xsl:value-of select="to"/></a></TD><TD><xsl:value-of select="power_in"/></TD><TD><xsl:value-of select="power_out"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>load objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>load_class</TH><TH>constant_power_A</TH><TH>constant_power_B</TH><TH>constant_power_C</TH><TH>constant_current_A</TH><TH>constant_current_B</TH><TH>constant_current_C</TH><TH>constant_impedance_A</TH><TH>constant_impedance_B</TH><TH>constant_impedance_C</TH><TH>bustype</TH><TH>busflags</TH><TH>reference_bus</TH><TH>maximum_voltage_error</TH><TH>voltage_A</TH><TH>voltage_B</TH><TH>voltage_C</TH><TH>voltage_AB</TH><TH>voltage_BC</TH><TH>voltage_CA</TH><TH>current_A</TH><TH>current_B</TH><TH>current_C</TH><TH>power_A</TH><TH>power_B</TH><TH>power_C</TH><TH>shunt_A</TH><TH>shunt_B</TH><TH>shunt_C</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/load_list/load"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="load_class"/></TD><TD><xsl:value-of select="constant_power_A"/></TD><TD><xsl:value-of select="constant_power_B"/></TD><TD><xsl:value-of select="constant_power_C"/></TD><TD><xsl:value-of select="constant_current_A"/></TD><TD><xsl:value-of select="constant_current_B"/></TD><TD><xsl:value-of select="constant_current_C"/></TD><TD><xsl:value-of select="constant_impedance_A"/></TD><TD><xsl:value-of select="constant_impedance_B"/></TD><TD><xsl:value-of select="constant_impedance_C"/></TD><TD><xsl:value-of select="bustype"/></TD><TD><xsl:value-of select="busflags"/></TD><TD><a href="#{reference_bus}"><xsl:value-of select="reference_bus"/></a></TD><TD><xsl:value-of select="maximum_voltage_error"/></TD><TD><xsl:value-of select="voltage_A"/></TD><TD><xsl:value-of select="voltage_B"/></TD><TD><xsl:value-of select="voltage_C"/></TD><TD><xsl:value-of select="voltage_AB"/></TD><TD><xsl:value-of select="voltage_BC"/></TD><TD><xsl:value-of select="voltage_CA"/></TD><TD><xsl:value-of select="current_A"/></TD><TD><xsl:value-of select="current_B"/></TD><TD><xsl:value-of select="current_C"/></TD><TD><xsl:value-of select="power_A"/></TD><TD><xsl:value-of select="power_B"/></TD><TD><xsl:value-of select="power_C"/></TD><TD><xsl:value-of select="shunt_A"/></TD><TD><xsl:value-of select="shunt_B"/></TD><TD><xsl:value-of select="shunt_C"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>regulator_configuration objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>connect_type</TH><TH>band_center</TH><TH>band_width</TH><TH>time_delay</TH><TH>raise_taps</TH><TH>lower_taps</TH><TH>current_transducer_ratio</TH><TH>power_transducer_ratio</TH><TH>compensator_r_setting</TH><TH>compensator_x_setting</TH><TH>CT_phase</TH><TH>PT_phase</TH><TH>regulation</TH><TH>high_voltage</TH><TH>tap_pos_A</TH><TH>tap_pos_B</TH><TH>tap_pos_C</TH></TR>
<xsl:for-each select="powerflow/regulator_configuration_list/regulator_configuration"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="connect_type"/></TD><TD><xsl:value-of select="band_center"/></TD><TD><xsl:value-of select="band_width"/></TD><TD><xsl:value-of select="time_delay"/></TD><TD><xsl:value-of select="raise_taps"/></TD><TD><xsl:value-of select="lower_taps"/></TD><TD><xsl:value-of select="current_transducer_ratio"/></TD><TD><xsl:value-of select="power_transducer_ratio"/></TD><TD><xsl:value-of select="compensator_r_setting"/></TD><TD><xsl:value-of select="compensator_x_setting"/></TD><TD><xsl:value-of select="CT_phase"/></TD><TD><xsl:value-of select="PT_phase"/></TD><TD><xsl:value-of select="regulation"/></TD><TD><xsl:value-of select="high_voltage"/></TD><TD><xsl:value-of select="tap_pos_A"/></TD><TD><xsl:value-of select="tap_pos_B"/></TD><TD><xsl:value-of select="tap_pos_C"/></TD></TR>
</xsl:for-each></TABLE>
<H4>regulator objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>configuration</TH><TH>status</TH><TH>from</TH><TH>to</TH><TH>power_in</TH><TH>power_out</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/regulator_list/regulator"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><a href="#{configuration}"><xsl:value-of select="configuration"/></a></TD><TD><xsl:value-of select="status"/></TD><TD><a href="#{from}"><xsl:value-of select="from"/></a></TD><TD><a href="#{to}"><xsl:value-of select="to"/></a></TD><TD><xsl:value-of select="power_in"/></TD><TD><xsl:value-of select="power_out"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>triplex_node objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>bustype</TH><TH>busflags</TH><TH>reference_bus</TH><TH>nominal_voltage</TH><TH>maximum_voltage_error</TH><TH>voltage_1</TH><TH>voltage_2</TH><TH>voltage_N</TH><TH>voltage_12</TH><TH>voltage_1N</TH><TH>voltage_2N</TH><TH>current_1</TH><TH>current_2</TH><TH>current_N</TH><TH>power_1</TH><TH>power_2</TH><TH>power_N</TH><TH>shunt_1</TH><TH>shunt_2</TH><TH>shunt_N</TH><TH>impedance_1</TH><TH>impedance_2</TH><TH>impedance_N</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/triplex_node_list/triplex_node"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="bustype"/></TD><TD><xsl:value-of select="busflags"/></TD><TD><a href="#{reference_bus}"><xsl:value-of select="reference_bus"/></a></TD><TD><xsl:value-of select="nominal_voltage"/></TD><TD><xsl:value-of select="maximum_voltage_error"/></TD><TD><xsl:value-of select="voltage_1"/></TD><TD><xsl:value-of select="voltage_2"/></TD><TD><xsl:value-of select="voltage_N"/></TD><TD><xsl:value-of select="voltage_12"/></TD><TD><xsl:value-of select="voltage_1N"/></TD><TD><xsl:value-of select="voltage_2N"/></TD><TD><xsl:value-of select="current_1"/></TD><TD><xsl:value-of select="current_2"/></TD><TD><xsl:value-of select="current_N"/></TD><TD><xsl:value-of select="power_1"/></TD><TD><xsl:value-of select="power_2"/></TD><TD><xsl:value-of select="power_N"/></TD><TD><xsl:value-of select="shunt_1"/></TD><TD><xsl:value-of select="shunt_2"/></TD><TD><xsl:value-of select="shunt_N"/></TD><TD><xsl:value-of select="impedance_1"/></TD><TD><xsl:value-of select="impedance_2"/></TD><TD><xsl:value-of select="impedance_N"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>triplex_meter objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>measured_energy</TH><TH>measured_power</TH><TH>measured_demand</TH><TH>measured_real_power</TH><TH>measured_voltage_1</TH><TH>measured_voltage_2</TH><TH>measured_voltage_N</TH><TH>measured_current_1</TH><TH>measured_current_2</TH><TH>measured_current_N</TH><TH>bustype</TH><TH>busflags</TH><TH>reference_bus</TH><TH>nominal_voltage</TH><TH>maximum_voltage_error</TH><TH>voltage_1</TH><TH>voltage_2</TH><TH>voltage_N</TH><TH>voltage_12</TH><TH>voltage_1N</TH><TH>voltage_2N</TH><TH>current_1</TH><TH>current_2</TH><TH>current_N</TH><TH>power_1</TH><TH>power_2</TH><TH>power_N</TH><TH>shunt_1</TH><TH>shunt_2</TH><TH>shunt_N</TH><TH>impedance_1</TH><TH>impedance_2</TH><TH>impedance_N</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/triplex_meter_list/triplex_meter"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="measured_energy"/></TD><TD><xsl:value-of select="measured_power"/></TD><TD><xsl:value-of select="measured_demand"/></TD><TD><xsl:value-of select="measured_real_power"/></TD><TD><xsl:value-of select="measured_voltage_1"/></TD><TD><xsl:value-of select="measured_voltage_2"/></TD><TD><xsl:value-of select="measured_voltage_N"/></TD><TD><xsl:value-of select="measured_current_1"/></TD><TD><xsl:value-of select="measured_current_2"/></TD><TD><xsl:value-of select="measured_current_N"/></TD><TD><xsl:value-of select="bustype"/></TD><TD><xsl:value-of select="busflags"/></TD><TD><a href="#{reference_bus}"><xsl:value-of select="reference_bus"/></a></TD><TD><xsl:value-of select="nominal_voltage"/></TD><TD><xsl:value-of select="maximum_voltage_error"/></TD><TD><xsl:value-of select="voltage_1"/></TD><TD><xsl:value-of select="voltage_2"/></TD><TD><xsl:value-of select="voltage_N"/></TD><TD><xsl:value-of select="voltage_12"/></TD><TD><xsl:value-of select="voltage_1N"/></TD><TD><xsl:value-of select="voltage_2N"/></TD><TD><xsl:value-of select="current_1"/></TD><TD><xsl:value-of select="current_2"/></TD><TD><xsl:value-of select="current_N"/></TD><TD><xsl:value-of select="power_1"/></TD><TD><xsl:value-of select="power_2"/></TD><TD><xsl:value-of select="power_N"/></TD><TD><xsl:value-of select="shunt_1"/></TD><TD><xsl:value-of select="shunt_2"/></TD><TD><xsl:value-of select="shunt_N"/></TD><TD><xsl:value-of select="impedance_1"/></TD><TD><xsl:value-of select="impedance_2"/></TD><TD><xsl:value-of select="impedance_N"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>triplex_line objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>configuration</TH><TH>length</TH><TH>status</TH><TH>from</TH><TH>to</TH><TH>power_in</TH><TH>power_out</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/triplex_line_list/triplex_line"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><a href="#{configuration}"><xsl:value-of select="configuration"/></a></TD><TD><xsl:value-of select="length"/></TD><TD><xsl:value-of select="status"/></TD><TD><a href="#{from}"><xsl:value-of select="from"/></a></TD><TD><a href="#{to}"><xsl:value-of select="to"/></a></TD><TD><xsl:value-of select="power_in"/></TD><TD><xsl:value-of select="power_out"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H4>triplex_line_configuration objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>conductor_1</TH><TH>conductor_2</TH><TH>conductor_N</TH><TH>insulation_thickness</TH><TH>diameter</TH><TH>spacing</TH></TR>
<xsl:for-each select="powerflow/triplex_line_configuration_list/triplex_line_configuration"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><a href="#{conductor_1}"><xsl:value-of select="conductor_1"/></a></TD><TD><a href="#{conductor_2}"><xsl:value-of select="conductor_2"/></a></TD><TD><a href="#{conductor_N}"><xsl:value-of select="conductor_N"/></a></TD><TD><xsl:value-of select="insulation_thickness"/></TD><TD><xsl:value-of select="diameter"/></TD><TD><a href="#{spacing}"><xsl:value-of select="spacing"/></a></TD></TR>
</xsl:for-each></TABLE>
<H4>triplex_line_conductor objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>resistance</TH><TH>geometric_mean_radius</TH></TR>
<xsl:for-each select="powerflow/triplex_line_conductor_list/triplex_line_conductor"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="resistance"/></TD><TD><xsl:value-of select="geometric_mean_radius"/></TD></TR>
</xsl:for-each></TABLE>
<H4>switch objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>status</TH><TH>from</TH><TH>to</TH><TH>power_in</TH><TH>power_out</TH><TH>phases</TH><TH>nominal_voltage</TH></TR>
<xsl:for-each select="powerflow/switch_list/switch"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="status"/></TD><TD><a href="#{from}"><xsl:value-of select="from"/></a></TD><TD><a href="#{to}"><xsl:value-of select="to"/></a></TD><TD><xsl:value-of select="power_in"/></TD><TD><xsl:value-of select="power_out"/></TD><TD><xsl:value-of select="phases"/></TD><TD><xsl:value-of select="nominal_voltage"/></TD></TR>
</xsl:for-each></TABLE>
<H3><A NAME="modules_residential">residential</A></H3><TABLE BORDER="1">
<TR><TH>version.major</TH><TD><xsl:value-of select="residential/version.major"/></TD></TR><TR><TH>version.minor</TH><TD><xsl:value-of select="residential/version.minor"/></TD></TR><TR><TH>default_line_voltage</TH><TD><xsl:value-of select="residential/default_line_voltage"/></TD></TR><TR><TH>default_line_current</TH><TD><xsl:value-of select="residential/default_line_current"/></TD></TR></TABLE>
<H4>house objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>floor_area</TH><TH>gross_wall_area</TH><TH>ceiling_height</TH><TH>aspect_ratio</TH><TH>envelope_UA</TH><TH>window_wall_ratio</TH><TH>glazing_shgc</TH><TH>airchange_per_hour</TH><TH>internal_gain</TH><TH>thermostat_deadband</TH><TH>heating_setpoint</TH><TH>cooling_setpoint</TH><TH>design_heating_capacity</TH><TH>design_cooling_capacity</TH><TH>heating_COP</TH><TH>cooling_COP</TH><TH>COP_coeff</TH><TH>air_temperature</TH><TH>mass_heat_coeff</TH><TH>heat_mode</TH><TH>power</TH><TH>current</TH><TH>admittance</TH></TR>
<xsl:for-each select="residential/house_list/house"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="floor_area"/></TD><TD><xsl:value-of select="gross_wall_area"/></TD><TD><xsl:value-of select="ceiling_height"/></TD><TD><xsl:value-of select="aspect_ratio"/></TD><TD><xsl:value-of select="envelope_UA"/></TD><TD><xsl:value-of select="window_wall_ratio"/></TD><TD><xsl:value-of select="glazing_shgc"/></TD><TD><xsl:value-of select="airchange_per_hour"/></TD><TD><xsl:value-of select="internal_gain"/></TD><TD><xsl:value-of select="thermostat_deadband"/></TD><TD><xsl:value-of select="heating_setpoint"/></TD><TD><xsl:value-of select="cooling_setpoint"/></TD><TD><xsl:value-of select="design_heating_capacity"/></TD><TD><xsl:value-of select="design_cooling_capacity"/></TD><TD><xsl:value-of select="heating_COP"/></TD><TD><xsl:value-of select="cooling_COP"/></TD><TD><xsl:value-of select="COP_coeff"/></TD><TD><xsl:value-of select="air_temperature"/></TD><TD><xsl:value-of select="mass_heat_coeff"/></TD><TD><xsl:value-of select="heat_mode"/></TD><TD><xsl:value-of select="power"/></TD><TD><xsl:value-of select="current"/></TD><TD><xsl:value-of select="admittance"/></TD></TR>
</xsl:for-each></TABLE>
<H4>waterheater objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>tank_volume</TH><TH>tank_UA</TH><TH>tank_diameter</TH><TH>water_demand</TH><TH>heating_element_capacity</TH><TH>inlet_water_temperature</TH><TH>heat_mode</TH><TH>location</TH><TH>tank_setpoint</TH><TH>thermostat_deadband</TH><TH>power</TH><TH>meter</TH><TH>temperature</TH></TR>
<xsl:for-each select="residential/waterheater_list/waterheater"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="tank_volume"/></TD><TD><xsl:value-of select="tank_UA"/></TD><TD><xsl:value-of select="tank_diameter"/></TD><TD><xsl:value-of select="water_demand"/></TD><TD><xsl:value-of select="heating_element_capacity"/></TD><TD><xsl:value-of select="inlet_water_temperature"/></TD><TD><xsl:value-of select="heat_mode"/></TD><TD><xsl:value-of select="location"/></TD><TD><xsl:value-of select="tank_setpoint"/></TD><TD><xsl:value-of select="thermostat_deadband"/></TD><TD><xsl:value-of select="power"/></TD><TD><xsl:value-of select="meter"/></TD><TD><xsl:value-of select="temperature"/></TD></TR>
</xsl:for-each></TABLE>
<H4>lights objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>type</TH><TH>placement</TH><TH>installed_power</TH><TH>circuit_split</TH><TH>demand</TH><TH>constant_power</TH><TH>constant_current</TH><TH>constant_admittance</TH><TH>internal_gains</TH><TH>energy_meter</TH></TR>
<xsl:for-each select="residential/lights_list/lights"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="type"/></TD><TD><xsl:value-of select="placement"/></TD><TD><xsl:value-of select="installed_power"/></TD><TD><xsl:value-of select="circuit_split"/></TD><TD><xsl:value-of select="demand"/></TD><TD><xsl:value-of select="constant_power"/></TD><TD><xsl:value-of select="constant_current"/></TD><TD><xsl:value-of select="constant_admittance"/></TD><TD><xsl:value-of select="internal_gains"/></TD><TD><xsl:value-of select="energy_meter"/></TD></TR>
</xsl:for-each></TABLE>
<H4>refrigerator objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>size </TH><TH>rated_capacity </TH><TH>power</TH><TH>power_factor</TH><TH>meter</TH></TR>
<xsl:for-each select="residential/refrigerator_list/refrigerator"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="size "/></TD><TD><xsl:value-of select="rated_capacity "/></TD><TD><xsl:value-of select="power"/></TD><TD><xsl:value-of select="power_factor"/></TD><TD><xsl:value-of select="meter"/></TD></TR>
</xsl:for-each></TABLE>
<H4>clotheswasher objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>motor_power</TH><TH>power_factor</TH><TH>circuit_split</TH><TH>heat_fraction</TH><TH>enduse_demand</TH><TH>enduse_queue</TH><TH>constant_power</TH><TH>constant_current</TH><TH>constant_admittance</TH><TH>internal_gains</TH><TH>energy_meter</TH><TH>stall_voltage</TH><TH>start_voltage</TH><TH>stall_impedance</TH><TH>trip_delay</TH><TH>reset_delay</TH><TH>state</TH></TR>
<xsl:for-each select="residential/clotheswasher_list/clotheswasher"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="motor_power"/></TD><TD><xsl:value-of select="power_factor"/></TD><TD><xsl:value-of select="circuit_split"/></TD><TD><xsl:value-of select="heat_fraction"/></TD><TD><xsl:value-of select="enduse_demand"/></TD><TD><xsl:value-of select="enduse_queue"/></TD><TD><xsl:value-of select="constant_power"/></TD><TD><xsl:value-of select="constant_current"/></TD><TD><xsl:value-of select="constant_admittance"/></TD><TD><xsl:value-of select="internal_gains"/></TD><TD><xsl:value-of select="energy_meter"/></TD><TD><xsl:value-of select="stall_voltage"/></TD><TD><xsl:value-of select="start_voltage"/></TD><TD><xsl:value-of select="stall_impedance"/></TD><TD><xsl:value-of select="trip_delay"/></TD><TD><xsl:value-of select="reset_delay"/></TD><TD><xsl:value-of select="state"/></TD></TR>
</xsl:for-each></TABLE>
<H4>dishwasher objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>installed_power</TH><TH>circuit_split</TH><TH>demand</TH><TH>constant_power</TH><TH>constant_current</TH><TH>constant_admittance</TH><TH>internal_gains</TH><TH>energy_meter</TH><TH>heat_fraction</TH></TR>
<xsl:for-each select="residential/dishwasher_list/dishwasher"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="installed_power"/></TD><TD><xsl:value-of select="circuit_split"/></TD><TD><xsl:value-of select="demand"/></TD><TD><xsl:value-of select="constant_power"/></TD><TD><xsl:value-of select="constant_current"/></TD><TD><xsl:value-of select="constant_admittance"/></TD><TD><xsl:value-of select="internal_gains"/></TD><TD><xsl:value-of select="energy_meter"/></TD><TD><xsl:value-of select="heat_fraction"/></TD></TR>
</xsl:for-each></TABLE>
<H4>occupantload objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>number_of_occupants</TH><TH>occupancy_fraction</TH><TH>heatgain_per_person</TH><TH>internal_gains</TH></TR>
<xsl:for-each select="residential/occupantload_list/occupantload"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="number_of_occupants"/></TD><TD><xsl:value-of select="occupancy_fraction"/></TD><TD><xsl:value-of select="heatgain_per_person"/></TD><TD><xsl:value-of select="internal_gains"/></TD></TR>
</xsl:for-each></TABLE>
<H4>plugload objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>circuit_split</TH><TH>demand</TH><TH>constant_power</TH><TH>constant_current</TH><TH>constant_admittance</TH><TH>internal_gains</TH><TH>energy_meter</TH><TH>heat_fraction</TH></TR>
<xsl:for-each select="residential/plugload_list/plugload"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="circuit_split"/></TD><TD><xsl:value-of select="demand"/></TD><TD><xsl:value-of select="constant_power"/></TD><TD><xsl:value-of select="constant_current"/></TD><TD><xsl:value-of select="constant_admittance"/></TD><TD><xsl:value-of select="internal_gains"/></TD><TD><xsl:value-of select="energy_meter"/></TD><TD><xsl:value-of select="heat_fraction"/></TD></TR>
</xsl:for-each></TABLE>
<H4>microwave objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>installed_power</TH><TH>standby_power</TH><TH>circuit_split</TH><TH>demand</TH><TH>constant_power</TH><TH>constant_current</TH><TH>constant_admittance</TH><TH>internal_gains</TH><TH>energy_meter</TH><TH>heat_fraction</TH><TH>state</TH><TH>runtime</TH><TH>state_time</TH></TR>
<xsl:for-each select="residential/microwave_list/microwave"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="installed_power"/></TD><TD><xsl:value-of select="standby_power"/></TD><TD><xsl:value-of select="circuit_split"/></TD><TD><xsl:value-of select="demand"/></TD><TD><xsl:value-of select="constant_power"/></TD><TD><xsl:value-of select="constant_current"/></TD><TD><xsl:value-of select="constant_admittance"/></TD><TD><xsl:value-of select="internal_gains"/></TD><TD><xsl:value-of select="energy_meter"/></TD><TD><xsl:value-of select="heat_fraction"/></TD><TD><xsl:value-of select="state"/></TD><TD><xsl:value-of select="runtime"/></TD><TD><xsl:value-of select="state_time"/></TD></TR>
</xsl:for-each></TABLE>
<H4>range objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>installed_power</TH><TH>circuit_split</TH><TH>demand</TH><TH>constant_power</TH><TH>constant_current</TH><TH>constant_admittance</TH><TH>internal_gains</TH><TH>energy_meter</TH><TH>heat_fraction</TH></TR>
<xsl:for-each select="residential/range_list/range"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="installed_power"/></TD><TD><xsl:value-of select="circuit_split"/></TD><TD><xsl:value-of select="demand"/></TD><TD><xsl:value-of select="constant_power"/></TD><TD><xsl:value-of select="constant_current"/></TD><TD><xsl:value-of select="constant_admittance"/></TD><TD><xsl:value-of select="internal_gains"/></TD><TD><xsl:value-of select="energy_meter"/></TD><TD><xsl:value-of select="heat_fraction"/></TD></TR>
</xsl:for-each></TABLE>
<H4>freezer objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>size </TH><TH>rated_capacity </TH><TH>power</TH><TH>power_factor</TH><TH>meter</TH></TR>
<xsl:for-each select="residential/freezer_list/freezer"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="size "/></TD><TD><xsl:value-of select="rated_capacity "/></TD><TD><xsl:value-of select="power"/></TD><TD><xsl:value-of select="power_factor"/></TD><TD><xsl:value-of select="meter"/></TD></TR>
</xsl:for-each></TABLE>
<H4>dryer objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>motor_power</TH><TH>power_factor</TH><TH>coil_power</TH><TH>circuit_split</TH><TH>heat_fraction</TH><TH>enduse_demand</TH><TH>enduse_queue</TH><TH>constant_power</TH><TH>constant_current</TH><TH>constant_admittance</TH><TH>internal_gains</TH><TH>energy_meter</TH><TH>stall_voltage</TH><TH>start_voltage</TH><TH>stall_impedance</TH><TH>trip_delay</TH><TH>reset_delay</TH><TH>state</TH></TR>
<xsl:for-each select="residential/dryer_list/dryer"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="motor_power"/></TD><TD><xsl:value-of select="power_factor"/></TD><TD><xsl:value-of select="coil_power"/></TD><TD><xsl:value-of select="circuit_split"/></TD><TD><xsl:value-of select="heat_fraction"/></TD><TD><xsl:value-of select="enduse_demand"/></TD><TD><xsl:value-of select="enduse_queue"/></TD><TD><xsl:value-of select="constant_power"/></TD><TD><xsl:value-of select="constant_current"/></TD><TD><xsl:value-of select="constant_admittance"/></TD><TD><xsl:value-of select="internal_gains"/></TD><TD><xsl:value-of select="energy_meter"/></TD><TD><xsl:value-of select="stall_voltage"/></TD><TD><xsl:value-of select="start_voltage"/></TD><TD><xsl:value-of select="stall_impedance"/></TD><TD><xsl:value-of select="trip_delay"/></TD><TD><xsl:value-of select="reset_delay"/></TD><TD><xsl:value-of select="state"/></TD></TR>
</xsl:for-each></TABLE>
<H4>evcharger objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>charger_power</TH><TH>vehicle_type</TH><TH>state</TH><TH>p_come_home</TH><TH>p_go_work</TH><TH>p_go_shorttrip</TH><TH>p_go_longtrip</TH><TH>work_dist</TH><TH>shorttrip_dist</TH><TH>longtrip_dist</TH><TH>capacity</TH><TH>charge</TH><TH>charge_at_work</TH><TH>power_factor</TH><TH>constant_power</TH><TH>constant_current</TH><TH>constant_admittance</TH><TH>internal_gains</TH><TH>energy_meter</TH><TH>heat_fraction</TH></TR>
<xsl:for-each select="residential/evcharger_list/evcharger"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="charger_power"/></TD><TD><xsl:value-of select="vehicle_type"/></TD><TD><xsl:value-of select="state"/></TD><TD><xsl:value-of select="p_come_home"/></TD><TD><xsl:value-of select="p_go_work"/></TD><TD><xsl:value-of select="p_go_shorttrip"/></TD><TD><xsl:value-of select="p_go_longtrip"/></TD><TD><xsl:value-of select="work_dist"/></TD><TD><xsl:value-of select="shorttrip_dist"/></TD><TD><xsl:value-of select="longtrip_dist"/></TD><TD><xsl:value-of select="capacity"/></TD><TD><xsl:value-of select="charge"/></TD><TD><xsl:value-of select="charge_at_work"/></TD><TD><xsl:value-of select="power_factor"/></TD><TD><xsl:value-of select="constant_power"/></TD><TD><xsl:value-of select="constant_current"/></TD><TD><xsl:value-of select="constant_admittance"/></TD><TD><xsl:value-of select="internal_gains"/></TD><TD><xsl:value-of select="energy_meter"/></TD><TD><xsl:value-of select="heat_fraction"/></TD></TR>
</xsl:for-each></TABLE>
<H3><A NAME="modules_tape">tape</A></H3><TABLE BORDER="1">
<TR><TH>version.major</TH><TD><xsl:value-of select="tape/version.major"/></TD></TR><TR><TH>version.minor</TH><TD><xsl:value-of select="tape/version.minor"/></TD></TR></TABLE>
<H4>player objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>property</TH><TH>file</TH><TH>filetype</TH><TH>loop</TH></TR>
<xsl:for-each select="tape/player_list/player"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="property"/></TD><TD><xsl:value-of select="file"/></TD><TD><xsl:value-of select="filetype"/></TD><TD><xsl:value-of select="loop"/></TD></TR>
</xsl:for-each></TABLE>
<H4>shaper objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>file</TH><TH>filetype</TH><TH>group</TH><TH>property</TH><TH>magnitude</TH><TH>events</TH></TR>
<xsl:for-each select="tape/shaper_list/shaper"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="file"/></TD><TD><xsl:value-of select="filetype"/></TD><TD><xsl:value-of select="group"/></TD><TD><xsl:value-of select="property"/></TD><TD><xsl:value-of select="magnitude"/></TD><TD><xsl:value-of select="events"/></TD></TR>
</xsl:for-each></TABLE>
<H4>recorder objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>property</TH><TH>trigger</TH><TH>file</TH><TH>interval</TH><TH>limit</TH><TH>plotcommands</TH><TH>xdata</TH><TH>columns</TH><TH>output</TH></TR>
<xsl:for-each select="tape/recorder_list/recorder"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="property"/></TD><TD><xsl:value-of select="trigger"/></TD><TD><xsl:value-of select="file"/></TD><TD><xsl:value-of select="interval"/></TD><TD><xsl:value-of select="limit"/></TD><TD><xsl:value-of select="plotcommands"/></TD><TD><xsl:value-of select="xdata"/></TD><TD><xsl:value-of select="columns"/></TD><TD><xsl:value-of select="output"/></TD></TR>
</xsl:for-each></TABLE>
<H4>collector objects</H4><TABLE BORDER="1">
<TR><TH>Name</TH><TH>property</TH><TH>trigger</TH><TH>file</TH><TH>interval</TH><TH>limit</TH><TH>group</TH></TR>
<xsl:for-each select="tape/collector_list/collector"><TR><TD><a name="#{name}"/><xsl:value-of select="name"/> (#<xsl:value-of select="id"/>)</TD><TD><xsl:value-of select="property"/></TD><TD><xsl:value-of select="trigger"/></TD><TD><xsl:value-of select="file"/></TD><TD><xsl:value-of select="interval"/></TD><TD><xsl:value-of select="limit"/></TD><TD><xsl:value-of select="group"/></TD></TR>
</xsl:for-each></TABLE>
<H2><A NAME="output">GLM Output</A></H2>
<table border="1" width="100%"><tr><td><pre>
# Generated by GridLAB-D <xsl:value-of select="version.major"/>.<xsl:value-of select="version.minor"/>
# Command line..... <xsl:value-of select="command_line"/>
# Model name....... <xsl:value-of select="modelname"/>
# Start at......... <xsl:value-of select="starttime"/>
#
clock {
	timestamp '<xsl:value-of select="clock"/>';
	timezone <xsl:value-of select="timezone"/>;
}
<xsl:for-each select="climate">
##############################################
# climate module
module climate {
	version.major <xsl:value-of select="version.major"/>;
	version.minor <xsl:value-of select="version.minor"/>;
}

# climate::climate objects
<xsl:for-each select="climate_list/climate"><a name="#GLM.{name}"/>object climate:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="city">	city <xsl:value-of select="city"/>;
</xsl:if><xsl:if test="tmyfile">	tmyfile <xsl:value-of select="tmyfile"/>;
</xsl:if><xsl:if test="temperature">	temperature <xsl:value-of select="temperature"/>;
</xsl:if><xsl:if test="humidity">	humidity <xsl:value-of select="humidity"/>;
</xsl:if><xsl:if test="solar_flux">	solar_flux <xsl:value-of select="solar_flux"/>;
</xsl:if><xsl:if test="wind_speed">	wind_speed <xsl:value-of select="wind_speed"/>;
</xsl:if><xsl:if test="wind_dir">	wind_dir <xsl:value-of select="wind_dir"/>;
</xsl:if><xsl:if test="wind_gust">	wind_gust <xsl:value-of select="wind_gust"/>;
</xsl:if>}
</xsl:for-each></xsl:for-each><xsl:for-each select="commercial">
##############################################
# commercial module
module commercial {
	version.major <xsl:value-of select="version.major"/>;
	version.minor <xsl:value-of select="version.minor"/>;
}

# commercial::office objects
<xsl:for-each select="office_list/office"><a name="#GLM.{name}"/>object office:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="occupancy">	occupancy <xsl:value-of select="occupancy"/>;
</xsl:if><xsl:if test="floor_height">	floor_height <xsl:value-of select="floor_height"/>;
</xsl:if><xsl:if test="exterior_ua">	exterior_ua <xsl:value-of select="exterior_ua"/>;
</xsl:if><xsl:if test="interior_ua">	interior_ua <xsl:value-of select="interior_ua"/>;
</xsl:if><xsl:if test="interior_mass">	interior_mass <xsl:value-of select="interior_mass"/>;
</xsl:if><xsl:if test="floor_area">	floor_area <xsl:value-of select="floor_area"/>;
</xsl:if><xsl:if test="glazing">	glazing <xsl:value-of select="glazing"/>;
</xsl:if><xsl:if test="glazing.north">	glazing.north <xsl:value-of select="glazing.north"/>;
</xsl:if><xsl:if test="glazing.northeast">	glazing.northeast <xsl:value-of select="glazing.northeast"/>;
</xsl:if><xsl:if test="glazing.east">	glazing.east <xsl:value-of select="glazing.east"/>;
</xsl:if><xsl:if test="glazing.southeast">	glazing.southeast <xsl:value-of select="glazing.southeast"/>;
</xsl:if><xsl:if test="glazing.south">	glazing.south <xsl:value-of select="glazing.south"/>;
</xsl:if><xsl:if test="glazing.southwest">	glazing.southwest <xsl:value-of select="glazing.southwest"/>;
</xsl:if><xsl:if test="glazing.west">	glazing.west <xsl:value-of select="glazing.west"/>;
</xsl:if><xsl:if test="glazing.northwest">	glazing.northwest <xsl:value-of select="glazing.northwest"/>;
</xsl:if><xsl:if test="glazing.horizontal">	glazing.horizontal <xsl:value-of select="glazing.horizontal"/>;
</xsl:if><xsl:if test="glazing.coefficient">	glazing.coefficient <xsl:value-of select="glazing.coefficient"/>;
</xsl:if><xsl:if test="occupants">	occupants <xsl:value-of select="occupants"/>;
</xsl:if><xsl:if test="schedule">	schedule <xsl:value-of select="schedule"/>;
</xsl:if><xsl:if test="air_temperature">	air_temperature <xsl:value-of select="air_temperature"/>;
</xsl:if><xsl:if test="mass_temperature">	mass_temperature <xsl:value-of select="mass_temperature"/>;
</xsl:if><xsl:if test="temperature_change">	temperature_change <xsl:value-of select="temperature_change"/>;
</xsl:if><xsl:if test="Qh">	Qh <xsl:value-of select="Qh"/>;
</xsl:if><xsl:if test="Qs">	Qs <xsl:value-of select="Qs"/>;
</xsl:if><xsl:if test="Qi">	Qi <xsl:value-of select="Qi"/>;
</xsl:if><xsl:if test="Qz">	Qz <xsl:value-of select="Qz"/>;
</xsl:if><xsl:if test="hvac.mode">	hvac.mode <xsl:value-of select="hvac.mode"/>;
</xsl:if><xsl:if test="hvac.cooling.balance_temperature">	hvac.cooling.balance_temperature <xsl:value-of select="hvac.cooling.balance_temperature"/>;
</xsl:if><xsl:if test="hvac.cooling.capacity">	hvac.cooling.capacity <xsl:value-of select="hvac.cooling.capacity"/>;
</xsl:if><xsl:if test="hvac.cooling.capacity_perF">	hvac.cooling.capacity_perF <xsl:value-of select="hvac.cooling.capacity_perF"/>;
</xsl:if><xsl:if test="hvac.cooling.design_temperature">	hvac.cooling.design_temperature <xsl:value-of select="hvac.cooling.design_temperature"/>;
</xsl:if><xsl:if test="hvac.cooling.efficiency">	hvac.cooling.efficiency <xsl:value-of select="hvac.cooling.efficiency"/>;
</xsl:if><xsl:if test="hvac.cooling.cop">	hvac.cooling.cop <xsl:value-of select="hvac.cooling.cop"/>;
</xsl:if><xsl:if test="hvac.heating.balance_temperature">	hvac.heating.balance_temperature <xsl:value-of select="hvac.heating.balance_temperature"/>;
</xsl:if><xsl:if test="hvac.heating.capacity">	hvac.heating.capacity <xsl:value-of select="hvac.heating.capacity"/>;
</xsl:if><xsl:if test="hvac.heating.capacity_perF">	hvac.heating.capacity_perF <xsl:value-of select="hvac.heating.capacity_perF"/>;
</xsl:if><xsl:if test="hvac.heating.design_temperature">	hvac.heating.design_temperature <xsl:value-of select="hvac.heating.design_temperature"/>;
</xsl:if><xsl:if test="hvac.heating.efficiency">	hvac.heating.efficiency <xsl:value-of select="hvac.heating.efficiency"/>;
</xsl:if><xsl:if test="hvac.heating.cop">	hvac.heating.cop <xsl:value-of select="hvac.heating.cop"/>;
</xsl:if><xsl:if test="lights.capacity">	lights.capacity <xsl:value-of select="lights.capacity"/>;
</xsl:if><xsl:if test="lights.fraction">	lights.fraction <xsl:value-of select="lights.fraction"/>;
</xsl:if><xsl:if test="plugs.capacity">	plugs.capacity <xsl:value-of select="plugs.capacity"/>;
</xsl:if><xsl:if test="plugs.fraction">	plugs.fraction <xsl:value-of select="plugs.fraction"/>;
</xsl:if><xsl:if test="demand">	demand <xsl:value-of select="demand"/>;
</xsl:if><xsl:if test="power">	power <xsl:value-of select="power"/>;
</xsl:if><xsl:if test="energy">	energy <xsl:value-of select="energy"/>;
</xsl:if><xsl:if test="power_factor">	power_factor <xsl:value-of select="power_factor"/>;
</xsl:if><xsl:if test="hvac.demand">	hvac.demand <xsl:value-of select="hvac.demand"/>;
</xsl:if><xsl:if test="hvac.power">	hvac.power <xsl:value-of select="hvac.power"/>;
</xsl:if><xsl:if test="hvac.energy">	hvac.energy <xsl:value-of select="hvac.energy"/>;
</xsl:if><xsl:if test="hvac.power_factor">	hvac.power_factor <xsl:value-of select="hvac.power_factor"/>;
</xsl:if><xsl:if test="lights.demand">	lights.demand <xsl:value-of select="lights.demand"/>;
</xsl:if><xsl:if test="lights.power">	lights.power <xsl:value-of select="lights.power"/>;
</xsl:if><xsl:if test="lights.energy">	lights.energy <xsl:value-of select="lights.energy"/>;
</xsl:if><xsl:if test="lights.power_factor">	lights.power_factor <xsl:value-of select="lights.power_factor"/>;
</xsl:if><xsl:if test="plugs.demand">	plugs.demand <xsl:value-of select="plugs.demand"/>;
</xsl:if><xsl:if test="plugs.power">	plugs.power <xsl:value-of select="plugs.power"/>;
</xsl:if><xsl:if test="plugs.energy">	plugs.energy <xsl:value-of select="plugs.energy"/>;
</xsl:if><xsl:if test="plugs.power_factor">	plugs.power_factor <xsl:value-of select="plugs.power_factor"/>;
</xsl:if><xsl:if test="control.cooling_setpoint">	control.cooling_setpoint <xsl:value-of select="control.cooling_setpoint"/>;
</xsl:if><xsl:if test="control.heating_setpoint">	control.heating_setpoint <xsl:value-of select="control.heating_setpoint"/>;
</xsl:if><xsl:if test="control.setpoint_deadband">	control.setpoint_deadband <xsl:value-of select="control.setpoint_deadband"/>;
</xsl:if><xsl:if test="control.ventilation_fraction">	control.ventilation_fraction <xsl:value-of select="control.ventilation_fraction"/>;
</xsl:if><xsl:if test="control.lighting_fraction">	control.lighting_fraction <xsl:value-of select="control.lighting_fraction"/>;
</xsl:if>}
</xsl:for-each>
# commercial::multizone objects
<xsl:for-each select="multizone_list/multizone"><a name="#GLM.{name}"/>object multizone:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="from">	from <a href="#GLM.{from}"><xsl:value-of select="from"/></a>;
</xsl:if><xsl:if test="to">	to <a href="#GLM.{to}"><xsl:value-of select="to"/></a>;
</xsl:if><xsl:if test="ua">	ua <xsl:value-of select="ua"/>;
</xsl:if>}
</xsl:for-each></xsl:for-each><xsl:for-each select="generators">
##############################################
# generators module
module generators {
	version.major <xsl:value-of select="version.major"/>;
	version.minor <xsl:value-of select="version.minor"/>;
}

# generators::diesel_dg objects
<xsl:for-each select="diesel_dg_list/diesel_dg"><a name="#GLM.{name}"/>object diesel_dg:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="Gen_mode">	Gen_mode <xsl:value-of select="Gen_mode"/>;
</xsl:if><xsl:if test="Gen_status">	Gen_status <xsl:value-of select="Gen_status"/>;
</xsl:if><xsl:if test="Rated_kVA">	Rated_kVA <xsl:value-of select="Rated_kVA"/>;
</xsl:if><xsl:if test="Rated_kV">	Rated_kV <xsl:value-of select="Rated_kV"/>;
</xsl:if><xsl:if test="Rs">	Rs <xsl:value-of select="Rs"/>;
</xsl:if><xsl:if test="Xs">	Xs <xsl:value-of select="Xs"/>;
</xsl:if><xsl:if test="Rg">	Rg <xsl:value-of select="Rg"/>;
</xsl:if><xsl:if test="Xg">	Xg <xsl:value-of select="Xg"/>;
</xsl:if><xsl:if test="voltage_A">	voltage_A <xsl:value-of select="voltage_A"/>;
</xsl:if><xsl:if test="voltage_B">	voltage_B <xsl:value-of select="voltage_B"/>;
</xsl:if><xsl:if test="voltage_C">	voltage_C <xsl:value-of select="voltage_C"/>;
</xsl:if><xsl:if test="current_A">	current_A <xsl:value-of select="current_A"/>;
</xsl:if><xsl:if test="current_B">	current_B <xsl:value-of select="current_B"/>;
</xsl:if><xsl:if test="current_C">	current_C <xsl:value-of select="current_C"/>;
</xsl:if><xsl:if test="EfA">	EfA <xsl:value-of select="EfA"/>;
</xsl:if><xsl:if test="EfB">	EfB <xsl:value-of select="EfB"/>;
</xsl:if><xsl:if test="EfC">	EfC <xsl:value-of select="EfC"/>;
</xsl:if><xsl:if test="power_A">	power_A <xsl:value-of select="power_A"/>;
</xsl:if><xsl:if test="power_B">	power_B <xsl:value-of select="power_B"/>;
</xsl:if><xsl:if test="power_C">	power_C <xsl:value-of select="power_C"/>;
</xsl:if><xsl:if test="power_A_sch">	power_A_sch <xsl:value-of select="power_A_sch"/>;
</xsl:if><xsl:if test="power_B_sch">	power_B_sch <xsl:value-of select="power_B_sch"/>;
</xsl:if><xsl:if test="power_C_sch">	power_C_sch <xsl:value-of select="power_C_sch"/>;
</xsl:if><xsl:if test="EfA_sch">	EfA_sch <xsl:value-of select="EfA_sch"/>;
</xsl:if><xsl:if test="EfB_sch">	EfB_sch <xsl:value-of select="EfB_sch"/>;
</xsl:if><xsl:if test="EfC_sch">	EfC_sch <xsl:value-of select="EfC_sch"/>;
</xsl:if><xsl:if test="SlackBus">	SlackBus <xsl:value-of select="SlackBus"/>;
</xsl:if><xsl:if test="phases">	phases <xsl:value-of select="phases"/>;
</xsl:if>}
</xsl:for-each>
# generators::windturb_dg objects
<xsl:for-each select="windturb_dg_list/windturb_dg"><a name="#GLM.{name}"/>object windturb_dg:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="Gen_status">	Gen_status <xsl:value-of select="Gen_status"/>;
</xsl:if><xsl:if test="Gen_type">	Gen_type <xsl:value-of select="Gen_type"/>;
</xsl:if><xsl:if test="Gen_mode">	Gen_mode <xsl:value-of select="Gen_mode"/>;
</xsl:if><xsl:if test="Turbine_Model">	Turbine_Model <xsl:value-of select="Turbine_Model"/>;
</xsl:if><xsl:if test="Rated_VA">	Rated_VA <xsl:value-of select="Rated_VA"/>;
</xsl:if><xsl:if test="Rated_V">	Rated_V <xsl:value-of select="Rated_V"/>;
</xsl:if><xsl:if test="Pconv">	Pconv <xsl:value-of select="Pconv"/>;
</xsl:if><xsl:if test="WSadj">	WSadj <xsl:value-of select="WSadj"/>;
</xsl:if><xsl:if test="Wind_Speed">	Wind_Speed <xsl:value-of select="Wind_Speed"/>;
</xsl:if><xsl:if test="pf">	pf <xsl:value-of select="pf"/>;
</xsl:if><xsl:if test="GenElecEff">	GenElecEff <xsl:value-of select="GenElecEff"/>;
</xsl:if><xsl:if test="TotalRealPow">	TotalRealPow <xsl:value-of select="TotalRealPow"/>;
</xsl:if><xsl:if test="TotalReacPow">	TotalReacPow <xsl:value-of select="TotalReacPow"/>;
</xsl:if><xsl:if test="voltage_A">	voltage_A <xsl:value-of select="voltage_A"/>;
</xsl:if><xsl:if test="voltage_B">	voltage_B <xsl:value-of select="voltage_B"/>;
</xsl:if><xsl:if test="voltage_C">	voltage_C <xsl:value-of select="voltage_C"/>;
</xsl:if><xsl:if test="current_A">	current_A <xsl:value-of select="current_A"/>;
</xsl:if><xsl:if test="current_B">	current_B <xsl:value-of select="current_B"/>;
</xsl:if><xsl:if test="current_C">	current_C <xsl:value-of select="current_C"/>;
</xsl:if><xsl:if test="EfA">	EfA <xsl:value-of select="EfA"/>;
</xsl:if><xsl:if test="EfB">	EfB <xsl:value-of select="EfB"/>;
</xsl:if><xsl:if test="EfC">	EfC <xsl:value-of select="EfC"/>;
</xsl:if><xsl:if test="Vrotor_A">	Vrotor_A <xsl:value-of select="Vrotor_A"/>;
</xsl:if><xsl:if test="Vrotor_B">	Vrotor_B <xsl:value-of select="Vrotor_B"/>;
</xsl:if><xsl:if test="Vrotor_C">	Vrotor_C <xsl:value-of select="Vrotor_C"/>;
</xsl:if><xsl:if test="Irotor_A">	Irotor_A <xsl:value-of select="Irotor_A"/>;
</xsl:if><xsl:if test="Irotor_B">	Irotor_B <xsl:value-of select="Irotor_B"/>;
</xsl:if><xsl:if test="Irotor_C">	Irotor_C <xsl:value-of select="Irotor_C"/>;
</xsl:if><xsl:if test="phases">	phases <xsl:value-of select="phases"/>;
</xsl:if>}
</xsl:for-each>
# generators::power_electronics objects
<xsl:for-each select="power_electronics_list/power_electronics"><a name="#GLM.{name}"/>object power_electronics:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="GENERATOR_MODE">	GENERATOR_MODE <xsl:value-of select="GENERATOR_MODE"/>;
</xsl:if><xsl:if test="GENERATOR_STATUS">	GENERATOR_STATUS <xsl:value-of select="GENERATOR_STATUS"/>;
</xsl:if><xsl:if test="CONVERTER_TYPE">	CONVERTER_TYPE <xsl:value-of select="CONVERTER_TYPE"/>;
</xsl:if><xsl:if test="SWITCH_TYPE">	SWITCH_TYPE <xsl:value-of select="SWITCH_TYPE"/>;
</xsl:if><xsl:if test="FILTER_TYPE">	FILTER_TYPE <xsl:value-of select="FILTER_TYPE"/>;
</xsl:if><xsl:if test="FILTER_IMPLEMENTATION">	FILTER_IMPLEMENTATION <xsl:value-of select="FILTER_IMPLEMENTATION"/>;
</xsl:if><xsl:if test="FILTER_FREQUENCY">	FILTER_FREQUENCY <xsl:value-of select="FILTER_FREQUENCY"/>;
</xsl:if><xsl:if test="POWER_TYPE">	POWER_TYPE <xsl:value-of select="POWER_TYPE"/>;
</xsl:if><xsl:if test="Rated_kW">	Rated_kW <xsl:value-of select="Rated_kW"/>;
</xsl:if><xsl:if test="Max_P">	Max_P <xsl:value-of select="Max_P"/>;
</xsl:if><xsl:if test="Min_P">	Min_P <xsl:value-of select="Min_P"/>;
</xsl:if><xsl:if test="Rated_kVA">	Rated_kVA <xsl:value-of select="Rated_kVA"/>;
</xsl:if><xsl:if test="Rated_kV">	Rated_kV <xsl:value-of select="Rated_kV"/>;
</xsl:if><xsl:if test="phases">	phases <xsl:value-of select="phases"/>;
</xsl:if>}
</xsl:for-each>
# generators::energy_storage objects
<xsl:for-each select="energy_storage_list/energy_storage"><a name="#GLM.{name}"/>object energy_storage:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="GENERATOR_MODE">	GENERATOR_MODE <xsl:value-of select="GENERATOR_MODE"/>;
</xsl:if><xsl:if test="GENERATOR_STATUS">	GENERATOR_STATUS <xsl:value-of select="GENERATOR_STATUS"/>;
</xsl:if><xsl:if test="POWER_TYPE">	POWER_TYPE <xsl:value-of select="POWER_TYPE"/>;
</xsl:if><xsl:if test="Rinternal">	Rinternal <xsl:value-of select="Rinternal"/>;
</xsl:if><xsl:if test="V_Max">	V_Max <xsl:value-of select="V_Max"/>;
</xsl:if><xsl:if test="I_Max">	I_Max <xsl:value-of select="I_Max"/>;
</xsl:if><xsl:if test="E_Max">	E_Max <xsl:value-of select="E_Max"/>;
</xsl:if><xsl:if test="Energy">	Energy <xsl:value-of select="Energy"/>;
</xsl:if><xsl:if test="efficiency">	efficiency <xsl:value-of select="efficiency"/>;
</xsl:if><xsl:if test="Rated_kVA">	Rated_kVA <xsl:value-of select="Rated_kVA"/>;
</xsl:if><xsl:if test="V_Out">	V_Out <xsl:value-of select="V_Out"/>;
</xsl:if><xsl:if test="I_Out">	I_Out <xsl:value-of select="I_Out"/>;
</xsl:if><xsl:if test="VA_Out">	VA_Out <xsl:value-of select="VA_Out"/>;
</xsl:if><xsl:if test="V_In">	V_In <xsl:value-of select="V_In"/>;
</xsl:if><xsl:if test="I_In">	I_In <xsl:value-of select="I_In"/>;
</xsl:if><xsl:if test="V_Internal">	V_Internal <xsl:value-of select="V_Internal"/>;
</xsl:if><xsl:if test="I_Internal">	I_Internal <xsl:value-of select="I_Internal"/>;
</xsl:if><xsl:if test="I_Prev">	I_Prev <xsl:value-of select="I_Prev"/>;
</xsl:if><xsl:if test="phases">	phases <xsl:value-of select="phases"/>;
</xsl:if>}
</xsl:for-each>
# generators::inverter objects
<xsl:for-each select="inverter_list/inverter"><a name="#GLM.{name}"/>object inverter:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="INVERTER_TYPE">	INVERTER_TYPE <xsl:value-of select="INVERTER_TYPE"/>;
</xsl:if><xsl:if test="GENERATOR_MODE">	GENERATOR_MODE <xsl:value-of select="GENERATOR_MODE"/>;
</xsl:if><xsl:if test="V_In">	V_In <xsl:value-of select="V_In"/>;
</xsl:if><xsl:if test="I_In">	I_In <xsl:value-of select="I_In"/>;
</xsl:if><xsl:if test="VA_In">	VA_In <xsl:value-of select="VA_In"/>;
</xsl:if><xsl:if test="Vdc">	Vdc <xsl:value-of select="Vdc"/>;
</xsl:if><xsl:if test="phaseA_V_Out">	phaseA_V_Out <xsl:value-of select="phaseA_V_Out"/>;
</xsl:if><xsl:if test="phaseB_V_Out">	phaseB_V_Out <xsl:value-of select="phaseB_V_Out"/>;
</xsl:if><xsl:if test="phaseC_V_Out">	phaseC_V_Out <xsl:value-of select="phaseC_V_Out"/>;
</xsl:if><xsl:if test="phaseA_I_Out">	phaseA_I_Out <xsl:value-of select="phaseA_I_Out"/>;
</xsl:if><xsl:if test="phaseB_I_Out">	phaseB_I_Out <xsl:value-of select="phaseB_I_Out"/>;
</xsl:if><xsl:if test="phaseC_I_Out">	phaseC_I_Out <xsl:value-of select="phaseC_I_Out"/>;
</xsl:if><xsl:if test="power_A">	power_A <xsl:value-of select="power_A"/>;
</xsl:if><xsl:if test="power_B">	power_B <xsl:value-of select="power_B"/>;
</xsl:if><xsl:if test="power_C">	power_C <xsl:value-of select="power_C"/>;
</xsl:if><xsl:if test="phases">	phases <xsl:value-of select="phases"/>;
</xsl:if>}
</xsl:for-each>
# generators::dc_dc_converter objects
<xsl:for-each select="dc_dc_converter_list/dc_dc_converter"><a name="#GLM.{name}"/>object dc_dc_converter:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="DC_DC_CONVERTER_TYPE">	DC_DC_CONVERTER_TYPE <xsl:value-of select="DC_DC_CONVERTER_TYPE"/>;
</xsl:if><xsl:if test="GENERATOR_MODE">	GENERATOR_MODE <xsl:value-of select="GENERATOR_MODE"/>;
</xsl:if><xsl:if test="V_Out">	V_Out <xsl:value-of select="V_Out"/>;
</xsl:if><xsl:if test="I_Out">	I_Out <xsl:value-of select="I_Out"/>;
</xsl:if><xsl:if test="Vdc">	Vdc <xsl:value-of select="Vdc"/>;
</xsl:if><xsl:if test="VA_Out">	VA_Out <xsl:value-of select="VA_Out"/>;
</xsl:if><xsl:if test="P_Out">	P_Out <xsl:value-of select="P_Out"/>;
</xsl:if><xsl:if test="Q_Out">	Q_Out <xsl:value-of select="Q_Out"/>;
</xsl:if><xsl:if test="service_ratio">	service_ratio <xsl:value-of select="service_ratio"/>;
</xsl:if><xsl:if test="V_In">	V_In <xsl:value-of select="V_In"/>;
</xsl:if><xsl:if test="I_In">	I_In <xsl:value-of select="I_In"/>;
</xsl:if><xsl:if test="VA_In">	VA_In <xsl:value-of select="VA_In"/>;
</xsl:if><xsl:if test="phases">	phases <xsl:value-of select="phases"/>;
</xsl:if>}
</xsl:for-each>
# generators::rectifier objects
<xsl:for-each select="rectifier_list/rectifier"><a name="#GLM.{name}"/>object rectifier:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="RECTIFIER_TYPE">	RECTIFIER_TYPE <xsl:value-of select="RECTIFIER_TYPE"/>;
</xsl:if><xsl:if test="GENERATOR_MODE">	GENERATOR_MODE <xsl:value-of select="GENERATOR_MODE"/>;
</xsl:if><xsl:if test="V_Out">	V_Out <xsl:value-of select="V_Out"/>;
</xsl:if><xsl:if test="I_Out">	I_Out <xsl:value-of select="I_Out"/>;
</xsl:if><xsl:if test="VA_Out">	VA_Out <xsl:value-of select="VA_Out"/>;
</xsl:if><xsl:if test="P_Out">	P_Out <xsl:value-of select="P_Out"/>;
</xsl:if><xsl:if test="Q_Out">	Q_Out <xsl:value-of select="Q_Out"/>;
</xsl:if><xsl:if test="Vdc">	Vdc <xsl:value-of select="Vdc"/>;
</xsl:if><xsl:if test="phaseA_V_In">	phaseA_V_In <xsl:value-of select="phaseA_V_In"/>;
</xsl:if><xsl:if test="phaseB_V_In">	phaseB_V_In <xsl:value-of select="phaseB_V_In"/>;
</xsl:if><xsl:if test="phaseC_V_In">	phaseC_V_In <xsl:value-of select="phaseC_V_In"/>;
</xsl:if><xsl:if test="phaseA_I_In">	phaseA_I_In <xsl:value-of select="phaseA_I_In"/>;
</xsl:if><xsl:if test="phaseB_I_In">	phaseB_I_In <xsl:value-of select="phaseB_I_In"/>;
</xsl:if><xsl:if test="phaseC_I_In">	phaseC_I_In <xsl:value-of select="phaseC_I_In"/>;
</xsl:if><xsl:if test="power_A_In">	power_A_In <xsl:value-of select="power_A_In"/>;
</xsl:if><xsl:if test="power_B_In">	power_B_In <xsl:value-of select="power_B_In"/>;
</xsl:if><xsl:if test="power_C_In">	power_C_In <xsl:value-of select="power_C_In"/>;
</xsl:if><xsl:if test="phases">	phases <xsl:value-of select="phases"/>;
</xsl:if>}
</xsl:for-each>
# generators::microturbine objects
<xsl:for-each select="microturbine_list/microturbine"><a name="#GLM.{name}"/>object microturbine:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="GENERATOR_MODE">	GENERATOR_MODE <xsl:value-of select="GENERATOR_MODE"/>;
</xsl:if><xsl:if test="GENERATOR_STATUS">	GENERATOR_STATUS <xsl:value-of select="GENERATOR_STATUS"/>;
</xsl:if><xsl:if test="POWER_TYPE">	POWER_TYPE <xsl:value-of select="POWER_TYPE"/>;
</xsl:if><xsl:if test="Rinternal">	Rinternal <xsl:value-of select="Rinternal"/>;
</xsl:if><xsl:if test="Rload">	Rload <xsl:value-of select="Rload"/>;
</xsl:if><xsl:if test="V_Max">	V_Max <xsl:value-of select="V_Max"/>;
</xsl:if><xsl:if test="I_Max">	I_Max <xsl:value-of select="I_Max"/>;
</xsl:if><xsl:if test="frequency">	frequency <xsl:value-of select="frequency"/>;
</xsl:if><xsl:if test="Max_Frequency">	Max_Frequency <xsl:value-of select="Max_Frequency"/>;
</xsl:if><xsl:if test="Min_Frequency">	Min_Frequency <xsl:value-of select="Min_Frequency"/>;
</xsl:if><xsl:if test="Fuel_Used">	Fuel_Used <xsl:value-of select="Fuel_Used"/>;
</xsl:if><xsl:if test="Heat_Out">	Heat_Out <xsl:value-of select="Heat_Out"/>;
</xsl:if><xsl:if test="KV">	KV <xsl:value-of select="KV"/>;
</xsl:if><xsl:if test="Power_Angle">	Power_Angle <xsl:value-of select="Power_Angle"/>;
</xsl:if><xsl:if test="Max_P">	Max_P <xsl:value-of select="Max_P"/>;
</xsl:if><xsl:if test="Min_P">	Min_P <xsl:value-of select="Min_P"/>;
</xsl:if><xsl:if test="phaseA_V_Out">	phaseA_V_Out <xsl:value-of select="phaseA_V_Out"/>;
</xsl:if><xsl:if test="phaseB_V_Out">	phaseB_V_Out <xsl:value-of select="phaseB_V_Out"/>;
</xsl:if><xsl:if test="phaseC_V_Out">	phaseC_V_Out <xsl:value-of select="phaseC_V_Out"/>;
</xsl:if><xsl:if test="phaseA_I_Out">	phaseA_I_Out <xsl:value-of select="phaseA_I_Out"/>;
</xsl:if><xsl:if test="phaseB_I_Out">	phaseB_I_Out <xsl:value-of select="phaseB_I_Out"/>;
</xsl:if><xsl:if test="phaseC_I_Out">	phaseC_I_Out <xsl:value-of select="phaseC_I_Out"/>;
</xsl:if><xsl:if test="power_A_Out">	power_A_Out <xsl:value-of select="power_A_Out"/>;
</xsl:if><xsl:if test="power_B_Out">	power_B_Out <xsl:value-of select="power_B_Out"/>;
</xsl:if><xsl:if test="power_C_Out">	power_C_Out <xsl:value-of select="power_C_Out"/>;
</xsl:if><xsl:if test="VA_Out">	VA_Out <xsl:value-of select="VA_Out"/>;
</xsl:if><xsl:if test="pf_Out">	pf_Out <xsl:value-of select="pf_Out"/>;
</xsl:if><xsl:if test="E_A_Internal">	E_A_Internal <xsl:value-of select="E_A_Internal"/>;
</xsl:if><xsl:if test="E_B_Internal">	E_B_Internal <xsl:value-of select="E_B_Internal"/>;
</xsl:if><xsl:if test="E_C_Internal">	E_C_Internal <xsl:value-of select="E_C_Internal"/>;
</xsl:if><xsl:if test="efficiency">	efficiency <xsl:value-of select="efficiency"/>;
</xsl:if><xsl:if test="Rated_kVA">	Rated_kVA <xsl:value-of select="Rated_kVA"/>;
</xsl:if><xsl:if test="VA_Out">	VA_Out <xsl:value-of select="VA_Out"/>;
</xsl:if><xsl:if test="phases">	phases <xsl:value-of select="phases"/>;
</xsl:if>}
</xsl:for-each>
# generators::battery objects
<xsl:for-each select="battery_list/battery"><a name="#GLM.{name}"/>object battery:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="GENERATOR_MODE">	GENERATOR_MODE <xsl:value-of select="GENERATOR_MODE"/>;
</xsl:if><xsl:if test="GENERATOR_STATUS">	GENERATOR_STATUS <xsl:value-of select="GENERATOR_STATUS"/>;
</xsl:if><xsl:if test="RFB_SIZE">	RFB_SIZE <xsl:value-of select="RFB_SIZE"/>;
</xsl:if><xsl:if test="POWER_TYPE">	POWER_TYPE <xsl:value-of select="POWER_TYPE"/>;
</xsl:if><xsl:if test="Rinternal">	Rinternal <xsl:value-of select="Rinternal"/>;
</xsl:if><xsl:if test="V_Max">	V_Max <xsl:value-of select="V_Max"/>;
</xsl:if><xsl:if test="I_Max">	I_Max <xsl:value-of select="I_Max"/>;
</xsl:if><xsl:if test="E_Max">	E_Max <xsl:value-of select="E_Max"/>;
</xsl:if><xsl:if test="Energy">	Energy <xsl:value-of select="Energy"/>;
</xsl:if><xsl:if test="efficiency">	efficiency <xsl:value-of select="efficiency"/>;
</xsl:if><xsl:if test="base_efficiency">	base_efficiency <xsl:value-of select="base_efficiency"/>;
</xsl:if><xsl:if test="Rated_kVA">	Rated_kVA <xsl:value-of select="Rated_kVA"/>;
</xsl:if><xsl:if test="V_Out">	V_Out <xsl:value-of select="V_Out"/>;
</xsl:if><xsl:if test="I_Out">	I_Out <xsl:value-of select="I_Out"/>;
</xsl:if><xsl:if test="VA_Out">	VA_Out <xsl:value-of select="VA_Out"/>;
</xsl:if><xsl:if test="V_In">	V_In <xsl:value-of select="V_In"/>;
</xsl:if><xsl:if test="I_In">	I_In <xsl:value-of select="I_In"/>;
</xsl:if><xsl:if test="V_Internal">	V_Internal <xsl:value-of select="V_Internal"/>;
</xsl:if><xsl:if test="I_Internal">	I_Internal <xsl:value-of select="I_Internal"/>;
</xsl:if><xsl:if test="I_Prev">	I_Prev <xsl:value-of select="I_Prev"/>;
</xsl:if><xsl:if test="phases">	phases <xsl:value-of select="phases"/>;
</xsl:if>}
</xsl:for-each>
# generators::solar objects
<xsl:for-each select="solar_list/solar"><a name="#GLM.{name}"/>object solar:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="GENERATOR_MODE">	GENERATOR_MODE <xsl:value-of select="GENERATOR_MODE"/>;
</xsl:if><xsl:if test="GENERATOR_STATUS">	GENERATOR_STATUS <xsl:value-of select="GENERATOR_STATUS"/>;
</xsl:if><xsl:if test="PANEL_TYPE">	PANEL_TYPE <xsl:value-of select="PANEL_TYPE"/>;
</xsl:if><xsl:if test="POWER_TYPE">	POWER_TYPE <xsl:value-of select="POWER_TYPE"/>;
</xsl:if><xsl:if test="NOCT">	NOCT <xsl:value-of select="NOCT"/>;
</xsl:if><xsl:if test="Tcell">	Tcell <xsl:value-of select="Tcell"/>;
</xsl:if><xsl:if test="Tambient">	Tambient <xsl:value-of select="Tambient"/>;
</xsl:if><xsl:if test="Insolation">	Insolation <xsl:value-of select="Insolation"/>;
</xsl:if><xsl:if test="Rinternal">	Rinternal <xsl:value-of select="Rinternal"/>;
</xsl:if><xsl:if test="Rated_Insolation">	Rated_Insolation <xsl:value-of select="Rated_Insolation"/>;
</xsl:if><xsl:if test="V_Max">	V_Max <xsl:value-of select="V_Max"/>;
</xsl:if><xsl:if test="Voc_Max">	Voc_Max <xsl:value-of select="Voc_Max"/>;
</xsl:if><xsl:if test="Voc">	Voc <xsl:value-of select="Voc"/>;
</xsl:if><xsl:if test="efficiency">	efficiency <xsl:value-of select="efficiency"/>;
</xsl:if><xsl:if test="area">	area <xsl:value-of select="area"/>;
</xsl:if><xsl:if test="Rated_kVA">	Rated_kVA <xsl:value-of select="Rated_kVA"/>;
</xsl:if><xsl:if test="V_Out">	V_Out <xsl:value-of select="V_Out"/>;
</xsl:if><xsl:if test="I_Out">	I_Out <xsl:value-of select="I_Out"/>;
</xsl:if><xsl:if test="VA_Out">	VA_Out <xsl:value-of select="VA_Out"/>;
</xsl:if><xsl:if test="phases">	phases <xsl:value-of select="phases"/>;
</xsl:if>}
</xsl:for-each></xsl:for-each><xsl:for-each select="market">
##############################################
# market module
module market {
	version.major <xsl:value-of select="version.major"/>;
	version.minor <xsl:value-of select="version.minor"/>;
}

# market::auction objects
<xsl:for-each select="auction_list/auction"><a name="#GLM.{name}"/>object auction:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="type">	type <xsl:value-of select="type"/>;
</xsl:if><xsl:if test="unit">	unit <xsl:value-of select="unit"/>;
</xsl:if><xsl:if test="period">	period <xsl:value-of select="period"/>;
</xsl:if><xsl:if test="latency">	latency <xsl:value-of select="latency"/>;
</xsl:if><xsl:if test="market_id">	market_id <xsl:value-of select="market_id"/>;
</xsl:if><xsl:if test="last.Q">	last.Q <xsl:value-of select="last.Q"/>;
</xsl:if><xsl:if test="last.P">	last.P <xsl:value-of select="last.P"/>;
</xsl:if><xsl:if test="next.Q">	next.Q <xsl:value-of select="next.Q"/>;
</xsl:if><xsl:if test="next.P">	next.P <xsl:value-of select="next.P"/>;
</xsl:if><xsl:if test="network">	network <a href="#GLM.{network}"><xsl:value-of select="network"/></a>;
</xsl:if>}
</xsl:for-each></xsl:for-each><xsl:for-each select="network">
##############################################
# network module
module network {
	version.major <xsl:value-of select="version.major"/>;
	version.minor <xsl:value-of select="version.minor"/>;
	acceleration_factor <xsl:value-of select="acceleration_factor"/>;
	convergence_limit <xsl:value-of select="convergence_limit"/>;
	mvabase <xsl:value-of select="mvabase"/>;
	kvbase <xsl:value-of select="kvbase"/>;
	model_year <xsl:value-of select="model_year"/>;
	model_case <xsl:value-of select="model_case"/>;
	model_name <xsl:value-of select="model_name"/>;
}

# network::node objects
<xsl:for-each select="node_list/node"><a name="#GLM.{name}"/>object node:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="V">	V <xsl:value-of select="V"/>;
</xsl:if><xsl:if test="S">	S <xsl:value-of select="S"/>;
</xsl:if><xsl:if test="G">	G <xsl:value-of select="G"/>;
</xsl:if><xsl:if test="B">	B <xsl:value-of select="B"/>;
</xsl:if><xsl:if test="Qmax_MVAR">	Qmax_MVAR <xsl:value-of select="Qmax_MVAR"/>;
</xsl:if><xsl:if test="Qmin_MVAR">	Qmin_MVAR <xsl:value-of select="Qmin_MVAR"/>;
</xsl:if><xsl:if test="type">	type <xsl:value-of select="type"/>;
</xsl:if><xsl:if test="flow_area_num">	flow_area_num <xsl:value-of select="flow_area_num"/>;
</xsl:if><xsl:if test="base_kV">	base_kV <xsl:value-of select="base_kV"/>;
</xsl:if>}
</xsl:for-each>
# network::link objects
<xsl:for-each select="link_list/link"><a name="#GLM.{name}"/>object link:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="Y">	Y <xsl:value-of select="Y"/>;
</xsl:if><xsl:if test="I">	I <xsl:value-of select="I"/>;
</xsl:if><xsl:if test="B">	B <xsl:value-of select="B"/>;
</xsl:if><xsl:if test="from">	from <a href="#GLM.{from}"><xsl:value-of select="from"/></a>;
</xsl:if><xsl:if test="to">	to <a href="#GLM.{to}"><xsl:value-of select="to"/></a>;
</xsl:if>}
</xsl:for-each>
# network::capbank objects
<xsl:for-each select="capbank_list/capbank"><a name="#GLM.{name}"/>object capbank:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="KVARrated">	KVARrated <xsl:value-of select="KVARrated"/>;
</xsl:if><xsl:if test="Vrated">	Vrated <xsl:value-of select="Vrated"/>;
</xsl:if><xsl:if test="state">	state <xsl:value-of select="state"/>;
</xsl:if><xsl:if test="CTlink">	CTlink <a href="#GLM.{CTlink}"><xsl:value-of select="CTlink"/></a>;
</xsl:if><xsl:if test="PTnode">	PTnode <a href="#GLM.{PTnode}"><xsl:value-of select="PTnode"/></a>;
</xsl:if><xsl:if test="VARopen">	VARopen <xsl:value-of select="VARopen"/>;
</xsl:if><xsl:if test="VARclose">	VARclose <xsl:value-of select="VARclose"/>;
</xsl:if><xsl:if test="Vopen">	Vopen <xsl:value-of select="Vopen"/>;
</xsl:if><xsl:if test="Vclose">	Vclose <xsl:value-of select="Vclose"/>;
</xsl:if>}
</xsl:for-each>
# network::fuse objects
<xsl:for-each select="fuse_list/fuse"><a name="#GLM.{name}"/>object fuse:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="TimeConstant">	TimeConstant <xsl:value-of select="TimeConstant"/>;
</xsl:if><xsl:if test="SetCurrent">	SetCurrent <xsl:value-of select="SetCurrent"/>;
</xsl:if><xsl:if test="SetBase">	SetBase <xsl:value-of select="SetBase"/>;
</xsl:if><xsl:if test="SetScale">	SetScale <xsl:value-of select="SetScale"/>;
</xsl:if><xsl:if test="SetCurve">	SetCurve <xsl:value-of select="SetCurve"/>;
</xsl:if><xsl:if test="TresetAvg">	TresetAvg <xsl:value-of select="TresetAvg"/>;
</xsl:if><xsl:if test="TresetStd">	TresetStd <xsl:value-of select="TresetStd"/>;
</xsl:if><xsl:if test="State">	State <xsl:value-of select="State"/>;
</xsl:if>}
</xsl:for-each>
# network::relay objects
<xsl:for-each select="relay_list/relay"><a name="#GLM.{name}"/>object relay:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="Curve">	Curve <xsl:value-of select="Curve"/>;
</xsl:if><xsl:if test="TimeDial">	TimeDial <xsl:value-of select="TimeDial"/>;
</xsl:if><xsl:if test="SetCurrent">	SetCurrent <xsl:value-of select="SetCurrent"/>;
</xsl:if><xsl:if test="State">	State <xsl:value-of select="State"/>;
</xsl:if>}
</xsl:for-each>
# network::regulator objects
<xsl:for-each select="regulator_list/regulator"><a name="#GLM.{name}"/>object regulator:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="Type">	Type <xsl:value-of select="Type"/>;
</xsl:if><xsl:if test="Vmax">	Vmax <xsl:value-of select="Vmax"/>;
</xsl:if><xsl:if test="Vmin">	Vmin <xsl:value-of select="Vmin"/>;
</xsl:if><xsl:if test="Vstep">	Vstep <xsl:value-of select="Vstep"/>;
</xsl:if><xsl:if test="CTlink">	CTlink <a href="#GLM.{CTlink}"><xsl:value-of select="CTlink"/></a>;
</xsl:if><xsl:if test="PTbus">	PTbus <a href="#GLM.{PTbus}"><xsl:value-of select="PTbus"/></a>;
</xsl:if><xsl:if test="TimeDelay">	TimeDelay <xsl:value-of select="TimeDelay"/>;
</xsl:if>}
</xsl:for-each>
# network::transformer objects
<xsl:for-each select="transformer_list/transformer"><a name="#GLM.{name}"/>object transformer:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="Type">	Type <xsl:value-of select="Type"/>;
</xsl:if><xsl:if test="Sbase">	Sbase <xsl:value-of select="Sbase"/>;
</xsl:if><xsl:if test="Vbase">	Vbase <xsl:value-of select="Vbase"/>;
</xsl:if><xsl:if test="Zpu">	Zpu <xsl:value-of select="Zpu"/>;
</xsl:if><xsl:if test="Vprimary">	Vprimary <xsl:value-of select="Vprimary"/>;
</xsl:if><xsl:if test="Vsecondary">	Vsecondary <xsl:value-of select="Vsecondary"/>;
</xsl:if>}
</xsl:for-each>
# network::meter objects
<xsl:for-each select="meter_list/meter"><a name="#GLM.{name}"/>object meter:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="type">	type <xsl:value-of select="type"/>;
</xsl:if><xsl:if test="demand">	demand <xsl:value-of select="demand"/>;
</xsl:if><xsl:if test="meter">	meter <xsl:value-of select="meter"/>;
</xsl:if><xsl:if test="line1_current">	line1_current <xsl:value-of select="line1_current"/>;
</xsl:if><xsl:if test="line2_current">	line2_current <xsl:value-of select="line2_current"/>;
</xsl:if><xsl:if test="line3_current">	line3_current <xsl:value-of select="line3_current"/>;
</xsl:if><xsl:if test="line1_admittance">	line1_admittance <xsl:value-of select="line1_admittance"/>;
</xsl:if><xsl:if test="line2_admittance">	line2_admittance <xsl:value-of select="line2_admittance"/>;
</xsl:if><xsl:if test="line3_admittance">	line3_admittance <xsl:value-of select="line3_admittance"/>;
</xsl:if><xsl:if test="line1_power">	line1_power <xsl:value-of select="line1_power"/>;
</xsl:if><xsl:if test="line2_power">	line2_power <xsl:value-of select="line2_power"/>;
</xsl:if><xsl:if test="line3_power">	line3_power <xsl:value-of select="line3_power"/>;
</xsl:if><xsl:if test="line1_volts">	line1_volts <xsl:value-of select="line1_volts"/>;
</xsl:if><xsl:if test="line2_volts">	line2_volts <xsl:value-of select="line2_volts"/>;
</xsl:if><xsl:if test="line3_volts">	line3_volts <xsl:value-of select="line3_volts"/>;
</xsl:if>}
</xsl:for-each>
# network::generator objects
<xsl:for-each select="generator_list/generator"><a name="#GLM.{name}"/>object generator:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="Pdesired_MW">	Pdesired_MW <xsl:value-of select="Pdesired_MW"/>;
</xsl:if><xsl:if test="Qdesired_MVAR">	Qdesired_MVAR <xsl:value-of select="Qdesired_MVAR"/>;
</xsl:if><xsl:if test="Qcontrolled">	Qcontrolled <xsl:value-of select="Qcontrolled"/>;
</xsl:if><xsl:if test="Pmax_MW">	Pmax_MW <xsl:value-of select="Pmax_MW"/>;
</xsl:if><xsl:if test="Qmin_MVAR">	Qmin_MVAR <xsl:value-of select="Qmin_MVAR"/>;
</xsl:if><xsl:if test="Qmax_MVAR">	Qmax_MVAR <xsl:value-of select="Qmax_MVAR"/>;
</xsl:if><xsl:if test="QVa">	QVa <xsl:value-of select="QVa"/>;
</xsl:if><xsl:if test="QVb">	QVb <xsl:value-of select="QVb"/>;
</xsl:if><xsl:if test="QVc">	QVc <xsl:value-of select="QVc"/>;
</xsl:if><xsl:if test="state">	state <xsl:value-of select="state"/>;
</xsl:if>}
</xsl:for-each></xsl:for-each><xsl:for-each select="plc">
##############################################
# plc module
module plc {
	version.major <xsl:value-of select="version.major"/>;
	version.minor <xsl:value-of select="version.minor"/>;
	libpath <xsl:value-of select="libpath"/>;
	incpath <xsl:value-of select="incpath"/>;
}

# plc::plc objects
<xsl:for-each select="plc_list/plc"><a name="#GLM.{name}"/>object plc:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="source">	source <xsl:value-of select="source"/>;
</xsl:if><xsl:if test="network">	network <a href="#GLM.{network}"><xsl:value-of select="network"/></a>;
</xsl:if>}
</xsl:for-each>
# plc::comm objects
<xsl:for-each select="comm_list/comm"><a name="#GLM.{name}"/>object comm:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="latency">	latency <xsl:value-of select="latency"/>;
</xsl:if><xsl:if test="reliability">	reliability <xsl:value-of select="reliability"/>;
</xsl:if><xsl:if test="bitrate">	bitrate <xsl:value-of select="bitrate"/>;
</xsl:if><xsl:if test="timeout">	timeout <xsl:value-of select="timeout"/>;
</xsl:if>}
</xsl:for-each></xsl:for-each><xsl:for-each select="powerflow">
##############################################
# powerflow module
module powerflow {
	version.major <xsl:value-of select="version.major"/>;
	version.minor <xsl:value-of select="version.minor"/>;
	show_matrix_values <xsl:value-of select="show_matrix_values"/>;
	primary_voltage_ratio <xsl:value-of select="primary_voltage_ratio"/>;
	nominal_frequency <xsl:value-of select="nominal_frequency"/>;
	require_voltage_control <xsl:value-of select="require_voltage_control"/>;
	geographic_degree <xsl:value-of select="geographic_degree"/>;
	fault_impedance <xsl:value-of select="fault_impedance"/>;
	warning_underfrequency <xsl:value-of select="warning_underfrequency"/>;
	warning_overfrequency <xsl:value-of select="warning_overfrequency"/>;
	warning_undervoltage <xsl:value-of select="warning_undervoltage"/>;
	warning_overvoltage <xsl:value-of select="warning_overvoltage"/>;
	warning_voltageangle <xsl:value-of select="warning_voltageangle"/>;
	maximum_voltage_error <xsl:value-of select="maximum_voltage_error"/>;
	solver_method <xsl:value-of select="solver_method"/>;
}

# powerflow::node objects
<xsl:for-each select="node_list/node"><a name="#GLM.{name}"/>object node:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="bustype">	bustype <xsl:value-of select="bustype"/>;
</xsl:if><xsl:if test="busflags">	busflags <xsl:value-of select="busflags"/>;
</xsl:if><xsl:if test="reference_bus">	reference_bus <a href="#GLM.{reference_bus}"><xsl:value-of select="reference_bus"/></a>;
</xsl:if><xsl:if test="maximum_voltage_error">	maximum_voltage_error <xsl:value-of select="maximum_voltage_error"/>;
</xsl:if><xsl:if test="voltage_A">	voltage_A <xsl:value-of select="voltage_A"/>;
</xsl:if><xsl:if test="voltage_B">	voltage_B <xsl:value-of select="voltage_B"/>;
</xsl:if><xsl:if test="voltage_C">	voltage_C <xsl:value-of select="voltage_C"/>;
</xsl:if><xsl:if test="voltage_AB">	voltage_AB <xsl:value-of select="voltage_AB"/>;
</xsl:if><xsl:if test="voltage_BC">	voltage_BC <xsl:value-of select="voltage_BC"/>;
</xsl:if><xsl:if test="voltage_CA">	voltage_CA <xsl:value-of select="voltage_CA"/>;
</xsl:if><xsl:if test="current_A">	current_A <xsl:value-of select="current_A"/>;
</xsl:if><xsl:if test="current_B">	current_B <xsl:value-of select="current_B"/>;
</xsl:if><xsl:if test="current_C">	current_C <xsl:value-of select="current_C"/>;
</xsl:if><xsl:if test="power_A">	power_A <xsl:value-of select="power_A"/>;
</xsl:if><xsl:if test="power_B">	power_B <xsl:value-of select="power_B"/>;
</xsl:if><xsl:if test="power_C">	power_C <xsl:value-of select="power_C"/>;
</xsl:if><xsl:if test="shunt_A">	shunt_A <xsl:value-of select="shunt_A"/>;
</xsl:if><xsl:if test="shunt_B">	shunt_B <xsl:value-of select="shunt_B"/>;
</xsl:if><xsl:if test="shunt_C">	shunt_C <xsl:value-of select="shunt_C"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::link objects
<xsl:for-each select="link_list/link"><a name="#GLM.{name}"/>object link:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="status">	status <xsl:value-of select="status"/>;
</xsl:if><xsl:if test="from">	from <a href="#GLM.{from}"><xsl:value-of select="from"/></a>;
</xsl:if><xsl:if test="to">	to <a href="#GLM.{to}"><xsl:value-of select="to"/></a>;
</xsl:if><xsl:if test="power_in">	power_in <xsl:value-of select="power_in"/>;
</xsl:if><xsl:if test="power_out">	power_out <xsl:value-of select="power_out"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::capacitor objects
<xsl:for-each select="capacitor_list/capacitor"><a name="#GLM.{name}"/>object capacitor:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="pt_phase">	pt_phase <xsl:value-of select="pt_phase"/>;
</xsl:if><xsl:if test="switch">	switch <xsl:value-of select="switch"/>;
</xsl:if><xsl:if test="control">	control <xsl:value-of select="control"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::fuse objects
<xsl:for-each select="fuse_list/fuse"><a name="#GLM.{name}"/>object fuse:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="current_limit">	current_limit <xsl:value-of select="current_limit"/>;
</xsl:if><xsl:if test="time_curve">	time_curve <xsl:value-of select="time_curve"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::meter objects
<xsl:for-each select="meter_list/meter"><a name="#GLM.{name}"/>object meter:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="measured_energy">	measured_energy <xsl:value-of select="measured_energy"/>;
</xsl:if><xsl:if test="measured_power">	measured_power <xsl:value-of select="measured_power"/>;
</xsl:if><xsl:if test="measured_demand">	measured_demand <xsl:value-of select="measured_demand"/>;
</xsl:if><xsl:if test="measured_real_power">	measured_real_power <xsl:value-of select="measured_real_power"/>;
</xsl:if><xsl:if test="measured_reactive_power">	measured_reactive_power <xsl:value-of select="measured_reactive_power"/>;
</xsl:if><xsl:if test="measured_voltage_A">	measured_voltage_A <xsl:value-of select="measured_voltage_A"/>;
</xsl:if><xsl:if test="measured_voltage_B">	measured_voltage_B <xsl:value-of select="measured_voltage_B"/>;
</xsl:if><xsl:if test="measured_voltage_C">	measured_voltage_C <xsl:value-of select="measured_voltage_C"/>;
</xsl:if><xsl:if test="measured_current_A">	measured_current_A <xsl:value-of select="measured_current_A"/>;
</xsl:if><xsl:if test="measured_current_B">	measured_current_B <xsl:value-of select="measured_current_B"/>;
</xsl:if><xsl:if test="measured_current_C">	measured_current_C <xsl:value-of select="measured_current_C"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::line objects
<xsl:for-each select="line_list/line"><a name="#GLM.{name}"/>object line:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="configuration">	configuration <a href="#GLM.{configuration}"><xsl:value-of select="configuration"/></a>;
</xsl:if><xsl:if test="length">	length <xsl:value-of select="length"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::line_spacing objects
<xsl:for-each select="line_spacing_list/line_spacing"><a name="#GLM.{name}"/>object line_spacing:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="distance_AB">	distance_AB <xsl:value-of select="distance_AB"/>;
</xsl:if><xsl:if test="distance_BC">	distance_BC <xsl:value-of select="distance_BC"/>;
</xsl:if><xsl:if test="distance_AC">	distance_AC <xsl:value-of select="distance_AC"/>;
</xsl:if><xsl:if test="distance_AN">	distance_AN <xsl:value-of select="distance_AN"/>;
</xsl:if><xsl:if test="distance_BN">	distance_BN <xsl:value-of select="distance_BN"/>;
</xsl:if><xsl:if test="distance_CN">	distance_CN <xsl:value-of select="distance_CN"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::overhead_line objects
<xsl:for-each select="overhead_line_list/overhead_line"><a name="#GLM.{name}"/>object overhead_line:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::underground_line objects
<xsl:for-each select="underground_line_list/underground_line"><a name="#GLM.{name}"/>object underground_line:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::overhead_line_conductor objects
<xsl:for-each select="overhead_line_conductor_list/overhead_line_conductor"><a name="#GLM.{name}"/>object overhead_line_conductor:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="geometric_mean_radius">	geometric_mean_radius <xsl:value-of select="geometric_mean_radius"/>;
</xsl:if><xsl:if test="resistance">	resistance <xsl:value-of select="resistance"/>;
</xsl:if><xsl:if test="rating.summer.continuous">	rating.summer.continuous <xsl:value-of select="rating.summer.continuous"/>;
</xsl:if><xsl:if test="rating.summer.emergency">	rating.summer.emergency <xsl:value-of select="rating.summer.emergency"/>;
</xsl:if><xsl:if test="rating.winter.continuous">	rating.winter.continuous <xsl:value-of select="rating.winter.continuous"/>;
</xsl:if><xsl:if test="rating.winter.emergency">	rating.winter.emergency <xsl:value-of select="rating.winter.emergency"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::underground_line_conductor objects
<xsl:for-each select="underground_line_conductor_list/underground_line_conductor"><a name="#GLM.{name}"/>object underground_line_conductor:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="outer_diameter">	outer_diameter <xsl:value-of select="outer_diameter"/>;
</xsl:if><xsl:if test="conductor_gmr">	conductor_gmr <xsl:value-of select="conductor_gmr"/>;
</xsl:if><xsl:if test="conductor_diameter">	conductor_diameter <xsl:value-of select="conductor_diameter"/>;
</xsl:if><xsl:if test="conductor_resistance">	conductor_resistance <xsl:value-of select="conductor_resistance"/>;
</xsl:if><xsl:if test="neutral_gmr">	neutral_gmr <xsl:value-of select="neutral_gmr"/>;
</xsl:if><xsl:if test="neutral_diameter">	neutral_diameter <xsl:value-of select="neutral_diameter"/>;
</xsl:if><xsl:if test="neutral_resistance">	neutral_resistance <xsl:value-of select="neutral_resistance"/>;
</xsl:if><xsl:if test="neutral_strands">	neutral_strands <xsl:value-of select="neutral_strands"/>;
</xsl:if><xsl:if test="shield_gmr">	shield_gmr <xsl:value-of select="shield_gmr"/>;
</xsl:if><xsl:if test="shield_resistance">	shield_resistance <xsl:value-of select="shield_resistance"/>;
</xsl:if><xsl:if test="rating.summer.continuous">	rating.summer.continuous <xsl:value-of select="rating.summer.continuous"/>;
</xsl:if><xsl:if test="rating.summer.emergency">	rating.summer.emergency <xsl:value-of select="rating.summer.emergency"/>;
</xsl:if><xsl:if test="rating.winter.continuous">	rating.winter.continuous <xsl:value-of select="rating.winter.continuous"/>;
</xsl:if><xsl:if test="rating.winter.emergency">	rating.winter.emergency <xsl:value-of select="rating.winter.emergency"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::line_configuration objects
<xsl:for-each select="line_configuration_list/line_configuration"><a name="#GLM.{name}"/>object line_configuration:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="conductor_A">	conductor_A <a href="#GLM.{conductor_A}"><xsl:value-of select="conductor_A"/></a>;
</xsl:if><xsl:if test="conductor_B">	conductor_B <a href="#GLM.{conductor_B}"><xsl:value-of select="conductor_B"/></a>;
</xsl:if><xsl:if test="conductor_C">	conductor_C <a href="#GLM.{conductor_C}"><xsl:value-of select="conductor_C"/></a>;
</xsl:if><xsl:if test="conductor_N">	conductor_N <a href="#GLM.{conductor_N}"><xsl:value-of select="conductor_N"/></a>;
</xsl:if><xsl:if test="spacing">	spacing <a href="#GLM.{spacing}"><xsl:value-of select="spacing"/></a>;
</xsl:if>}
</xsl:for-each>
# powerflow::relay objects
<xsl:for-each select="relay_list/relay"><a name="#GLM.{name}"/>object relay:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="time_to_change">	time_to_change <xsl:value-of select="time_to_change"/>;
</xsl:if><xsl:if test="recloser_delay">	recloser_delay <xsl:value-of select="recloser_delay"/>;
</xsl:if><xsl:if test="recloser_tries">	recloser_tries <xsl:value-of select="recloser_tries"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::transformer_configuration objects
<xsl:for-each select="transformer_configuration_list/transformer_configuration"><a name="#GLM.{name}"/>object transformer_configuration:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="connect_type">	connect_type <xsl:value-of select="connect_type"/>;
</xsl:if><xsl:if test="install_type">	install_type <xsl:value-of select="install_type"/>;
</xsl:if><xsl:if test="primary_voltage">	primary_voltage <xsl:value-of select="primary_voltage"/>;
</xsl:if><xsl:if test="secondary_voltage">	secondary_voltage <xsl:value-of select="secondary_voltage"/>;
</xsl:if><xsl:if test="power_rating">	power_rating <xsl:value-of select="power_rating"/>;
</xsl:if><xsl:if test="powerA_rating">	powerA_rating <xsl:value-of select="powerA_rating"/>;
</xsl:if><xsl:if test="powerB_rating">	powerB_rating <xsl:value-of select="powerB_rating"/>;
</xsl:if><xsl:if test="powerC_rating">	powerC_rating <xsl:value-of select="powerC_rating"/>;
</xsl:if><xsl:if test="resistance">	resistance <xsl:value-of select="resistance"/>;
</xsl:if><xsl:if test="reactance">	reactance <xsl:value-of select="reactance"/>;
</xsl:if><xsl:if test="impedance">	impedance <xsl:value-of select="impedance"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::transformer objects
<xsl:for-each select="transformer_list/transformer"><a name="#GLM.{name}"/>object transformer:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="configuration">	configuration <a href="#GLM.{configuration}"><xsl:value-of select="configuration"/></a>;
</xsl:if>}
</xsl:for-each>
# powerflow::load objects
<xsl:for-each select="load_list/load"><a name="#GLM.{name}"/>object load:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="load_class">	load_class <xsl:value-of select="load_class"/>;
</xsl:if><xsl:if test="constant_power_A">	constant_power_A <xsl:value-of select="constant_power_A"/>;
</xsl:if><xsl:if test="constant_power_B">	constant_power_B <xsl:value-of select="constant_power_B"/>;
</xsl:if><xsl:if test="constant_power_C">	constant_power_C <xsl:value-of select="constant_power_C"/>;
</xsl:if><xsl:if test="constant_current_A">	constant_current_A <xsl:value-of select="constant_current_A"/>;
</xsl:if><xsl:if test="constant_current_B">	constant_current_B <xsl:value-of select="constant_current_B"/>;
</xsl:if><xsl:if test="constant_current_C">	constant_current_C <xsl:value-of select="constant_current_C"/>;
</xsl:if><xsl:if test="constant_impedance_A">	constant_impedance_A <xsl:value-of select="constant_impedance_A"/>;
</xsl:if><xsl:if test="constant_impedance_B">	constant_impedance_B <xsl:value-of select="constant_impedance_B"/>;
</xsl:if><xsl:if test="constant_impedance_C">	constant_impedance_C <xsl:value-of select="constant_impedance_C"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::regulator_configuration objects
<xsl:for-each select="regulator_configuration_list/regulator_configuration"><a name="#GLM.{name}"/>object regulator_configuration:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="connect_type">	connect_type <xsl:value-of select="connect_type"/>;
</xsl:if><xsl:if test="band_center">	band_center <xsl:value-of select="band_center"/>;
</xsl:if><xsl:if test="band_width">	band_width <xsl:value-of select="band_width"/>;
</xsl:if><xsl:if test="time_delay">	time_delay <xsl:value-of select="time_delay"/>;
</xsl:if><xsl:if test="raise_taps">	raise_taps <xsl:value-of select="raise_taps"/>;
</xsl:if><xsl:if test="lower_taps">	lower_taps <xsl:value-of select="lower_taps"/>;
</xsl:if><xsl:if test="current_transducer_ratio">	current_transducer_ratio <xsl:value-of select="current_transducer_ratio"/>;
</xsl:if><xsl:if test="power_transducer_ratio">	power_transducer_ratio <xsl:value-of select="power_transducer_ratio"/>;
</xsl:if><xsl:if test="compensator_r_setting">	compensator_r_setting <xsl:value-of select="compensator_r_setting"/>;
</xsl:if><xsl:if test="compensator_x_setting">	compensator_x_setting <xsl:value-of select="compensator_x_setting"/>;
</xsl:if><xsl:if test="CT_phase">	CT_phase <xsl:value-of select="CT_phase"/>;
</xsl:if><xsl:if test="PT_phase">	PT_phase <xsl:value-of select="PT_phase"/>;
</xsl:if><xsl:if test="regulation">	regulation <xsl:value-of select="regulation"/>;
</xsl:if><xsl:if test="high_voltage">	high_voltage <xsl:value-of select="high_voltage"/>;
</xsl:if><xsl:if test="tap_pos_A">	tap_pos_A <xsl:value-of select="tap_pos_A"/>;
</xsl:if><xsl:if test="tap_pos_B">	tap_pos_B <xsl:value-of select="tap_pos_B"/>;
</xsl:if><xsl:if test="tap_pos_C">	tap_pos_C <xsl:value-of select="tap_pos_C"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::regulator objects
<xsl:for-each select="regulator_list/regulator"><a name="#GLM.{name}"/>object regulator:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="configuration">	configuration <a href="#GLM.{configuration}"><xsl:value-of select="configuration"/></a>;
</xsl:if>}
</xsl:for-each>
# powerflow::triplex_node objects
<xsl:for-each select="triplex_node_list/triplex_node"><a name="#GLM.{name}"/>object triplex_node:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="bustype">	bustype <xsl:value-of select="bustype"/>;
</xsl:if><xsl:if test="busflags">	busflags <xsl:value-of select="busflags"/>;
</xsl:if><xsl:if test="reference_bus">	reference_bus <a href="#GLM.{reference_bus}"><xsl:value-of select="reference_bus"/></a>;
</xsl:if><xsl:if test="nominal_voltage">	nominal_voltage <xsl:value-of select="nominal_voltage"/>;
</xsl:if><xsl:if test="maximum_voltage_error">	maximum_voltage_error <xsl:value-of select="maximum_voltage_error"/>;
</xsl:if><xsl:if test="voltage_1">	voltage_1 <xsl:value-of select="voltage_1"/>;
</xsl:if><xsl:if test="voltage_2">	voltage_2 <xsl:value-of select="voltage_2"/>;
</xsl:if><xsl:if test="voltage_N">	voltage_N <xsl:value-of select="voltage_N"/>;
</xsl:if><xsl:if test="voltage_12">	voltage_12 <xsl:value-of select="voltage_12"/>;
</xsl:if><xsl:if test="voltage_1N">	voltage_1N <xsl:value-of select="voltage_1N"/>;
</xsl:if><xsl:if test="voltage_2N">	voltage_2N <xsl:value-of select="voltage_2N"/>;
</xsl:if><xsl:if test="current_1">	current_1 <xsl:value-of select="current_1"/>;
</xsl:if><xsl:if test="current_2">	current_2 <xsl:value-of select="current_2"/>;
</xsl:if><xsl:if test="current_N">	current_N <xsl:value-of select="current_N"/>;
</xsl:if><xsl:if test="power_1">	power_1 <xsl:value-of select="power_1"/>;
</xsl:if><xsl:if test="power_2">	power_2 <xsl:value-of select="power_2"/>;
</xsl:if><xsl:if test="power_N">	power_N <xsl:value-of select="power_N"/>;
</xsl:if><xsl:if test="shunt_1">	shunt_1 <xsl:value-of select="shunt_1"/>;
</xsl:if><xsl:if test="shunt_2">	shunt_2 <xsl:value-of select="shunt_2"/>;
</xsl:if><xsl:if test="shunt_N">	shunt_N <xsl:value-of select="shunt_N"/>;
</xsl:if><xsl:if test="impedance_1">	impedance_1 <xsl:value-of select="impedance_1"/>;
</xsl:if><xsl:if test="impedance_2">	impedance_2 <xsl:value-of select="impedance_2"/>;
</xsl:if><xsl:if test="impedance_N">	impedance_N <xsl:value-of select="impedance_N"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::triplex_meter objects
<xsl:for-each select="triplex_meter_list/triplex_meter"><a name="#GLM.{name}"/>object triplex_meter:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="measured_energy">	measured_energy <xsl:value-of select="measured_energy"/>;
</xsl:if><xsl:if test="measured_power">	measured_power <xsl:value-of select="measured_power"/>;
</xsl:if><xsl:if test="measured_demand">	measured_demand <xsl:value-of select="measured_demand"/>;
</xsl:if><xsl:if test="measured_real_power">	measured_real_power <xsl:value-of select="measured_real_power"/>;
</xsl:if><xsl:if test="measured_voltage_1">	measured_voltage_1 <xsl:value-of select="measured_voltage_1"/>;
</xsl:if><xsl:if test="measured_voltage_2">	measured_voltage_2 <xsl:value-of select="measured_voltage_2"/>;
</xsl:if><xsl:if test="measured_voltage_N">	measured_voltage_N <xsl:value-of select="measured_voltage_N"/>;
</xsl:if><xsl:if test="measured_current_1">	measured_current_1 <xsl:value-of select="measured_current_1"/>;
</xsl:if><xsl:if test="measured_current_2">	measured_current_2 <xsl:value-of select="measured_current_2"/>;
</xsl:if><xsl:if test="measured_current_N">	measured_current_N <xsl:value-of select="measured_current_N"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::triplex_line objects
<xsl:for-each select="triplex_line_list/triplex_line"><a name="#GLM.{name}"/>object triplex_line:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::triplex_line_configuration objects
<xsl:for-each select="triplex_line_configuration_list/triplex_line_configuration"><a name="#GLM.{name}"/>object triplex_line_configuration:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="conductor_1">	conductor_1 <a href="#GLM.{conductor_1}"><xsl:value-of select="conductor_1"/></a>;
</xsl:if><xsl:if test="conductor_2">	conductor_2 <a href="#GLM.{conductor_2}"><xsl:value-of select="conductor_2"/></a>;
</xsl:if><xsl:if test="conductor_N">	conductor_N <a href="#GLM.{conductor_N}"><xsl:value-of select="conductor_N"/></a>;
</xsl:if><xsl:if test="insulation_thickness">	insulation_thickness <xsl:value-of select="insulation_thickness"/>;
</xsl:if><xsl:if test="diameter">	diameter <xsl:value-of select="diameter"/>;
</xsl:if><xsl:if test="spacing">	spacing <a href="#GLM.{spacing}"><xsl:value-of select="spacing"/></a>;
</xsl:if>}
</xsl:for-each>
# powerflow::triplex_line_conductor objects
<xsl:for-each select="triplex_line_conductor_list/triplex_line_conductor"><a name="#GLM.{name}"/>object triplex_line_conductor:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="resistance">	resistance <xsl:value-of select="resistance"/>;
</xsl:if><xsl:if test="geometric_mean_radius">	geometric_mean_radius <xsl:value-of select="geometric_mean_radius"/>;
</xsl:if>}
</xsl:for-each>
# powerflow::switch objects
<xsl:for-each select="switch_list/switch"><a name="#GLM.{name}"/>object switch:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if>}
</xsl:for-each></xsl:for-each><xsl:for-each select="residential">
##############################################
# residential module
module residential {
	version.major <xsl:value-of select="version.major"/>;
	version.minor <xsl:value-of select="version.minor"/>;
	default_line_voltage <xsl:value-of select="default_line_voltage"/>;
	default_line_current <xsl:value-of select="default_line_current"/>;
}

# residential::house objects
<xsl:for-each select="house_list/house"><a name="#GLM.{name}"/>object house:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="floor_area">	floor_area <xsl:value-of select="floor_area"/>;
</xsl:if><xsl:if test="gross_wall_area">	gross_wall_area <xsl:value-of select="gross_wall_area"/>;
</xsl:if><xsl:if test="ceiling_height">	ceiling_height <xsl:value-of select="ceiling_height"/>;
</xsl:if><xsl:if test="aspect_ratio">	aspect_ratio <xsl:value-of select="aspect_ratio"/>;
</xsl:if><xsl:if test="envelope_UA">	envelope_UA <xsl:value-of select="envelope_UA"/>;
</xsl:if><xsl:if test="window_wall_ratio">	window_wall_ratio <xsl:value-of select="window_wall_ratio"/>;
</xsl:if><xsl:if test="glazing_shgc">	glazing_shgc <xsl:value-of select="glazing_shgc"/>;
</xsl:if><xsl:if test="airchange_per_hour">	airchange_per_hour <xsl:value-of select="airchange_per_hour"/>;
</xsl:if><xsl:if test="internal_gain">	internal_gain <xsl:value-of select="internal_gain"/>;
</xsl:if><xsl:if test="thermostat_deadband">	thermostat_deadband <xsl:value-of select="thermostat_deadband"/>;
</xsl:if><xsl:if test="heating_setpoint">	heating_setpoint <xsl:value-of select="heating_setpoint"/>;
</xsl:if><xsl:if test="cooling_setpoint">	cooling_setpoint <xsl:value-of select="cooling_setpoint"/>;
</xsl:if><xsl:if test="design_heating_capacity">	design_heating_capacity <xsl:value-of select="design_heating_capacity"/>;
</xsl:if><xsl:if test="design_cooling_capacity">	design_cooling_capacity <xsl:value-of select="design_cooling_capacity"/>;
</xsl:if><xsl:if test="heating_COP">	heating_COP <xsl:value-of select="heating_COP"/>;
</xsl:if><xsl:if test="cooling_COP">	cooling_COP <xsl:value-of select="cooling_COP"/>;
</xsl:if><xsl:if test="COP_coeff">	COP_coeff <xsl:value-of select="COP_coeff"/>;
</xsl:if><xsl:if test="air_temperature">	air_temperature <xsl:value-of select="air_temperature"/>;
</xsl:if><xsl:if test="mass_heat_coeff">	mass_heat_coeff <xsl:value-of select="mass_heat_coeff"/>;
</xsl:if><xsl:if test="heat_mode">	heat_mode <xsl:value-of select="heat_mode"/>;
</xsl:if><xsl:if test="power">	power <xsl:value-of select="power"/>;
</xsl:if><xsl:if test="current">	current <xsl:value-of select="current"/>;
</xsl:if><xsl:if test="admittance">	admittance <xsl:value-of select="admittance"/>;
</xsl:if>}
</xsl:for-each>
# residential::waterheater objects
<xsl:for-each select="waterheater_list/waterheater"><a name="#GLM.{name}"/>object waterheater:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="tank_volume">	tank_volume <xsl:value-of select="tank_volume"/>;
</xsl:if><xsl:if test="tank_UA">	tank_UA <xsl:value-of select="tank_UA"/>;
</xsl:if><xsl:if test="tank_diameter">	tank_diameter <xsl:value-of select="tank_diameter"/>;
</xsl:if><xsl:if test="water_demand">	water_demand <xsl:value-of select="water_demand"/>;
</xsl:if><xsl:if test="heating_element_capacity">	heating_element_capacity <xsl:value-of select="heating_element_capacity"/>;
</xsl:if><xsl:if test="inlet_water_temperature">	inlet_water_temperature <xsl:value-of select="inlet_water_temperature"/>;
</xsl:if><xsl:if test="heat_mode">	heat_mode <xsl:value-of select="heat_mode"/>;
</xsl:if><xsl:if test="location">	location <xsl:value-of select="location"/>;
</xsl:if><xsl:if test="tank_setpoint">	tank_setpoint <xsl:value-of select="tank_setpoint"/>;
</xsl:if><xsl:if test="thermostat_deadband">	thermostat_deadband <xsl:value-of select="thermostat_deadband"/>;
</xsl:if><xsl:if test="power">	power <xsl:value-of select="power"/>;
</xsl:if><xsl:if test="meter">	meter <xsl:value-of select="meter"/>;
</xsl:if><xsl:if test="temperature">	temperature <xsl:value-of select="temperature"/>;
</xsl:if>}
</xsl:for-each>
# residential::lights objects
<xsl:for-each select="lights_list/lights"><a name="#GLM.{name}"/>object lights:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="type">	type <xsl:value-of select="type"/>;
</xsl:if><xsl:if test="placement">	placement <xsl:value-of select="placement"/>;
</xsl:if><xsl:if test="installed_power">	installed_power <xsl:value-of select="installed_power"/>;
</xsl:if><xsl:if test="circuit_split">	circuit_split <xsl:value-of select="circuit_split"/>;
</xsl:if><xsl:if test="demand">	demand <xsl:value-of select="demand"/>;
</xsl:if><xsl:if test="constant_power">	constant_power <xsl:value-of select="constant_power"/>;
</xsl:if><xsl:if test="constant_current">	constant_current <xsl:value-of select="constant_current"/>;
</xsl:if><xsl:if test="constant_admittance">	constant_admittance <xsl:value-of select="constant_admittance"/>;
</xsl:if><xsl:if test="internal_gains">	internal_gains <xsl:value-of select="internal_gains"/>;
</xsl:if><xsl:if test="energy_meter">	energy_meter <xsl:value-of select="energy_meter"/>;
</xsl:if>}
</xsl:for-each>
# residential::refrigerator objects
<xsl:for-each select="refrigerator_list/refrigerator"><a name="#GLM.{name}"/>object refrigerator:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="size ">	size  <xsl:value-of select="size "/>;
</xsl:if><xsl:if test="rated_capacity ">	rated_capacity  <xsl:value-of select="rated_capacity "/>;
</xsl:if><xsl:if test="power">	power <xsl:value-of select="power"/>;
</xsl:if><xsl:if test="power_factor">	power_factor <xsl:value-of select="power_factor"/>;
</xsl:if><xsl:if test="meter">	meter <xsl:value-of select="meter"/>;
</xsl:if>}
</xsl:for-each>
# residential::clotheswasher objects
<xsl:for-each select="clotheswasher_list/clotheswasher"><a name="#GLM.{name}"/>object clotheswasher:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="motor_power">	motor_power <xsl:value-of select="motor_power"/>;
</xsl:if><xsl:if test="power_factor">	power_factor <xsl:value-of select="power_factor"/>;
</xsl:if><xsl:if test="circuit_split">	circuit_split <xsl:value-of select="circuit_split"/>;
</xsl:if><xsl:if test="heat_fraction">	heat_fraction <xsl:value-of select="heat_fraction"/>;
</xsl:if><xsl:if test="enduse_demand">	enduse_demand <xsl:value-of select="enduse_demand"/>;
</xsl:if><xsl:if test="enduse_queue">	enduse_queue <xsl:value-of select="enduse_queue"/>;
</xsl:if><xsl:if test="constant_power">	constant_power <xsl:value-of select="constant_power"/>;
</xsl:if><xsl:if test="constant_current">	constant_current <xsl:value-of select="constant_current"/>;
</xsl:if><xsl:if test="constant_admittance">	constant_admittance <xsl:value-of select="constant_admittance"/>;
</xsl:if><xsl:if test="internal_gains">	internal_gains <xsl:value-of select="internal_gains"/>;
</xsl:if><xsl:if test="energy_meter">	energy_meter <xsl:value-of select="energy_meter"/>;
</xsl:if><xsl:if test="stall_voltage">	stall_voltage <xsl:value-of select="stall_voltage"/>;
</xsl:if><xsl:if test="start_voltage">	start_voltage <xsl:value-of select="start_voltage"/>;
</xsl:if><xsl:if test="stall_impedance">	stall_impedance <xsl:value-of select="stall_impedance"/>;
</xsl:if><xsl:if test="trip_delay">	trip_delay <xsl:value-of select="trip_delay"/>;
</xsl:if><xsl:if test="reset_delay">	reset_delay <xsl:value-of select="reset_delay"/>;
</xsl:if><xsl:if test="state">	state <xsl:value-of select="state"/>;
</xsl:if>}
</xsl:for-each>
# residential::dishwasher objects
<xsl:for-each select="dishwasher_list/dishwasher"><a name="#GLM.{name}"/>object dishwasher:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="installed_power">	installed_power <xsl:value-of select="installed_power"/>;
</xsl:if><xsl:if test="circuit_split">	circuit_split <xsl:value-of select="circuit_split"/>;
</xsl:if><xsl:if test="demand">	demand <xsl:value-of select="demand"/>;
</xsl:if><xsl:if test="constant_power">	constant_power <xsl:value-of select="constant_power"/>;
</xsl:if><xsl:if test="constant_current">	constant_current <xsl:value-of select="constant_current"/>;
</xsl:if><xsl:if test="constant_admittance">	constant_admittance <xsl:value-of select="constant_admittance"/>;
</xsl:if><xsl:if test="internal_gains">	internal_gains <xsl:value-of select="internal_gains"/>;
</xsl:if><xsl:if test="energy_meter">	energy_meter <xsl:value-of select="energy_meter"/>;
</xsl:if><xsl:if test="heat_fraction">	heat_fraction <xsl:value-of select="heat_fraction"/>;
</xsl:if>}
</xsl:for-each>
# residential::occupantload objects
<xsl:for-each select="occupantload_list/occupantload"><a name="#GLM.{name}"/>object occupantload:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="number_of_occupants">	number_of_occupants <xsl:value-of select="number_of_occupants"/>;
</xsl:if><xsl:if test="occupancy_fraction">	occupancy_fraction <xsl:value-of select="occupancy_fraction"/>;
</xsl:if><xsl:if test="heatgain_per_person">	heatgain_per_person <xsl:value-of select="heatgain_per_person"/>;
</xsl:if><xsl:if test="internal_gains">	internal_gains <xsl:value-of select="internal_gains"/>;
</xsl:if>}
</xsl:for-each>
# residential::plugload objects
<xsl:for-each select="plugload_list/plugload"><a name="#GLM.{name}"/>object plugload:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="circuit_split">	circuit_split <xsl:value-of select="circuit_split"/>;
</xsl:if><xsl:if test="demand">	demand <xsl:value-of select="demand"/>;
</xsl:if><xsl:if test="constant_power">	constant_power <xsl:value-of select="constant_power"/>;
</xsl:if><xsl:if test="constant_current">	constant_current <xsl:value-of select="constant_current"/>;
</xsl:if><xsl:if test="constant_admittance">	constant_admittance <xsl:value-of select="constant_admittance"/>;
</xsl:if><xsl:if test="internal_gains">	internal_gains <xsl:value-of select="internal_gains"/>;
</xsl:if><xsl:if test="energy_meter">	energy_meter <xsl:value-of select="energy_meter"/>;
</xsl:if><xsl:if test="heat_fraction">	heat_fraction <xsl:value-of select="heat_fraction"/>;
</xsl:if>}
</xsl:for-each>
# residential::microwave objects
<xsl:for-each select="microwave_list/microwave"><a name="#GLM.{name}"/>object microwave:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="installed_power">	installed_power <xsl:value-of select="installed_power"/>;
</xsl:if><xsl:if test="standby_power">	standby_power <xsl:value-of select="standby_power"/>;
</xsl:if><xsl:if test="circuit_split">	circuit_split <xsl:value-of select="circuit_split"/>;
</xsl:if><xsl:if test="demand">	demand <xsl:value-of select="demand"/>;
</xsl:if><xsl:if test="constant_power">	constant_power <xsl:value-of select="constant_power"/>;
</xsl:if><xsl:if test="constant_current">	constant_current <xsl:value-of select="constant_current"/>;
</xsl:if><xsl:if test="constant_admittance">	constant_admittance <xsl:value-of select="constant_admittance"/>;
</xsl:if><xsl:if test="internal_gains">	internal_gains <xsl:value-of select="internal_gains"/>;
</xsl:if><xsl:if test="energy_meter">	energy_meter <xsl:value-of select="energy_meter"/>;
</xsl:if><xsl:if test="heat_fraction">	heat_fraction <xsl:value-of select="heat_fraction"/>;
</xsl:if><xsl:if test="state">	state <xsl:value-of select="state"/>;
</xsl:if><xsl:if test="runtime">	runtime <xsl:value-of select="runtime"/>;
</xsl:if><xsl:if test="state_time">	state_time <xsl:value-of select="state_time"/>;
</xsl:if>}
</xsl:for-each>
# residential::range objects
<xsl:for-each select="range_list/range"><a name="#GLM.{name}"/>object range:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="installed_power">	installed_power <xsl:value-of select="installed_power"/>;
</xsl:if><xsl:if test="circuit_split">	circuit_split <xsl:value-of select="circuit_split"/>;
</xsl:if><xsl:if test="demand">	demand <xsl:value-of select="demand"/>;
</xsl:if><xsl:if test="constant_power">	constant_power <xsl:value-of select="constant_power"/>;
</xsl:if><xsl:if test="constant_current">	constant_current <xsl:value-of select="constant_current"/>;
</xsl:if><xsl:if test="constant_admittance">	constant_admittance <xsl:value-of select="constant_admittance"/>;
</xsl:if><xsl:if test="internal_gains">	internal_gains <xsl:value-of select="internal_gains"/>;
</xsl:if><xsl:if test="energy_meter">	energy_meter <xsl:value-of select="energy_meter"/>;
</xsl:if><xsl:if test="heat_fraction">	heat_fraction <xsl:value-of select="heat_fraction"/>;
</xsl:if>}
</xsl:for-each>
# residential::freezer objects
<xsl:for-each select="freezer_list/freezer"><a name="#GLM.{name}"/>object freezer:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="size ">	size  <xsl:value-of select="size "/>;
</xsl:if><xsl:if test="rated_capacity ">	rated_capacity  <xsl:value-of select="rated_capacity "/>;
</xsl:if><xsl:if test="power">	power <xsl:value-of select="power"/>;
</xsl:if><xsl:if test="power_factor">	power_factor <xsl:value-of select="power_factor"/>;
</xsl:if><xsl:if test="meter">	meter <xsl:value-of select="meter"/>;
</xsl:if>}
</xsl:for-each>
# residential::dryer objects
<xsl:for-each select="dryer_list/dryer"><a name="#GLM.{name}"/>object dryer:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="motor_power">	motor_power <xsl:value-of select="motor_power"/>;
</xsl:if><xsl:if test="power_factor">	power_factor <xsl:value-of select="power_factor"/>;
</xsl:if><xsl:if test="coil_power">	coil_power <xsl:value-of select="coil_power"/>;
</xsl:if><xsl:if test="circuit_split">	circuit_split <xsl:value-of select="circuit_split"/>;
</xsl:if><xsl:if test="heat_fraction">	heat_fraction <xsl:value-of select="heat_fraction"/>;
</xsl:if><xsl:if test="enduse_demand">	enduse_demand <xsl:value-of select="enduse_demand"/>;
</xsl:if><xsl:if test="enduse_queue">	enduse_queue <xsl:value-of select="enduse_queue"/>;
</xsl:if><xsl:if test="constant_power">	constant_power <xsl:value-of select="constant_power"/>;
</xsl:if><xsl:if test="constant_current">	constant_current <xsl:value-of select="constant_current"/>;
</xsl:if><xsl:if test="constant_admittance">	constant_admittance <xsl:value-of select="constant_admittance"/>;
</xsl:if><xsl:if test="internal_gains">	internal_gains <xsl:value-of select="internal_gains"/>;
</xsl:if><xsl:if test="energy_meter">	energy_meter <xsl:value-of select="energy_meter"/>;
</xsl:if><xsl:if test="stall_voltage">	stall_voltage <xsl:value-of select="stall_voltage"/>;
</xsl:if><xsl:if test="start_voltage">	start_voltage <xsl:value-of select="start_voltage"/>;
</xsl:if><xsl:if test="stall_impedance">	stall_impedance <xsl:value-of select="stall_impedance"/>;
</xsl:if><xsl:if test="trip_delay">	trip_delay <xsl:value-of select="trip_delay"/>;
</xsl:if><xsl:if test="reset_delay">	reset_delay <xsl:value-of select="reset_delay"/>;
</xsl:if><xsl:if test="state">	state <xsl:value-of select="state"/>;
</xsl:if>}
</xsl:for-each>
# residential::evcharger objects
<xsl:for-each select="evcharger_list/evcharger"><a name="#GLM.{name}"/>object evcharger:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="charger_power">	charger_power <xsl:value-of select="charger_power"/>;
</xsl:if><xsl:if test="vehicle_type">	vehicle_type <xsl:value-of select="vehicle_type"/>;
</xsl:if><xsl:if test="state">	state <xsl:value-of select="state"/>;
</xsl:if><xsl:if test="p_come_home">	p_come_home <xsl:value-of select="p_come_home"/>;
</xsl:if><xsl:if test="p_go_work">	p_go_work <xsl:value-of select="p_go_work"/>;
</xsl:if><xsl:if test="p_go_shorttrip">	p_go_shorttrip <xsl:value-of select="p_go_shorttrip"/>;
</xsl:if><xsl:if test="p_go_longtrip">	p_go_longtrip <xsl:value-of select="p_go_longtrip"/>;
</xsl:if><xsl:if test="work_dist">	work_dist <xsl:value-of select="work_dist"/>;
</xsl:if><xsl:if test="shorttrip_dist">	shorttrip_dist <xsl:value-of select="shorttrip_dist"/>;
</xsl:if><xsl:if test="longtrip_dist">	longtrip_dist <xsl:value-of select="longtrip_dist"/>;
</xsl:if><xsl:if test="capacity">	capacity <xsl:value-of select="capacity"/>;
</xsl:if><xsl:if test="charge">	charge <xsl:value-of select="charge"/>;
</xsl:if><xsl:if test="charge_at_work">	charge_at_work <xsl:value-of select="charge_at_work"/>;
</xsl:if><xsl:if test="power_factor">	power_factor <xsl:value-of select="power_factor"/>;
</xsl:if><xsl:if test="constant_power">	constant_power <xsl:value-of select="constant_power"/>;
</xsl:if><xsl:if test="constant_current">	constant_current <xsl:value-of select="constant_current"/>;
</xsl:if><xsl:if test="constant_admittance">	constant_admittance <xsl:value-of select="constant_admittance"/>;
</xsl:if><xsl:if test="internal_gains">	internal_gains <xsl:value-of select="internal_gains"/>;
</xsl:if><xsl:if test="energy_meter">	energy_meter <xsl:value-of select="energy_meter"/>;
</xsl:if><xsl:if test="heat_fraction">	heat_fraction <xsl:value-of select="heat_fraction"/>;
</xsl:if>}
</xsl:for-each></xsl:for-each><xsl:for-each select="tape">
##############################################
# tape module
module tape {
	version.major <xsl:value-of select="version.major"/>;
	version.minor <xsl:value-of select="version.minor"/>;
}

# tape::player objects
<xsl:for-each select="player_list/player"><a name="#GLM.{name}"/>object player:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="property">	property <xsl:value-of select="property"/>;
</xsl:if><xsl:if test="file">	file <xsl:value-of select="file"/>;
</xsl:if><xsl:if test="filetype">	filetype <xsl:value-of select="filetype"/>;
</xsl:if><xsl:if test="loop">	loop <xsl:value-of select="loop"/>;
</xsl:if>}
</xsl:for-each>
# tape::shaper objects
<xsl:for-each select="shaper_list/shaper"><a name="#GLM.{name}"/>object shaper:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="file">	file <xsl:value-of select="file"/>;
</xsl:if><xsl:if test="filetype">	filetype <xsl:value-of select="filetype"/>;
</xsl:if><xsl:if test="group">	group <xsl:value-of select="group"/>;
</xsl:if><xsl:if test="property">	property <xsl:value-of select="property"/>;
</xsl:if><xsl:if test="magnitude">	magnitude <xsl:value-of select="magnitude"/>;
</xsl:if><xsl:if test="events">	events <xsl:value-of select="events"/>;
</xsl:if>}
</xsl:for-each>
# tape::recorder objects
<xsl:for-each select="recorder_list/recorder"><a name="#GLM.{name}"/>object recorder:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="property">	property <xsl:value-of select="property"/>;
</xsl:if><xsl:if test="trigger">	trigger <xsl:value-of select="trigger"/>;
</xsl:if><xsl:if test="file">	file <xsl:value-of select="file"/>;
</xsl:if><xsl:if test="interval">	interval <xsl:value-of select="interval"/>;
</xsl:if><xsl:if test="limit">	limit <xsl:value-of select="limit"/>;
</xsl:if><xsl:if test="plotcommands">	plotcommands <xsl:value-of select="plotcommands"/>;
</xsl:if><xsl:if test="xdata">	xdata <xsl:value-of select="xdata"/>;
</xsl:if><xsl:if test="columns">	columns <xsl:value-of select="columns"/>;
</xsl:if><xsl:if test="output">	output <xsl:value-of select="output"/>;
</xsl:if>}
</xsl:for-each>
# tape::collector objects
<xsl:for-each select="collector_list/collector"><a name="#GLM.{name}"/>object collector:<xsl:value-of select="id"/> {
<xsl:if test="name!=''">	name "<xsl:value-of select="name"/>";
</xsl:if><xsl:if test="parent!=''">	parent "<a href="#GLM.{parent}"><xsl:value-of select="parent"/></a>";
</xsl:if><xsl:if test="clock!=''">	clock '<xsl:value-of select="clock"/>';
</xsl:if><xsl:if test="in_svc!=''">	in_svc '<xsl:value-of select="in_svc"/>';
</xsl:if><xsl:if test="out_svc!=''">	out_svc '<xsl:value-of select="out_svc"/>';
</xsl:if><xsl:if test="latitude!=''">	latitude <xsl:value-of select="latitude"/>;
</xsl:if><xsl:if test="longitude!=''">	longitude <xsl:value-of select="longitude"/>;
</xsl:if><xsl:if test="rank!=''">	rank <xsl:value-of select="rank"/>;
</xsl:if><xsl:if test="property">	property <xsl:value-of select="property"/>;
</xsl:if><xsl:if test="trigger">	trigger <xsl:value-of select="trigger"/>;
</xsl:if><xsl:if test="file">	file <xsl:value-of select="file"/>;
</xsl:if><xsl:if test="interval">	interval <xsl:value-of select="interval"/>;
</xsl:if><xsl:if test="limit">	limit <xsl:value-of select="limit"/>;
</xsl:if><xsl:if test="group">	group <xsl:value-of select="group"/>;
</xsl:if>}
</xsl:for-each></xsl:for-each></pre></td></tr></table>
</body>
</xsl:for-each></html>
