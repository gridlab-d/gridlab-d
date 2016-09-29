'''
Created on Oct 27, 2014

@author: mayh819
'''
import datetime, copy, os, re, warnings

def parse(inputStr, filePath=True):
    ''' Parse a GLM into an omf.feeder tree. This is so we can walk the tree, change things in bulk, etc.
    Input can be a filepath or GLM string.
    '''
    tokens = _tokenizeGlm(inputStr, filePath)
    return _parseTokenList(tokens)

def _tokenizeGlm(inputStr, filePath=True):
    ''' Turn a GLM file/string into a linked list of tokens.

    E.g. turn a string like this:
    clock {clockey valley;};
    object house {name myhouse; object ZIPload {inductance bigind; power newpower;}; size 234sqft;}; 

    Into a Python list like this:
    ['clock','{','clockey','valley','}','object','house','{','name','myhouse',';',
        'object','ZIPload','{','inductance','bigind',';','power','newpower','}','size','234sqft','}']
    '''
    if filePath:
        with open(inputStr,'r') as glmFile:
            data = glmFile.read()
    else:
        data = inputStr
    # Get rid of http for stylesheets because we don't need it and it conflicts with comment syntax.
    data = re.sub(r'http:\/\/', '', data)  
    # Strip comments.
    data = re.sub(r'\/\/.*\n', '', data)
    # Also strip non-single whitespace because it's only for humans:
    data = data.replace('\r','').replace('\t',' ')
    # Tokenize around semicolons, braces and whitespace.
    tokenized = re.split(r'(;|\}|\{|\s)',data)
    # Get rid of whitespace strings.
    basicList = filter(lambda x:x!='' and x!=' ', tokenized)
    return basicList

def _parseTokenList(tokenList):
    ''' Given a list of tokens from a GLM, parse those into a tree data structure. '''
    def currentLeafAdd(key, value, tree, guidStack):
        # Helper function to add to the current leaf we're visiting.
        current = tree
        for x in guidStack:
            current = current[x]
        current[key] = value
    def listToString(listIn):
        # Helper function to turn a list of strings into one string with some decent formatting.
        if len(listIn) == 0:
            return ''
        else:
            return reduce(lambda x,y:str(x)+' '+str(y),listIn[1:-1])
    # Tree variables.
    tree = {}
    guid = 0
    guidStack = []
    # Pop off a full token, put it on the tree, rinse, repeat.
    while tokenList != []:
        # Pop, then keep going until we have a full token (i.e. 'object house', not just 'object')
        fullToken = []
        while fullToken == [] or fullToken[-1] not in ['{',';','}','\n','shape']:
            fullToken.append(tokenList.pop(0))
        # Work with what we've collected.
        if fullToken[0] == '#set':
            if fullToken[-1] == ';':
                tree[guid] = {'omftype':fullToken[0],'argument':listToString(fullToken)}
            else:
                tree[guid] = {'#set':fullToken[0],'#set':listToString(fullToken)}
            guid += 1
        elif fullToken[0] == '#include':
            if fullToken[-1] == ';':
                tree[guid] = {'omftype':fullToken[0],'argument':listToString(fullToken)}
            else:
                tree[guid] = {'#include':fullToken[0],'#include':listToString(fullToken)}            
            guid += 1
        elif fullToken[0] == 'shape':
            while fullToken[-1] not in ['\n']:
                fullToken.append(tokenList.pop(0))
            fullToken[-2]=''
            currentLeafAdd(fullToken[0],listToString(fullToken[0:-1]), tree, guidStack)
            guid += 1                
        elif fullToken[-1] == '\n' or fullToken[-1] == ';':
            # Special case when we have zero-attribute items (like #include, #set, module).
            if guidStack == [] and fullToken != ['\n'] and fullToken != [';']:
                tree[guid] = {'omftype':fullToken[0],'argument':listToString(fullToken)}
                guid += 1
            # We process if it isn't the empty token (';')
            elif len(fullToken) > 1:
                currentLeafAdd(fullToken[0],listToString(fullToken), tree, guidStack)
        elif fullToken[-1] == '}':
            if len(fullToken) > 1:
                currentLeafAdd(fullToken[0],listToString(fullToken), tree, guidStack)
            guidStack.pop()
        elif fullToken[0] == 'schedule':
            # Special code for those ugly schedule objects:
            if fullToken[0] == 'schedule':
                while fullToken[-1] not in ['}']:
                    fullToken.append(tokenList.pop(0))
                tree[guid] = {'object':'schedule','name':fullToken[1], 'cron':' '.join(fullToken[3:-2])}
                guid += 1
        elif fullToken[-1] == '{':
            currentLeafAdd(guid,{}, tree, guidStack)
            guidStack.append(guid)
            guid += 1
            # Wrapping this currentLeafAdd is defensive coding so we don't crash on malformed glms.
            if len(fullToken) > 1:
                # Do we have a clock/object or else an embedded configuration object?
                if len(fullToken) < 4:
                    currentLeafAdd(fullToken[0],fullToken[-2], tree, guidStack)
                else:
                    currentLeafAdd('omfEmbeddedConfigObject', fullToken[0] + ' ' + listToString(fullToken), tree, guidStack)
    return tree
def sortedWrite(inTree):
    ''' Write out a GLM from a tree, and order all tree objects by their key. 
    Sometimes Gridlab breaks if you rearrange a GLM.
    '''
    sortedKeys = sorted(inTree.keys(), key=int)
    output = ''
    try:
        for key in sortedKeys:
            output += _dictToString(inTree[key]) + '\n'
    except ValueError:
        raise Exception
    return output

def _dictToString(inDict):
    ''' Helper function: given a single dict representing a GLM object, concatenate it into a string. '''
    # Handle the different types of dictionaries that are leafs of the tree root:
    if 'omftype' in inDict:
        return inDict['omftype'] + ' ' + inDict['argument'] + ';'
    elif 'module' in inDict:
        return 'module ' + inDict['module'] + ' {\n' + _gatherKeyValues(inDict, 'module') + '}\n'
    elif 'clock' in inDict:
        #return 'clock {\n' + gatherKeyValues(inDict, 'clock') + '};\n'
        # This object has known property order issues writing it out explicitly
        clock_string = 'clock {\n' + '\ttimezone ' + inDict['timezone'] + ';\n' + '\tstarttime ' + inDict['starttime'] + ';\n' + '\tstoptime ' + inDict['stoptime'] + ';\n}\n'
        return clock_string
    elif 'object' in inDict and inDict['object'] == 'schedule':
        return 'schedule ' + inDict['name'] + ' {\n' + inDict['cron'] + '\n};\n'
    elif 'object' in inDict:
        return 'object ' + inDict['object'] + ' {\n' + _gatherKeyValues(inDict, 'object') + '};\n'
    elif 'omfEmbeddedConfigObject' in inDict:
        return inDict['omfEmbeddedConfigObject'] + ' {\n' + _gatherKeyValues(inDict, 'omfEmbeddedConfigObject') + '};\n'
    elif '#include' in inDict:
        return '#include ' + inDict['#include']
    elif '#define' in inDict:
        return '#define ' + inDict['#define'] + '\n'
    elif '#set' in inDict:
        return '#set ' + inDict['#set'] 
    elif 'class' in inDict:
      #  prop = ''
    #    if 'variable_types' in inDict.keys() and 'variable_names' in inDict.keys() and len(inDict['variable_types'])==len(inDict['variable_names']):
      #  for x in xrange(len(inDict['variable_types'])):
      #      prop += '\t' + inDict['variable_types'][x] + ' ' + inDict['variable_names'][x] + ';\n'
        return 'class ' + inDict['class'] + ' {\n' + _gatherKeyValues(inDict, 'class') + '}\n'
       # else:
           # return '\n'
def _gatherKeyValues(inDict, keyToAvoid):
    ''' Helper function: put key/value pairs for objects into the format Gridlab needs. '''
    otherKeyValues = ''
    for key in inDict:
        if type(key) is int:
            # WARNING: RECURSION HERE
            otherKeyValues += _dictToString(inDict[key])
        elif key != keyToAvoid:
            if key == 'comment':
                otherKeyValues += (inDict[key] + '\n')
            elif key == 'name' or key == 'parent':
                if len(inDict[key]) <= 62:
                    otherKeyValues += ('\t' + key + ' ' + str(inDict[key]) + ';\n')
                else:
                    warnings.warn("{:s} argument is longer that 64 characters. Truncating {:s}.".format(key, inDict[key]), RuntimeWarning)
                    otherKeyValues += ('\t' + key + ' ' + str(inDict[key])[0:62] + '; // truncated from {:s}\n'.format(inDict[key]))
            else:
                otherKeyValues += ('\t' + key + ' ' + str(inDict[key]) + ';\n')
    return otherKeyValues
def _test():
    feeder_dictionary = parse('C:/Users/mayh819/CSI/eclipse workspace/CSI GLM scripts/Source/Python Scripts/CSI inputs/run_winter1.glm')
    print(feeder_dictionary)
    feeder_str = sortedWrite(feeder_dictionary)
    glm_file = open('./run_winter1_output2.glm','w')
    glm_file.write(feeder_str)
    glm_file.close()
    print('success!')
if __name__ == '__main__':
    _test()