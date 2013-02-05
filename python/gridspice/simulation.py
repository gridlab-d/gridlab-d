'''
Created on Jul 20, 2012

@author: jimmy
'''

# Intro To Python:  Modules
# book.py

import requests
import urllib
import json
import config
import result

class Simulation:

    """
      The GridSpice simulation object contains a project's simulation results
    """
    def __init__(self, id, project):
        self.id = id
        self.projectId = project.id
        self.APIKey = project.APIKey
        self.timezone = config.DEFAULT_TIMEZONE
        
    def load(self):
        """
            loads the Simulation report - May be removing this?
        """
        if (self.id != None):
            payload = {'id':self.id}
            headers = {'APIKey':self.APIKey}
            r = requests.get(config.URL + "simulations/ids", params = payload, headers = headers)
            if (r.status_code == requests.codes.ok):
                data = r.text
                if (data != config.INVALID_API_KEY):
                    jsonSimulation = json.loads(data)
                    self.id = int(jsonSimulation['id'])
                    self.lastHeartbeat = jsonSimulation['lastHeartbeat'].encode('ascii')
                    self.timestamp = jsonSimulation['date'].encode('ascii')
                    self.message = jsonSimulation['message'].encode('ascii')
                    self.xmlRequest = jsonSimulation['xmlRequest'].encode('ascii')
                    self.status = jsonSimulation['status'].encode('ascii')
                else:
                    raise ValueError("'" + self.APIKey + "'"  + " is not a valid API key.")
            print "Simulation " + repr(self.id) + " has been loaded."
        else:
            print "Simulation " + repr(self.id) + " has not yet been stored in the database."

    def getResults(self):
        """    
        Gets the models associated with this project (Models need to be loaded.)
        """
        emptyResults = []
        outputString = ""
        if (self.id != None):
            payload = {'id':self.id}
            headers = {'APIKey':self.APIKey}
            r = requests.get(config.URL + "multipleresults.json", params = payload, headers = headers)
            count = 0
            if (r.status_code == requests.codes.ok):
                data = r.text
                if (data != config.INVALID_API_KEY):
                    jsonList = json.loads(data)
                    for x in jsonList:
                        res = result.Result(x.encode('ascii'), self, x.encode('ascii'))
                        emptyResults.append(res)
                        outputString += "(" + repr(count) + ") " + res.filename + "  "
                        count = count + 1
                else:
                    raise ValueError("'" + self.APIKey + "'"  + " is not a valid API key.")
        else:
            print "This simulation has no id."
        
        print outputString
        return emptyResults
        



