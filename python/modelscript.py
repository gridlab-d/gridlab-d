from gridspice import account, project, model, simulation, result, element
import sys
if len(sys.argv) < 3:
    raise ValueError( 'please input api key and model name to create')
apiKey = sys.argv[1]
acc = account.Account(apiKey)
proj = acc.getProjects()[0]
model_name = sys.argv[2]
mod = model.Model(model_name, proj)
lat = 37.449
long = -122.1960
for x in range(1, 4):
	nod = element.powerflow.powerflow_object.node()
	nod.latitude = repr(lat + x*0.001)
	nod.longitude = repr(long + x*0.001)
	nod.name = 'node-' + repr(x)
	mod.add(nod)
mod.save()
