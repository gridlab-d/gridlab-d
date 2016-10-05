'''
Created on Nov 7, 2014

@author: srid966
'''
import Sid_PNNL_Solar
import feeder_parse_mod
import os
import shutil
import math
  
feeder_name = 'Mallet'  
PV_penetration = [5, 10, 15, 20, 25, 30, 35, 40, 45, 50]
num_cases = 250

# path for feeder files
current_feeder = "C:\Users\srid966\Desktop\My Work\California Solar Initiative\Feeders Post Upgrades\%s" %(feeder_name)
 
# path for GLD build
src = "C:\Users\srid966\Desktop\My Work\California Solar Initiative\Release 10 30 2015"
# path for cases generated
dst = "C:\Users\srid966\Desktop\My Work\California Solar Initiative\Final_Tests\%s" %(feeder_name)
# path for CYME database
Custom_CYME_DB = "C:\Users\srid966\Desktop\My Work\California Solar Initiative\CYME_Conductor.csv"    
 
feeder_name, feeder_glm, house_glm, run_season_glm, commercial_glm, new_feeder_glm, commercial_solar_GLM = Sid_PNNL_Solar.find_files(current_feeder, 'None', 1, 1)
 
feeder_numbers = {'Solitaire':2,'Wildwood': 3, 'Mallet': 4, 'Diana': 6, 'Price': 7,\
                  'Rossi': 8, 'Doerner': 11, 'Moran': 720, 'Parrot': 17, 'Cawston': 19,\
                   'Miles': 21, 'Major': 22, 'Caldwell': 23, 'Homer': 24,'Mirror': 29, 'Etting': 5}
     
Peak_kVA = {'Cawston': 1750.88, 'Miles': 1578.67, 'Caldwell': 7923.26, 'Wildwood': 11106.29, 'Mallet': 8845.87,\
            'Diana': 7016.44, 'Major': 2096.75, 'Homer': 2951.41, 'Mirror': 7081.18, 'Moran': 9856.65, 'Price': 6230.78,\
            'Rossi': 6076.67, 'Parrot': 7917.70, 'Solitaire': 5783.40, 'Doerner': 5971.25, 'Etting':5690.05}
   
Existing_PV = {'Cawston': 37.6, 'Miles': 6, 'Caldwell': 85.6, 'Mallet': 185.1,\
                'Major': 66.9, 'Rossi': 82, 'Diana': 27.2, 'Doerner': 0, 'Parrot': 1.3,\
                'Price': 66.9, 'Solitaire': 0, 'Wildwood': 320, 'Homer': 16.8, 'Mirror': 81.3, 'Etting': 4.7}
  
  
src_va = "C:\\Users\\srid966\\Desktop\\My Work\\California Solar Initiative\\Feeders Post Upgrades\\voltage_players\\voltage_schedule_%d_A.player" %(feeder_numbers[feeder_name])
src_vb = "C:\\Users\\srid966\\Desktop\\My Work\\California Solar Initiative\\Feeders Post Upgrades\\voltage_players\\voltage_schedule_%d_B.player" %(feeder_numbers[feeder_name])
src_vc = "C:\\Users\\srid966\\Desktop\\My Work\\California Solar Initiative\\Feeders Post Upgrades\\voltage_players\\voltage_schedule_%d_C.player" %(feeder_numbers[feeder_name])
     
for pen_lev in range(0,len(PV_penetration)):
      
    penetration_kVA = Peak_kVA[feeder_name] * PV_penetration[pen_lev]/100
      
    penetration_kVA = penetration_kVA - Existing_PV[feeder_name]
      
# Creating a new folder Major_new to be used for testing and moving GLD executables into it
    if pen_lev == 0:
        if os.path.exists(dst):shutil.rmtree(dst)
        os.makedirs(dst)
        Sid_PNNL_Solar.copytree(src, dst, symlinks=False, ignore=None)
   
     
    for count in range(num_cases):
         
        current_feeder = "C:\Users\srid966\Desktop\My Work\California Solar Initiative\Feeders Post Upgrades\%s" %(feeder_name)
         
        print 'Penetration Level:', PV_penetration[pen_lev], ', Case:', count
       
        new_feeder_glm = '%s_%dpct_case_%d_' %(feeder_name, PV_penetration[pen_lev], count)+ feeder_glm
        new_commercial_glm = '%s_%dpct_case_%d_' %(feeder_name, PV_penetration[pen_lev], count)+ commercial_glm
         
        Sid_PNNL_Solar.add_violation_object(current_feeder, feeder_name, feeder_glm, house_glm, run_season_glm, commercial_glm, PV_penetration[pen_lev], count, dst, new_feeder_glm)
       
        Sid_PNNL_Solar.modify_run_season_file(current_feeder, run_season_glm, feeder_glm, new_feeder_glm, feeder_name, count, house_glm, commercial_glm, PV_penetration[pen_lev], dst, new_commercial_glm)
       
        Sid_PNNL_Solar.add_conductor_ratings(current_feeder, feeder_glm, dst, new_feeder_glm, Custom_CYME_DB)
          
        Sid_PNNL_Solar.add_commercial_conductor_ratings(current_feeder, commercial_glm, dst, new_commercial_glm, Custom_CYME_DB)
          
        if feeder_name == 'Parrot':
            Sid_PNNL_Solar.add_conductor_ratings(current_feeder, 'Missing_Load_17.glm', dst, 'new_Missing_Load_17.glm', Custom_CYME_DB)
       
       
        if PV_penetration[pen_lev] != 0:
            if feeder_name != 'Solitaire' and feeder_name != 'Doerner':
                sorted_triplex, triplex_meter_data = Sid_PNNL_Solar.residential_data(os.path.join(current_feeder, house_glm))
                  
                solar_installation_data = Sid_PNNL_Solar.PV_residential_allocation(feeder_name, sorted_triplex, penetration_kVA, triplex_meter_data)
                  
                solar_dict = Sid_PNNL_Solar.residential_solar_create(solar_installation_data, triplex_meter_data)
                feeder_str = feeder_parse_mod.sortedWrite(solar_dict)
                                 
                filename = '%s\%s_%dpct_case_%d_residential_solar.glm'%(dst, feeder_name, PV_penetration[pen_lev], count)  
                glm_file = open(filename,'w')
                glm_file.write(feeder_str)
                glm_file.close()
              
            com_sorted_meters, com_meter_data = Sid_PNNL_Solar.commercial_data(os.path.join(current_feeder, commercial_glm), feeder_name)
            com_solar_installation_data = Sid_PNNL_Solar.PV_commercial_allocation(feeder_name, com_sorted_meters, penetration_kVA)
  
            com_solar_dict = Sid_PNNL_Solar.com_solar_create(com_solar_installation_data, com_meter_data)
              
            feeder_str = feeder_parse_mod.sortedWrite(com_solar_dict)
         
            filename = '%s\%s_%dpct_case_%d_commercial_solar.glm'%(dst, feeder_name, PV_penetration[pen_lev], count)                  
            glm_file = open(filename,'w')
            glm_file.write(feeder_str)
            glm_file.close()
           
 
        solar_installation_data = None
        solar_dict = None
        com_solar_installation_data = None
        com_solar_dict = None
              
#Moving all feeder files to the new location as well
#MOVED ALL LINES BELOW TO RIGHT
 
        Sid_PNNL_Solar.copytree(current_feeder, dst, symlinks=False, ignore=False)
          
        current_feeder = dst              
        feeder_name, feeder_glm, house_glm, run_season_glm, commercial_glm, new_feeder_glm, commercial_solar_GLM = Sid_PNNL_Solar.find_files(current_feeder, feeder_name , PV_penetration[pen_lev], count)
         
        transformer_solar_map, transformer_meter_map, line_solar_map = Sid_PNNL_Solar.com_transformers_and_solar_map(current_feeder, commercial_solar_GLM)
        line_solar_map = Sid_PNNL_Solar.modify_node_to_triplex_node(new_feeder_glm, current_feeder, dst, feeder_numbers[feeder_name], transformer_solar_map, transformer_meter_map,  line_solar_map)
        Sid_PNNL_Solar.remodify_com_conductor_data(line_solar_map, current_feeder, commercial_solar_GLM)
         
#         print line_solar_map
#         print transformer_meter_map
#         print transformer_solar_map
         
        line_solar_map = None
        transformer_meter_map = None
        transformer_solar_map = None
     
shutil.copy2(src_va, dst) 
shutil.copy2(src_vb, dst)    
shutil.copy2(src_vc, dst)    
 
print 'DONE 1 - Case Files Generated!'

filename2 = dst + '\\new_run_all.bat' 


feeder_name, feeder_glm, house_glm, run_season_glm, commercial_glm, new_feeder_glm, temp = Sid_PNNL_Solar.find_files(dst, 'None', 1, 1)

#if num_cases > 72:num_cores=72
#else: num_cores=num_cases
num_cores = num_cases*len(PV_penetration)
#extra_cases = num_cases % num_cores
#for count in range(0,int(math.ceil(num_cases/30))):
count=0

for i in range(0,int(math.ceil(num_cases))):

    
    for k in range(0,len(PV_penetration)):     
        new_batch_all = 'new_run_core_%d.bat' %(count)
        filename = '%s\\%s' %(dst, new_batch_all)
        if os.path.exists(filename):os.remove(filename)
    
    
        batch_file = open(filename,'a')
        batch_file.write('gridlabdVS2010  --define SEASON=Summer --define CASE=%d --define PEN_LEV=%d %s_%dpct_case_%d_run_season_%s.glm > %s_%dpct_case_%d_run_summer.txt 2>&1\n' % (i, PV_penetration[k], feeder_name, PV_penetration[k], i, feeder_numbers[feeder_name], feeder_name, PV_penetration[k],i)) 
        batch_file.write('gridlabdVS2010  --define SEASON=Fall --define CASE=%d --define PEN_LEV=%d %s_%dpct_case_%d_run_season_%s.glm > %s_%dpct_case_%d_run_fall.txt 2>&1\n' % (i, PV_penetration[k], feeder_name, PV_penetration[k], i, feeder_numbers[feeder_name], feeder_name, PV_penetration[k], i))
        batch_file.write('gridlabdVS2010  --define SEASON=Winter --define CASE=%d --define PEN_LEV=%d %s_%dpct_case_%d_run_season_%s.glm > %s_%dpct_case_%d_run_winter.txt 2>&1\n' % (i, PV_penetration[k], feeder_name, PV_penetration[k], i, feeder_numbers[feeder_name], feeder_name, PV_penetration[k], i))
        batch_file.write('gridlabdVS2010  --define SEASON=Spring --define CASE=%d --define PEN_LEV=%d %s_%dpct_case_%d_run_season_%s.glm > %s_%dpct_case_%d_run_spring.txt 2>&1\n' % (i, PV_penetration[k], feeder_name, PV_penetration[k], i, feeder_numbers[feeder_name], feeder_name, PV_penetration[k], i))
        batch_file.close()


        count = count + 1          
        batch_file_all = open(filename2,'a')    
        batch_file_all.write('start %s\n' % (new_batch_all))
        batch_file_all.close()
    
print('DONE 2 - Batch Scripts Generated!')