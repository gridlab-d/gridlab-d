# Units

Units are used to automatically convert double to and from the internal representation declared in a class and the definition or output (see [tape]). All units have two parts, the scalar and the fundamental unit. 

## Scalars

The following scalars are supported 

  * Y = 10^24
  * Z = 10^21
  * E = 10^18
  * P = 10^15
  * T = 10^12
  * G = 10^9
  * M = 10^6
  * k = 10^3
  * h = 10^2
  * da = 10^1
  * d = 10^-1
  * c = 10^-2
  * m = 10^-3
  * u = 10^-6
  * n = 10^-9
  * p = 10^-12
  * f = 10^-15
  * a = 10^-18
  * z = 10^-21
  * y = 10^-24

## Fundamental units

### Dimensionless units

  * unit = 1
  * ratio = 1 unit
  * % = 0.01 unit
  * pu = 1/unit
  * /% = 1/%
### SI Units

The basic SI units are defined in terms of the following constants 

  * _c_ = 2.997925e8
  * _e_ = 1.602189246e-19
  * _h_ = 6.62617636e-34
  * _k_ = 1.38066244e-23
  * _m_ = 9.10953447e-31
  * _s_ = 1.233270e4
The basic SI units are defined as follows: 

  * m = _c_ ^-1 * _h_ * _m_ ^-1 * 4.121487*10
  * g = _m_ * 1.09775094*10^27
  * s = _c_ ^-2 * _h_ * _m_ ^-1 * 1.235591*10^10
  * A = _c_ ^2 * _e_ * _h_ ^-1 * _m_ * 5.051397*10^8
  * K = _c_ ^2 * _k_ ^-1 * _m_ * 1.686358
  * cd = _c_ ^4 * _h_ ^-1 * _m_ ^2 * 1.447328
  * 1990$ = _m_ * _s_ * 1.097751*10^30

### Angular measures

  * pi = 3.1415926536
  * rad = 0.159155 unit
  * deg = 0.0027777778 unit
  * grad = 0.0025 unit
  * quad = 0.25 unit
  * sr = 0.5 rad

### Derived SI

  * R = 0.55555556 K
  * degC = K-273.14
  * degF = R-459.65
  * N = 1 m*kg/s^2
  * Pa = N/m^2
  * J = N*m
### Currency

  * 1975$ = 0.42 1990$$
  * 1980$ = 1.60 1990$$
  * 1985$ = 0.83 1990$$
  * 1995$ = 1.00 1990$$
  * 1996$ = 1.01 1990$$
  * $  = 1.00 1996$
  * CA$ = 0.85 $$

### Time

  * min = 60 s
  * h = 60 min
  * day = 24 h
  * wk = 7 day
  * yr = 365 day
  * syr = 365.24 day

### Length

  * in = 0.0254 m
  * ft = 12 in
  * yd = 3 ft
  * mile = 5280 ft

### Area

  * sf = ft^2
  * sy = yd^2

### Volume

  * cf = ft^3
  * cy = yd^3
  * gal = 0.0037854118 m^3
  * l = 0.001 m^3

### Mass

  * lb = 0.453592909436 kg
  * tonne = 1000 kg

### Velocity

  * mph = 1 mile/h
  * fps = 1 ft/s
  * fpm = 1 ft/min
  * mps = 1 m/s

### Flow rates

  * gps = 1 gal/s
  * gpm = 1 gal/min
  * gph = 1 gal/h
  * cfm = 1 ft^3/min
  * ach = 1/h

### Frequency

  * Hz = 1/s

### EM units

  * W = J/s
  * Wh = 1 W*h
  * Btu = 0.293 W*h
  * ton = 12000 Btu/h ; ton cooling
  * tons = 1 ton*s ; ton.second cooling
  * tonh = 1 ton*h ; ton.hour cooling
  * hp = 746 W ; horsepower
  * V = W/A ; Volt
  * C = A*s ; Coulomb
  * F = C/V ; Farad
  * Ohm = V/A ; resistance
  * H = Ohm*s ; Henry
  * VA = V*A ; Volt-Amp
  * VAr = 1 V*A ; Volt-Amp reactive
  * VAh = 1 VA*h
  * Wb = J/A ; Weber
  * lm = cd*sr ; lumen
  * lx = lm/m^2 ; lux
  * Bq = 1/s ; Becquerel
  * Gy = J/kg ; Grey
  * Sv = J/kg ; Sievert
  * S = 1/Ohm ; Siemens

### Data

  * b = 1 unit ; 1 bit
  * B = 8 b ; 1 byte

### Custom

  * EER = Btu/Wh
  * ccf = 1000 Btu ; this conflict with centi-cubic-feet (ccf)
  * therm = 100000 Btu

### Bad usage

ohm=Ohm ; should be capitalized but often isn't 

## Derived units

Any unit may be specified as a composite or derived unit. For example, 
    
    
    object my_class {
      my_double 12 V*A;
    }
    

is equivalent to 
    
    
    object my_class {
      my_double 12 W;
    }
    

## Caveats

### Unexpected Unit Names

Be careful about unit names. For example "F" and "C" are not degrees Fahrenheit and Celcius, respectively. They are Farads and Colombs, respectively. The correct unit for degrees Fahrenheit and Celcius are "degF" and "degC", respectively. 

### Operator precedence

The syntax for deriving units does not obey the customary operator precedence for multiplication and division. In particular, only one division sign is recognized, with everything before it being in the numerator and everything after it being in the denominator. After the first division, all remaining operators are interpreted as placing the unit in the denominator. For example 
    
    
    Btu/degF*h
    

is interpreted the same as 
    
    
    Btu/degF/h
    

Furthermore, parentheses are not recognized. 

### Improper Syntax

The only valid unit product syntax is an asterisk. In the past users and programmers sometimes used a period instead of an asterisk when specifying derived units. This usage results in incorrect unit specifications but was not always detected in [Hassayampa (Version 3.0)] and earlier. 

## Wish list

TODO - Check status - Although the `.` syntax is not valid now, it should be acceptable as a multiplication on the appropriate side of the `/`. For example `a.b/c.d` should be the same as `a*b/c/d`. This would be very much more user friendly and quite easy to implement. 

## Related Concepts:

  * double
  * complex

