#!/usr/bin/env python3
#

import re
import evdev
import subprocess
import time
import argparse

def process_test_line(line, controls):
   tmp = line.strip()
   fields = tmp.split()
   operation = fields[1].lower()
   if operation == 'receive':
      target = fields[2].lower()
      if target == 'syn':
         return (operation, 0, 0, 0)
      elif target == 'axis':
         ctrl_type = evdev.ecodes.EV_ABS
      else:
         ctrl_type = evdev.ecodes.EV_KEY
      control = int(fields[3])
      value = int(fields[4])
   else:
      control_str = fields[2]
      if not control_str in controls:
         print('Warning: Control {0} unknown.'.format(control_str))
         print(line)
         return None
      (ctrl_type, control) = controls[control_str]
      value = int(fields[3])
   return (operation, ctrl_type, control, value)

def read_config(fname):
   sequence = []
   devname = ''
   controls = {}
   f = open(fname)

   test_re = re.compile('//\*(.*)$')
   dev_re = re.compile('^\s*(grab\s+)?device\s+"([^"]+)"')
   def_re = re.compile('^\s*(button|axis)\s+(\S+)\s*=\s*(\S+)')

   for line in f:
      m = test_re.match(line)
      if m:
         tst = process_test_line(line, controls)
         if tst:
            sequence.append(tst)
            continue
      m = dev_re.match(line)
      if m:
         devname = m.group(2)
         continue
      m = def_re.match(line)
      if m:
         if m.group(1) == 'axis':
            controls[m.group(2)] = (evdev.ecodes.EV_ABS, int(m.group(3)));
         else:
            controls[m.group(2)] = (evdev.ecodes.EV_KEY, int(m.group(3)));

   f.close()
   return {'seq':sequence, 'devname': devname, 'controls': controls}


def make_cap(config):
   axes = []
   buttons = []

   # loops through keys of dictionary
   for ctrl in config['controls']:
      (ctrl_type, ctrl_id) = config['controls'][ctrl]
      if ctrl_type == evdev.ecodes.EV_KEY:
         buttons.append(ctrl_id)
      else:
         axes.append((ctrl_id, evdev.AbsInfo(0, 255, 0, 15, 0, 0)))

   # sort the arrays
   axes.sort()
   buttons.sort()

   cap = {}

   if axes:
      cap[evdev.ecodes.EV_ABS] = axes;
   if buttons:
      cap[evdev.ecodes.EV_KEY] = buttons;

   return cap


def find_device(name):
   patt = re.compile(name)
   devices = [evdev.InputDevice(fn) for fn in evdev.list_devices()]
   for device in devices:
      if patt.match(device.name):
         return device

parser = argparse.ArgumentParser(description = 'Test evdevshift using specially prepared config.')
parser.add_argument('--config', type=str, dest='arg')

args = parser.parse_args()

arg = args.arg

# read the config and prepare the caps of the source device
config = read_config(arg)
cap = make_cap(config)

# create the source device
ui = evdev.UInput(cap, name=config['devname'], vendor = 0xf30, product = 0x110, version=0x110)
print(ui)

# start the evdevshift and point it to the config
eds = subprocess.Popen(['./evdevshift', '--config={0}'.format(arg)])

# temporary, to make sure the evdevshift started and created the device...
time.sleep(1)

# find the newly created device
dev = find_device('evdevshift')
print(dev)

#send the test sequence and check the outputs
buffered = False
problems = 0
for ev in config['seq']:
   if ev[0] == 'send':
      print('=====================================')
      print('Sending (type {0} code {1} val {2})'.format(ev[1], ev[2], ev[3]))
      sent = True
      ui.write(ev[1], ev[2], ev[3])
   else:
      if sent:
         #print('syn')
         ui.syn()
         sent = False
      # give the stuff some time to pass the events
      # not nice, will need to rework to avoid races
      time.sleep(0.1)
      in_ev = dev.read_one()
      if in_ev:
         if (in_ev.type == ev[1]) and (in_ev.code == ev[2]) and (in_ev.value == ev[3]):
            print('Response OK (type {0} code {1} val {2})'.format(ev[1], ev[2], ev[3]))
         else:
            problems += 1
            print('Error: Expected (type {0} code {1} val {2})'.format(ev[1], ev[2], ev[3]))
            print('       Received (type {0} code {1} val {2})'.format(in_ev.type, in_ev.code, in_ev.value))

print('=====================================')
print('Expected error (Read wrong number of bytes (-1)!)')
ui.close()
time.sleep(1)

if problems == 0:
   print('\n\nNo problems encountered!')
else:
   print('\n\n{0} problems found.'.format(problems))

