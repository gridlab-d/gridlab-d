## Meter

!!! warning
    This page was automatically generated and requires review.

Meters provide a measurement point for power and energy on the system at a specific point. Coupled with a **recorder** or **collector**, the **meter** object provides a method determine how much power and energy have been used by downstream connections, as well as how much current is flowing through the meter object at the present time. A typical implementation would be 
    
    
    object meter {
    	name Mtr1;
    	phases ABC;
    	nominal_voltage 4800.0;
    	}

### Meter Parameters

#### Properties

**meter** objects are derived from **[node](03-node.md)** objects, so any parameters of the **[node](03-node.md)** object are available as well.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| measured_real_energy | double | Wh | ✓ | ✓ | Measurement of the real energy (accumulation of the real power) that has flowed through the **meter** since it was reset. |
| measured_real_energy_delta | double | Wh | ✓ | ✓ | ⚠️ delta in metered real energy consumption from last specified measured_energy_delta_timestep |
| measured_reactive_energy | double | VAh | ✓ | ✓ | Measurement of the reactive energy (accumulation of the reactive power) that has flowed through the **meter** since it was reset. |
| measured_reactive_energy_delta | double | VAh | ✓ | ✓ | ⚠️ delta in metered reactive energy consumption from last specified measured_energy_delta_timestep |
| measured_energy_delta_timestep | double | s | ✓ |  | ⚠️ Period of timestep for real and reactive delta energy calculation |
| measured_power | complex | VA | ✓ | ✓ | Measurement of the complex power flowing through the meter at that instant in time. |
| measured_power_A | complex | VA | ✓ | ✓ | Measurement of the complex power flowing through the meter at that instant in time on phase `A`. |
| measured_power_B | complex | VA | ✓ | ✓ | Measurement of the complex power flowing through the meter at that instant in time on phase `B`. |
| measured_power_C | complex | VA | ✓ | ✓ | Measurement of the complex power flowing through the meter at that instant in time on phase `C`. |
| measured_demand | double | W | ✓ | ✓ | Measurement of the peak power demand of downstream objects. |
| measured_real_power | double | W | ✓ | ✓ | Measurement of the real portion of the power flowing through the meter at that instant in time. |
| measured_reactive_power | double | VAr | ✓ | ✓ | Measurement of the reactive portion of the power flowing through the meter at that instant in time. |
| meter_power_consumption | complex | VA | ✓ |  | ⚠️ metered power used for operating the meter; standby and communication losses |
| measured_voltage_A | complex | V | ✓ | ✓ | Measurement of the voltage on phase `A` of the meter. May or may not be as up to date as reading `voltage_A` directly. |
| measured_voltage_B | complex | V | ✓ | ✓ | Measurement of the voltage on phase `B` of the meter. May or may not be as up to date as reading `voltage_B` directly. |
| measured_voltage_C | complex | V | ✓ | ✓ | Measurement of the voltage on phase `C` of the meter. May or may not be as up to date as reading `voltage_C` directly. |
| measured_voltage_AB | complex | V | ✓ | ✓ | ⚠️ measured line-to-line voltage on phase AB |
| measured_voltage_BC | complex | V | ✓ | ✓ | ⚠️ measured line-to-line voltage on phase BC |
| measured_voltage_CA | complex | V | ✓ | ✓ | ⚠️ measured line-to-line voltage on phase CA |
| measured_real_max_voltage_A_in_interval | double | V | ✓ | ✓ | ⚠️ measured real max line-to-neutral voltage on phase A over a specified interval |
| measured_real_max_voltage_B_in_interval | double | V | ✓ | ✓ | ⚠️ measured real max line-to-neutral voltage on phase B over a specified interval |
| measured_real_max_voltage_C_in_interval | double | V | ✓ | ✓ | ⚠️ measured real max line-to-neutral voltage on phase C over a specified interval |
| measured_reactive_max_voltage_A_in_interval | double | V | ✓ | ✓ | ⚠️ measured reactive max line-to-neutral voltage on phase A over a specified interval |
| measured_reactive_max_voltage_B_in_interval | double | V | ✓ | ✓ | ⚠️ measured reactive max line-to-neutral voltage on phase B over a specified interval |
| measured_reactive_max_voltage_C_in_interval | double | V | ✓ | ✓ | ⚠️ measured reactive max line-to-neutral voltage on phase C over a specified interval |
| measured_real_max_voltage_AB_in_interval | double | V | ✓ | ✓ | ⚠️ measured real max line-to-line voltage on phase A over a specified interval |
| measured_real_max_voltage_BC_in_interval | double | V | ✓ | ✓ | ⚠️ measured real max line-to-line voltage on phase B over a specified interval |
| measured_real_max_voltage_CA_in_interval | double | V | ✓ | ✓ | ⚠️ measured real max line-to-line voltage on phase C over a specified interval |
| measured_reactive_max_voltage_AB_in_interval | double | V | ✓ | ✓ | ⚠️ measured reactive max line-to-line voltage on phase A over a specified interval |
| measured_reactive_max_voltage_BC_in_interval | double | V | ✓ | ✓ | ⚠️ measured reactive max line-to-line voltage on phase B over a specified interval |
| measured_reactive_max_voltage_CA_in_interval | double | V | ✓ | ✓ | ⚠️ measured reactive max line-to-line voltage on phase C over a specified interval |
| measured_real_min_voltage_A_in_interval | double | V | ✓ | ✓ | ⚠️ measured real min line-to-neutral voltage on phase A over a specified interval |
| measured_real_min_voltage_B_in_interval | double | V | ✓ | ✓ | ⚠️ measured real min line-to-neutral voltage on phase B over a specified interval |
| measured_real_min_voltage_C_in_interval | double | V | ✓ | ✓ | ⚠️ measured real min line-to-neutral voltage on phase C over a specified interval |
| measured_reactive_min_voltage_A_in_interval | double | V | ✓ | ✓ | ⚠️ measured reactive min line-to-neutral voltage on phase A over a specified interval |
| measured_reactive_min_voltage_B_in_interval | double | V | ✓ | ✓ | ⚠️ measured reactive min line-to-neutral voltage on phase B over a specified interval |
| measured_reactive_min_voltage_C_in_interval | double | V | ✓ | ✓ | ⚠️ measured reactive min line-to-neutral voltage on phase C over a specified interval |
| measured_real_min_voltage_AB_in_interval | double | V | ✓ | ✓ | ⚠️ measured real min line-to-line voltage on phase A over a specified interval |
| measured_real_min_voltage_BC_in_interval | double | V | ✓ | ✓ | ⚠️ measured real min line-to-line voltage on phase B over a specified interval |
| measured_real_min_voltage_CA_in_interval | double | V | ✓ | ✓ | ⚠️ measured real min line-to-line voltage on phase C over a specified interval |
| measured_reactive_min_voltage_AB_in_interval | double | V | ✓ | ✓ | ⚠️ measured reactive min line-to-line voltage on phase A over a specified interval |
| measured_reactive_min_voltage_BC_in_interval | double | V | ✓ | ✓ | ⚠️ measured reactive min line-to-line voltage on phase B over a specified interval |
| measured_reactive_min_voltage_CA_in_interval | double | V | ✓ | ✓ | ⚠️ measured reactive min line-to-line voltage on phase C over a specified interval |
| measured_avg_voltage_A_mag_in_interval | double | V | ✓ | ✓ | ⚠️ measured avg line-to-neutral voltage magnitude on phase A over a specified interval |
| measured_avg_voltage_B_mag_in_interval | double | V | ✓ | ✓ | ⚠️ measured avg line-to-neutral voltage magnitude on phase B over a specified interval |
| measured_avg_voltage_C_mag_in_interval | double | V | ✓ | ✓ | ⚠️ measured avg line-to-neutral voltage magnitude on phase C over a specified interval |
| measured_avg_voltage_AB_mag_in_interval | double | V |  | ✓ | ⚠️ measured avg line-to-line voltage magnitude on phase A over a specified interval |
| measured_avg_voltage_BC_mag_in_interval | double | V |  | ✓ | ⚠️ measured avg line-to-line voltage magnitude on phase B over a specified interval |
| measured_avg_voltage_CA_mag_in_interval | double | V |  | ✓ | ⚠️ measured avg line-to-line voltage magnitude on phase C over a specified interval |
| measured_real_max_power_in_interval | double | W | ✓ | ✓ | ⚠️ measured maximum real power over a specified interval |
| measured_reactive_max_power_in_interval | double | VAr | ✓ | ✓ | ⚠️ measured maximum reactive power over a specified interval |
| measured_real_min_power_in_interval | double | W | ✓ | ✓ | ⚠️ measured minimum real power over a specified interval |
| measured_reactive_min_power_in_interval | double | VAr | ✓ | ✓ | ⚠️ measured minimum reactive power over a specified interval |
| measured_avg_real_power_in_interval | double | W | ✓ | ✓ | ⚠️ measured average real power over a specified interval |
| measured_avg_reactive_power_in_interval | double | VAr | ✓ | ✓ | ⚠️ measured average reactive power over a specified interval |
| measured_real_max_power_A_in_interval | double | W | ✓ | ✓ | ⚠️ measured A phase maximum real power over a specified interval |
| measured_reactive_max_power_A_in_interval | double | VAr | ✓ | ✓ | ⚠️ measured A phase maximum reactive power over a specified interval |
| measured_real_min_power_A_in_interval | double | W | ✓ | ✓ | ⚠️ measured A phase minimum real power over a specified interval |
| measured_reactive_min_power_A_in_interval | double | VAr | ✓ | ✓ | ⚠️ measured A phase minimum reactive power over a specified interval |
| measured_avg_real_power_A_in_interval | double | W | ✓ | ✓ | ⚠️ measured A phase average real power over a specified interval |
| measured_avg_reactive_power_A_in_interval | double | VAr | ✓ | ✓ | ⚠️ measured A phase average reactive power over a specified interval |
| measured_real_max_power_B_in_interval | double | W | ✓ | ✓ | ⚠️ measured B phase maximum real power over a specified interval |
| measured_reactive_max_power_B_in_interval | double | VAr | ✓ | ✓ | ⚠️ measured B phase maximum reactive power over a specified interval |
| measured_real_min_power_B_in_interval | double | W | ✓ | ✓ | ⚠️ measured B phase minimum real power over a specified interval |
| measured_reactive_min_power_B_in_interval | double | VAr | ✓ | ✓ | ⚠️ measured B phase minimum reactive power over a specified interval |
| measured_avg_real_power_B_in_interval | double | W | ✓ | ✓ | ⚠️ measured B phase average real power over a specified interval |
| measured_avg_reactive_power_B_in_interval | double | VAr | ✓ | ✓ | ⚠️ measured B phase average reactive power over a specified interval |
| measured_real_max_power_C_in_interval | double | W | ✓ | ✓ | ⚠️ measured C phase maximum real power over a specified interval |
| measured_reactive_max_power_C_in_interval | double | VAr | ✓ | ✓ | ⚠️ measured C phase maximum reactive power over a specified interval |
| measured_real_min_power_C_in_interval | double | W | ✓ | ✓ | ⚠️ measured C phase minimum real power over a specified interval |
| measured_reactive_min_power_C_in_interval | double | VAr | ✓ | ✓ | ⚠️ measured C phase minimum reactive power over a specified interval |
| measured_avg_real_power_C_in_interval | double | W | ✓ | ✓ | ⚠️ measured C phase average real power over a specified interval |
| measured_avg_reactive_power_C_in_interval | double | VAr | ✓ | ✓ | ⚠️ measured C phase average reactive power over a specified interval |
| measured_stats_interval | double | s | ✓ |  | ⚠️ Period of timestep for min/max/average calculations |
| measured_current_A | complex | A | ✓ | ✓ | Measurement of the current on phase `A` of the meter at that instant in time. |
| measured_current_B | complex | A | ✓ | ✓ | Measurement of the current on phase `B` of the meter at that instant in time. |
| measured_current_C | complex | A | ✓ | ✓ | Measurement of the current on phase `C` of the meter at that instant in time. |
| customer_interrupted | bool | N/A | ✓ | ✓ | ⚠️ Reliability flag - goes active if the customer is in an &#x27;interrupted&#x27; state |
| customer_interrupted_secondary | bool | N/A | ✓ | ✓ | ⚠️ Reliability flag - goes active if the customer is in an &#x27;secondary interrupted&#x27; state - i.e., momentary |
| sustained_count | int16 | N/A | ✓ |  |  |
| momentary_count | int16 | N/A | ✓ |  |  |
| total_count | int16 | N/A | ✓ |  |  |
| s_flag | int16 | N/A | ✓ |  |  |
| t_flag | int16 | N/A | ✓ |  |  |
| pre_load | complex | N/A | ✓ |  |  |
| monthly_bill | double | $ | ✓ | ✓ | This is the running monthly bill at the particular meter as a function of price and the amount of energy used in that month. |
| previous_monthly_bill | double | $ |  | ✓ | This stores the total bill from the previous month after the bill has been processed on the `bill_day`. |
| previous_monthly_energy | double | kWh | ✓ | ✓ | Stores the previous month&#x27;s total energy consumption. |
| monthly_fee | double | $ | ✓ |  | This is a recurrent monthly service charge that is added into the bill on the first day of the billing cycle (no pro-rating). |
| monthly_energy | double | kWh | ✓ | ✓ | The rolling amount of energy consumed during the current month at that meter. Used to calculate `monthly_bill`. |
| bill_mode | enumeration | N/A | ✓ | ✓ | Describes the method in which the meter receives its price signal. &lt;br/&gt; 0 - `NONE` \- Billing is not used (default). &lt;br/&gt; 1 - `UNIFORM` \- A static price is used through variable `price`, however, this may change over time using a player or schedule. &lt;br/&gt; 2 - `TIERED` \- Tiered pricing plan where the price changes as a function of the amount of energy used in the month. See `tier_price` and `tier_energy`. &lt;br/&gt; 3 - `HOURLY` \- This is used in conjunction with an `auction` or `stubauction` object. Receives its price directly from a market signal, but only updates on an hourly basis. Used in conjunction with `power_market`. |
| power_market | object | N/A | ✓ | ✓ | When using ` bill_mode HOURLY`, this points the meter to the object where it will receive a price signal. |
| bill_day | int32 | N/A | ✓ |  | Sets the date of the month at which the final monthly bill is calculated (at midnight). Maximum value is 28. |
| price | double | $/kWh | ✓ | ✓ | Determines the instantaneous market price of energy. Where the price comes from depends upon the `bill_mode`. |
| price_base | double | $/kWh | ✓ |  | ⚠️ Used only in TIERED_RTP mode to describe the price before the first tier |
| first_tier_price | double | $/kWh | ✓ | ✓ | When using ` bill_mode TIERED`, this determines the energy price after energy increases above `first_tier_energy`, but below `second_tier_energy`. If `second_tier_energy` is not defined, then this price will be used to infinity. While energy is below `first_tier_energy`, `price` is used to calculate the `monthly_bill`. |
| first_tier_energy | double | kWh | ✓ | ✓ | Determines the point at which the price of energy changes from `price` to `first_tier_price`. |
| second_tier_price | double | $/kWh | ✓ | ✓ | When using ` bill_mode TIERED`, this determines the energy price after energy increases above `second_tier_energy`, but below `third_tier_energy`. If `third_tier_energy` is not defined, then this price will be used to infinity. |
| second_tier_energy | double | kWh | ✓ | ✓ | Determines the point at which the price of energy changes from `first_tier_price` to `second_tier_price`. |
| third_tier_price | double | $/kWh | ✓ | ✓ | When using ` bill_mode TIERED`, this determines the energy price after energy increases above `third_tier_energy` and is used to infinite energy. |
| third_tier_energy | double | kWh | ✓ | ✓ | Determines the point at which the price of energy changes from `second_tier_price` to `third_tier_price`. |
| measured_reactive | double | kVar | ✓ |  |  |

### Meter State of Development

Meter is considered a highly developed and validated model in terms of powerflow solutions, however, models using billing features have not been fully validated. 
