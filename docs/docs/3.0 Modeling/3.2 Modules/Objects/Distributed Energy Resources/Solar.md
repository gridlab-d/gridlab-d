# Solar - Solar Panel

**Source URL:** https://gridlab-d.shoutwiki.com/wiki/Solar

## Synopsis
    
    module generators;
    class solar {
        enumeration {SUPPLY_DRIVEN=5, CONSTANT_PF=4, CONSTANT_PQ=2, CONSTANT_V=1, UNKNOWN=0} generator_mode;
        enumeration {ONLINE=2, OFFLINE=1} generator_status;
        enumeration {CONCENTRATOR=5, THIN_FILM_GA_AS=4, AMORPHOUS_SILICON=3, MULTI_CRYSTAL_SILICON=2, SINGLE_CRYSTAL_SILICON=1} panel_type;
        enumeration {DC=1, AC=2} power_type;
        enumeration {GROUND_MOUNTED=2, ROOF_MOUNTED=1} INSTALLATION_TYPE;
        enumeration {DEFAULT=1, SOLPOS=2} SOLAR_TILT_MODEL; 
        enumeration {DEFAULT=1, FLATPLATE=2} SOLAR_POWER_MODEL; 
        double a_coeff; 
        double b_coeff; 
        double dT_coeff; 
        double T_coeff[%/degC]; 
        double NOCT[degF];
        double Tmodule[degF];
        double Tambient[degF];
        double wind_speed[mph];
        double ambient_temeprature[degF];
        double Insolation[W/sf];
        double Rinternal[Ohm];
        double Rated_Insolation[W/sf];
        double Pmax_temp_coeff;
        double Voc_temp_coeff;
        complex V_Max[V];
        complex Voc_Max[V];
        complex Voc[V];
        double efficiency[unit];
        double area[sf];
        double soiling[pu]; 
        double derating[pu]; 
        double rated_power[W];
        complex P_Out[kW];
        complex V_Out[V];
        complex I_Out[A];
        complex VA_Out[VA];
        object weather;
        double shading_factor[pu];  
        double tilt_angle[deg]; 
        double orientation_azimuth[deg]; 
        bool latitude_angle_fix; 
        enumeration {DEFAULT=0,FIXED_AXIS=1,ONE_AXIS=2,TWO_AXIS=3,AZIMUTH_AXIS=4}orientation; 
        set {S=5, N=4, C=3, B=2, A=1} phases;
    }
    

## Remarks

A solar panel (also known as solar module or photovoltaic module/panel) is an assembly of solar cells. Solar panels must be connected via a parent inverter. 

## Properties

Property name | Type | Unit | Description   
---|---|---|---  
generator_mode | enumeration | none | (UNKNOWN,CONSTANT_V,CONSTANT_PQ,CONSTANT_PF,SUPPLY_DRIVEN) Currently solar must operate in SUPPLY_DRIVEN.   
generator_status | enumeration | none | (ONLINE, OFFLINE)   
panel_type | enumeration | none | Uses pre-defined panel technologies.(SINGLE_CRYSTAL_SILICON, MULTI_CRYSTAL_SILICON, AMORPHOUS_SILICON, THIN_FILM_GA_AS, CONCENTRATOR)   
power_type | enumeration | none | Defines whether the connection is AC or DC. _Currently not used._  
INSTALLATION_TYPE | enumeration | none | (ROOF_MOUNTED, GROUND_MOUNTED) _Currently not used._  
SOLAR_TILT_MODEL | enumeration | none | (DEFAULT, SOLPOS) Defines the tilt model to utilize for tilted array calculations.   
SOLAR_POWER_MODEL | enumeration | none | (DEFAULT, FLATPLATE,PV_CURVE) Defines if the PV array output efficiency should be adjusted for temperatures of the cells using a simple efficiency method, or the SAM simple flat plate efficiency model, or use the PV Curve (DC bus) model.   
a_coeff | double | none | _a_ coefficient for temperature correction forumula   
b_coeff | double | none | _b_ coefficient for temperature correction forumula   
dT_coeff | double | %/degC | Temperature difference coefficient for temperature correction forumula   
T_coeff | double | none | Maximum power temperature coefficient for temperature correction forumula   
NOCT | double | degF | Nominal operating cell temperature.   
Tmodule | double | degF | Calculated internal temperature of the PV module.   
Tambient | double | degF | Outside air temperature.   
wind_speed | double | mph | Outside wind speed. Currently not used.   
ambient_temperature | double | degF | Current ambient temperature of air   
Insolation | double | W/sf | Solar radiation incident upon the solar panel.   
Rinternal | double | Ohm | _Currently not used._  
Rated_Insolation | double | W/sf | Insolation level that the cell is rated for.   
Pmax_temp_coeff | double |  | Coefficient for the effects of temperature changes on the actual power output.   
Voc_temp_coeff | double |  | Coefficient for the effects of temperature changes on the DC terminal voltage.   
V_Max | complex | V | Defines the maximum operating voltage of the PV module.   
Voc_Max | complex | V | Voc max of the solar module   
Voc | complex | V | Defines the open circuit voltage as specified by the PV manufacturer.   
efficiency | double | unit | Defines the efficiency of power conversion from the solar insolation to DC power.   
area | double | sf | Defines the surface area of the solar module.   
soiling | double | pu | Soiling of the array factor - representing dirt on the array.   
derating | double | pu | Panel derating to account for manufacturing variances.   
Rated kVA | double | kVA | _Currently not used._  
P_Out | complex | kW | _Currently not used._  
V_Out | complex | V | DC voltage passed to the inverter object   
I_Out | complex | A | DC current passed to the inverter object   
VA_Out | complex | VA | Actual power delivered to the inverter   
weather | object | n/a | Reference to a climate object from which temperature, humidity, and solar flux are collected   
shading_factor | double | pu | Shading factor for scaling solar power to the array   
tilt_angle | double | deg | Tilt angle of PV array   
orientation_azimuth | double | deg | Facing direction of the PV array   
latitude_angle_fix | bool | n/a | Fix tilt angle to installation latitude value (latitude comes from climate data)  
orientation | enumeration | n/a | Type of panel orientation. Types DEFAULT and FIXED_AXIS are currently implemented   
phases | set | n/a | (A,B,C,N,S) _Currently not used._  
pvc_U_oc_V | double | V | Open circuit voltage   
pvc_I_sc_A | double | A | Short circuit current   
pvc_U_m_V | double | V | Voltage at maximum power point   
pvc_I_m_A | double | A | Current at maximum power point   
  
## Default

Default Parameter Values  Parameter | Default value   
---|---  
NOTOC | 118.4 [degF]  
Tcell | 69.8 [degF]  
Tambient | 77 [degF]  
Insolation | 0   
Rinternal | 0.05   
Rated_Insolation | 92.902   
V_Max | 79.34   
Voc | 91.22   
Voc_Max | 91.22   
area | 323 [sf]  
soiling_factor | 0.95   
derating_factor | 0.95   
a_coeff | -2.81   
b_coeff | -0.0455   
dT_coeff | 0.0   
T_coeff | -0.5 [%/degC]  
efficiency | 0.10   
generator_mode | SUPPLY_DRIVEN  
generator_status | ONLINE  
panel_type | SINGLE_CRYSTAL_SILICON  
shading_factor | 1 (no shading)   
tilt_angle | 45 [deg] 
orientation_azimuth | 0 (equator facing)   
latitude_angle_fix | FALSE   
orientation | DEFAULT  
SOLAR_TILT_MODEL | DEFAULT  
SOLAR_POWER_MODEL | DEFAULT  

## Orientation

Type of panel orientation 

### Synopsis
    
    
    module generators;
    class solar {
         enumeration {DEFAULT=0, FIXED_AXIS=1, ONE_AXIS=2, TWO_AXIS=3, AZIMUTH_AXIS=4} orientation;
    }
    object solar {
         orientation _value_ ;
    }
    

### Remarks

Currently implemented orientation types include DEFAULT and FIXED_AXIS. DEFAULT implies the panel is tracking for ideal insolation. FIXED_AXIS represents an array with a fixed tilt angle and a fixed direction it faces. 

### DEFAULT

This orientation uses ideal insolation. Insolation is calculated by 
    
    
    Insolation = solar_flux * shading_factor
    

where the value for solar_flux either comes from a climate object, or if no climate object is included solar_flux is set to 1000. 

* FIXED_AXIS -
When orientation is set as FIXED_AXIS, weather, tilt_angle, orientation_azimuth, SOLAR_TILT_MODEL, and shading_factor are all used to calculate the solar radiation. 

* ONE_AXIS -
_Not yet implemented_

* TWO_AXIS -
_Not yet implemented_

* AZIMUTH_AXIS - 
_Not yet implemented_

#### Example

A minimal model could be created via: 
    
    
    object solar {
        generator_mode SUPPLY_DRIVEN;
        generator_status ONLINE;
        panel_type SINGLE_CRYSTAL_SILICON;
        efficiency 0.2;
        parent inverter1;
        area 2500;
    }
    

## Model with DC Bus Model

The solar panel, when properly interfaced with an inverter_dyn object in grid-forming mode, will provide DC bus changes and some minor transient detail. A behavioral model of PV cell is adopted here, the advantage of this model is that it only needs parameters of output characteristics of PV cell such as open-circuit voltage ($U_{oc}$), short-circuit current ($I_{sc}$), voltage and current at maximum power operating point ($U_m$ and $I_m$). 

The formulas which describe the behavioral model are given as below: 

$\displaystyle{}I=I_{sc}\left[1-C_1\left(\exp{\frac{U-dU}{C_2U_{oc}}}-1\right)\right]+dI$ | (1)   
---|---  
$\displaystyle{}C_1=\left(1+\frac{I_m}{I_{sc}}-\right)\exp{\frac{-U_m}{C_2U_{oc}}}$ | (2)   
$\displaystyle{}C_2=\left(\frac{U_m}{U_{oc}}-1\right)\left[\ln{1-\frac{I_m}{I_{sc}}}\right]^{-1}$ |  (3)   
$\displaystyle{}dU=-b_1U_{oc}(t-t_{ref})$ | (4)   
$\displaystyle{}dI=I_{sc}\left[a_1\frac{S}{S_{ref}}\left(t-t_{ref}\right)+\left(\frac{S}{S_{ref}}-1\right)\right]$ | (5)   
  
Where $S_{ref}$ and $t_{ref}$ are light intensity and temperature in standard environment ($S_{ref}=1000\frac{w}{m^2}$, $t_{ref}=25^{\circ}C$), $S$ and $t$ are real light intensity and temperature, $a_1$ and $b_1$ are parameters that used to revise the output characteristics of PV panel in different environment. $a_1$ and $b_1$ are set zero in this instance. 

A real PV panel has been modeled according to the formulas above, the output parameters of which are given in Table 1: 

Table 1 - Output Parameters of PV Panel  Variable | Units | Value   
---|---|---  
$\displaystyle{}U_{oc}$ | V | 1005   
$\displaystyle{}I_{sc}$ | A | 100   
$\displaystyle{}U_{m}$ | V | 750   
$\displaystyle{}I_{m}$ | A | 84   
  
In GridLAB-D simulation, the PV panel is modeled as a controllable current source, with the light intensity ($S$), temperature ($t$) and voltage of PV panel as inputs. The output of the model is the current of PV panel. 

The P-V curve of this PV panel is given in Figure 1, the maximum power is about 1400kW and the voltage at maximum power point is 850V. ($t=25^{\circ}C, S=600\frac{w}{m^2}$). 

![P-V Curve of PV Panel](../../../../images/300px-PV_fig1.png)

Figure 1 - P-V Curve of PV Panel

