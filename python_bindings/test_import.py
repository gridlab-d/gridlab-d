import os
import sys
import traceback

os.environ['GRIDLABD_HOME'] = r'C:\dev\gridlab-d_fork'

try:
    from gridlabd import GridLabD
    print("Import successful, creating instance...")
    gld = GridLabD()
    print('Success!')
except Exception as e:
    print(f'Error: {type(e).__name__}: {e}')
    traceback.print_exc()
