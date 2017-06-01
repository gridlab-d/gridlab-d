'''
Created on Mar 31, 2015

@author: srid966
'''
import os
import math 
import Sid_PNNL_Solar

num_cases = 1000
feeder_name = 'Major'
PV_penetration = [25, 30, 35, 40, 45, 50, 55, 60, 65, 70, 75]

dst = "C:\\Users\\mayh819\\CSI\\eclipse workspace\\CSI GLM scripts\\Source\\Python Scripts\\Statistical Runs - PV penetration\\Runs\\%s" %(feeder_name)
filename2 = dst + '\\new_run_all.bat' 


feeder_name, feeder_glm, house_glm, run_season_glm, commercial_glm, new_feeder_glm = Sid_PNNL_Solar.find_files(dst)

feeder_numbers = {'Woodhaven':720, 'Solitaire':2,'Wildwood': 3, 'Mallet': 4, 'Etting': 600,\
                     'Diana': 6, 'Price': 7,'Rossi': 8, 'Drive': 400, 'Soto': 600, 'Doerner': 11,\
                     'Moran': 720, 'Tamarind': 720, 'Libra': 840, 'Lynx': 720, 'Wasp': 600, 'Parrot': 17,\
                     'Kneeland': 600, 'Cawston': 19, 'Miracle': 480, 'Miles': 21, 'Major': 22,\
                     'Caldwell': 23, 'Homer': 300, 'Armona': 480, 'Lauterbach': 300, 'Smoke': 400,\
                     'Joshua': 720, 'Mirror': 720, 'Hammock':480}
if num_cases > 30:num_cores=30
else: num_cores=num_cases
extra_cases = num_cases % num_cores
#for count in range(0,int(math.ceil(num_cases/30))):
batch_start_num = 0
for count in range(0,int(math.ceil(num_cores))):
    new_batch_all = 'new_run_core_%d.bat' %(count)
    filename = '%s\\%s' %(dst, new_batch_all)
    if os.path.exists(filename):os.remove(filename)

    if count < extra_cases:
      num_cases_per_batch = int(math.ceil(num_cases / num_cores))+1
    else:
      num_cases_per_batch = int(math.ceil(num_cases / num_cores))
            
    for i in range(batch_start_num, batch_start_num+num_cases_per_batch):

        for k in range(0,len(PV_penetration)): 
            batch_file = open(filename,'a')
            batch_file.write('gridlabdVS2010  --define SEASON=Summer --define CASE=%d --define PEN_LEV=%d %s_%dpct_case_%d_run_season_%s.glm > %s_%dpct_case_%d_run_summer.txt 2>&1\n' % (i, PV_penetration[k], feeder_name, PV_penetration[k], i, feeder_numbers[feeder_name], feeder_name, PV_penetration[k],i)) 
            batch_file.write('gridlabdVS2010  --define SEASON=Fall --define CASE=%d --define PEN_LEV=%d %s_%dpct_case_%d_run_season_%s.glm > %s_%dpct_case_%d_run_fall.txt 2>&1\n' % (i, PV_penetration[k], feeder_name, PV_penetration[k], i, feeder_numbers[feeder_name], feeder_name, PV_penetration[k], i))
            batch_file.write('gridlabdVS2010  --define SEASON=Winter --define CASE=%d --define PEN_LEV=%d %s_%dpct_case_%d_run_season_%s.glm > %s_%dpct_case_%d_run_winter.txt 2>&1\n' % (i, PV_penetration[k], feeder_name, PV_penetration[k], i, feeder_numbers[feeder_name], feeder_name, PV_penetration[k], i))
            batch_file.write('gridlabdVS2010  --define SEASON=Spring --define CASE=%d --define PEN_LEV=%d %s_%dpct_case_%d_run_season_%s.glm > %s_%dpct_case_%d_run_spring.txt 2>&1\n' % (i, PV_penetration[k], feeder_name, PV_penetration[k], i, feeder_numbers[feeder_name], feeder_name, PV_penetration[k], i))
            batch_file.close()

    batch_start_num = i + 1          
    batch_file_all = open(filename2,'a')    
    batch_file_all.write('start %s\n' % (new_batch_all))
    batch_file_all.close()
    
print('DONE!')