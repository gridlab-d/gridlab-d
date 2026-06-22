# Emissions

!!! warning

    The Emissions object is not actively maintained and is not validated. It may contain bugs. The module may be removed in a future release.

The Emissions object estimates pollutant emissions (CO2, SO2, NOx) produced to meet the electrical demand measured at a parent meter. It must be attached to a meter, from which it periodically samples `measured_power` and accumulates energy over a configurable `cycle_interval` (default 15 minutes). At the end of each interval, it dispatches the accumulated energy demand across nine generation sources (nuclear, hydro, solar thermal, biomass, wind, coal, natural gas, geothermal, petroleum) in a user-specified merit order, where each source supplies up to its `Max_Out` capacity until demand is met, with any leftover demand assigned to coal as a fallback. For each source used, it multiplies the energy produced by that source's conversion efficiency (Btu/kWh) and emission factors (lb/Btu) to compute per-source and total emissions, which are then published as object properties for reporting.

### Emissions Parameters

#### Properties

**emissions** does not declare inherited parent classes.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

Table: emissions table 1 { #tbl:42-emissions-1 }

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| **Nuclear_Order** | double | N/A | I |  |
| **Hydroelectric_Order** | double | N/A | I |  |
| **Solarthermal_Order** | double | N/A | I |  |
| **Biomass_Order** | double | N/A | I |  |
| **Wind_Order** | double | N/A | I |  |
| **Coal_Order** | double | N/A | I |  |
| **Naturalgas_Order** | double | N/A | I |  |
| **Geothermal_Order** | double | N/A | I |  |
| **Petroleum_Order** | double | N/A | I |  |
| **Naturalgas_Max_Out** | double | kWh | IO |  |
| **Coal_Max_Out** | double | kWh | IO |  |
| **Biomass_Max_Out** | double | kWh | IO |  |
| **Geothermal_Max_Out** | double | kWh | IO |  |
| **Hydroelectric_Max_Out** | double | kWh | IO |  |
| **Nuclear_Max_Out** | double | kWh | IO |  |
| **Wind_Max_Out** | double | kWh | IO |  |
| **Petroleum_Max_Out** | double | kWh | IO |  |
| **Solarthermal_Max_Out** | double | kWh | IO |  |
| **Naturalgas_Out** | double | kWh | IO |  |
| **Coal_Out** | double | kWh | IO |  |
| **Biomass_Out** | double | kWh | IO |  |
| **Geothermal_Out** | double | kWh | IO |  |
| **Hydroelectric_Out** | double | kWh | IO |  |
| **Nuclear_Out** | double | kWh | IO |  |
| **Wind_Out** | double | kWh | IO |  |
| **Petroleum_Out** | double | kWh | IO |  |
| **Solarthermal_Out** | double | kWh | IO |  |
| **Naturalgas_Conv_Eff** | double | Btu/kWh | I |  |
| **Coal_Conv_Eff** | double | Btu/kWh | I |  |
| **Biomass_Conv_Eff** | double | Btu/kWh | I |  |
| **Geothermal_Conv_Eff** | double | Btu/kWh | I |  |
| **Hydroelectric_Conv_Eff** | double | Btu/kWh | I |  |
| **Nuclear_Conv_Eff** | double | Btu/kWh | I |  |
| **Wind_Conv_Eff** | double | Btu/kWh | I |  |
| **Petroleum_Conv_Eff** | double | Btu/kWh | I |  |
| **Solarthermal_Conv_Eff** | double | Btu/kWh | I |  |
| **Naturalgas_CO2** | double | lb/Btu | I |  |
| **Coal_CO2** | double | lb/Btu | I |  |
| **Biomass_CO2** | double | lb/Btu | I |  |
| **Geothermal_CO2** | double | lb/Btu | I |  |
| **Hydroelectric_CO2** | double | lb/Btu | I |  |
| **Nuclear_CO2** | double | lb/Btu | I |  |
| **Wind_CO2** | double | lb/Btu | I |  |
| **Petroleum_CO2** | double | lb/Btu | I |  |
| **Solarthermal_CO2** | double | lb/Btu | I |  |
| **Naturalgas_SO2** | double | lb/Btu | I |  |
| **Coal_SO2** | double | lb/Btu | I |  |
| **Biomass_SO2** | double | lb/Btu | I |  |
| **Geothermal_SO2** | double | lb/Btu | I |  |
| **Hydroelectric_SO2** | double | lb/Btu | I |  |
| **Nuclear_SO2** | double | lb/Btu | I |  |
| **Wind_SO2** | double | lb/Btu | I |  |
| **Petroleum_SO2** | double | lb/Btu | I |  |
| **Solarthermal_SO2** | double | lb/Btu | I |  |
| **Naturalgas_NOx** | double | lb/Btu | I |  |
| **Coal_NOx** | double | lb/Btu | I |  |
| **Biomass_NOx** | double | lb/Btu | I |  |
| **Geothermal_NOx** | double | lb/Btu | I |  |
| **Hydroelectric_NOx** | double | lb/Btu | I |  |
| **Nuclear_NOx** | double | lb/Btu | I |  |
| **Wind_NOx** | double | lb/Btu | I |  |
| **Petroleum_NOx** | double | lb/Btu | I |  |
| **Solarthermal_NOx** | double | lb/Btu | I |  |
| **Naturalgas_emissions_CO2** | double | lb | IO |  |
| **Naturalgas_emissions_SO2** | double | lb | IO |  |
| **Naturalgas_emissions_NOx** | double | lb | IO |  |
| **Coal_emissions_CO2** | double | lb | IO |  |
| **Coal_emissions_SO2** | double | lb | IO |  |
| **Coal_emissions_NOx** | double | lb | IO |  |
| **Biomass_emissions_CO2** | double | lb | IO |  |
| **Biomass_emissions_SO2** | double | lb | IO |  |
| **Biomass_emissions_NOx** | double | lb | IO |  |
| **Geothermal_emissions_CO2** | double | lb | IO |  |
| **Geothermal_emissions_SO2** | double | lb | IO |  |
| **Geothermal_emissions_NOx** | double | lb | IO |  |
| **Hydroelectric_emissions_CO2** | double | lb | IO |  |
| **Hydroelectric_emissions_SO2** | double | lb | IO |  |
| **Hydroelectric_emissions_NOx** | double | lb | IO |  |
| **Nuclear_emissions_CO2** | double | lb | IO |  |
| **Nuclear_emissions_SO2** | double | lb | IO |  |
| **Nuclear_emissions_NOx** | double | lb | IO |  |
| **Wind_emissions_CO2** | double | lb | IO |  |
| **Wind_emissions_SO2** | double | lb | IO |  |
| **Wind_emissions_NOx** | double | lb | IO |  |
| **Petroleum_emissions_CO2** | double | lb | IO |  |
| **Petroleum_emissions_SO2** | double | lb | IO |  |
| **Petroleum_emissions_NOx** | double | lb | IO |  |
| **Solarthermal_emissions_CO2** | double | lb | IO |  |
| **Solarthermal_emissions_SO2** | double | lb | IO |  |
| **Solarthermal_emissions_NOx** | double | lb | IO |  |
| **Total_emissions_CO2** | double | lb | IO |  |
| **Total_emissions_SO2** | double | lb | IO |  |
| **Total_emissions_NOx** | double | lb | IO |  |
| **Total_energy_out** | double | kWh | IO |  |
| **Region** | double | N/A | I |  |
| **cycle_interval** | double | s | I |  |

### Emissions State of Development

In Development. 

