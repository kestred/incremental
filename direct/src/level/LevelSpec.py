"""LevelSpec module: contains the LevelSpec class"""

from PythonUtil import list2dict
import string

class LevelSpec:
    """contains spec data for a level, is responsible for handing the data
    out upon request, as well as recording changes made during editing, and
    saving out modified spec data"""
    def __init__(self, specDict, scenario=0):
        self.specDict = specDict

        # this maps an entId to the dict that holds its spec;
        # entities are either in the global dict or a scenario dict
        # update the map of entId to spec dict
        self.entId2specDict = {}
        self.entId2specDict.update(
            list2dict(self.getGlobalEntIds(),
                      value=self.privGetGlobalEntityDict()))
        for i in range(self.getNumScenarios()):
            self.entId2specDict.update(
                list2dict(self.getScenarioEntIds(i),
                          value=self.privGetScenarioEntityDict(i)))

        self.setScenario(scenario)

    def getNumScenarios(self):
        return len(self.specDict['scenarios'])

    def getScenarioWeights(self):
        weights = []
        for entry in self.specDict['scenarios']:
            weights.append(entry[1])
        return weights

    def setScenario(self, scenario):
        assert scenario in range(0, self.getNumScenarios())
        self.scenario = scenario

    def getScenario(self):
        return self.scenario

    def getGlobalEntIds(self):
        return self.privGetGlobalEntityDict().keys()

    def getScenarioEntIds(self, scenario=None):
        if scenario is None:
            scenario = self.scenario
        return self.privGetScenarioEntityDict(scenario).keys()

    def getAllEntIds(self):
        return self.getGlobalEntIds() + self.getScenarioEntIds()

    def getEntitySpec(self, entId):
        assert entId in self.entId2specDict
        specDict = self.entId2specDict[entId]
        return specDict[entId]

    def getEntityType(self, entId):
        return self.getEntitySpec(entId)['type']

    def getEntType2ids(self, entIds):
        """given list of entIds, return dict of entType 2 entIds"""
        entType2ids = {}
        for entId in entIds:
            type = self.getEntityType(entId)
            entType2ids.setdefault(type, [])
            entType2ids[type].append(entId)
        return entType2ids

    # private support functions to abstract dict structure
    def privGetGlobalEntityDict(self):
        return self.specDict['globalEntities']

    def privGetScenarioEntityDict(self, scenario):
        return self.specDict['scenarios'][scenario][0]

    if __debug__:
        def setAttribEditEventName(self, event):
            self.attribEditEventName = event
        def setAttribEdit(self, entId, attrib, value):
            # This is a proposed change; it has not been approved yet.
            # broadcast the change to someone else that knows what to do
            # with it
            messenger.send(self.attribEditEventName, [entId, attrib, value])

        def setAttribChangeEventName(self, event):
            self.attribChangeEventName = event
        def setAttribChange(self, entId, attrib, value):
            specDict = self.entId2specDict[entId]
            specDict[entId][attrib] = value
            # locally broadcast the fact that this attribute value has
            # officially changed
            if self.attribChangeEventName is not None:
                messenger.send(self.attribChangeEventName,
                               [entId, attrib, value])

        def getSpecImportsModuleName(self):
            # name of module that should be imported by spec py file
            return 'SpecImports'

        def getPrettyString(self):
            """Returns a string that contains the spec data, nicely formatted.
            This should be used when writing the spec out to file."""
            import pprint
            
            tabWidth = 4
            tab = ' ' * tabWidth
            # structure names
            globalEntitiesName = 'GlobalEntities'
            scenarioEntitiesName = 'Scenario%s'
            scenarioWeightName = 'Scenarios'
            topLevelName = 'levelSpec'
            def getPrettyEntityDictStr(name, dict, tabs=0):
                def t(n):
                    return (tabs+n)*tab
                def sortList(lst, firstElements=[]):
                    """sort list; elements in firstElements will be put
                    first, in the order that they appear in firstElements;
                    rest of elements will follow, sorted"""
                    elements = list(lst)
                    # put elements in order
                    result = []
                    for el in firstElements:
                        if el in elements:
                            result.append(el)
                            elements.remove(el)
                    elements.sort()
                    result.extend(elements)
                    return result
   
                firstTypes = ('levelMgr', 'zone',)
                firstAttribs = ('type', 'name', 'comment', 'parent',
                                'pos', 'x', 'y', 'z',
                                'hpr', 'h', 'p', 'r',
                                'scale', 'sx', 'sy', 'sz',
                                'color',
                                'model',
                                )
                str = t(0)+'%s = {\n' % name
                # get list of types
                entIds = dict.keys()
                entType2ids = self.getEntType2ids(entIds)
                # put types in order
                types = sortList(entType2ids.keys(), firstTypes)
                for type in types:
                    str += t(1)+'# %s\n' % string.upper(type)
                    entIds = entType2ids[type]
                    entIds.sort()
                    for entId in entIds:
                        str += t(1)+'%s: {\n' % entId
                        spec = dict[entId]
                        attribs = sortList(spec.keys(), firstAttribs)
                        for attrib in attribs:
                            str += t(2)+"'%s': %s,\n" % (attrib,
                                                         repr(spec[attrib]))
                        str += t(2)+'},\n'
                        
                str += t(1)+'}\n'
                return str
            def getPrettyScenarioWeightTableStr(tabs=0, self=self):
                def t(n):
                    return (tabs+n)*tab
                str  = t(0)+'%s = [\n' % scenarioWeightName
                for i in range(self.getNumScenarios()):
                    str += t(1)+'[%s, %s],\n' % (scenarioEntitiesName % i,
                                                  self.getScenarioWeights()[i])
                str += t(1)+']\n'
                return str
            def getPrettyTopLevelDictStr(tabs=0):
                def t(n):
                    return (tabs+n)*tab
                str  = t(0)+'%s = {\n' % topLevelName
                str += t(1)+"'globalEntities': %s,\n" % globalEntitiesName
                str += t(1)+"'scenarios': %s,\n" % scenarioWeightName
                str += t(1)+'}\n'
                return str
            
            str  = 'from %s import *\n' % self.getSpecImportsModuleName()
            str += '\n'

            # add the global entities
            str += getPrettyEntityDictStr('GlobalEntities',
                                          self.privGetGlobalEntityDict())
            str += '\n'

            # add the scenario entities
            numScenarios = self.getNumScenarios()
            for i in range(numScenarios):
                str += getPrettyEntityDictStr('Scenario%s' % i,
                                              self.privGetScenarioEntityDict(i))
                str += '\n'

            # add the scenario weight table
            str += getPrettyScenarioWeightTableStr()
            str += '\n'

            # add the top-level table
            str += getPrettyTopLevelDictStr()

            self.testPrettyString(prettyString=str)

            return str

        def testPrettyString(self, prettyString=None):
            # execute the pretty output in our local scope
            if prettyString is None:
                prettyString=self.getPrettyString()
            exec(prettyString)
            assert levelSpec == self.specDict, (
                'LevelSpec pretty string does not match spec data.\n'
                'pretty=%s\n'
                'specData=%s' %
                (levelSpec, self.specDict)
                )

        def __repr__(self):
            return 'LevelSpec(%s, scenario=%s)' % (repr(self.specDict),
                                                   self.scenario)
