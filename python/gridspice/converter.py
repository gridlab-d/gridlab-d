import json

def object_to_json(obj):
	jsonObject = { }
	jsonObject.update(obj.__dict__)
	return jsonObject


