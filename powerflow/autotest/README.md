This file communicate what JSON are still considered error and under investigation.
It all lists the files will not be converted and why.

All problems seems to be Deltamode in JSON loader 
/gridlab-d/powerflow/autotest/test_SPIM_under_voltage_contactor_converted.json
/gridlab-d/powerflow/autotest/test_SPIM_under_voltage_protection_converted.json
/gridlab-d/powerflow/autotest/test_TPIM_under_voltage_contactor_converted.json
/gridlab-d/powerflow/autotest/test_TPIM_under_voltage_protection_converted.json
/gridlab-d/powerflow/autotest/test_fault_inrush_transformer_FPI_converted.json
/gridlab-d/powerflow/autotest/test_fault_inrush_transformer_converted.json
/gridlab-d/powerflow/autotest/test_vfd_delta_converted.json


GLMs not convert to JSON when using indexing [ object house:..20 ], because is uses sub objects
/gridlab-d/powerflow/autotest/test_powerflow_exercise_4_1_3_converted.glm