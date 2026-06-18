# Introduction

In a power system, an _inverter_ is a device that converts DC (Direct Current) electricity into AC (Alternating Current) electricity. Most power grids operate on AC, but many sources—solar panels, batteries, fuel cells—produce DC. Inverters solve the disconnect between DC supply and AC demand, making it possible to use or inject DC-sourced power into the grid or feed AC loads. An inverter can also provide additional functionalities such as voltage regulation, frequency support, and reactive power compensation, which are essential for maintaining grid stability and reliability.

As a distribution system simulation platform, GridLAB-D™ offers the means to particularly design and analyze use cases with high penetration of distributed energy resources (DERs). Therefore the inverter has been modeled as a necessary physical interface between time-varying DC energy resources and the AC network, while also capturing the evolving control behavior across a simulation window. Without it, the DC-side objects have nowhere to connect and the time-series power flow loses physical meaning.

GridLAB-D™'s native engine is a quasisteady-state (QSTS) solver, and assumes the network reaches steady state at each time step. However, inverters have been designed to have fast internal dynamics (switching, control loops, phase-locked loop (PLL)) that operate at faster time scales than the typical QSTS time step. the inverter model is designed to capture dynamic responses to changes in operating conditions, such as fluctuations in solar irradiance or load. This allows users to analyze how inverters will behave under real-world conditions, even within the QSTS framework.

To this end,GridLAB-D™ modeled inverters at different stages of its development accounting for

- type of grid analysis, such as quasisteady-state or dynamic,
- use cases requiring particular model properties to be easily customizable.

Hence, GridLAB-D™ includes two objects, **inverter** and **inverter_dyn**, defined and declared in `inverter.[h/cpp]` and `inverter_dyn.[h/cpp]`, respectively. They inherit the generator object and extend it with specific properties and control methods/algorithms.
