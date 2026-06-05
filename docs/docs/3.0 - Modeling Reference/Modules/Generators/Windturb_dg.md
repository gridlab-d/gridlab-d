# Wind Turbine

The wind turbine distributed generation object is implemented by class windturb_dg in the generators module.

The implementation supports two operating modes:

1. POWER_CURVE: output is computed from wind speed using a power curve (default mode).
2. COEFF_OF_PERFORMANCE: legacy aerodynamic and generator-parameter model.

## Power Curve Implementation

The power curve implementation models turbine output directly from wind-speed-to-power data instead of explicitly modeling internal aerodynamic and machine dynamics. At each timestep, wind speed is adjusted to hub height and then mapped to electrical output using linear interpolation between power-curve points. The model can use either a built-in default curve or a user-provided CSV curve (`power_curve_csv`), and user data may be interpreted as per-unit or absolute power using `power_curve_pu`.

This approach is intended for cases where manufacturer curve data is available and a simpler input/output representation is preferred. In POWER_CURVE mode, triplex parent support is available; in COEFF_OF_PERFORMANCE mode, triplex parent connection is rejected.

## Coefficient of Performance Based Implementation

The coefficient of performance-based implementation was the original wind turbine implementation. It includes an explicit model of the wind turbine and the electrical machine/generator parameters. The implementation contains highly granular models of the synchronous and induction generators with their respective impedances. It uses the wind turbine coefficient of performance data to generate the output for a given wind speed input. The coefficient of performance is defined as the ratio of the power captured by the rotor of the wind turbine divided by the total power available in the wind just before it interacts with the turbine.

## Status

This model remains in the experimental level of development

## Published Properties

Note that if a field exists in the C++ class but is not published, it is not user-configurable from GLM.

Table: Windturb_DG Parameters { #tbl:table-wind }

Property Name | Type | Unit | Description | Default
---|---|---|---|---
**Gen_status** | enumeration | N/A | Online/offline state. Values: OFFLINE, ONLINE | ONLINE
**Gen_type** | enumeration | N/A | Generator type. Values: INDUCTION, SYNCHRONOUS | Model-dependent
**Gen_mode** | enumeration | N/A | Generator control mode. Values: CONSTANTE, CONSTANTP, CONSTANTPQ | CONSTANTP
**Turbine_Model** | enumeration | N/A | Generic/manufacturer model selector. Includes GENERIC_* and named turbine options | GENERIC_DEFAULT
**Turbine_implementation** | enumeration | N/A | Implementation mode. Values: POWER_CURVE, COEFF_OF_PERFORMANCE | POWER_CURVE
**Wind_speed_source** | enumeration | N/A | Wind speed source. Values: DEFAULT, BUILT_IN, WIND_SPEED, CLIMATE_DATA | DEFAULT
**turbine_height** | double | m | Hub height above ground | Inferred or defaulted by model logic
**roughness_length_factor** | double | N/A | Terrain roughness correction factor | 0.055
**blade_diam** | double | m | Blade diameter | Model-dependent
**cut_in_ws** | double | m/s | Cut-in wind speed | Model-dependent
**cut_out_ws** | double | m/s | Cut-out wind speed | Model-dependent
**ws_rated** | double | m/s | Rated wind speed | Model-dependent
**ws_maxcp** | double | m/s | Wind speed at max Cp | Model-dependent
**Cp_max** | double | pu | Maximum coefficient of performance | Model-dependent
**Cp_rated** | double | pu | Rated coefficient of performance | Model-dependent
**Cp** | double | pu | Current coefficient of performance | Computed
**Rated_VA** | double | VA | Rated generator apparent power | Inferred or model default
**Rated_V** | double | V | Rated generator voltage | Model-dependent
**Pconv** | double | W | Converted electrical power from mechanical input | Computed
**GenElecEff** | double | % | Electrical conversion efficiency | Computed
**TotalRealPow** | double | W | Total real power output | Computed
**TotalReacPow** | double | VA | Total reactive power output | Computed
**power_A** | complex | VA | Complex power output phase A | Computed
**power_B** | complex | VA | Complex power output phase B | Computed
**power_C** | complex | VA | Complex power output phase C | Computed
**power_12** | complex | VA | Triplex complex power output line 1-2 | Computed
**WSadj** | double | m/s | Wind speed adjusted to hub height | Computed
**Wind_Speed** | double | m/s | Wind speed value used by the model | Computed/input-dependent
**air_density** | double | kg/m^3 | Calculated/used air density | Computed
**wind_speed_hub_ht** | double | m/s | User wind speed at hub height input | 10
**R_stator** | double | pu*Ohm | Induction generator stator resistance | Model-dependent
**X_stator** | double | pu*Ohm | Induction generator stator reactance | Model-dependent
**R_rotor** | double | pu*Ohm | Induction generator rotor resistance | Model-dependent
**X_rotor** | double | pu*Ohm | Induction generator rotor reactance | Model-dependent
**R_core** | double | pu*Ohm | Induction generator core resistance | Model-dependent
**X_magnetic** | double | pu*Ohm | Induction generator magnetizing reactance | Model-dependent
**Max_Vrotor** | double | pu*V | Maximum induced rotor voltage | 1.2
**Min_Vrotor** | double | pu*V | Minimum induced rotor voltage | 0.8
**Rs** | double | pu*Ohm | Synchronous generator stator resistance | Model-dependent
**Xs** | double | pu*Ohm | Synchronous generator stator reactance | Model-dependent
**Rg** | double | pu*Ohm | Synchronous generator grounding resistance | Model-dependent
**Xg** | double | pu*Ohm | Synchronous generator grounding reactance | Model-dependent
**Max_Ef** | double | pu*V | Maximum induced field voltage | 1.2
**Min_Ef** | double | pu*V | Minimum induced field voltage | 0.8
**pf** | double | pu | Power factor target in CONSTANTP mode | Model-dependent
**power_curve_csv** | char1024 | N/A | CSV file path for user-defined power curve | empty string
**power_curve_pu** | bool | N/A | true when user CSV power values are per-unit | false
**voltage_A** | complex | V | Terminal voltage phase A (hidden) | Computed
**voltage_B** | complex | V | Terminal voltage phase B (hidden) | Computed
**voltage_C** | complex | V | Terminal voltage phase C (hidden) | Computed
**voltage_12** | complex | V | Triplex L1-L2 voltage (hidden) | Computed
**voltage_1N** | complex | V | Triplex L1-N voltage (hidden) | Computed
**voltage_2N** | complex | V | Triplex L2-N voltage (hidden) | Computed
**current_A** | complex | A | Terminal current phase A | Computed
**current_B** | complex | A | Terminal current phase B | Computed
**current_C** | complex | A | Terminal current phase C | Computed
**current_12** | complex | A | Triplex line current 1-2 | Computed
**EfA** | complex | V | Synchronous induced voltage phase A | Computed
**EfB** | complex | V | Synchronous induced voltage phase B | Computed
**EfC** | complex | V | Synchronous induced voltage phase C | Computed
**Vrotor_A** | complex | V | Induction rotor voltage phase A (per-unit quantity represented as complex) | Computed
**Vrotor_B** | complex | V | Induction rotor voltage phase B (per-unit quantity represented as complex) | Computed
**Vrotor_C** | complex | V | Induction rotor voltage phase C (per-unit quantity represented as complex) | Computed
**Irotor_A** | complex | V | Induction rotor current phase A (per-unit quantity represented as complex) | Computed
**Irotor_B** | complex | V | Induction rotor current phase B (per-unit quantity represented as complex) | Computed
**Irotor_C** | complex | V | Induction rotor current phase C (per-unit quantity represented as complex) | Computed
**internal_model_current_convergence** | double | pu | Internal convergence threshold (hidden) | 0.005
**phases** | set | N/A | Connection phases. Keywords: A, B, C, N, S | Required by connection

## Property Aliases

The following alternate names are also published and map to the same internal variables:

- blade_diameter -> blade_diam
- P_converted -> Pconv
- generator_efficiency -> GenElecEff
- total_real_power -> TotalRealPow
- total_reactive_power -> TotalReacPow
- wind_speed_adjusted -> WSadj
- wind_speed -> Wind_Speed
- power_factor -> pf

## Example

	module generators;

	object windturb_dg {
		parent my_meter1;
		phases ABCN;
		name windturb1;
		Turbine_implementation POWER_CURVE;
		Turbine_Model GEN_TURB_POW_CURVE_1_5MW;
		Wind_speed_source CLIMATE_DATA;
	}

## Related Concepts

- Generators module
- Climate object
- Inverter and meter parent interfaces
    


  
