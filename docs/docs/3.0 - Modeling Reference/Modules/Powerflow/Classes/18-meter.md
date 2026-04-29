<details type="reasoning" done="true" duration="78">
<summary>Thought for 78 seconds</summary>

</details>

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

These properties control meter behavior and are input only. They are not modified by the simulation at runtime. Timestep properties must be set to a positive value (in seconds) to activate their corresponding measurement features; the default value of âˆ’1 disables them.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| meter_power_consumption | complex | VA | — | Power consumed by the meter itself (standby, communication). Divided equally across active phases and added as a constant power load. |
| measured_energy_delta_timestep | double | s | — | Interval for delta energy calculations (`measured_real_energy_delta`, `measured_reactive_energy_delta`). |
| measured_stats_interval | double | s | — | Interval for voltage and power min/max/average statistics. |

#### Instantaneous Measurement Properties

These properties report the meter's current electrical state. All are computed during the postsync pass of each powerflow iteration and are output only, except `measured_demand` which can also be set by the user to establish an initial floor value. The `measured_demand` property can be reset to zero by calling the meter reset function.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| measured_power | complex | VA | — | Total complex power across all phases. |
| measured_power_A | complex | VA | — | Complex power on phase A. |
| measured_power_B | complex | VA | — | Complex power on phase B. |
| measured_power_C | complex | VA | — | Complex power on phase C. |
| measured_real_power | double | W | — | Total real power across all phases. |
| measured_reactive_power | double | VAr | — | Total reactive power across all phases. |
| measured_demand | double | W | — | Greatest real power recorded during the simulation. |
| measured_voltage_A | complex | V | — | Line-to-neutral voltage on phase A. |
| measured_voltage_B | complex | V | — | Line-to-neutral voltage on phase B. |
| measured_voltage_C | complex | V | — | Line-to-neutral voltage on phase C. |
| measured_voltage_AB | complex | V | — | Line-to-line voltage on phases AB. |
| measured_voltage_BC | complex | V | — | Line-to-line voltage on phases BC. |
| measured_voltage_CA | complex | V | — | Line-to-line voltage on phases CA. |
| measured_current_A | complex | A | — | Current on phase A. |
| measured_current_B | complex | A | — | Current on phase B. |
| measured_current_C | complex | A | — | Current on phase C. |

#### Energy Measurement Properties

These properties track energy consumption over time. Cumulative values are both input and output â€” they accumulate from the start of the simulation using `+=`, so a user-set initial value will offset all subsequent readings (the simulation logs a warning if initial values are nonzero). Note that the `meter_reset` function does not clear energy values.

Delta values are output only. They report the change in energy at each interval boundary defined by `measured_energy_delta_timestep` and are only updated when that property is set to a positive value.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| measured_real_energy | double | Wh | — | Cumulative real energy consumption. |
| measured_reactive_energy | double | VAh | — | Cumulative reactive energy consumption. |
| measured_real_energy_delta | double | Wh | — | Change in real energy since the last interval boundary. |
| measured_reactive_energy_delta | double | VAh | — | Change in reactive energy since the last interval boundary. |

#### Interval Voltage Statistics

These properties report voltage statistics computed over a repeating interval defined by `measured_stats_interval`. They are only active when `measured_stats_interval` is set to a positive value, and values are updated at the end of each interval. All properties in this section are output only.

For max and min properties, voltage samples are compared by **magnitude** each timestep. The "real" and "reactive" variants report the real and imaginary components of whichever sample had the greatest or least magnitude during the interval. Average properties track a time-weighted average of the voltage magnitude.

> **Note:** Due to a probable source code bug, the three `measured_reactive_max_voltage_A/B/C_in_interval` properties incorrectly report the imaginary component from the line-to-line peak-magnitude sample instead of the line-to-neutral sample.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| measured_real_max_voltage_A_in_interval | double | V | — | Real component of peak-magnitude line-to-neutral voltage on phase A. |
| measured_real_max_voltage_B_in_interval | double | V | — | Real component of peak-magnitude line-to-neutral voltage on phase B. |
| measured_real_max_voltage_C_in_interval | double | V | — | Real component of peak-magnitude line-to-neutral voltage on phase C. |
| measured_reactive_max_voltage_A_in_interval | double | V | — | Imaginary component of peak-magnitude line-to-neutral voltage on phase A. See note above. |
| measured_reactive_max_voltage_B_in_interval | double | V | — | Imaginary component of peak-magnitude line-to-neutral voltage on phase B. See note above. |
| measured_reactive_max_voltage_C_in_interval | double | V | — | Imaginary component of peak-magnitude line-to-neutral voltage on phase C. See note above. |
| measured_real_max_voltage_AB_in_interval | double | V | — | Real component of peak-magnitude line-to-line voltage on phases AB. |
| measured_real_max_voltage_BC_in_interval | double | V | — | Real component of peak-magnitude line-to-line voltage on phases BC. |
| measured_real_max_voltage_CA_in_interval | double | V | — | Real component of peak-magnitude line-to-line voltage on phases CA. |
| measured_reactive_max_voltage_AB_in_interval | double | V | — | Imaginary component of peak-magnitude line-to-line voltage on phases AB. |
| measured_reactive_max_voltage_BC_in_interval | double | V | — | Imaginary component of peak-magnitude line-to-line voltage on phases BC. |
| measured_reactive_max_voltage_CA_in_interval | double | V | — | Imaginary component of peak-magnitude line-to-line voltage on phases CA. |
| measured_real_min_voltage_A_in_interval | double | V | — | Real component of minimum-magnitude line-to-neutral voltage on phase A. |
| measured_real_min_voltage_B_in_interval | double | V | — | Real component of minimum-magnitude line-to-neutral voltage on phase B. |
| measured_real_min_voltage_C_in_interval | double | V | — | Real component of minimum-magnitude line-to-neutral voltage on phase C. |
| measured_reactive_min_voltage_A_in_interval | double | V | — | Imaginary component of minimum-magnitude line-to-neutral voltage on phase A. |
| measured_reactive_min_voltage_B_in_interval | double | V | — | Imaginary component of minimum-magnitude line-to-neutral voltage on phase B. |
| measured_reactive_min_voltage_C_in_interval | double | V | — | Imaginary component of minimum-magnitude line-to-neutral voltage on phase C. |
| measured_real_min_voltage_AB_in_interval | double | V | — | Real component of minimum-magnitude line-to-line voltage on phases AB. |
| measured_real_min_voltage_BC_in_interval | double | V | — | Real component of minimum-magnitude line-to-line voltage on phases BC. |
| measured_real_min_voltage_CA_in_interval | double | V | — | Real component of minimum-magnitude line-to-line voltage on phases CA. |
| measured_reactive_min_voltage_AB_in_interval | double | V | — | Imaginary component of minimum-magnitude line-to-line voltage on phases AB. |
| measured_reactive_min_voltage_BC_in_interval | double | V | — | Imaginary component of minimum-magnitude line-to-line voltage on phases BC. |
| measured_reactive_min_voltage_CA_in_interval | double | V | — | Imaginary component of minimum-magnitude line-to-line voltage on phases CA. |
| measured_avg_voltage_A_mag_in_interval | double | V | — | Average line-to-neutral voltage magnitude on phase A. |
| measured_avg_voltage_B_mag_in_interval | double | V | — | Average line-to-neutral voltage magnitude on phase B. |
| measured_avg_voltage_C_mag_in_interval | double | V | — | Average line-to-neutral voltage magnitude on phase C. |
| measured_avg_voltage_AB_mag_in_interval | double | V | — | Average line-to-line voltage magnitude on phases AB. |
| measured_avg_voltage_BC_mag_in_interval | double | V | — | Average line-to-line voltage magnitude on phases BC. |
| measured_avg_voltage_CA_mag_in_interval | double | V | — | Average line-to-line voltage magnitude on phases CA. |

#### Interval Power Statistics

These properties report power statistics computed over the same repeating interval defined by `measured_stats_interval`, with the same activation and update behavior as the voltage statistics above. All are output only. Total three-phase statistics are listed first, followed by per-phase breakdowns. Averages are time-weighted.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| measured_real_max_power_in_interval | double | W | — | Maximum total real power. |
| measured_reactive_max_power_in_interval | double | VAr | — | Maximum total reactive power. |
| measured_real_min_power_in_interval | double | W | — | Minimum total real power. |
| measured_reactive_min_power_in_interval | double | VAr | — | Minimum total reactive power. |
| measured_avg_real_power_in_interval | double | W | — | Average total real power. |
| measured_avg_reactive_power_in_interval | double | VAr | — | Average total reactive power. |
| measured_real_max_power_A_in_interval | double | W | — | Maximum real power on phase A. |
| measured_reactive_max_power_A_in_interval | double | VAr | — | Maximum reactive power on phase A. |
| measured_real_min_power_A_in_interval | double | W | — | Minimum real power on phase A. |
| measured_reactive_min_power_A_in_interval | double | VAr | — | Minimum reactive power on phase A. |
| measured_avg_real_power_A_in_interval | double | W | — | Average real power on phase A. |
| measured_avg_reactive_power_A_in_interval | double | VAr | — | Average reactive power on phase A. |
| measured_real_max_power_B_in_interval | double | W | — | Maximum real power on phase B. |
| measured_reactive_max_power_B_in_interval | double | VAr | — | Maximum reactive power on phase B. |
| measured_real_min_power_B_in_interval | double | W | — | Minimum real power on phase B. |
| measured_reactive_min_power_B_in_interval | double | VAr | — | Minimum reactive power on phase B. |
| measured_avg_real_power_B_in_interval | double | W | — | Average real power on phase B. |
| measured_avg_reactive_power_B_in_interval | double | VAr | — | Average reactive power on phase B. |
| measured_real_max_power_C_in_interval | double | W | — | Maximum real power on phase C. |
| measured_reactive_max_power_C_in_interval | double | VAr | — | Maximum reactive power on phase C. |
| measured_real_min_power_C_in_interval | double | W | — | Minimum real power on phase C. |
| measured_reactive_min_power_C_in_interval | double | VAr | — | Minimum reactive power on phase C. |
| measured_avg_real_power_C_in_interval | double | W | — | Average real power on phase C. |
| measured_avg_reactive_power_C_in_interval | double | VAr | — | Average reactive power on phase C. |

#### Billing Properties

These properties configure and report the meter's energy billing functionality. Billing is only active when `bill_mode` is set to a value other than `NONE`. The monthly bill accumulates during each billing cycle and is finalized at midnight on `bill_day`, at which point `previous_monthly_bill` and `previous_monthly_energy` are updated.

Tiered pricing modes (`TIERED`, `TIERED_RTP`, `TIERED_TOU`) use up to three energy tiers. Energy consumed below `first_tier_energy` is priced at the base `price` (or `price_base` for `TIERED_RTP`). Energy above each tier threshold is priced at the corresponding tier price. If a higher tier is not defined, the previous tier's price applies to all remaining energy.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| bill_mode | enumeration | N/A | — | Billing structure. Valid values: <br/> 0 - `NONE` - Billing disabled (default). <br/> 1 - `UNIFORM` - Flat rate from `price`. <br/> 2 - `TIERED` - Price increases at energy thresholds. <br/> 3 - `HOURLY` - Real-time market pricing via `power_market`. <br/> 4 - `TIERED_RTP` - Real-time market pricing plus tiered base charges via `price_base`. <br/> 5 - `TIERED_TOU` - Time-of-use with tier selection based on cumulative energy. |
| bill_day | int32 | N/A | — | Day of month the bill is finalized (1â€“28). |
| monthly_fee | double | $ | — | Flat monthly service fee included as the base of each bill. |
| price | double | $/kWh | — | Current energy price. Updated from `power_market` in `HOURLY` and `TIERED_RTP` modes. |
| price_base | double | $/kWh | — | Base energy price used in `TIERED_RTP` mode for energy below the first tier. |
| power_market | object | N/A | — | Market object (auction or stubauction) providing the price signal. Required for `HOURLY` and `TIERED_RTP` modes. |
| first_tier_price | double | $/kWh | — | Energy price between `first_tier_energy` and `second_tier_energy`. |
| first_tier_energy | double | kWh | — | Energy threshold between base price and first tier. |
| second_tier_price | double | $/kWh | — | Energy price between `second_tier_energy` and `third_tier_energy`. |
| second_tier_energy | double | kWh | — | Energy threshold between first and second tiers. |
| third_tier_price | double | $/kWh | — | Energy price above `third_tier_energy`. |
| third_tier_energy | double | kWh | — | Energy threshold between second and third tiers. |
| monthly_bill | double | $ | — | Running bill for the current month. |
| monthly_energy | double | kWh | — | Cumulative energy consumed during the current billing cycle. |
| previous_monthly_bill | double | $ | — | Final bill from the previous billing cycle. |
| previous_monthly_energy | double | kWh | — | Total energy consumption from the previous billing cycle. |

#### Reliability Properties

These properties report the meter's service interruption status. The `customer_interrupted` flag is set automatically when a phase loss is detected through the fault check mechanism in the NR solver. The `customer_interrupted_secondary` flag indicates a momentary interruption and is automatically cleared to `false` at the start of each timestep.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| customer_interrupted | bool | N/A | — | Indicates sustained service interruption due to phase loss. |
| customer_interrupted_secondary | bool | N/A | — | Indicates momentary service interruption. |

The following properties are only available in builds compiled with `SUPPORT_OUTAGES` defined. All are input only.

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| sustained_count | int16 | N/A | — | Sustained interruption event counter. |
| momentary_count | int16 | N/A | — | Momentary interruption event counter. |
| total_count | int16 | N/A | — | Total interruption event counter. |
| s_flag | int16 | N/A | — | Sustained interruption state flag. |
| t_flag | int16 | N/A | — | Temporary interruption state flag. |
| pre_load | complex | N/A | — | Load value prior to an outage event. |

### Meter State of Development

Meter is considered a highly developed and validated model in terms of powerflow solutions, however, models using billing features have not been fully validated.


---

## Review Summary

### I/O Classification Errors Corrected

**16 properties changed from Input+Output to Output only** (unconditionally overwritten each postsync):
`measured_power`, `measured_power_A`, `measured_power_B`, `measured_power_C`, `measured_real_power`, `measured_reactive_power`, `measured_voltage_A`, `measured_voltage_B`, `measured_voltage_C`, `measured_voltage_AB`, `measured_voltage_BC`, `measured_voltage_CA`, `measured_current_A`, `measured_current_B`, `measured_current_C`, `monthly_energy`.

**6 properties changed from Input+Output to Input only** (only written during `init()`/`check_prices()`, not at runtime):
`first_tier_price`, `first_tier_energy`, `second_tier_price`, `second_tier_energy`, `third_tier_price`, `third_tier_energy`.

**4 properties changed from Output only to Input+Output** (no access restriction, conditionally written):
`previous_monthly_bill`, `measured_avg_voltage_AB_mag_in_interval`, `measured_avg_voltage_BC_mag_in_interval`, `measured_avg_voltage_CA_mag_in_interval`.

**1 property removed** (`measured_reactive`): commented out in source, not published.

### Factual Errors Fixed

- **`measured_real_energy` / `measured_reactive_energy`**: Removed "since it was reset" â€” `meter_reset` only clears `measured_demand`, not energy.
- **`measured_demand`**: Changed "peak power demand of downstream objects" to "greatest real power recorded" â€” code tracks max `measured_real_power` at this meter.
- **`bill_mode`**: Added missing `TIERED_RTP` and `TIERED_TOU` enumeration values.
- **`power_market`**: Extended scope from `HOURLY` only to also include `TIERED_RTP` â€” both modes require it per `check_prices()`.
- **`monthly_fee`**: Corrected "added on the first day of the billing cycle" â€” it is the base of `monthly_bill` every time `process_bill()` runs.
- **15 line-to-line interval voltage properties**: Corrected phase references from single phase ("phase A") to phase pairs ("phases AB").
- **`bill_mode` `HOURLY` description**: Removed claim that pricing "only updates on an hourly basis" â€” charges accumulate at simulation timestep resolution.

### Behavioral Claims Added to Section Intros (with source evidence)

| Claim | Source Evidence |
|---|---|
| Timestep properties default to âˆ’1 and must be positive to activate | `create()`: `measured_energy_delta_timestep = -1; measured_min_max_avg_timestep = -1;` and `postsync()` guards: `if (measured_energy_delta_timestep > 0)`, `if (measured_min_max_avg_timestep > 0)` |
| All instantaneous measurements are computed during postsync | Assignments in `postsync()`: `measured_voltage[0] = voltageA;`, `indiv_measured_power[0] = measured_voltage[0]*(~measured_current[0]);`, etc. |
| `measured_demand` can be reset via meter reset function | `meter_reset()`: `pMeter->measured_demand = 0;` |
| Cumulative energy uses `+=` preserving user initial values | `postsync()`: `measured_real_energy += measured_real_power * TO_HOURS(dt);` |
| `meter_reset` does not clear energy values | `meter_reset()` only assigns `measured_demand = 0` |
| Simulation warns if energy initial values are nonzero | `init()`: `gl_warning("meter:%d - %s - measured power or energy is not initialized to zero...")` |
| Interval voltage max/min compared by magnitude | `postsync()`: `if (last_measured_voltage[0].Mag() > last_measured_max_voltage_mag[0].Mag())` |
| Interval averages are time-weighted | `postsync()`: `last_measured_avg_voltage_mag[0] = ((interval_dt * last_measured_avg_voltage_mag[0]) + (dt * last_measured_voltage[0].Mag())) / (interval_dt + dt);` |
| `meter_power_consumption` divided equally across active phases | `BOTH_meter_sync_fxn()`: `power[0] += meter_power_consumption / no_phases;` |
| `customer_interrupted_secondary` cleared every timestep | `presync()`: `if (meter_interrupted_secondary) meter_interrupted_secondary = false;` |
| `customer_interrupted` set by phase-loss detection in NR solver | `BOTH_meter_sync_fxn()`: `if ((NR_busdata[TempNodeRef].origphases & NR_busdata[TempNodeRef].phases) != NR_busdata[TempNodeRef].origphases) meter_interrupted = true;` |
| Bill finalized on `bill_day` at midnight | `process_bill()`: `if (dtime.day == bill_day && dtime.hour == 0 && dtime.month != last_bill_month)` |
| Undefined higher tiers cascade from lower tier | `check_prices()`: `if(tier_price[1] == 0){ tier_price[1] = tier_price[0]; tier_energy[1] = tier_energy[0]; }` |
| `TIERED_RTP` uses `price_base` for base tier | `process_bill()` under `BM_TIERED_RTP`: `monthly_bill += last_price_base * monthly_energy;` |
| Probable source code bug in reactive max LN voltage | `postsync()`: `measured_reactive_max_voltage_in_interval[0] = last_measured_max_voltageD_mag[0].Im();` reads LL tracker instead of LN tracker `last_measured_max_voltage_mag[0].Im()` |

### Summary of Additional Changes

**58 properties changed from Input+Output to Output only** beyond the original Step 1 corrections:
- `measured_real_energy_delta`, `measured_reactive_energy_delta` â€” overwritten at interval boundary; no meaningful input use case.
- 30 interval voltage statistics â€” overwritten at interval boundary; no meaningful input use case.
- 24 interval power statistics â€” same reasoning.
- `monthly_bill` â€” overwritten by `process_bill()` whenever billing is active.
- `monthly_energy` â€” already corrected in Step 1.

**Kept as Input+Output** where a meaningful user-set value persists:
- `measured_real_energy`, `measured_reactive_energy` â€” `+=` accumulation preserves initial value.
- `measured_demand` â€” conditional write preserves user-set floor.
- `previous_monthly_bill`, `previous_monthly_energy` â€” only updated on bill day; user can represent pre-simulation history.
- `customer_interrupted`, `customer_interrupted_secondary` â€” user can set initial interruption state.




<details type="reasoning" done="true" duration="58">
<summary>Thought for 58 seconds</summary>

</details>


---

### Summary of Additional Changes

**58 properties changed from Input+Output to Output only** beyond the original Step 1 corrections:
- `measured_real_energy_delta`, `measured_reactive_energy_delta` â€” overwritten at interval boundary; no meaningful input use case.
- 30 interval voltage statistics â€” overwritten at interval boundary; no meaningful input use case.
- 24 interval power statistics â€” same reasoning.
- `monthly_bill` â€” overwritten by `process_bill()` whenever billing is active.
- `monthly_energy` â€” already corrected in Step 1.

**Kept as Input+Output** where a meaningful user-set value persists:
- `measured_real_energy`, `measured_reactive_energy` â€” `+=` accumulation preserves initial value.
- `measured_demand` â€” conditional write preserves user-set floor.
- `previous_monthly_bill`, `previous_monthly_energy` â€” only updated on bill day; user can represent pre-simulation history.
- `customer_interrupted`, `customer_interrupted_secondary` â€” user can set initial interruption state.
