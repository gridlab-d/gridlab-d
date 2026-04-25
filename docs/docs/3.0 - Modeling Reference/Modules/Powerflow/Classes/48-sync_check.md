## Sync Check

!!! warning
    This page was automatically generated and requires review.

### Sync Check Parameters

#### Properties

**sync_check** does not declare inherited parent classes.

Input indicates properties you can set in models. Output marks properties produced or modified during simulation runtime.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| armed | bool | N/A |  | ✓ | ⚠️ Flag to arm the synchronization close |
| frequency_tolerance | double | Hz |  | ✓ | ⚠️ tolerance for checking the frequency metric |
| voltage_tolerance_pu | double | pu |  | ✓ | ⚠️ voltage_tolerance in per-unit - used in MAG_DIFF mode |
| voltage_tolerance | double | V |  | ✓ | ⚠️ voltage_tolerance in Volts - used in MAG_DIFF mode - prioritized over voltage_tolerance_pu |
| metrics_period | double | s |  | ✓ | ⚠️ period when both metrics are satisfied |
| volt_compare_mode | enumeration | N/A |  | ✓ | ⚠️ Determines which voltage difference calculation approach is used Valid values: `MAG_DIFF`, `SEP_DIFF`. |
| voltage_magnitude_tolerance_pu | double | pu |  | ✓ | ⚠️ tolerance in per-unit for the difference in voltage magnitudes - used in SEP_DIFF mode |
| voltage_magnitude_tolerance | double | V |  | ✓ | ⚠️ tolerance in Volts for the difference in voltage magnitudes - used in SEP_DIFF mode - prioritized over voltage_magnitude_tolerance_pu |
| voltage_angle_tolerance | double | deg |  | ✓ | ⚠️ tolerance in degrees for the difference in voltage angles - used in SEP_DIFF mode |
| delta_trigger_mult | double | N/A |  | ✓ | ⚠️ multiplier against voltage and frequency tolerances to trigger/maintain deltamode |

#### Developer Properties

These properties are published with `PA_HIDDEN` and are intended for internal or developer use.

| Property Name | Type | Unit | Input | Output | Description |
| --- | --- | --- | --- | --- | --- |
| freq_diff_noabs_hz | double | N/A |  | ✓ | ⚠️ Measurement property: frequency difference in Hz without abs() |
| volt_A_mag_diff_noabs_pu | double | N/A |  | ✓ | ⚠️ Measurement property: Difference of phase A voltage magnitude in pu without abs() |
| volt_B_mag_diff_noabs_pu | double | N/A |  | ✓ | ⚠️ Measurement property: Difference of phase B voltage magnitude in pu without abs() |
| volt_C_mag_diff_noabs_pu | double | N/A |  | ✓ | ⚠️ Measurement property: Difference of phase C voltage magnitude in pu without abs() |
| freq_diff_hz | double | N/A |  | ✓ | ⚠️ Measurement property: frequency difference in Hz |
| volt_A_diff | double | N/A |  | ✓ | ⚠️ Measurement property: Magnitude of phase A voltage phasor difference in volt |
| volt_B_diff | double | N/A |  | ✓ | ⚠️ Measurement property: Magnitude of phase B voltage phasor difference in volt |
| volt_C_diff | double | N/A |  | ✓ | ⚠️ Measurement property: Magnitude of phase C voltage phasor difference in volt |
| volt_A_diff_pu | double | N/A |  | ✓ | ⚠️ Measurement property: Magnitude of phase A voltage phasor difference in pu |
| volt_B_diff_pu | double | N/A |  | ✓ | ⚠️ Measurement property: Magnitude of phase B voltage phasor difference in pu |
| volt_C_diff_pu | double | N/A |  | ✓ | ⚠️ Measurement property: Magnitude of phase C voltage phasor difference in pu |
| volt_A_mag_diff | double | N/A |  | ✓ | ⚠️ Measurement property: Difference of phase A voltage magnitude in volt |
| volt_B_mag_diff | double | N/A |  | ✓ | ⚠️ Measurement property: Difference of phase B voltage magnitude in volt |
| volt_C_mag_diff | double | N/A |  | ✓ | ⚠️ Measurement property: Difference of phase C voltage magnitude in volt |
| volt_A_mag_diff_pu | double | N/A |  | ✓ | ⚠️ Measurement property: Difference of phase A voltage magnitude in pu |
| volt_B_mag_diff_pu | double | N/A |  | ✓ | ⚠️ Measurement property: Difference of phase B voltage magnitude in pu |
| volt_C_mag_diff_pu | double | N/A |  | ✓ | ⚠️ Measurement property: Difference of phase C voltage magnitude in pu |
| volt_A_ang_deg_diff | double | N/A |  | ✓ | ⚠️ Measurement property: Difference of phase A voltage angle in degree |
| volt_B_ang_deg_diff | double | N/A |  | ✓ | ⚠️ Measurement property: Difference of phase B voltage angle in degree |
| volt_C_ang_deg_diff | double | N/A |  | ✓ | ⚠️ Measurement property: Difference of phase C voltage angle in degree |
| nominal_volt_v | double | N/A |  | ✓ | ⚠️ Measurement Property: Nominal voltage of from/to node of the parent switch |
