"""Track module: contains the Track class"""

from Interval import *

import types

PREVIOUS_END = 1
PREVIOUS_START = 2
TRACK_START = 3

IDATA_IVAL = 0
IDATA_TIME = 1
IDATA_TYPE = 2
IDATA_START = 3
IDATA_END = 4

class Track(Interval):
    # Name counter
    trackNum = 1
    # Class methods
    def __init__(self, intervalList, name=None):
        """__init__(intervalList, name)
        """
        # Record instance variables
	self.__buildIlist(intervalList)
	self.currentInterval = None
        # Generate unique name if necessary
	if (name == None):
	    name = 'Track-%d' % Track.trackNum
	    Track.trackNum = Track.trackNum + 1
        # Compute duration
	duration = self.__computeDuration()
        # Initialize superclass
	Interval.__init__(self, name, duration)

    def __getitem__(self, item):
        return self.ilist[item]

    def __buildIlist(self, intervalList):
	self.ilist = []
	for i in intervalList:
            if isinstance(i, Interval):
                self.ilist.append([i, 0.0, PREVIOUS_END, 0.0, 0.0])
            elif (isinstance(i, types.ListType) or
                  isinstance(i, types.TupleType)):
                t0 = i[0]
                ival = i[1]
                try:
                    type = i[2]
                except IndexError:
                    type = TRACK_START
                self.ilist.append([ival, t0, type, 0.0, 0.0])
            else:
                print 'Track.__buildIlist: Invalid intervallist entry'

    def __computeDuration(self):
	""" __computeDuration()
	"""
	duration = 0.0
	prev = None
        for idata in self.ilist:
	    ival = idata[IDATA_IVAL]
	    t0 = idata[IDATA_TIME]
	    type = idata[IDATA_TYPE]
	    assert(t0 >= 0.0)
            # Compute fill time, time between end of last interval and
            # start of this one
	    fillTime = t0 
	    if (type == PREVIOUS_END):
                pass
	    elif (type == PREVIOUS_START):
		if (prev != None):
		    fillTime = t0 - prev.getDuration()
	    elif (type == TRACK_START):
		fillTime = t0 - duration
	    else:
		Interval.notify.error(
			'Track.__computeDuration(): unknown type: %d' % type)
	    if (fillTime < 0.0):
		Interval.notify.error(
			'Track.__computeDuration(): overlap detected')
            # Compute start time of interval
            idata[IDATA_START] = duration + fillTime
            # Compute end time of interval
            idata[IDATA_END] = idata[IDATA_START] + ival.getDuration()
            # Keep track of current duration
	    duration = idata[IDATA_END]
	    prev = ival
	return duration

    def setIntervalStartTime(self, name, t0, type=TRACK_START):
	""" setIntervalStartTime(name, t0, type)
	"""
	found = 0
        for idata in self.ilist:
	    if (idata[IDATA_IVAL].getName() == name):
                idata[IDATA_TIME] = t0
                idata[IDATA_TYPE] = type
		found = 1
		break
	if (found):
	    self.duration = self.__computeDuration()	
	else:
	    Interval.notify.warning(
		'Track.setIntervalStartTime(): no Interval named: %s' % name)

    def getIntervalStartTime(self, name):
	""" getIntervalStartTime(name)
	"""
	for idata in self.ilist:
	    if (idata[IDATA_IVAL].getName() == name):
		return idata[IDATA_START]
	Interval.notify.warning(
		'Track.getIntervalStartTime(): no Interval named: %s' % name)
	return None

    def __getIntervalStartTime(self, interval):
	""" __getIntervalStartTime(interval)
	"""
	for idata in self.ilist:
	    if (idata[IDATA_IVAL] == interval):
		return idata[IDATA_START]
	Interval.notify.warning(
		'Track.getIntervalStartTime(): Interval not found')
	return None

    def getIntervalEndTime(self, name):
	""" getIntervalEndTime(name)
	"""
	for idata in self.ilist:
	    if (idata[IDATA_IVAL].getName() == name):	
		return idata[IDATA_END]
	Interval.notify.warning(
		'Track.getIntervalEndTime(): no Interval named: %s' % name)
	return None

    def updateFunc(self, t, event = IVAL_NONE):
	""" updateFunc(t, event)
	    Go to time t
	"""
        # Make sure track actually contains some intervals
        if not self.ilist:
	    Interval.notify.warning(
                'Track.updateFunc(): track has no intervals')
	    return
        # Deterimine which interval, if any to evaluate
	if (t < 0):
            # Before start of track, do nothing
	    pass
	else:
            # Initialize local variables
            currentInterval = None
            # First entry, re-init instance variables
            if (event == IVAL_INIT):
                # Initialize prev_t to max t of track
                self.prev_t = self.getDuration()
                # Clear record of currentInterval
                self.currentInterval = None
            # Compare t with start and end of each interval to determine
            # which interval(s) to execute.
            # If t falls between the start and end of an interval, that
            # becomes the current interval.  If we've crossed over the end
            # of an interval ((prev_t < tEnd) and (t > tEnd)) then execute
            # that interval at its final value.  If we've crossed over the
            # start of an interval ((prev_t > tStart) and (t < tStart))
            # then execute that interval at its start value
	    for ival, itime, itype, tStart, tEnd in self.ilist:
                # Compare time with ival's start/end times
                if (t < tStart):
                    if self.prev_t > tStart:
                        # We just crossed the start of this interval
                        # going backwards (e.g. via the slider)
                        # Execute this interval at its start time
                        ival.setT(0.0)
                    # Done checking intervals
                    break
                elif (t >= tStart) and (t <= tEnd):
                    # Between start/end, record current interval
                    currentInterval = ival
                    # Make sure event == IVAL_INIT if entering new interval
                    if ((self.prev_t < tStart) or
                        (ival != self.currentInterval)):
                        event = IVAL_INIT
                    # Evaluate interval at interval relative time
                    currentInterval.setT(t - tStart, event)
                    # Done checking intervals
                    break
                elif (t > tEnd):
                    # Crossing over interval end 
                    if ((self.prev_t < tEnd) or
                        ((event == IVAL_INIT) and ival.getfOpenEnded())):
                        # If we've just crossed the end of this interval
                        # or its an INIT event after the interval's end
                        # and the interval is openended,
                        # then execute the interval at its end time
                        ival.setT(ival.getDuration())
                     # May not be the last, keep checking other intervals
            # Record current interval (may be None)
            self.currentInterval = currentInterval

    def __repr__(self, indent=0):
	""" __repr__(indent)
	"""
	str = Interval.__repr__(self, indent) + '\n'
	for idata in self.ilist:	
	    str = (str + idata[IDATA_IVAL].__repr__(indent+1) +
                   (' start: %0.2f end: %0.2f' %
                    (idata[IDATA_START], idata[IDATA_END])) + '\n'
                   )
        return str
