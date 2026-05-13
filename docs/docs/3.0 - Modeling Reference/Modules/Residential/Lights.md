# Lights

The `lights` object is an explicit residential end-use model for interior and exterior lighting. It inherits from `residential_enduse` and applies voltage-sensitive ZIP behavior using type-specific default power factors and ZIP fractions.

Unlike implicit lighting end uses, explicit `lights` objects can be placed per house, assigned to indoor or outdoor locations, and connected to schedules through the inherited loadshape interface.

## Synopsis

    class lights {
    	parent residential_enduse;
    	class residential_enduse {
    		loadshape shape;
    		end use load; // the end use load description
    		complex energy[kVAh]; // the total energy consumed since the last meter reading
    		complex power[kVA]; // the total power consumption of the load
    		complex peak_demand[kVA]; // the peak power consumption since the last meter reading
    		double heatgain[Btu/h]; // the heat transferred from the end use to the parent
    		double heatgain_fraction[pu]; // the fraction of the heat that goes to the parent
    		double current_fraction[pu]; // the fraction of total power that is constant current
    		double impedance_fraction[pu]; // the fraction of total power that is constant impedance
    		double power_fraction[pu]; // the fraction of the total power that is constant power
    		double power_factor; // the power factor of the load
    		complex constant_power[kVA]; // the constant power portion of the total load
    		complex constant_current[kVA]; // the constant current portion of the total load
    		complex constant_admittance[kVA]; // the constant admittance portion of the total load
    		double voltage_factor[pu]; // the voltage change factor
    		double breaker_amps[A]; // the rated breaker amperage
    		set {IS220=1} configuration; // the load configuration options
    		enumeration {OFF=4294967295, NORMAL=0, ON=1} override;
    		enumeration {ON=1, OFF=0, UNKNOWN=4294967295} power_state;
    	}
    
    	enumeration {HID=4, SSL=3, CFL=2, FLUORESCENT=1, INCANDESCENT=0} type; // lighting type (affects power_factor)
    	enumeration {OUTDOOR=1, INDOOR=0} placement; // lighting location (affects where heatgains go)
    	double installed_power[kW]; // installed lighting capacity
    	double power_density[W/sf]; // installed power density
    	double curtailment[pu]; // lighting curtailment factor
    	double demand[pu]; // the current lighting demand
    	complex actual_power[kVA]; // actual power demand of lights object
    }

## Model behavior

### Initialization and defaults

- If no `power_factor` is provided (inherited from `residential_enduse`), the object defaults to a value based on `type`.
- If no ZIP fractions are provided (`power_fraction`, `current_fraction`, `impedance_fraction`), the object defaults them based on `type`.
- If neither `installed_power` nor a schedule is provided, installed power is inferred from `power_density` and parent `floor_area`.
  - If `floor_area` is unavailable, a 2500 sf fallback is used.
- If `placement` is `INDOOR`, `heatgain_fraction` is set to 0.90.
- If `placement` is `OUTDOOR`, `heatgain_fraction` is set to 0.0.

### Runtime updates

- The model updates `voltage_factor` using the attached circuit voltage magnitude and default line voltage.
- For manual-demand mode (`shape.type == MT_UNKNOWN`), `demand` (`shape.load`) is clamped to [0, 1].
- Real and reactive power are computed from `installed_power`, `demand`, ZIP fractions, and `power_factor`.
- `actual_power` is reported as voltage-adjusted total apparent power.

## Properties

Property Name | Type | Unit | Description
---|---|---|---
**type** | enumeration | - | Lighting type. One of `INCANDESCENT`, `FLUORESCENT`, `CFL`, `SSL`, `HID`.
**placement** | enumeration | - | Lighting location. One of `INDOOR` or `OUTDOOR`.
**installed_power** | double | kW | Installed lighting capacity (`shape.params.analog.power`).
**power_density** | double | W/sf | Installed lighting power density used to infer installed power.
**curtailment** | double | pu | Lighting curtailment factor (0 to 1 expected).
**demand** | double | pu | Current lighting demand multiplier (`shape.load`).
**actual_power** | complex | kVA | Voltage-adjusted apparent power of the lights object.

The following inherited `residential_enduse` properties are commonly used with `lights`:

- `power_factor`
- `power_fraction`, `current_fraction`, `impedance_fraction`
- `heatgain_fraction`
- `shape` / schedule linkage

## Type-based defaults

### Default power factor by type

Type | Default power factor
---|---
`INCANDESCENT` | 1.00
`FLUORESCENT` | 0.95
`CFL` | 0.92
`SSL` | 0.90
`HID` | 0.97

### Default ZIP fractions by type

Fractions are shown in `(Z, I, P)` order.

Type | Default fractions
---|---
`INCANDESCENT` | (1.0, 0.0, 0.0)
`FLUORESCENT` | (0.4, 0.0, 0.6)
`CFL` | (0.4, 0.0, 0.6)
`SSL` | (0.8, 0.1, 0.1)
`HID` | (0.8, 0.1, 0.1)

## Example

```glm
object lights {
  name house1_lights;
  parent house1;
  type SSL;
  placement INDOOR;
  installed_power 1.8 kW;
  demand LIGHTS*1.0;
  curtailment 1.0;
}
```

This configuration models indoor SSL lighting with a scheduled demand multiplier and full output (no curtailment).
