!
! Purpose:
!  - Main subroutine to simulate the dynamic behaviour of a heat-pump/electric-resistance water heater running in either one of the 3 operating modes:
!     - Efficiency mode
!     - Hybrid mode
!     - Electric-only mode (logic for this mode has been reversed to the original logic, but there's also the new logic implemented)
!
!===================================================================================================================================================================================
! Copyright (C) 2013, Battelle Memorial Institute
! Written by Laurentiu Dan Marinovici, Pacific Northwest National Laboratory
! Modified and developed from the initial PDE model from Zhijie Xu ("Modeling of Electric Water Heaters for Demand Response: a Baseline PDE Model")
! Updated on: 03/07/2014
!    Changes: - The heating control logic code was brought before solving the PDE equations, such that the control is applied for the current time step
!             - The heating control logic in Mode 3 (electric mode only) has been modified back to the original code related to the electric resistance water heater (ERWH)
!             - In the old ERWH control logic, priority was given to the lower element, rather than the upper one (as initially coded), when both heating elements
!               are turned on by temperature conditions being satisfied
!             - Energy is calculated and saved in output file
! Updated on: 03/10/2014
!    Changes: - Heating control logic has been transformed into a subroutine
! Last modified: 03/11/2014
!    Changes: - Cleaned the code to avoid useless code run when not needed, e.g. COP should only be calculated in EFFICIENT or HYBRID operating modes and only when compressor is ON.
!             - Created internal functions to calculate COP and Input Power for the compressor. Formula-wise these functions look the same, so technically
!               they could be lumped into one and just called with different parameters. But for the sake of "they-might-be-changed-one-day", let's keep them as different functions
!===================================================================================================================================================================================
!      
SUBROUTINE run_WH (op_mode, sim_time, large_bins, small_bins, heater_h, heater_D, dr_signal, sensor_pos, heater_q, heater_size,&
                   heat_loss_rate, water_rho, water_k0, water_alpha, water_cv, temp_amb, hum_amb, temp_in, temp_set, temp_db,&
                   comp_power, comp_off, low_amb_lim, up_amb_lim, water_low_lim, mode_3_off, t_db, t_wb, upper_elem_off,&
                   v_flow_threshold, v_flow, init_tank_temp, lowerf, upperf, ncomp, nheat, heat_up, ca, power, COP, energy)
   
   IMPLICIT DOUBLE PRECISION (a-h, o-z)
   REAL, PARAMETER :: PI = 3.141593
!  ==== Input and Output parameters for the subroutine ===========================================
   INTEGER, INTENT(IN) :: op_mode, large_bins, small_bins, dr_signal
   DOUBLE PRECISION, INTENT(IN) :: heater_h, heater_D, heat_loss_rate
   DOUBLE PRECISION, INTENT(IN) :: sensor_pos(2), heater_q(2), heater_size(2)
   DOUBLE PRECISION, INTENT(IN) :: sim_time, water_rho, water_k0, water_alpha, water_cv
   DOUBLE PRECISION, INTENT(IN) :: temp_amb, hum_amb, temp_in, temp_set(2), temp_db, v_flow_threshold, v_flow, lowerf, upperf
   DOUBLE PRECISION, INTENT(IN) :: comp_power, low_amb_lim, up_amb_lim, water_low_lim, mode_3_off, t_db, t_wb, upper_elem_off
   DOUBLE PRECISION, INTENT(IN) :: init_tank_temp(large_bins*small_bins), comp_off
   INTEGER, INTENT(INOUT) :: ncomp, nheat(2)
   LOGICAL, INTENT(INOUT) :: heat_up
   DOUBLE PRECISION, INTENT(INOUT) :: ca(large_bins*small_bins)
   DOUBLE PRECISION, INTENT(INOUT) :: power, COP, energy
!  ===============================================================================================
!  ==== Internal parameters for the function =====================================================
   DOUBLE PRECISION :: water_diff, V, heater_A, heater_As, heater_pos(2), temp_amb_F, temp_set_F
   DOUBLE PRECISION :: heat_loss_rate_WK, heater_k, low_amb_lim_C, up_amb_lim_C, water_low_lim_C
   INTEGER :: nsensor(2), nheater_up(2), nheater_low(2), nsource(2)
   DOUBLE PRECISION :: IP, sum_Qelef, v_flow_kgs ! v_flow is in GPM and needs to be in kg/s
   DOUBLE PRECISION :: ca_i(large_bins*small_bins), ca_F(large_bins*small_bins)
   DOUBLE PRECISION :: Qele(large_bins*small_bins), Qelef(large_bins), Tbb(large_bins)
   DOUBLE PRECISION :: Tbb_F(large_bins), sum_Tbb(large_bins)
   DOUBLE PRECISION :: tmp11(large_bins*small_bins), tmp12(large_bins*small_bins)
   DOUBLE PRECISION :: tmp21(large_bins*small_bins), tmp22(large_bins*small_bins)
   DOUBLE PRECISION :: gradtf, gradtb, tmp1, tmp2, Df, Db
   INTEGER ind, stats, mesh, big_ind
   INTEGER cur_min, nts
   INTEGER n6tank, n6tank_bb ! number of the SMALL/BIG bin coresponding to a 6th of the tank height
!  ==========================================================================
!  new parameters for the compressor COP and input power
!  I will keep these new parameters as local variables for the time being.
!  Later they can be added ti the input data file and read from there.
   DOUBLE PRECISION :: a1, a2, COP_sub, a3, a4, IP_sub ! for COP_sub and IP_sub
   DOUBLE PRECISION :: c70, c50, i70, i50 ! for COP and input power
!  ===========================================================================
   DOUBLE PRECISION :: s, p, offset ! shrinkage, shape, and offset for the fraction of total heat added to each node
   DOUBLE PRECISION :: COP_sub_1, COP_sub_2, T1, T2
   DOUBLE PRECISION :: ca1, cb1, cc1, ca2, cb2, cc2
   DOUBLE PRECISION :: IP_sub_1, IP_sub_2
   DOUBLE PRECISION :: ia1, ib1, ic1, ia2, ib2, ic2
   DOUBLE PRECISION :: curr_temp_sens(2), dx, dt
   T1 = 50 ! 47
   T2 = 70 ! 67
!  Initialize these newly added variables
!  ========== SECOND ORDER FUNCTION FOR COP AND IP ===========================
   ca1 = 0
   cb1 = -0.02374
   cc1 = 4.719314
   ca2 = 0
   cb2 = -0.02935
   cc2 = 5.845126
   ia1 = 0
   ib1 = 0.003913
   ic1 = 0.43728
   ia2 = 0
   ib2 = 0.004547
   ic2 = 0.422182
!  ===========================================================================
!  ========== FIRST ORDER FUNCTION FOR COP AND IP ===========================
   a1 = 5.259
   a2 = -0.0255
   a3 = 0.379
   a4 = 0.00364
   c70 = 1.21 ! 1.1 ! 1.21
   c50 = 1.04 ! 0.95 ! 1.04
   i70 = 1.17 ! 0.93 ! 1.17
   i50 = 1.09 ! 0.83 ! 1.09
!  ===========================================================================
   s = 3.7
   p = 1
   offset = 5
!  ===========================================================================

901   FORMAT (25e15.6)
902   FORMAT (A65, F10.8)
903   FORMAT (A45, I2, A18, F10.4)
904   FORMAT (A45, I2, A10, I3, A10)
905   FORMAT (A65, I10)
906   FORMAT (A65, F10.4)

!   WRITE(*,*) CHAR(27), '[2J', CHAR(27), '[1;1H'
   mesh = large_bins*small_bins
   cur_min = 0
!  ===== Calculating the parameters needed for simulation, from the input paremeters
   heater_A = PI * heater_D * heater_D / 4 ! water tank base area
   heater_As = PI * heater_D * heater_h ! water tank lateral surface area, used for losses calculation purposes
!  Water thermal diffusity watr_diff = k/(rho*cv)
!  water_k0 = Water thermal conductivity
!  water_rho = Water density
!  water_cv = Water heat capacity
   water_diff = water_k0/water_rho/water_cv
!   WRITE(*, 902) 'Water Diffusity Dc -->> ', water_diff
!   WRITE(*, 902) 'Tank Heat Loss Rate (BTU/hrF) UAs = ', heat_loss_rate
   heat_loss_rate_WK = heat_loss_rate/1.895525 ! 1 W/K = 1.895525 BTU/hrF
   heater_k = heat_loss_rate_WK/heater_As
!   WRITE(*, 902) 'Tank Heat Loss Rate (W/K) UAs = ', heat_loss_rate_WK
!   WRITE(*, 902) 'Water Heater Thermal Conductance (W/K/m^2) U = ', heater_k
!  Tank height sample
!  heater_h = Tank height
!  mesh = Number of tank height divisions
!  dx =Tank height division 
   dx = heater_h/mesh
!   WRITE(*, 902) 'Mesh size dx = ', dx
   DO ii = 1, 2
!     sensor_pos = position of the sensors
!     nsensor = number of bins to where the sensors are placed
      nsensor(ii) = NINT(sensor_pos(ii)/dx)
!      WRITE(*, 903) 'Sensor ', ii, ' position -->>', sensor_pos(ii)
!      WRITE(*, 904) 'Sensor ', ii, ' after -->> ', nsensor(ii), ' bins'
      heater_pos(ii) = sensor_pos(ii) - 0.2
!     heater_pos = position of the heater
!     nheater_up/low = number of upper/lower bins to where the heater elements are placed
      nheater_up(ii) =  NINT((heater_pos(ii)+heater_size(ii))/dx)
      nheater_low(ii) = NINT((heater_pos(ii))/dx)
!      WRITE(*, '(A65, F10.4)') 'Heater position (m) = ', heater_pos(ii)
!      WRITE(*,'(A65, I10)') 'nheater_up = ', nheater_up(ii)
!      WRITE(*,'(A65, I10)') 'nheater_low = ', nheater_low(ii)
   END DO
!  number of the SMALL bin coresponding to a 6th of the tank height, in terms of units, for the small bins of dimension dx; technically, it should be the same as nint(large_bins*small_bins/6)
   n6tank = NINT((heater_h/6)/dx)
!   WRITE(*, '(A65, I10)') 'n6tank = ', n6tank
!  number of the BIG bin coresponding to a 6th of the tank height
   n6tank_bb = NINT(REAL(large_bins)/6)
!   WRITE(*, '(A65, I10)') 'n6tank_bb = ', n6tank_bb
!  dt = Time sample calculated based on the tank division length and the water diffusity
   dt = dx*dx/2.0D0/water_diff/20000.0D0
!  Simulation time (seconds)
   T = sim_time*3600
!  nts = Number of Time Samples
   nts = INT(T/dt)

!   WRITE(*, 906) 'Initial ambient temperature (C) = ', temp_amb
!   WRITE(*, 906) 'Initial inlet water temperature temperature (C) = ', temp_in
!   WRITE(*, 906) 'Simulation time (HOURS) sim_time = ', sim_time
!   WRITE(*, 906) 'Simulation time (SECONDS) T = ', T
!   WRITE(*, 905) 'Time samples nts = ', nts
!   WRITE(*, 902) 'Time sample interval dt = ', dt
!  =========================================================================
!  Initial water temperature in the tank for each small bin.
   ca = init_tank_temp
!  Initial water temperature in the tank for each big bin.
   DO i = 1, large_bins
      Tbb(i) = SUM(ca(((i - 1)*small_bins + 1):(i*small_bins)))/large_bins
   END DO
!  =============================================================================================================================
!  Initial water temperature for each bin at time i
   ca_i = ca
!  Initial tank temperature
   t_tank = (3*ca(NINT(upperf*heater_h/dx)) + ca(NINT(lowerf*heater_h/dx)))/4
   t_tank_F = Celsius2Far(t_tank)

   ii = 0

!  New parameters introduced for the water heat pump simulation
!  Need to do some conversions from F to C and vice-versa, depending on what unit I have initially, to make temperatures 
!  compatible with the formulae.
   low_amb_lim_C = Far2Celsius(low_amb_lim)
   up_amb_lim_C = Far2Celsius(up_amb_lim)
   water_low_lim_C = Far2Celsius(water_low_lim)
   temp_set_F = Celsius2Far(temp_set(1))
   ca_F(1) = Celsius2Far(ca(1))
   ca_F(mesh) = Celsius2Far(ca(mesh))
   temp_amb_F = Celsius2Far(temp_amb) ! ambient temperature in Fahrenheit; when integrated in GLD, it should stay constant till the next time GLD comunicates with the model
!  V = flow velocity (m/s)
!  In the GLD integration, a non-zero positive value for v_flow means we have mass flow
!  depends on water flow rate m_dot = v_flow[Kg/s] / water_rho[Kg/m^3]
!  v_flow = mass flow
   v_flow_kgs = v_flow * 0.06225 ! transforming the GPM flow into Kg/sec
   V = v_flow_kgs/water_rho/heater_A
!  ============================================================================================================================================
!  Do the following for each time sample
   DO i = 1, nts
!     Heating control strategy
      curr_temp_sens(1) = ca(nsensor(1))
      curr_temp_sens(2) = ca(nsensor(2))
!     Calling the control logic
      CALL wh_control_logic(op_mode, dr_signal, temp_amb, t_tank, temp_set, temp_db, curr_temp_sens, comp_off,&
                               upper_elem_off, low_amb_lim_C, up_amb_lim_C, water_low_lim_C, mode_3_off, ncomp, nheat, heat_up)
!  If control logic activated the compressor, calculate COP and IP for current time sample
!  ===============================================================================
      IF ((op_mode .EQ. 1 .OR. op_mode .EQ. 2) .AND. ncomp .EQ. 1) THEN
!        COP = Coefficient of Performance
!        ================================================================================
         COP = comp_cop(a1, a2, c50, c70, t_tank_F, temp_amb_F, T1, T2)
!        ===================================================================================
!        Input power function for compressor
         IP = comp_ip(a3, a4, i50, i70, t_tank_F, temp_amb_F, T1, T2) ! these are in kW
      END IF
!  ===============================================================================
!     energy = Total heat
      IF (i .GT. 1) energy = energy
      IF (ncomp .EQ. 1) THEN
! =====================================================================================
!        The new power when the compressor in ON
         energy = energy + IP*1000*dt
         power = IP*1000; ! this is supposed to be in W
! =====================================================================================
      ELSE 
         power = 0
!        heater_q = Heater Element Power Q [W]
!        nheat = 0, if heating element is inactive
!                1, if heating element is active
!        nheat is used to control the calculation of power
!        If either of the heating elements is active
         DO ii = 1, 2
            IF (nheat(ii) .EQ. 1) THEN
	       energy = energy + heater_q(ii)*dt
	       power = heater_q(ii)
	    END IF
         END DO
      END IF
! ==================== NORMALIZATION BASED ON BIG BINS ========================================
!     This portion is only considered in the EFFICIENCY or HYBRID modes when compressor is ON
      IF ((op_mode .EQ. 1 .OR. op_mode .EQ. 2) .AND. ncomp .EQ. 1) THEN
!        Normalizing the the fraction of heat coming from the compressor and added to the water, such that all fractions add up to 1
         sum_Qelef = 0.0D0 ! If we work with scalars, this value has to be reinitialized to 0 every time sample to avoid adding up to huge values
         DO big_ind = 1, INT(large_bins)
            Tbb_F(big_ind) = Celsius2Far(Tbb(big_ind))
            Qelef(big_ind) = (temp_set_F - Tbb_F(big_ind))**p/(1 + EXP((Tbb_F(big_ind) - Tbb_F(1))/s - offset))
            sum_Qelef = sum_Qelef + Qelef(big_ind)
         END DO
      END IF
! =================================================================================================
      DO j = 2, mesh-1
!        nsource = used to calculate the temperature dynamics in the water in
!        direct contact with the heating element
	 nsource = 0
         DO ii = 1, 2
            IF ((j .LT. nheater_up(ii) .AND. j .GE. nheater_low(ii))) THEN
               nsource(ii) = 1
            END IF
         END DO
! =================== CALCULATING PDE COEFFICIENTS =============================================================================================   
        IF (v_flow .GT. 0) THEN
!          tmp11(j) = V(i)*(dT/dx)
!          tmp11(j) = V(i)*(ca(j+1)-ca(j-1))/2.0D0/dx 
	   tmp11(j) = V * (ca(j) - ca(j-1))/dx 
	END IF
        IF (v_flow .EQ. 0) tmp11(j) = 0.0D0
! ==============================================================================================================================================
!       Calculating the second-order derivative of temperature with respect to position
!       (Dc + Da)(d2T/dx2), where Dc =  water difussity, and Da = alpha*sqrt(-dT/dx) in the paper.
!       In the code it is not dirrectly calculated using this formula, but Da = alpha*sqrt(0.5*(abs(dT/dx) - dT/dx))
!       If dT/dx < 0, then abs(dT/dx) - dT/dx = 2*abs(dT/dx) = -2*dT/dx, which eventually leads to the same formula.
        gradtf = (ca(j+1) - ca(j))/dx
        gradtb = (ca(j) - ca(j-1))/dx
        tmp1 = 0.5D0*(abs(gradtf) - gradtf)
        tmp2 = 0.5D0*(abs(gradtb) - gradtb)
        Df = water_diff + water_alpha*sqrt(tmp1)
        Db = water_diff + water_alpha*sqrt(tmp2)
!       tmp12(j) = (Dc + Da)*(d2T/dx2)
        tmp12(j) = (Df*gradtf - Db*gradtb)/dx
! ==============================================================================================================================================
        tmp22(j) = 0.0D0
        Qele(j) = 0.0D0
!       Switching between heatng sources
!       tmp22(j) = Qele/(rho*cv) = (Q/(A*x))/(rho*cv)
        IF (ncomp .EQ. 1) THEN
!          call Celsius2Far(ca(j), ca_F(j))
!          Qelef(j) = (temp_set_F - ca_F(j))**p/(1 + exp((ca_F(mesh) - ca_F(1))/s - offset))
!          Qelef(j) = (temp_set_F - ca_F(j))**p/(1 + exp((ca_F(ncomp_up) - ca_F(ncomp_low))/s - offset))
!          Find the index of the big bin where the small bin resides
           big_ind = floor(real(j)/large_bins) + 1
           Qele(j) = IP*1000*COP*Qelef(big_ind)/sum_Qelef/heater_A/dx/small_bins
           tmp22(j) = Qele(j)/water_rho/water_cv
         ELSE
!          For each heating element, if it is ON
  	   DO ii = 1, 2  
	      IF (nsource(ii) .EQ. 1 .AND. nheat(ii) .EQ. 1) THEN
                 Qele(j) = heater_q(ii)/heater_A/heater_size(ii)
	         tmp22(j) = Qele(j)/water_rho/water_cv
	         EXIT
	      END IF
	   END DO
          END IF
! ==============================================================================================================================================
!         As = pi * d * H = external surface (area) of the tank
!         A = pi * (d^2) / 4 = base are of the tank
!         tmp21(j) = U*As/(A*H*rho*cv)
	  tmp21(j) = heater_k*(temp_amb-ca(j))/water_rho/water_cv/(heater_D/4.0D0)
! ==============================================================================================================================================
	  ca_i(j) = ca(j) + dt*(tmp12(j) - tmp11(j) + tmp22(j) + tmp21(j))
! ================ ALGORITHM DIVERGENCE STOPPING CRITERIA ======================================================================================
	  IF (ca_i(j)+1.0 .eq. ca_i(j)) THEN
!             WRITE(*, *) ca_i(nsensor(1))*9/5 + 32, ca_i(nsensor(2))*9/5 + 32, ca_i(1)*9/5 + 32, ca_i(2)*9/5 + 32,&
!                         ca_i(3)*9/5 + 32, ca_i(4)*9/5 + 32, ca_i(5)*9/5 + 32
             WRITE(*, '(A65)') 'I AM STOPPING NOW!!!!'
	     STOP
	  END IF
      END DO ! from the mesh temperature calculation
! ============ END OF PDE COEFFICIENTS CALCULATIONS =============================================================================================
! ============ Boundary conditions ==============================================================================================================
!     If there is in-coming water flow, in bin 1 (x = 0), temperature equals the
!     in-coming flow temperature
      IF (v_flow .GT. 0) ca_i(1) = temp_in
!     If there is no in-coming water flow
      IF (v_flow .EQ. 0) ca_i(1) = ca_i(2)
      ca_i(mesh) = ca(mesh - 1)
! ==============================================================================================================================================
!     Updating water temperature for current time for all small bins
      ca = ca_i
!     Tank water at each simulation time, based on the formula that takes into account 3 quarters of the temperature close to the upper sensor, and 
!     a quarter of the temperature close to the lower sensor. Unlike the real system, the model assumes the water draw is right at the bottom small bin,
!     and therefore, the formula was adapted to consider the temperatures of some specific levels in the tank, that could be adjusted by
!     the percentages given in the input data file fort.115.
      t_tank = (3*ca(NINT(upperf*heater_h/dx)) + ca(NINT(lowerf*heater_h/dx)))/4 ! these are degrees Celsius
      t_tank_F = Celsius2Far(t_tank) ! need the value of tank water temperature in F for COP formula to work
! ==============================================================================================================================================
      IF ((op_mode .EQ. 1 .OR. op_mode .EQ. 2) .AND. ncomp .EQ. 1) THEN
!        To apply a fraction of the compresor heat to different zones of the tank, we will consider the tank divided into
!        large_bins big bins consisting of a number of smaller bins (10 in our case) created to accurately model the water dynamics inside the tank
!        big_ind = big bin index
!        j = small bin index
         sum_Tbb = 0.0D0
         DO big_ind = 1, INT(large_bins)
!        Sum up the temperatures in the small bins contained within one big bin
            DO j  = INT(small_bins*(big_ind - 1) + 1), INT(small_bins*(big_ind - 1) + small_bins)
               sum_Tbb(big_ind) = sum_Tbb(big_ind) + ca(j)
            END DO
!        The average temperature of a big bin, based on the small bins that form it
           Tbb(big_ind) = sum_Tbb(big_ind)/small_bins
         END DO
      END IF
! ==============================================================================================================================================
   END DO
! ==============================================================================================================================================
!   WRITE(*,*) CHAR(27), '[2J', CHAR(27), '[1;1H'
!   WRITE(*, '(A75)') '======================================='
!   IF (dr_signal .EQ. 1) THEN
!      WRITE(*, '(A65)') 'Water heater is FREE!'
!   ELSE
!      WRITE(*, '(A65)') 'Water heater is CURTAILED!'
!   END IF
!   WRITE(*, '(A65, I10)'), 'Operating mode number = ', op_mode
!   IF (nheat(1) .EQ. 1) THEN
!      WRITE(*, '(A65)') 'UPPER heating element is ON.'
!   ELSE IF (nheat(1) .EQ. 0) THEN
!      WRITE(*, '(A65)') 'UPPER heating element is OFF.'
!   END IF
!   IF (nheat(2) .EQ. 1) THEN
!      WRITE(*, '(A65)') 'LOWER heating element is ON.'
!   ELSE IF (nheat(2) .EQ. 0) THEN
!      WRITE(*, '(A65)') 'LOWER heating element is OFF.'
!   END IF
!   IF (ncomp .EQ. 1) THEN
!      WRITE(*, '(A65)') 'COMPRESSOR is ON.'
!   ELSE IF (ncomp .EQ. 0) THEN
!      WRITE(*, '(A65)') 'COMPRESSOR is OFF.'
!   END IF
!   WRITE(*, '(A65, F10.4)') 'Tank temperature (report formula) (F) = ', t_tank_F
!   WRITE(*, '(A65, F10.4)') 'Upper sensor temperature (F) = ', ca(nsensor(1))*9/5 + 32
!   WRITE(*, '(A65, F10.4)') 'Lower sensor temperature (F) = ', ca(nsensor(2))*9/5 + 32
!   IF ((op_mode .EQ. 1 .OR. op_mode .EQ. 2) .AND. ncomp .EQ. 1) THEN
!      WRITE(*, '(A65, F10.4)') 'Compressor COP = ', COP
!   END IF
!   WRITE(*, '(A65, F10.4)') 'Total energy (kWh) = ', energy/1000/3600
!   WRITE(*, '(A65, F10.4)') 'inlet flow [GPM] = ', V*water_rho*heater_A*60/8.3/0.45
!   WRITE(*, '(A65, F10.4)') 'ambient temperature [F] = ', temp_amb_F
!   IF (v_flow .GT. 0) THEN
!      WRITE(*, '(A65, F10.4)') 'inlet temperature [F] = ', temp_in*9/5 + 32
!   END IF
RETURN
!  ============================================================================================================================================
CONTAINS
! Formula-wise the following functions look the same, so technically they could be lumped into one and just called with different parameters.
! But for the sake of "they-might-be_changed_one-day", let's keep them as different functions
! ===========================================================================================================================================
   DOUBLE PRECISION FUNCTION comp_cop(x1, x2, x3, x4, tt, ta, t1, t2) ! tt = tank temperature, ta = ambient temperature, both in F
!           
!  PURPOSE:
!    Function to calculate the compressor COP
!    WARNING!!!!!! Formulae are based on temperatures given in Fahrenheit. So be careful!!!!
!
      DOUBLE PRECISION :: x1, x2, x3, x4, tt, ta, t1, t2
      comp_cop = (x3 + (x4 - x3)*(ta - t1)/(t2 - t1))*(x1 + x2 * tt)
   END FUNCTION comp_cop
! ===========================================================================================================================================
   DOUBLE PRECISION FUNCTION comp_ip(x1, x2, x3, x4, tt, ta, t1, t2) ! tt = tank temperature, ta = ambient temperature, both in F
!           
!  PURPOSE:
!    Function to calculate the compressor input power in kW
!    WARNING!!!!!! Formulae are based on temperatures given in Fahrenheit. So be careful!!!!
!
      DOUBLE PRECISION :: x1, x2, x3, x4, tt, ta, t1, t2
      comp_ip = (x3 + (x4 - x3)*(ta - t1)/(t2 - t1))*(x1 + x2 * tt)
   END FUNCTION comp_ip
! ===========================================================================================================================================
   DOUBLE PRECISION FUNCTION Far2Celsius(degreesF)
      DOUBLE PRECISION :: degreesF
      ! Celsius = (Fahrenheit - 32)*5/9
      Far2Celsius = (degreesF - 32)*5/9
   END FUNCTION Far2Celsius
! ===========================================================================================================================================
   DOUBLE PRECISION FUNCTION Celsius2Far(degreesC)
      DOUBLE PRECISION :: degreesC
      ! Fahrenheit = 9*Celsius/5 + 32
      Celsius2Far = 9*degreesC/5 + 32
   END FUNCTION Celsius2Far
! ===========================================================================================================================================
END SUBROUTINE run_WH
