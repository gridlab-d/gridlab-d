#!/usr/bin/python

import sys
from math import *
from xml.dom.minidom import parse
from xml.dom import Node


class Object:
	def __init__(self, **props):
		self.__dict__ = props


def quadrant(x, y):
	if x >= 0:
		if y >= 0: return 0
		else: return 3
	else:
		if y >= 0: return 1
		else: return 2

# convert line-to-line voltage/angle to cartesian phase (line-to-neutral) voltage
def CPV(V, angle):
	rads = radians(angle)
	return (V / sqrt(3.0)) * cos(rads), (V / sqrt(3.0)) * sin(rads)

# convert cartesian voltage to angular voltage
def AV(x, y):
	if x:
		angle = atan(y / x)
	else:
		angle = 0.0
	V = sqrt(x**2 + y**2)
	return (V, [0, 180, -180, 0][quadrant(x, y)] + degrees(angle))

power = lambda kW, pf: complex(kW * 1000.0, tan(acos(pf)) * kW * 1000.0)

def stringify(val):
	if not val:
		return ''
	else:
		return str(val)

def get_nodes(path):
	objs = []
	doc = parse(path)
	nodes = [e for e in doc.getElementsByTagName('object') if str(e.getAttribute('type')) in ('node', 'load')]
	for node in nodes:
		name = node.getAttribute('name')
		props = dict([(str(n.tagName), stringify(n.firstChild.nodeValue)) for n in node.childNodes if n.nodeType == Node.ELEMENT_NODE and n.firstChild and n.firstChild.nodeType == Node.TEXT_NODE])
		for k in props:
			if k.endswith('_V'):
				if props[k]:
					props[k] = complex(props[k])
				else:
					props[k] = complex(0.0)
		objs.append(Object(name=name, **props))
	objs.sort(cmp=lambda x, y: cmp(y.rank, x.rank))
	return objs

def main():
#	objs = get_nodes(sys.argv[1])
        objs = get_nodes('outYB.xml')
	i = 1
	for obj in objs:
		if 'N' in obj.phases:
			V1 = obj.phaseA_V
			V2 = obj.phaseB_V
			V3 = obj.phaseC_V
		else:
			V1 = (obj.phaseA_V) - (obj.phaseB_V)
			V2 = (obj.phaseB_V) - (obj.phaseC_V)
			V3 = (obj.phaseC_V) - (obj.phaseA_V)
		print 'Node %s   '%i, obj.name
		print '     V1   %0.1f/%0.1f'%AV(V1.real, V1.imag)
		print '     V2   %0.1f/%0.1f'%AV(V2.real, V2.imag)
		print '     V3   %0.1f/%0.1f'%AV(V3.real, V3.imag)
		i += 1

if __name__ == '__main__':
	main()
