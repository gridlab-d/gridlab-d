## Meter

Meters provide a measurement point for power and energy on the distribution system. Coupled with a **recorder** or **collector**, the **meter** object provides measurements of power, energy, voltage, and current at a specific location. Total cumulative energy, instantaneous power, peak demand, and per-phase measurements are available. The meter also includes optional billing functionality and interval-based statistics for voltage and power.

A typical implementation would be:

    object meter {
        name Mtr1;
        phases ABC;
        nominal_voltage 4800.0;
    }

### Meter Parameters

**meter** objects are derived from **[node](03-node.md)** objects, so any parameters of the **[node](03-node.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

#### Meter Configuration Properties

These properties control meter behavior and are input only. They are not modified by the simulation at runtime. Timestep properties must be set to a positive value (in seconds) to activate their corresponding measurement features; the default value of -1 disables them.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| meter_power_consumption | complex | VA | I | Power consumed by the meter itself (standby, communication). Divided equally across active phases and added as a constant power load. |
| measured_energy_delta_timestep | double | s | I | Interval for delta energy calculations (`measured_real_energy_delta`, `measured_reactive_energy_delta`). |
| measured_stats_interval | double | s | I | Interval for voltage and power min/max/average statistics. |

#### Instantaneous Measurement Properties

These properties report the meter's current electrical state. All are computed during the postsync pass of each powerflow iteration and are output only, except `measured_demand` which can also be set by the user to establish an initial floor value. The `measured_demand` property can be reset to zero by calling the meter reset function.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| measured_power | complex | VA | O | Total complex power across all phases. |
| measured_power_A | complex | VA | O | Complex power on phase A. |
| measured_power_B | complex | VA | O | Complex power on phase B. |
| measured_power_C | complex | VA | O | Complex power on phase C. |
| measured_real_power | double | W | O | Total real power across all phases. |
| measured_reactive_power | double | VAr | O | Total reactive power across all phases. |
| measured_demand | double | W | IO | Greatest real power recorded during the simulation. |
| measured_voltage_A | complex | V | O | Line-to-neutral voltage on phase A. |
| measured_voltage_B | complex | V | O | Line-to-neutral voltage on phase B. |
| measured_voltage_C | complex | V | O | Line-to-neutral voltage on phase C. |
| measured_voltage_AB | complex | V | O | Line-to-line voltage on phases AB. |
| measured_voltage_BC | complex | V | O | Line-to-line voltage on phases BC. |
| measured_voltage_CA | complex | V | O | Line-to-line voltage on phases CA. |
| measured_current_A | complex | A | O | Current on phase A. |
| measured_current_B | complex | A | O | Current on phase B. |
| measured_current_C | complex | A | O | Current on phase C. |

#### Energy Measurement Properties

These properties track energy consumption over time. Cumulative values are both input and output. They accumulate from the start of the simulation using `+=`, so a user-set initial value will offset all subsequent readings (the simulation logs a warning if initial values are nonzero).

Delta values are output only. They report the change in energy at each interval boundary defined by `measured_energy_delta_timestep` and are only updated when that property is set to a positive value.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| measured_real_energy | double | Wh | IO | Cumulative real energy consumption. |
| measured_reactive_energy | double | VAh | IO | Cumulative reactive energy consumption. |
| measured_real_energy_delta | double | Wh | O | Change in real energy since the last interval boundary. |
| measured_reactive_energy_delta | double | VAh | O | Change in reactive energy since the last interval boundary. |

#### Interval Voltage Statistics

These properties report voltage statistics computed over a repeating interval defined by `measured_stats_interval`. They are only active when `measured_stats_interval` is set to a positive value, and values are updated at the end of each interval. All properties in this section are output only.

For max and min properties, voltage samples are compared by **magnitude** each timestep. The "real" and "reactive" variants report the real and imaginary components of whichever sample had the greatest or least magnitude during the interval. Average properties track a time-weighted average of the voltage magnitude.

> **Note:** Due to a probable source code bug, the three `measured_reactive_max_voltage_A/B/C_in_interval` properties incorrectly report the imaginary component from the line-to-line peak-magnitude sample instead of the line-to-neutral sample.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| measured_real_max_voltage_A_in_interval | double | V | O | Real component of peak-magnitude line-to-neutral voltage on phase A. |
| measured_real_max_voltage_B_in_interval | double | V | O | Real component of peak-magnitude line-to-neutral voltage on phase B. |
| measured_real_max_voltage_C_in_interval | double | V | O | Real component of peak-magnitude line-to-neutral voltage on phase C. |
| measured_reactive_max_voltage_A_in_interval | double | V | O | Imaginary component of peak-magnitude line-to-neutral voltage on phase A. See note above. |
| measured_reactive_max_voltage_B_in_interval | double | V | O | Imaginary component of peak-magnitude line-to-neutral voltage on phase B. See note above. |
| measured_reactive_max_voltage_C_in_interval | double | V | O | Imaginary component of peak-magnitude line-to-neutral voltage on phase C. See note above. |
| measured_real_max_voltage_AB_in_interval | double | V | O | Real component of peak-magnitude line-to-line voltage on phases AB. |
| measured_real_max_voltage_BC_in_interval | double | V | O | Real component of peak-magnitude line-to-line voltage on phases BC. |
| measured_real_max_voltage_CA_in_interval | double | V | O | Real component of peak-magnitude line-to-line voltage on phases CA. |
| measured_reactive_max_voltage_AB_in_interval | double | V | O | Imaginary component of peak-magnitude line-to-line voltage on phases AB. |
| measured_reactive_max_voltage_BC_in_interval | double | V | O | Imaginary component of peak-magnitude line-to-line voltage on phases BC. |
| measured_reactive_max_voltage_CA_in_interval | double | V | O | Imaginary component of peak-magnitude line-to-line voltage on phases CA. |
| measured_real_min_voltage_A_in_interval | double | V | O | Real component of minimum-magnitude line-to-neutral voltage on phase A. |
| measured_real_min_voltage_B_in_interval | double | V | O | Real component of minimum-magnitude line-to-neutral voltage on phase B. |
| measured_real_min_voltage_C_in_interval | double | V | O | Real component of minimum-magnitude line-to-neutral voltage on phase C. |
| measured_reactive_min_voltage_A_in_interval | double | V | O | Imaginary component of minimum-magnitude line-to-neutral voltage on phase A. |
| measured_reactive_min_voltage_B_in_interval | double | V | O | Imaginary component of minimum-magnitude line-to-neutral voltage on phase B. |
| measured_reactive_min_voltage_C_in_interval | double | V | O | Imaginary component of minimum-magnitude line-to-neutral voltage on phase C. |
| measured_real_min_voltage_AB_in_interval | double | V | O | Real component of minimum-magnitude line-to-line voltage on phases AB. |
| measured_real_min_voltage_BC_in_interval | double | V | O | Real component of minimum-magnitude line-to-line voltage on phases BC. |
| measured_real_min_voltage_CA_in_interval | double | V | O | Real component of minimum-magnitude line-to-line voltage on phases CA. |
| measured_reactive_min_voltage_AB_in_interval | double | V | O | Imaginary component of minimum-magnitude line-to-line voltage on phases AB. |
| measured_reactive_min_voltage_BC_in_interval | double | V | O | Imaginary component of minimum-magnitude line-to-line voltage on phases BC. |
| measured_reactive_min_voltage_CA_in_interval | double | V | O | Imaginary component of minimum-magnitude line-to-line voltage on phases CA. |
| measured_avg_voltage_A_mag_in_interval | double | V | O | Average line-to-neutral voltage magnitude on phase A. |
| measured_avg_voltage_B_mag_in_interval | double | V | O | Average line-to-neutral voltage magnitude on phase B. |
| measured_avg_voltage_C_mag_in_interval | double | V | O | Average line-to-neutral voltage magnitude on phase C. |
| measured_avg_voltage_AB_mag_in_interval | double | V | O | Average line-to-line voltage magnitude on phases AB. |
| measured_avg_voltage_BC_mag_in_interval | double | V | O | Average line-to-line voltage magnitude on phases BC. |
| measured_avg_voltage_CA_mag_in_interval | double | V | O | Average line-to-line voltage magnitude on phases CA. |

#### Interval Power Statistics

These properties report power statistics computed over the same repeating interval defined by `measured_stats_interval`, with the same activation and update behavior as the voltage statistics above. All are output only. Total three-phase statistics are listed first, followed by per-phase breakdowns. Averages are time-weighted.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| measured_real_max_power_in_interval | double | W | O | Maximum total real power. |
| measured_reactive_max_power_in_interval | double | VAr | O | Maximum total reactive power. |
| measured_real_min_power_in_interval | double | W | O | Minimum total real power. |
| measured_reactive_min_power_in_interval | double | VAr | O | Minimum total reactive power. |
| measured_avg_real_power_in_interval | double | W | O | Average total real power. |
| measured_avg_reactive_power_in_interval | double | VAr | O | Average total reactive power. |
| measured_real_max_power_A_in_interval | double | W | O | Maximum real power on phase A. |
| measured_reactive_max_power_A_in_interval | double | VAr | O | Maximum reactive power on phase A. |
| measured_real_min_power_A_in_interval | double | W | O | Minimum real power on phase A. |
| measured_reactive_min_power_A_in_interval | double | VAr | O | Minimum reactive power on phase A. |
| measured_avg_real_power_A_in_interval | double | W | O | Average real power on phase A. |
| measured_avg_reactive_power_A_in_interval | double | VAr | O | Average reactive power on phase A. |
| measured_real_max_power_B_in_interval | double | W | O | Maximum real power on phase B. |
| measured_reactive_max_power_B_in_interval | double | VAr | O | Maximum reactive power on phase B. |
| measured_real_min_power_B_in_interval | double | W | O | Minimum real power on phase B. |
| measured_reactive_min_power_B_in_interval | double | VAr | O | Minimum reactive power on phase B. |
| measured_avg_real_power_B_in_interval | double | W | O | Average real power on phase B. |
| measured_avg_reactive_power_B_in_interval | double | VAr | O | Average reactive power on phase B. |
| measured_real_max_power_C_in_interval | double | W | O | Maximum real power on phase C. |
| measured_reactive_max_power_C_in_interval | double | VAr | O | Maximum reactive power on phase C. |
| measured_real_min_power_C_in_interval | double | W | O | Minimum real power on phase C. |
| measured_reactive_min_power_C_in_interval | double | VAr | O | Minimum reactive power on phase C. |
| measured_avg_real_power_C_in_interval | double | W | O | Average real power on phase C. |
| measured_avg_reactive_power_C_in_interval | double | VAr | O | Average reactive power on phase C. |

#### Billing Properties

These properties configure and report the meter's energy billing functionality. Billing is only active when `bill_mode` is set to a value other than `NONE`. The monthly bill accumulates during each billing cycle and is finalized at midnight on `bill_day`, at which point `previous_monthly_bill` and `previous_monthly_energy` are updated.

Tiered pricing modes (`TIERED`, `TIERED_RTP`, `TIERED_TOU`) use up to three energy tiers. Energy consumed below `first_tier_energy` is priced at the base `price` (or `price_base` for `TIERED_RTP`). Energy above each tier threshold is priced at the corresponding tier price. If a higher tier is not defined, the previous tier's price applies to all remaining energy.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| bill_mode | enumeration | N/A | I | Billing structure. Valid values: <br/> 0 - `NONE` - Billing disabled (default). <br/> 1 - `UNIFORM` - Flat rate from `price`. <br/> 2 - `TIERED` - Price increases at energy thresholds. <br/> 3 - `HOURLY` - Real-time market pricing via `power_market`. <br/> 4 - `TIERED_RTP` - Real-time market pricing plus tiered base charges via `price_base`. <br/> 5 - `TIERED_TOU` - Time-of-use with tier selection based on cumulative energy. |
| bill_day | int32 | N/A | I | Day of month the bill is finalized (1-28). |
| monthly_fee | double | $ | I | Flat monthly service fee included as the base of each bill. This is a recurrent monthly service charge that is added into the bill on the first day of the billing cycle (no pro-rating). |
| price | double | $/kWh | I | Current energy price. Updated from `power_market` in `HOURLY` and `TIERED_RTP` modes. |
| price_base | double | $/kWh | I | Base energy price used in `TIERED_RTP` mode for energy below the first tier. |
| power_market | object | N/A | I | Market object (auction or stubauction) providing the price signal. Required for `HOURLY` and `TIERED_RTP` modes. |
| first_tier_price | double | $/kWh | I | Energy price between `first_tier_energy` and `second_tier_energy`. |
| first_tier_energy | double | kWh | I | Energy threshold between base price and first tier. |
| second_tier_price | double | $/kWh | I | Energy price between `second_tier_energy` and `third_tier_energy`. |
| second_tier_energy | double | kWh | I | Energy threshold between first and second tiers. |
| third_tier_price | double | $/kWh | I | Energy price above `third_tier_energy`. |
| third_tier_energy | double | kWh | I | Energy threshold between second and third tiers. |
| monthly_bill | double | $ | O | Running bill for the current month. |
| monthly_energy | double | kWh | O | Cumulative energy consumed during the current billing cycle. |
| previous_monthly_bill | double | $ | O | Final bill from the previous billing cycle. |
| previous_monthly_energy | double | kWh | O | Total energy consumption from the previous billing cycle. |

#### Reliability Properties

These properties report the meter's service interruption status. The `customer_interrupted` flag is set automatically when a phase loss is detected through the fault check mechanism in the NR solver. The `customer_interrupted_secondary` flag indicates a momentary interruption and is automatically cleared to `false` at the start of each timestep.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| customer_interrupted | bool | N/A | IO | Indicates sustained service interruption due to phase loss. |
| customer_interrupted_secondary | bool | N/A | IO | Indicates momentary service interruption. |

The following properties are only available in builds compiled with `SUPPORT_OUTAGES` defined. All are input only.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| sustained_count | int16 | N/A | I | Sustained interruption event counter. |
| momentary_count | int16 | N/A | I | Momentary interruption event counter. |
| total_count | int16 | N/A | I | Total interruption event counter. |
| s_flag | int16 | N/A | I | Sustained interruption state flag. |
| t_flag | int16 | N/A | I | Temporary interruption state flag. |
| pre_load | complex | N/A | I | Load value prior to an outage event. |

### Meter State of Development

Meter is considered a highly developed and validated model in terms of powerflow solutions, however, models using billing features have not been fully validated.

