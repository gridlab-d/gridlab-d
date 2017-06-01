'''
Created on Jul 11, 2012

@author: jimmy
'''
# Intro To Python:  Modules
# book.py

import config
import urllib
import json
import requests
import element
import json

class SchematicType:
    """
        SchematicType contains three constants which represent the model type: UNKNOWN, TRANSMISSION, DISTRIBUTION
    """
    UNKNOWN = "UNKNOWN" 
    TRANSMISSION = "TRANSMISSION"
    DISTRIBUTION = "DISTRIBUTION"
    
class MapType:
    """
        MapType contains two constants which represent the map type: BLANK, POLITICAL
    """
    BLANK = "BLANK"
    POLITICAL = "POLITICAL"

class Model:    
    """
      The GridSpice model contains the network model (transmission, distribution, etc)
    """
    def __init__(self, name, project=None, schematicType = SchematicType.DISTRIBUTION, mapType = MapType.POLITICAL, empty = 0):
        if (project.id != None):
            self.id = None
            self.name = name
            self.loaded = 0
            self.APIKey = project.APIKey
            self.projectId = project.id
            if (empty == 0):
                self.elementDict = []
                self.counter = 0
                self.climate = config.DEFAULT_CLIMATE
                self.schematicType = schematicType
                self.mapType = mapType
                self.loaded = 1
                
        else:
            raise ValueError("'" + project.name + "'"  + " has not yet been stored.")

    def _loadElements(self, key):
        self.elementDict = []
        if (key != None):
            payload = {'id':key}
            headers = {'APIKey':self.APIKey}
            r = requests.get(config.URL + "multipleelements.json", params = payload, headers = headers)
            if (r.status_code == requests.codes.ok):
                data = r.text
                if (data != config.INVALID_API_KEY):
                    jsondata = data.encode('ascii') #temporary attribute
                    jsonElemList = json.loads(jsondata)
                    for jsonElem in jsonElemList:
                        elemType = eval(jsonElem['objectType'].encode('ascii'))
                        dictCopy = {}
                        for key in jsonElem:
                            if (jsonElem[key] != None):
                                dictCopy[key.encode('ascii')] = jsonElem[key].encode('ascii')
                        del dictCopy['objectType']
                        elem = elemType()
                        elem.__dict__.update(dictCopy)
                        self.elementDict.append(elem)
                else: 
                    raise ValueError("'" + self.APIKey + "'"  + " is not a valid API key.")


    def load(self):
        """
            fills in the other information to the model object
    	"""
        if (self.id != None):
            payload = {'id':self.id}
            headers = {'APIKey':self.APIKey}
            r = requests.get(config.URL + "models/ids", params = payload, headers = headers)
            if (r.status_code == requests.codes.ok):
                data = r.text
                if (data != config.INVALID_API_KEY):
                    jsonModel = json.loads(data);
                    self.counter = int(jsonModel['counter'])
                    self.climate = jsonModel['climate'].encode('ascii')
                    self.schematicType = jsonModel['schematicType'].encode('ascii')
                    self.mapType = jsonModel['mapType'].encode('ascii')
                    blobkey = None
                    if ('modelDataKey' in jsonModel):
                        blobkey = jsonModel['modelDataKey'].encode('ascii')
                    self._loadElements(blobkey)
                    self.loaded = 1
                    print self.name + " has been loaded."
                else:
                    raise ValueError("'" + self.APIKey + "'"  + " is not a valid API key.")
        else:
            print self.name + " has not yet been stored in the database."
            
            
    def _store(self):  
        dictCopy = self.__dict__.copy()
        del dictCopy['APIKey']
        headers = {'APIKey':self.APIKey}
        elementDictCopy = map(lambda x: dict(x.__dict__.items() + {"objectType":(x.__class__.__module__ +"." + x.__class__.__name__)}.items()), dictCopy['elementDict'])
        jsonElems = json.dumps(elementDictCopy)
        dictCopy['elementDict'] = jsonElems
        payload = urllib.urlencode(dictCopy)
        
        r = requests.post(config.URL + "models/create", data=payload, headers = headers)
        if (r.status_code == requests.codes.ok):
            data = r.text
            if (data != config.INVALID_API_KEY):
                if (data != "null"):
                    result = int(data)
                    self.id = result
                    print self.name + " has been stored in the database."
                else:
                    print "Error saving. A different version of this model already exists. Has '" + self.name + "' been loaded?"
            else:
                raise ValueError("'" + self.APIKey + "'"  + " is not a valid API key.")
        else:
            print "Error in the server."    
            
    def _update(self):
        dictCopy = self.__dict__.copy()
        del dictCopy['APIKey']
        headers = {'APIKey':self.APIKey}
        elementDictCopy = map(lambda x: dict(x.__dict__.items() + {"objectType":(x.__class__.__module__ +"." + x.__class__.__name__)}.items()), dictCopy['elementDict'])
        jsonElems = json.dumps(elementDictCopy)
        dictCopy['elementDict'] = jsonElems
        payload = urllib.urlencode(dictCopy)
        r = requests.post(config.URL + "models/update", data=payload, headers = headers)
        
        if (r.status_code == requests.codes.ok):
            data = r.text
            if (data != config.INVALID_API_KEY):
                if (data != "null"):
                    result = int(data)
                    self.id = result
                    print self.name + " has been updated."
                else:
                    print "Error updating."
            else:
                raise ValueError("'" + self.APIKey + "'"  + " is not a valid API key.")
        else:
            print "Error in the server."
            
    def save(self):
        """
            saves this model
        """
        if (self.loaded == 1):
            if (self.id is None):
                self._store()
            else:
                self._update()
        else:
            print "Please load " + self.name + " before updating."

    def	delete(self):
        """
           deletes this model
        """
        if (self.id != None):
            headers = {'APIKey':self.APIKey, 'Content-Length':'0'}
            r = requests.post(config.URL + "models/destroy/" + repr(self.id), headers = headers)
            if (r.status_code == requests.codes.ok):
                data = r.text
                if (data != config.INVALID_API_KEY):
                    if (data != "null"):
                        self.id = None
                        print self.name + " has been deleted from the database."
                    else:
                        print "Error deleting."
                else:
                    raise ValueError("'" + self.APIKey + "'"  + " is not a valid API key.")
            else:
                print "Error in the server."
        else:
            print self.name + "has not yet been stored in the database"


    def	add (self, elem):
        """
    	   Adds the element to the model
    	"""
        if (elem.name == None):
            raise ValueError("All elements must have names.");
        if (self.loaded == 1):
            self.elementDict.append(elem)
        else:
            print "Please load this model first."
        
    def	remove(self, elem):
        """	
    	   Removes the element from the model
    	"""
        if (self.loaded == 1):
            self.elementDict.remove(elem)
        else:
            print "Please load this model first."
       
    def	copy(self, project):
        """
    	   Returns a copy of this model within the provided project.
        """

