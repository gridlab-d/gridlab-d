# Occupant Load

The `occupantload` object models internal residential heat gains from people in a house. It inherits from `residential_enduse` and contributes a heat load to the parent house based on occupant count, occupancy fraction, and per-person heat gain assumptions.

`occupantload` is a heat-centric end use. It does not model a detailed electrical appliance profile; instead, it provides thermal gains that affect house thermodynamics.

## Model behavior

### Parent and attachment requirements

- `occupantload` must be parented to a `house` or `house_e` object.
- During initialization, it attaches its end-use structure to the parent via `attach_enduse()`.

### Defaults and bounds

- `number_of_occupants` defaults to 4 when unspecified.
- `heatgain_per_person` defaults to 400 Btu/h (DOE-2 based assumption).
- At runtime, `heatgain_per_person` is bounded to [0, 1600] Btu/h. Values outside this range are reset to 400 Btu/h.
- `number_of_occupants` less than 0 is reset to 0.
- `occupancy_fraction` less than 0 is reset to 0.
- If `occupancy_fraction * number_of_occupants` exceeds 300, occupancy is reset to 0 as a sanity guard.

### Heat gain calculation

The internal heat gain applied to the house is

$$
heatgain = number\_of\_occupants \times occupancy\_fraction \times heatgain\_per\_person
$$

If no explicit shape mode is used (`shape.type == MT_UNKNOWN`), this equation is evaluated directly during sync.

## Properties

Table: Occupant Load Properties { #tbl:table-occ }

Property Name | Type | Unit | Description
---|---|---|---
**number_of_occupants** | int32 | - | Number of occupants represented by the object.
**occupancy_fraction** | double | unit | Occupancy multiplier, typically in [0, 1], often schedule-driven.
**heatgain_per_person** | double | Btu/h | Heat gain per person (sensible + latent), default 400 Btu/h.

The following inherited `residential_enduse` fields are relevant:

- `load.heatgain`
- `shape` / schedule linkage

## Example

```glm
object occupantload {
  name house1_occ;
  parent house1;
  number_of_occupants 3;
  occupancy_fraction OCCUPANCY_SCHEDULE*1.0;
  heatgain_per_person 380 Btu/h;
}
```

This example adds occupancy-driven internal heat gains to `house1` using a schedule-defined occupancy profile.
