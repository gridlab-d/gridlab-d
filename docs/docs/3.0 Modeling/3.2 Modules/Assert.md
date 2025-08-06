# Assert (module)

Assert objects can be used to monitor the properties of other objects. The assert class is used purely for validation purposes. They are used to assert that a given parameter will have a certain value at a certain time. In most case, this is used in conjunction with the auto-validation process performed on the nightly builds. More information on the validation process can be found at: Integrated_Testing

## Synopsis

    module assert;
    class assert;
    class double_assert;
    class complex_assert;
    class enum_assert;
    
# Assert Objects

Assert objects work by taking a user defined value and comparing it against a property that is also specified by the user. There are three different assert objects that may be used, `_complex_assert_` , `_double_assert_` , and `_enum_assert_`. These are used to test against complex numbers $(r + j*x)$, double values (real numbers), and enumerations (strings), respectively. Each object also contains a number of operating modes that will further explained below. 

To create an assert test, an assert object is created as a child of the object to be tested. Three inputs must be specified to create the assert: 

  * _value_ \- This is the "right" answer, or the value to be checked against, that is provide by the user. The type of value used depends on the type of assert used (complex, double, or enumeration). Units are not used in this value, so must be in the same units as the answer to be compared against.
  * _within_ \- This allows the user to specify a range of answer. **Not used in _enum_assert_ \- _value_ must be the exact string.**
  * _target_ \- This tells the assert object what _value_ is being tested against. Quotes must encompass the name of the property. Any public variable may be used. The default units of the target property will be used when comparing against _value_.
Here's an example: 
    
        object node {
            name test_node_1;
            phases ABCN;
            nominal_voltage 7200;
            voltage_A +7199.558+0.000j;
            voltage_B -3599.779-6235.000j;
            voltage_C -3599.779+6235.000j; 
            object double_assert {
              value 7199;
              within 2;
              target "nominal_voltage";
            };
          }
    

This assert test says that `_nominal_voltage_` of `_test_node_1_` must equal 7199 +/- 2 (or be within the range of 7197-7201). Since `_nominal_voltage_` is 7200, this test would pass. When an assert passes, it will appear nothing happens (unless verbose is set to true). However, when an assert fails, the system will throw an error message and end the simulation. This error throw is what allows the autotest script to know that the test failed and collect the needed information. To actually see what the error was and how far apart the values were, verbose needs to be set to true (#set verbose=1). 

Additionally, other operating modes may be used: 

  * _status_ \- This allows the user to set whether the assert should test for a true or false value. 
    * ASSERT_TRUE (default) - This test will pass if the _value_ given is within the specified range of the _target_ property.
    * ASSERT_FALSE - This test will pass if the _value_ given is outside of the specified range of the _target_ property.
    * ASSERT_NONE - This will disengage the assert test and pass every value given. This is useful when fed in via a tape player (e.g. ASSERT_NONE may be played in via a tape player, except at given times where it is switched to ASSERT_TRUE. The test will then only pass/fail when ASSERT_TRUE is used.)
  * _once_ (not used with _enum_assert_) - This allows the user to specify that a test be run only once during a simulation. Used in conjunction with _in_. 
    * ONCE_FALSE (default) - In this mode, the assert value will be tested continuously at all times during the simulation.
    * ONCE_TRUE - When used, this tells the assert to only test the given _value_ against the _target_ value at the time specified by _in_.
  * _in_ \- Specifies the time at which a test is to be performed. Uses standard GridLAB-D time formats with single quotes around it (e.g. '2001-01-02 00:00:00 CST').


If the assert fails, the simulation halts. 

## Synopsis
    
    
    module assert;
    class assert {
    	enumeration {NONE=3, FALSE=2, TRUE=1} status; 
    	char1024 target; 
    	char32 part; 
    	enumeration {outside=7, inside=6, !==3, >==2, >=5, <==1, <=4, ===0} relation; 
    	char1024 value; 
    	char1024 within; 
    	char1024 lower; 
    	char1024 upper; 
    }

- lower: `lower "_value_ _unit_ ";`
    The lower property specifies the lower bound when using the inside or outside relations.

-  part:  `part "name";`
    The part property specifies the property part to use when comparing values. All property parts are considered double or complex without units. The following property parts are supported:
    * complex: real, imag, mag, ang, arg
    * enduse: total (complex), energy (complex), demand (complex), breaker_amps, admittance (complex), current (complex), power (complex), impedance_fraction, current_fraction, power_fraction, power_factor, voltage_factor, heatgain, heatgain_fraction.
    Parts noted as (complex) must have the complex part specified, e.g., total.real, current.mag, power.ang.

-  relation- `relation "op";`
    The relation operator specifies the relationship to test.

- inside - Compares the target property to determine whether it is between the lower and upper values (inclusive).

- outside - 
    Compares the target property to determine whether it is outside the lower and upper values (exclusive).

-  `!=` - Compares the target property to determine whether it is different from the value.

-  `>=` - Compares the target property to determine whether it is greater than or equal to the value.

- `>` - Compares the target property to determine whether it is greater than the value.

- `<=` - Compares the target property to determine whether it is less than or equal to the value.

- `<` - Compares the target property to determine whether it is less than the value.

- `==` -     Compares the target property to determine whether it is equal to the value.

- status - `status NONE|TRUE|FALSE;`
    The status enumeration specifies the desired outcome of the test.

- FALSE - The `FALSE` status is used to specify that the assert test should fail.

- NONE - The `NONE` status is used to specify that the assert test should be ignore.

- TRUE - The `TRUE` status is used to specify that the assert test should succeed.

- target - `target "_property_ ";`
    The target specifies the name of the property to examine.

- upper - `upper "_value_ _unit_ ";`
    The upper property specifies the upper bound when using the inside or outside relation.

- value - `value "_value_ _unit_ ";`
    The value property specifies the value to compare to when using the !=, >=, >, <=, <, or == relations.

- within - `within "_value_ _unit_ ";`
    The within property specifies the accuracy to which == and != comparisons are performed.
    **WARNING:** Units that have an absolute offset (e.g., degC, degF) will convert in absolute value, not relative value. Thus `within 0.01 degF` will not work as expected when compared to a property in degC because 0.01 degF is about -17 degC.

## Examples

The first example asserts that the temperature is 50 degF. The second example asserts that the temperature is between 0 and 40 degC. The third example asserts that the temperature is within 1 degF of 49.5 degF. 
    
    
    module assert;
    module climate; 
    
    object climate {
      temperature 50 degF;
      object assert { // example 1
        target climate#temperature;
        relation "==";
        value 50 degF;
      };
      object assert { // example 2
        target temperature;
        relation inside;
        lower 0 degC;
        upper 40 degC;
      };
      object assert { // example 3
        target climate#temperature|temperature;
        relation "==";
        value 49.5 degF;
        within 1 degF;
    }
    }
    

The fourth example asserts that the real part of the voltage is 120. The fifth example asserts that the magnitude of the voltage exceeds 120. The sixth example asserts that the angle of the voltage is between -45 deg and 45 deg. 
    
    
    module assert;
    module powerflow;
    
    object meter {
      nominal_voltage 120;
      phases A;
      voltage_A 120+1j;
      object assert { // example 4
        target voltage_A;
        part real;
        relation "==";
        value 120.0;
      };
      object assert { // example 5
        target voltage_A;
        part complex#mag;
        relation ">";
        value 120;
      };
      object assert { // example 6
        target voltage_A;
        part ang;
        relation inside;
        lower -45;
        upper 45;
      };
    }


## Double Assert

**TODO**: 

## Complex Assert

_complex_assert_ has the ability to look at only the real value, the imaginary, the magnitude, or the angle: 

  * _operation_ \- Allows the user to look at various aspects of the complex number. 
    * FULL (default) - This compares both the real and imaginary portion of the complex _value_. _within_ will be applied to both components ( real(_target_) must be _within_ xx of real(_value_) and same applies to the imaginary portion).
    * REAL - Only compares the real portion of _value_ to be _within_ xx of _target_. _value_ is still specified as a complex number.
    * IMAGINARY - Only compares the imaginary portion of _value_ to be _within_ xx of _target_.
    * MAGNITUDE - Compares the magnitude of _value_ to be _within_ xx of _target_.
    * ANGLE - Compares the angle of _value_ to be _within_ xx of _target_ (must be specified in radians).
For example: 
    
    
    object node {
      name test_node_1;
      phases ABCN;
      nominal_voltage 7200;
      voltage_A +7199.558+0.000j;
      voltage_B -3599.779-6235.000j;
      voltage_C -3599.779+6235.000j; 
      object complex_assert {
         value -2600-7000j;
         within 2;
         target "voltage_B";
         operation MAGNITUDE;
         status ASSERT_FALSE;
         once ONCE_TRUE;
         in '2001-01-02 00:00:00 CST';
      };
    }
    

## Enumeration Assert

**TODO**: 

# See also

  * assert (module)
    * assert (object)
    * complex_assert (object)
    * double_assert (object)
    * enum_assert (object)
  * Integrated Testing

