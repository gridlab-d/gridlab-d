from gridspice import account, project, model, simulation, result, element
import sys
if (sys.len < 2):
	raise ValueError( 'please input api key')
apiKey = sys.argv[1]
acc = account.Account(apiKey)
projs = acc.getProjects()
proj = None
if (len(projs) == 0):
	proj = project.Project("1st Project", acc)
else:
	proj = projs[0]

mods = proj.getModels()
if (len(mods) == 0):
	mod = model.Model("1st distribution", proj)

sim = proj.runSimulation()

