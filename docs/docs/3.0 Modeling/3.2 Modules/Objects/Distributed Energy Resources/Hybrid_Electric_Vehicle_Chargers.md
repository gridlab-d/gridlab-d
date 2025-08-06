# Hybrid Electric Vehicle Chargers

In December 2008 the **residential** module received a preliminary implementation of all-electric and plug-in hybrid-electric vehicle charger loads for inclusion in Diablo (Version 2.0). The following describes that implementation and the requiremenets for subsequent enhancements (noted as **TODO**). 

## EV/PHEV Model

There are only two types of electric vehicles supported: 

  * **ELECTRIC** pure electric vehicle with limited range based on battery capacity
  * **HYBRID** hybrid electric vehicle with longer range based on fuel capacity
The **evcharger** simulation is based on demand state profile of the vehicle. When the vehicle is at home, it has a probability of leaving on one of 3 trips, as shown in Figure 1. 

  * **WORK** trip to work (standard distance defined by trip.d_work)
  * **SHORTTRIP** random trip up to 50 miles (battery will not be fully discharged) (**TODO**)
  * **LONGTRIP** random trip over 50 miles (battery will be discharged up to 25%) but only possible for **HYBRID** vehicles (**TODO**)

![EV/PHEV trip state diagram](../../images/300px-Slide1.PNG)


Figure 1 - EV/PHEV trip state diagram

When away, the probability of a return is used to determine when the vehicle returns, as shown in Figure 2. 

![Daily EV/PHEV arrival/departure probabilities](../../images/300px-Slide2.PNG)

Figure 2 - Daily EV/PHEV arrival/departure probabilities

The charger power can have one of three levels: 

  * LOW 120V/15A charger
  * MEDIUM 240V/30A charger
  * HIGH 240V/60A charger
The trip distances are used to estimate the battery charge upon return according to the following rules: 

  1. A work trip discharges the battery depending on whether charge_at_work is defined. If charging at work is allowed, the battery will discharge for 1 trip (back only), otherwise it will discharge for 2 trips (there and back). The assumption at this time is that there is enough charging capacity at work to make the time at work inconsequential.
  2. A short trip discharges the battery based on the distance traveled.
  3. A long trip discharges the battery to 25%, but is only possible with hybrids.
In all cases, if the trip distance is greater than 50 miles and the car is a hybrid, the discharge will be down to 25%. 

Heat fraction ratio is used to calculate the internal gain from plug loads during the heating phase. Note that this heat gain is to the residential indoor air, not the garage air. So the fraction must account for the very weak coupling between the two, if any. 

## Demand profiles

The format of the demand profile is as follows: 
    
    _DAYTYPE_
    _DIRTRIP_ ,_DIRTRIP_ ,...
    #.###,#.###,...
    #.###,#.###,...
    .
    .
    .
    #.###,#.###,...

where 

  * _DAYTYPE_ is either **WEEKDAY** or **WEEKEND** ,
  * _DIR_ is either **ARR** or **DEP** , and
  * _TRIP_ is either **HOME** , **WORK** , **SHRT** , or **LONG**.
  
There must be 24 rows of numbers, the numbers must be positive numbers between 0 and 1, and each column must be normalized (they must add up to 1.000 over the 24 hour period). 

You may introduce as many _DAYTYPE_ blocks as are supported simultaneously (2 max at this time). 


