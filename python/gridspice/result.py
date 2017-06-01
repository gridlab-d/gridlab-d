'''
Created on Jul 31, 2012

@author: jimmy
'''
import config
import requests
import json
import simulation

class Result:
    """
        The GridSpice result object contains a simulation result's content and filename.
    """
    def __init__(self, name, simulation, blobkey):
        self.filename = name
        self.simulationId = simulation.id
        self.blobkey = blobkey
        self.APIKey = simulation.APIKey
    
    def load (self):
        if (self.blobkey != None):
            payload = {'id': self.blobkey}
            headers = {'APIKey':self.APIKey}
            r = requests.get(config.URL + "results/ids", params = payload, headers = headers)
            if (r.status_code == requests.codes.ok):
                data = r.text
                if (data != config.INVALID_API_KEY):
                    self.content = data.encode('ascii')
                else:
                    raise ValueError("'" + self.APIKey + "'"  + " is not a valid API key.")
            print "Result " + self.filename + " has been loaded."
        else:
            print "File " + self.filename + " cannot be found."