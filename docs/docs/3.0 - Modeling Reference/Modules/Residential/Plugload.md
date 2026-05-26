# Plug Load

The `plugload` object is an explicit residential end-use model for generic plug-connected devices. It inherits from `residential_enduse` and computes voltage-dependent power using ZIP-style fractions.

`plugload` is typically used to represent aggregate miscellaneous electric loads in a house when a more specific appliance model is not needed.

## Model behavior

### Initialization and defaults

- `heatgain_fraction` defaults to 0.90.
- `power_factor` defaults to 0.90.
- `voltage_factor` initializes to 1.0.
- `demand` (`shape.load`) is initialized randomly between 0 and 0.1 (uniform).
- If ZIP fractions are not provided, defaults are set to:
  - `power_fraction = 1.0`
  - `current_fraction = 0.0`
  - `impedance_fraction = 0.0`

### Runtime updates

- The model updates `voltage_factor` from the attached circuit voltage.
- If the parent circuit breaker is open, power/current/admittance contributions are forced to zero.
- In manual-demand mode (`shape.type == MT_UNKNOWN`):
  - Negative `demand` is clamped to 0.
  - Base ZIP terms are formed from `demand` and ZIP fractions.
  - Reactive power is derived from `power_factor` when valid.
- `actual_power` is computed as voltage-adjusted apparent power:

$$
actual\_power = power + (current + admittance \cdot voltage\_factor) \cdot voltage\_factor
$$

## Properties

Table: Plug Load Properties { #tbl:table-plugloads }

Property Name | Type | Unit | Description
---|---|---|---
**circuit_split** | double | - | Split/balance indicator for circuit assignment (`-1` to `+1`).
**demand** | double | unit | Demand multiplier (`shape.load`) for aggregate plug load behavior.
**installed_power** | double | kW | Installed plug-load capacity (`shape.params.analog.power`).
**actual_power** | complex | kVA | Voltage-adjusted apparent power of the plugload object.

The following inherited `residential_enduse` properties are commonly used with `plugload`:

- `power_factor`
- `power_fraction`, `current_fraction`, `impedance_fraction`
- `heatgain_fraction`
- `shape` / schedule linkage

## Example

```glm
object plugload {
  name house1_plugs;
  parent house1;
  installed_power 1.2 kW;
  demand PLUGS*1.0;
  power_factor 0.95;
  power_fraction 0.5;
  current_fraction 0.3;
  impedance_fraction 0.2;
}
```

This example models schedule-driven plug loads with custom ZIP fractions and power factor.
