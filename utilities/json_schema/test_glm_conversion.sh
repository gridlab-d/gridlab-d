#!/bin/bash

indir=./glmFiles
outdir=./output

echo -e "\n== Test cap_ref_obj =="
python glm_to_json.py -d $indir -o $outdir cap_ref_obj
echo -e "\n== Test commercial_schedules =="
python glm_to_json.py -d $indir -o $outdir commercial_schedules
echo -e "\n== Test default =="
python glm_to_json.py -d $indir -o $outdir default
echo -e "\n== Test solar =="
python glm_to_json.py -d $indir -o $outdir solar
echo -e "\n== Test Te_Challange_test =="
python glm_to_json.py -d $indir -o $outdir Te_Challange_test
echo -e "\n== Test TE_Challenge =="
python glm_to_json.py -d $indir -o $outdir TE_Challenge
echo -e "\n== Test TEController =="
python glm_to_json.py -d $indir -o $outdir TEController
echo -e "\n== Test test_solargains =="
python glm_to_json.py -d $indir -o $outdir test_solargains
echo -e "\n== Test test_triplex_meter_parent_accum_NR_FPI =="
python glm_to_json.py -d $indir -o $outdir test_triplex_meter_parent_accum_NR_FPI
echo -e "\n== Test water_and_setpoint_schedule_v3 =="
python glm_to_json.py -d $indir -o $outdir water_and_setpoint_schedule_v3


echo -e "\n== Validate cap_ref_obj =="
python validate_json.py cap_ref_obj.json
echo -e "\n== Validate commercial_schedules =="
python validate_json.py commercial_schedules.json
echo -e "\n== Validate default =="
python validate_json.py default.json
echo -e "\n== Validate solar =="
python validate_json.py solar.json
echo -e "\n== Validate Te_Challange_test =="
python validate_json.py Te_Challange_test.json
echo -e "\n== Validate TE_Challenge =="
python validate_json.py -TE_Challenge.json
echo -e "\n== Validate TE_Challenge =="
python validate_json.py TE_Challenge.json
echo -e "\n== Validate TEController =="
python validate_json.py TEController.json
echo -e "\n== Validate test_solargains =="
python validate_json.py test_solargains.json
echo -e "\n== Validate test_triplex_meter_parent_accum_NR_FPI =="
python validate_json.py test_triplex_meter_parent_accum_NR_FPI.json
echo -e "\n== Validate water_and_setpoint_schedule_v3 =="
python validate_json.py water_and_setpoint_schedule_v3.json

echo -e "\n== Validate test_market_auction_clearing_3 =="
python validate_json.py test_market_auction_clearing_3.json
