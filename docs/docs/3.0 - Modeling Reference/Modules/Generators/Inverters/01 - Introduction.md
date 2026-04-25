# Introduction

In a power system, an inverter is a device that converts DC (Direct Current) electricity into AC (Alternating Current) electricity. Most power grids operate on AC, but many sources—solar panels, batteries, fuel cells—produce DC. Inverters solve the disconnect between DC supply and AC demand, making it possible to use or inject DC-sourced power into the grid or feed AC loads.

GridLAB-D™ modelled inverters at different stages of its development accounting for

- type of grid analysis, such as quasi-steady-state or dynamic,
- use cases requiring particular model properties to be easily customizable.

Hence, GridLAB-D™ includes two objects, **inverter** and **inverter_dyn**, defined and declared in `inverter.[h/cpp]` and `inverter_dyn.[h/cpp]`, respectively. They inherit the generator object and extend it with specific properties and control methods/algorithms.

These represent the specifications and some examples for the **inverter_dyn** object, which is to represent grid-forming and grid-following inverters. The **inverter_dyn** is a reworked inverter object. The **inverter_dyn** object was created to focus on dynamic responses of the inverter, especially in a microgrid or islanded system scenario. The existing inverter object has significant extra modes and some incompatible code. In order to maintain backwards compatibility with that functionality, the inverter was left, as-is, and the **inverter_dyn** was created to handle the new dynamics-oriented development.
