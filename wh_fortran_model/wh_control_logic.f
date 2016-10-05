! Water heater control logic
! ==========================================================================================
! Copyright (C) 2013, Battelle Memorial Institute
! Written by Laurentiu Dan Marinovici, Pacific Northwest National Laboratory
! Modified and developed from the initial PDE model from Zhijie Xu ("Modeling of Electric Water Heaters for Demand Response: a Baseline PDE Model")
! Last modified: 03/10/2014 
!==========================================================================================

SUBROUTINE wh_control_logic(op, sch, ambT, tankT, setT, dbT, sensT, offsetC, offsetUE,&
 low_ambT, up_ambT, low_waterT, mode_3_off, nc, nh, hu)
    IMPLICIT NONE
!   Input parameters
    LOGICAL, INTENT (IN) :: sch
    INTEGER, INTENT (IN) :: op
    DOUBLE PRECISION, INTENT (IN) :: ambT, tankT, setT(2), dbT, sensT(2), offsetC, offsetUE
    DOUBLE PRECISION, INTENT (IN) :: low_ambT, up_ambT, low_waterT, mode_3_off
    INTEGER :: ii
!   Output parameters
    INTEGER, INTENT (INOUT) :: nc, nh(2)
    LOGICAL, INTENT (INOUT) :: hu
!   Heating control strategy
    IF (op == 1) THEN ! if in Efficiency mode
       IF (sch) THEN
!         In extreme conditions, first use the heating elements to heat-up the water
          IF (ambT .LT. low_ambT .OR. ambT .GT. up_ambT .OR. tankT .LT. low_waterT) THEN
            nc = 0
            IF (tankT .LT. setT(1) - mode_3_off*5/9) THEN
               IF (sensT(1) .LT. setT(1) - dbT) THEN
                  nh(1) = 1
                  nh(2) = 0
               ELSE IF (sensT(1) .GE. setT(1) + dbT) THEN
                  nh(1) = 0
                  nh(2) = 1
               ELSE
                  nh(1) = 1
                  nh(2) = 0
               END IF
            ELSE IF (tankT .GE. setT(1)) THEN
               nh(1) = 0
               nh(2) = 0
            ELSE
               nh(1) = 0
               nh(2) = 1
            END IF
          ELSE
!            write(*, *) 'Resistance elements do not turn on, and we go into Efficiency Mode activating the compressor'
            IF (tankT < setT(1) - offsetC*5/9) THEN
              nh(1) = 0
              nh(2) = 0
              nc = 1
            ELSE IF (tankT >= setT(1) - dbT) THEN
              nh(1) = 0
              nh(2) = 0
              nc = 0
            END IF
          END IF
       ELSE ! from sch
              nh(1) = 0
              nh(2) = 0
              nc = 0
       END IF ! from sch
    ELSE IF (op == 2) THEN ! If in Hybrid Mode
       IF (sch) THEN
!         If temperature at the upper sensor is 18-20F below the set point,
!         activate the upper element.
          IF (tankT < setT(1) - offsetC*5/9) THEN
             IF (sensT(1) < setT(1) - offsetUE*5/9) THEN
                nc = 0
                nh(1) = 1
                nh(2) = 0
!            If upper sensor temperature is above the threshold, then turn off heating elements
!            and turn on compressor to heat the rest of the tank, if the temperature in the tank
!            is 8-10F under the setpoint.
             ELSE IF (sensT(1) >= setT(1) + dbT) THEN
                nh(1) = 0
                nh(2) = 0
                nc = 1
             ELSE IF (nh(1) == 1) THEN
                nh(1) = 1
                nh(2) = 0
                nc = 0
             ELSE
                nc = 1
                nh(1) = 0
                nh(2) = 0
             END IF
          ELSE IF (tankT >= setT(1) - dbT) THEN
            nc = 0
            nh(1) = 0
            nh(2) = 0
          END IF
       ELSE ! from sch
              nh(1) = 0
              nh(2) = 0
              nc = 0
       END IF ! from sch
    ELSE IF (op == 3) THEN ! If in Electric only Mode
! ========================== THIS IS THE LOGIC FOR MODE 3 OF THE HEAT PUMP WATER HEATER =============================================
!          IF (sch) THEN
!          If tank temperature drops 5F below the set point, activate the upper element
!           nc = 0
!           IF (tankT .LT. setT(1) - mode_3_off*5/9) THEN
!              IF (sensT(1) .LT. setT(1) - dbT) THEN
!                 nh(1) = 1
!                 nh(2) = 0
!              ELSE IF (sensT(1) .GE. setT(1) + dbT) THEN
!                 nh(1) = 0
!                 nh(2) = 1
!              ELSE
!                 nh(1) = 1
!                 nh(2) = 0
!              END IF
!           ELSE IF (tankT .GE. setT(1)) THEN
!              nh(1) = 0
!              nh(2) = 0
!          ELSE
!             nh(1) = 0
!             nh(2) = 1
!          END IF
!          ELSE ! from sch
!              nh(1) = 0
!              nh(2) = 0
!              nc = 0
!          END IF ! from sch
!        END IF
! =====================================================================================================================================
!       This is the control logic for the previous Electric Water Heater
       IF (sch) THEN
          DO ii = 1, 2
             IF (sensT(ii) .GT. setT(ii) + dbT) THEN
                nh(ii) = 0
             END IF
             IF (sensT(ii) .LT. setT(ii) - dbT) THEN
                nh(ii) = 1
             END IF
          END DO
!         Both heaters cannot be ON at the same time, but the bottom one takes precedence
          IF (nh(2) .EQ. 1) nh(1) = 0
       ELSE ! from sch
          DO ii = 1, 2
             nh(ii) = 0
          END DO
       END IF ! from sch
    END IF
    RETURN    
END SUBROUTINE wh_control_logic
