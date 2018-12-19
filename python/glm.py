class GLM :
	def __init__(self,filename=None) :
		if type(filename) != type(None) :
			import json
			with open(filename,'r') as fh :
				data = json.load(fh)
			assert(data['application']=='gridlabd')
			assert(data['version'] >= '4.0.0')
			self.version = data['version']
			self.modules = data['modules']
			self.classes = data['classes']
			self.objects = data['objects']
			self.types = data['types']
			self.globals = data['globals']

	def check_properties(self) :
		'''Return list of class with bad property definitions'''
		# TODO
		return None		

	def check_parents(self) :
		'''Returns list of objects with invalid parents.'''
		result = []
		for obj, properties in self.objects.items() :
			if 'parent' in properties.keys() :
				if not properties['parent'] in self.objects.keys() :
					result.append(obj)
		if not result :
			return None
		else :	
			return result

