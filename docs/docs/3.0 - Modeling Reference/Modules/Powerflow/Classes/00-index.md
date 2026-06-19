# Powerflow Classes

This page lists all class reference pages for the **powerflow** module. Nearly all objects derive from either **node** or **link** — see [Inherited Classes](#inherited-classes) and [Powerflow Objects](02-powerflow_object.md) for the shared property foundations.

## Inherited Classes

Nearly all objects within the **powerflow** module are derived from two primary objects: **node** and **link**. Therefore, any properties defined for these two objects are also available to any derived object. For example, a **node** has voltage properties, so a **load** automatically has these properties available as well. Any **powerflow** objects that inherit properties from **node** or **link** will be labeled as such. Furthermore, **node** and **link** contain most relevant default quantities. Derived objects often assume zero value or throw an error if an explicit property is not indicated. Any exceptions to this rule will be indicated in the parameter list of the particular object.


## Nodes and Loads

- [Inherited Classes](#inherited-classes)
- [Powerflow Objects](02-powerflow_object.md)
- [Node](03-node.md)
- [Load](17-load.md)
- [Meter](18-meter.md)
- [PQ Load](30-pqload.md)

### Triplex (Split-Phase) Nodes and Loads

- [Triplex Node](19-triplex_node.md)
- [Triplex Meter](20-triplex_meter.md)
- [Triplex Load](21-triplex_load.md)


## Lines

- [Link](04-link.md)
- [Line](05-line.md)

### Triplex (Split-Phase) Lines

- [Triplex Line](12-triplex_line.md)

## Transformers

- [Transformer](15-transformer.md)

## Controls and Switching Devices

- [Regulator](22-regulator.md)
- [Capacitor](24-capacitor.md)
- [Fuse](25-fuse.md)
- [Switch](26-switch.md)
- [Recloser](27-recloser.md)
- [Sectionalizer](40-sectionalizer.md)
- [Sync Check](48-sync_check.md)
- [Series Compensator](47-series_compensator.md)
- [Series Reactor](39-series_reactor.md)
- [Volt-VAr Control](31-volt_var_control.md)

## Substation

- [Substation](29-substation.md)

## Diagnostic and Output Objects

- [Volt Dump](32-volt_dump.md)
- [Current Dump](33-current_dump.md)
- [Impedance Dump](34-impedance_dump.md)
- [Bill Dump](35-bill_dump.md)
- [JSON Dump](44-jsondump.md)
- [Load Tracker](46-load_tracker.md)

## Reliability and Restoration

- [Fault Check](36-fault_check.md)
- [Restoration](38-restoration.md)
- [Power Metrics](41-power_metrics.md)

## In Development

- [Motor](37-motor.md)
- [Emissions](42-emissions.md)
- [Performance Motor](45-performance_motor.md)
- [VFD](49-vfd.md)


