## Triplex Meter

!!! warning
    This page was automatically generated and requires review.

Triplex meters provide similar functionality for triplex systems that **meter** objects do in three-phase systems. A triplex meter provides a measurement point for power and energy on the system at a specific point. Coupled with a **recorder** or **collector**, the **triplex_meter** object provides a method determine how much power and energy have been used by downstream connections, as well as how much current is flowing through the meter object at the present time. A typical implementation would be 
    
    
    object triplex_meter {
    	name TrplMtr1;
    	phases AS;
    	nominal_voltage 120.0;
    	}

### Triplex Meter Parameters

#### Properties

**triplex_meter** objects are derived from **[triplex_node](19-triplex_node.md)** objects, so any parameters of the **[triplex_node](19-triplex_node.md)** object are available as well.

The I/O column indicates whether a property is user-settable input (I), simulation-computed output (O), or both (IO).

| Property Name | Type | Unit | I/O | Description |
| --- | --- | --- | --- | --- |
| measured_real_energy | double | Wh | IO | Measurement of the real energy (accumulation of the real power) that has flowed through the **triplex_meter** since it was reset. |
| measured_real_energy_delta | double | Wh | IO | ⚠️ delta in metered real energy consumption from last specified measured_energy_delta_timestep |
| measured_reactive_energy | double | VAh | IO | Measurement of the reactive energy (accumulation of the reactive power) that has flowed through the **triplex_meter** since it was reset. |
| measured_reactive_energy_delta | double | VAh | IO | ⚠️ delta in metered reactive energy consumption from last specified measured_energy_delta_timestep |
| measured_energy_delta_timestep | double | s | I | ⚠️ Period of timestep for real and reactive delta energy calculation |
| measured_power | complex | VA | IO | Measurement of the complex power flowing through the triplex meter at that instant in time. |
| indiv_measured_power_1 | complex | VA | O | Measures the complex power flowing through the meter on phase `1`. |
| indiv_measured_power_2 | complex | VA | O | Measures the complex power flowing through the meter on phase `2`. |
| indiv_measured_power_N | complex | VA | O | Measures the complex power flowing through the meter on phase `N`. |
| measured_demand | double | W | IO | Measurement of the peak power demand of downstream objects. |
| measured_real_power | double | W | IO | Measurement of the real portion of the power flowing through the triplex meter at that instant in time. |
| measured_reactive_power | double | VAr | IO | Measurement of the reactive portion of the power flowing through the meter at that instant in time. |
| meter_power_consumption | complex | VA | I | ⚠️ power consumed by meter operation |
| measured_voltage_1 | complex | V | O | Measurement of the voltage on phase `1` of the split-phase or triplex system. May or may not be as up to date as reading `voltage_1` directly. |
| measured_voltage_2 | complex | V | O | Measurement of the voltage on phase `2` of the split-phase or triplex system. May or may not be as up to date as reading `voltage_2` directly. |
| measured_voltage_N | complex | V | O | Measurement of the voltage on the neutral phase of the split-phase or triplex system.. May or may not be as up to date as reading `voltage_N` directly. |
| measured_real_max_voltage_1_in_interval | double | V | IO | ⚠️ measured real max line-to-ground voltage on phase 1 over a specified interval |
| measured_real_max_voltage_2_in_interval | double | V | IO | ⚠️ measured real max line-to-ground voltage on phase 2 over a specified interval |
| measured_real_max_voltage_12_in_interval | double | V | IO | ⚠️ measured real max line-to-ground voltage on phase 12 over a specified interval |
| measured_imag_max_voltage_1_in_interval | double | V | IO | ⚠️ measured imaginary max line-to-ground voltage on phase 1 over a specified interval |
| measured_imag_max_voltage_2_in_interval | double | V | IO | ⚠️ measured imaginary max line-to-ground voltage on phase 2 over a specified interval |
| measured_imag_max_voltage_12_in_interval | double | V | IO | ⚠️ measured imaginary max line-to-ground voltage on phase 12 over a specified interval |
| measured_real_min_voltage_1_in_interval | double | V | IO | ⚠️ measured real min line-to-ground voltage on phase 1 over a specified interval |
| measured_real_min_voltage_2_in_interval | double | V | IO | ⚠️ measured real min line-to-ground voltage on phase 2 over a specified interval |
| measured_real_min_voltage_12_in_interval | double | V | IO | ⚠️ measured real min line-to-ground voltage on phase 12 over a specified interval |
| measured_imag_min_voltage_1_in_interval | double | V | IO | ⚠️ measured imaginary min line-to-ground voltage on phase 1 over a specified interval |
| measured_imag_min_voltage_2_in_interval | double | V | IO | ⚠️ measured imaginary min line-to-ground voltage on phase 2 over a specified interval |
| measured_imag_min_voltage_12_in_interval | double | V | IO | ⚠️ measured imaginary min line-to-ground voltage on phase 12 over a specified interval |
| measured_avg_voltage_1_mag_in_interval | double | V | IO | ⚠️ measured average line-to-ground voltage magnitude on phase 1 over a specified interval |
| measured_avg_voltage_2_mag_in_interval | double | V | IO | ⚠️ measured average line-to-ground voltage magnitude on phase 2 over a specified interval |
| measured_avg_voltage_12_mag_in_interval | double | V | IO | ⚠️ measured average line-to-ground voltage magnitude on phase 12 over a specified interval |
| measured_real_max_power_in_interval | double | W | IO | ⚠️ measured maximum real power over a specified interval |
| measured_reactive_max_power_in_interval | double | VAr | IO | ⚠️ measured maximum reactive power over a specified interval |
| measured_real_min_power_in_interval | double | W | IO | ⚠️ measured minimum real power over a specified interval |
| measured_reactive_min_power_in_interval | double | VAr | IO | ⚠️ measured minimum reactive power over a specified interval |
| measured_avg_real_power_in_interval | double | W | IO | ⚠️ measured average real power over a specified interval |
| measured_avg_reactive_power_in_interval | double | VAr | IO | ⚠️ measured average reactive power over a specified interval |
| measured_stats_interval | double | s | I | ⚠️ Period of timestep for min/max/average calculations |
| measured_current_1 | complex | A | O | Measurement of the current on phase `1` of the triplex meter at that instant in time. |
| measured_current_2 | complex | A | O | Measurement of the current on phase `2` of the triplex meter at that instant in time. |
| measured_current_N | complex | A | O | Measurement of the current on the neutral phase of the triplex meter at that instant in time. |
| customer_interrupted | bool | N/A | IO | ⚠️ Reliability flag - goes active if the customer is in an interrupted state |
| customer_interrupted_secondary | bool | N/A | IO | ⚠️ Reliability flag - goes active if the customer is in a secondary interrupted state - i.e., momentary |
| sustained_count | int16 | N/A | I | ⚠️ reliability sustained event counter |
| momentary_count | int16 | N/A | I | ⚠️ reliability momentary event counter |
| total_count | int16 | N/A | I | ⚠️ reliability total event counter |
| s_flag | int16 | N/A | I | ⚠️ reliability flag that gets set if the meter experienced more than n sustained interruptions |
| t_flag | int16 | N/A | I | ⚠️ reliability flage that gets set if the meter experienced more than n events total |
| pre_load | complex | N/A | I | ⚠️ the load prior to being interrupted |
| monthly_bill | double | $ | IO | This is the running monthly bill at the particular meter as a function of price and the amount of energy used in that month. |
| previous_monthly_bill | double | $ | O | This stores the total bill from the previous month after the bill has been processed on the `bill_day`. |
| previous_monthly_energy | double | kWh | IO | Stores the previous month's total energy consumption. |
| monthly_fee | double | $ | I | This is a recurrent monthly service charge that is added into the bill on the first day of the billing cycle (no pro-rating). |
| monthly_energy | double | kWh | IO | The rolling amount of energy consumed during the current month at that meter. Used to calculate `monthly_bill`. |
| bill_mode | enumeration | N/A | I | Describes the method in which the meter receives its price signal. <br/> 0 - `NONE` \- Billing is not used (default). <br/> 1 - `UNIFORM` \- A static price is used through variable `price`, however, this may change over time using a player or schedule. <br/> 2 - `TIERED` \- Tiered pricing plan where the price changes as a function of the amount of energy used in the month. See `tier_price` and `tier_energy`. <br/> 3 - `HOURLY` \- This is used in conjunction with an `auction` or `stubauction` object. Receives its price directly from a market signal, but only updates on an hourly basis. Used in conjunction with `power_market`. NOTE: while this says "hourly", it will actually update any time the price changes in the auction. <br/> 4 - `TIERED_RTP` \- Merges TIERED and HOURLY modes. Applies both a real time price via the auction to energy usage, but then also applies block / tiered rates to the total monthly energy use. <br/> power_market |
| power_market | object | N/A | I | ⚠️ Designates the auction object where prices are read from for bill mode |
| bill_day | int32 | N/A | I | Sets the date of the month at which the final monthly bill is calculated (at midnight). Maximum value is 28. |
| price | double | $/kWh | IO | Determines the instantaneous market price of energy. Where the price comes from depends upon the `bill_mode`. |
| price_base | double | $/kWh | I | ⚠️ Used only in TIERED_RTP mode to describe the price before the first tier |
| first_tier_price | double | $/kWh | IO | When using ` bill_mode TIERED`, this determines the energy price after energy increases above `first_tier_energy`, but below `second_tier_energy`. If `second_tier_energy` is not defined, then this price will be used to infinity. While energy is below `first_tier_energy`, `price` is used to calculate the `monthly_bill`. |
| first_tier_energy | double | kWh | IO | Determines the point at which the price of energy changes from `price` to `first_tier_price`. |
| second_tier_price | double | $/kWh | IO | When using ` bill_mode TIERED`, this determines the energy price after energy increases above `second_tier_energy`, but below `third_tier_energy`. If `third_tier_energy` is not defined, then this price will be used to infinity. |
| second_tier_energy | double | kWh | IO | Determines the point at which the price of energy changes from `first_tier_price` to `second_tier_price`. |
| third_tier_price | double | $/kWh | IO | When using ` bill_mode TIERED`, this determines the energy price after energy increases above `third_tier_energy` and is used to infinite energy. |
| third_tier_energy | double | kWh | IO | Determines the point at which the price of energy changes from `second_tier_price` to `third_tier_price`. |

### Triplex Meter State of Development

Triplex Meter is considered a highly developed and validated model in terms of powerflow solutions, however, models using billing have not been fully validated. Additional features will be added as needed. 
