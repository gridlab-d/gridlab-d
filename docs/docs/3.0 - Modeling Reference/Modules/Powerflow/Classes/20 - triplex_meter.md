## Triplex Meter

Triplex meters provide similar functionality for triplex systems that **meter** objects do in three-phase systems. A triplex meter provides a measurement point for power and energy on the system at a specific point. Coupled with a **recorder** or **collector**, the **triplex_meter** object provides a method determine how much power and energy have been used by downstream connections, as well as how much current is flowing through the meter object at the present time. A typical implementation would be 
    
    
    object triplex_meter {
    	name TrplMtr1;
    	phases AS;
    	nominal_voltage 120.0;
    	}
    

### Triplex Meter Parameters

A **triplex_meter** object is a derivation of the **triplex_node** object and thus inherits all of its parameters. Most **triplex_meter** parameters are meant to be read-only, but can be set if the need arises. 

Property Name  | Type  | Unit  | Description   
---|---|---|---  
**measured_real_energy**  | double  | Watt-hours  | Measurement of the real energy (accumulation of the real power) that has flowed through the **triplex_meter** since it was reset.   
**measured_reactive_energy**  | double  | Volt-Amperes-hours  | Measurement of the reactive energy (accumulation of the reactive power) that has flowed through the **triplex_meter** since it was reset.   
**measured_power**  | complex  | Volt-Amperes  | Measurement of the complex power flowing through the triplex meter at that instant in time.   
**measured_demand**  | double  | Watts  | Measurement of the peak power demand of downstream objects.   
**measured_real_power**  | double  | Watts  | Measurement of the real portion of the power flowing through the triplex meter at that instant in time.   
**measured_reactive_power**  | double  | Volt-Amperes reactive  | Measurement of the reactive portion of the power flowing through the meter at that instant in time.   
**indiv_measured_power_1**  | complex  | Volt-Amperes  | Measures the complex power flowing through the meter on phase `1`.   
**indiv_measured_power_2**  | complex  | Volt-Amperes  | Measures the complex power flowing through the meter on phase `2`.   
**indiv_measured_power_N**  | complex  | Volt-Amperes  | Measures the complex power flowing through the meter on phase `N`.   
**measured_voltage_1**  | complex  | Volts  | Measurement of the voltage on phase `1` of the split-phase or triplex system. May or may not be as up to date as reading `voltage_1` directly.   
**measured_voltage_2**  | complex  | Volts  | Measurement of the voltage on phase `2` of the split-phase or triplex system. May or may not be as up to date as reading `voltage_2` directly.   
**measured_voltage_N**  | complex  | Volts  | Measurement of the voltage on the neutral phase of the split-phase or triplex system.. May or may not be as up to date as reading `voltage_N` directly.   
**measured_current_1**  | complex  | Amperes  | Measurement of the current on phase `1` of the triplex meter at that instant in time.   
**measured_current_2**  | complex  | Amperes  | Measurement of the current on phase `2` of the triplex meter at that instant in time.   
**measured_current_N**  | complex  | Amperes  | Measurement of the current on the neutral phase of the triplex meter at that instant in time.   
**bill_day**  | int32  | N/A  | Sets the date of the month at which the final monthly bill is calculated (at midnight). Maximum value is 28.   
**price**  | double  | $/kWh  | Determines the instantaneous market price of energy. Where the price comes from depends upon the `bill_mode`.   
**monthly_fee**  | double  | $  | This is a recurrent monthly service charge that is added into the bill on the first day of the billing cycle (no pro-rating).   
**monthly_bill**  | double  | $  | This is the running monthly bill at the particular meter as a function of price and the amount of energy used in that month.   
**previous_monthly_bill**  | double  | $  | This stores the total bill from the previous month after the bill has been processed on the `bill_day`.   
**monthly_energy**  | double  | kWh  | The rolling amount of energy consumed during the current month at that meter. Used to calculate `monthly_bill`.   
**previous_monthly_energy**  | double  | kWh  | Stores the previous month's total energy consumption.   
**bill_mode**  | enumeration  | N/A  | Describes the method in which the meter receives its price signal. <br/> 0 - `NONE` \- Billing is not used (default). <br/> 1 - `UNIFORM` \- A static price is used through variable `price`, however, this may change over time using a player or schedule. <br/> 2 - `TIERED` \- Tiered pricing plan where the price changes as a function of the amount of energy used in the month. See `tier_price` and `tier_energy`. <br/> 3 - `HOURLY` \- This is used in conjunction with an `auction` or `stubauction` object. Receives its price directly from a market signal, but only updates on an hourly basis. Used in conjunction with `power_market`. NOTE: while this says "hourly", it will actually update any time the price changes in the auction. <br/> 4 - `TIERED_RTP` \- Merges TIERED and HOURLY modes. Applies both a real time price via the auction to energy usage, but then also applies block / tiered rates to the total monthly energy use. <br/> power_market  | object  | N/A  | When using ` bill_mode HOURLY`, this points the meter to the object where it will receive a price signal.
**first_tier_price**  | double  | $/kWh  | When using ` bill_mode TIERED`, this determines the energy price after energy increases above `first_tier_energy`, but below `second_tier_energy`. If `second_tier_energy` is not defined, then this price will be used to infinity. While energy is below `first_tier_energy`, `price` is used to calculate the `monthly_bill`.   
**second_tier_price**  | double  | $/kWh  | When using ` bill_mode TIERED`, this determines the energy price after energy increases above `second_tier_energy`, but below `third_tier_energy`. If `third_tier_energy` is not defined, then this price will be used to infinity.   
**third_tier_price**  | double  | $/kWh  | When using ` bill_mode TIERED`, this determines the energy price after energy increases above `third_tier_energy` and is used to infinite energy.   
**first_tier_energy**  | double  | kWh  | Determines the point at which the price of energy changes from `price` to `first_tier_price`.   
**second_tier_energy**  | double  | kWh  | Determines the point at which the price of energy changes from `first_tier_price` to `second_tier_price`.   
**third_tier_energy**  | double  | kWh  | Determines the point at which the price of energy changes from `second_tier_price` to `third_tier_price`.   
  
### Triplex Meter State of Development

Triplex Meter is considered a highly developed and validated model in terms of powerflow solutions, however, models using billing have not been fully validated. Additional features will be added as needed. 

