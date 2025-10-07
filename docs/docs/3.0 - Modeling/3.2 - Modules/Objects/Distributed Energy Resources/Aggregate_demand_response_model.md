# Aggregate Demand Response Model

Explicit modeling of individual devices does produce a very accurate simulation, but it can be very computationally intensive. For some time, there has been a desire to build an aggregate load model that incorporates the essential features of demand response, and in particular the three primary types of demand response DR control signals 

* **Direct load control** -
    These are DR control strategies that directly command devices to turn $on$ or $off$ deterministically or probabilistically. The parameter that describes this behavior is $\eta$. Values of $\eta$ that are positive describe the rate at which devices turn $on$ and values of $\eta$ that are negative describe the rate at which devices turn $off$ per unit of time.
* **Thermostat reset control** -
    These are DR control strategies that adjust the thermostat control band, by increasing the hysteresis or by moving the temperature band. The parameters that describes this behavior is $L$ and $\delta$, which respectively describe the size of the control band and the rate at which the control moves up in units of $L$ per unit time.
* **Duty cycle control** -
    These are DR control strategies that adjust the duty cycle of the device by adjusting the fractional runtime of the devices. The parameter that describes the nominal duty cycle of a device is $\varphi$, which is unitless. It is related to the rate at which devices move up $r_{on}$ and down $r_{off}$ the control band $L$ per unit time such that $r_{on}=\varphi$ and $r_{off}=1-\varphi$.

# Demand Response Model

The DR model is based on two state queues of size $L$, one for those devices in the $off$ regime and one for those in the $on$ regime. The rate at which devices migrate down the $off$ queue toward the lower control band limit $0$ is given by the parameter $r_{off}$. The rate at which devices migrate up the $on$ queue toward the upper control band limit $L$ is given by the parameter $r_{on}$. 

The duty cycle $\varphi$ is the fraction of the time at a device is on with respect to the total time $T$ it takes for the device to complete a cycle. If all the devices have the same load $q$, then this is also the fraction of devices that are $on$ at any given time as well as the fraction $Q=N_{on}q$ of the maximum load $\hat Q = Nq$. Thus, nominally 

$$\varphi = \frac{t_{on}}{T} = \frac{r_{off}}{r_{on}+r_{off}} = \frac{Q}{\hat Q} = \frac{N_{on}}{N}$$ 

We will see that this is true only if all the devices are identical, and there are no devices that are "short cycling", i.e., changing state from $on$ to $off$ or from $off$ to $on$ at any point other than the control band limits $0$ and $L$. 

If there is a non-zero probably $\eta$ that a device turns $on$ arbitrarily, regardless of the temperature $x \in (0,L)$, then we must consider the fact that $r_{off}$ is effectively shorter than if all devices reached the control band limit $0$ in due course without short-cycling. We call the value $\eta$ the **excess demand** , in contrast the value $\varphi$ which we call the **base demand** or **natural demand**. 

# Equilibrium Solution

The key to the behavior of a population of $N$ devices is to recognize that any change in the values $L$, $\varphi$, or $\eta$ will disturb the distribution of devices at the various temperatures $x$. The effective value of $r_{off}$ in the case that devices are turned $on$ permaturely (when $\eta \ge 0$) has been shown to be 

$$\rho(\eta) = (1-\eta)r_{off} + \eta$$ 

The natural distribution of devices is given by the density functions 

$$n_{off}(x) = \frac{N \eta r_{on}}{\rho (r_{on}+\rho) (e^{\eta L / \rho}-1)}e^{\eta x/\rho}$$

$$n_{on}(x) = \frac{N \eta}{(r_{on}+\rho) (e^{\eta L / \rho}-1)}e^{\eta x/\rho}$$

The total number of devices that are $on$ is 

$$N_{on} = N \frac{\rho}{r_{on}+\rho}$$

Thus, we find that the effective duty-cycle $\Phi$ of a population of such devices when the demand is non-zero 

$$\Phi(\eta) = \frac{\rho(\eta)}{r_{on}+\rho(\eta)}$$

which is what is generally called the **total demand** or just **demand**. We use the symbol $\Phi$ to distinguish the diversity of the population from the duty cycle of single device. 

When $\eta$, $\varphi$, and $L$ change sufficiently slowly, the equilibrium solution given here is sufficient and accurate. Otherwise, a dynamic solution must be considered. 

# Dynamic Solution

When $\eta$, $\varphi$, or $L$ change too quickly for the equilibrium solution to be valid, a dynamic model must be used. Unfortunately, a solution to the differential equations used to derive the equilibrium model has not yet been found. Instead a set of finite difference equations must be used, one for cases where $\eta > 0 $ and one for cases where $\eta < 0$. 

When $\eta > 0$ we have 

$$\Delta n_{on}(L,t+\Delta t) = -r_{on} n_{on}(L,t) + \eta n_{off}(L,t) + (1-\eta) r_{off} n_{off}(L,t)$$

$$\Delta n_{on}(x,t+\Delta t) = -r_{on} n_{on}(x,t) + \eta n_{off}(x,t) + r_{on} n_{on}(x+\Delta x,t)$$

$$\Delta n_{off}(0,t+\Delta t) = -(1-\eta) r_{off} n_{off}(0,t) - \eta n_{off}(0,t) + r_{on} n_{on}(0,t)$$

$$\Delta n_{off}(x,t+\Delta t) = -(1-\eta) r_{off} n_{off}(x,t) - \eta n_{off}(x,t) + (1-\eta) r_{off} n_{off}(x-\Delta x,t)$$

and when $\eta < 0$ we have 

$$\Delta n_{on}(L,t+\Delta t) = -r_{on} (1+\eta) n_{on}(L,t) + \eta n_{on}(L,t) + r_{off} n_{off}(L,t)$$

$$\Delta n_{on}(x,t+\Delta t) = -r_{on} (1+\eta) n_{on}(x,t) + \eta n_{on}(x,t) + r_{on} (1+\eta) n_{on}(x+\Delta x,t)$$

$$\Delta n_{off}(0,t+\Delta t) = -r_{off} n_{off}(0,t) - \eta n_{on}(0,t) + r_{on} (1+\eta) n_{on}(0,t)$$

$$\Delta n_{off}(x,t+\Delta t) = -r_{off} n_{off}(x,t) - \eta n_{on}(x,t) + r_{off} n_{off}(x-\Delta x,t)$$

Note that to guarantee the stability of the numerical solution, we must have $r_{on} + r_{off} \le 1$, so that we always have at least 
$r_{on} = \varphi$ and $r_{off} = 1- \varphi$.


