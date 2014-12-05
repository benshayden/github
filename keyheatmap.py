#!/usr/bin/env python2.6
import sys
import os
import ctypes

x11 = ctypes.CDLL('libX11.so')

XKeysymToString = x11.XKeysymToString
XKeysymToString.restype = ctypes.c_char_p
XGetModifierMapping = x11.XGetModifierMapping
KeyCode = ctypes.c_ubyte
class XModifierKeymap(ctypes.Structure):
    __slots__ = [
        'max_keypermod',
        'modifiermap',
    ]

XModifierKeymap._fields_ = [
    ('max_keypermod', ctypes.c_int),
    ('modifiermap', ctypes.POINTER(KeyCode)),
]

XGetModifierMapping.restype = ctypes.POINTER(XModifierKeymap)

MODIFIERS = ['Shift', 'Lock', 'Control', 'Alt', 'Mod2', 'Mod3', 'Super', 'Mod5']
DELAY_US = 1000
FLUSH_KEYS = 1

def bit(buf, i):
  return ord(buf[i/8]) & (1 << (i % 8)) != 0

def histinc(histogram, keyname):
  try: histogram[keyname] += 1
  except KeyError: histogram[keyname] = 1

def flush(histogram, digraphs, logfile):
  histogram = histogram.items()
  histogram.sort(key=lambda (key, value): -value)
  digraphs = digraphs.items()
  digraphs.sort(key=lambda (key, value): -value)
  logfile = file(logfile, 'w')
  for key, value in histogram:
    logfile.write('%s %d\n' % (key, value))
  for key, value in digraphs:
    logfile.write('%s,%s %d\n' % (key[0], key[1], value))

def view_keys():
  buf_a = ctypes.create_string_buffer(32)
  buf_b = ctypes.create_string_buffer(32)
  current_buf, prev_buf = buf_a, buf_b
  prev_key = None
  captured = 0
  display = x11.XOpenDisplay(os.environ.get('DISPLAY', ':0'))
  if not display:
    return
  x11.XSynchronize(display, True)
  mod_map = x11.XGetModifierMapping(display)[0]
  alt_mods = set()
  x11.XQueryKeymap(display, current_buf)
  while True:
    x11.usleep(DELAY_US)
    current_buf, prev_buf = prev_buf, current_buf
    x11.XQueryKeymap(display, current_buf)
    shift = False
    for i in range(mod_map.max_keypermod):
      modkc = mod_map.modifiermap[i]
      if modkc and bit(current_buf, modkc):
        shift = True
    for i in range(32 * 8):
      if bit(current_buf, i) and not bit(prev_buf, i):
        key_name = XKeysymToString(x11.XKeycodeToKeysym(display, i, shift))
        print key_name

def keyheatmap(logfile):
  logfile = os.path.abspath(os.path.expanduser(logfile))
  histogram = {}
  digraphs = {}
  if os.path.exists(logfile):
    contents = file(logfile).read()
    for line in contents.split('\n'):
      if line and ' ' in line:
        keyname, count = line.split()
        if ',' in keyname:
          digraphs[tuple(keyname.split(','))] = int(count)
        else:
          histogram[keyname] = int(count)
  else:
    file(logfile, 'w').write('')
  os.chmod(logfile, 0600)
  buf_a = ctypes.create_string_buffer(32)
  buf_b = ctypes.create_string_buffer(32)
  current_buf, prev_buf = buf_a, buf_b
  prev_key = None
  captured = 0
  display = x11.XOpenDisplay(os.environ.get('DISPLAY', ':0'))
  if not display:
    return
  x11.XSynchronize(display, True)
  mod_map = x11.XGetModifierMapping(display)[0]
  alt_mods = set()
  x11.XQueryKeymap(display, current_buf)
  while True:
    x11.usleep(DELAY_US)
    current_buf, prev_buf = prev_buf, current_buf
    x11.XQueryKeymap(display, current_buf)
    modifiers = []
    shift = False
    for mask_bit in range(8):
      for i in range(mod_map.max_keypermod):
        modkc = mod_map.modifiermap[mask_bit * mod_map.max_keypermod + i]
        if modkc and bit(current_buf, modkc):
          modifiers.append(MODIFIERS[mask_bit])
          if mask_bit == 0:
            shift = True
    for alt_mod in alt_mods:
      if alt_mod.startswith('Alt') and not 'Alt' in modifiers:
        modifiers.append('Alt')
    modifiers.sort()
    modifiers = '.'.join(modifiers)
    if modifiers:
      modifiers += '.'
    for i in range(32 * 8):
      if bit(current_buf, i) and not bit(prev_buf, i):
        # KeyDown event
        key_name = XKeysymToString(x11.XKeycodeToKeysym(display, i, shift))
        if key_name.startswith('Alt'):
          alt_mods.add(key_name)
        key_name = modifiers + key_name
        histinc(histogram, key_name)
        if prev_key:
          histinc(digraphs, (prev_key, key_name))
        prev_key = key_name
        captured += 1
        if captured == FLUSH_KEYS:
          flush(histogram, digraphs, logfile)
          captured = 0
      elif bit(prev_buf, i) and not bit(current_buf, i):
        # KeyUp event
        key_name = XKeysymToString(x11.XKeycodeToKeysym(display, i, shift))
        if key_name.startswith('Alt'):
          try:
            alt_mods.remove(key_name)
          except:
            pass

if __name__ == '__main__':
  if len(sys.argv) < 2:
    view_keys()
  else:
    keyheatmap(sys.argv[1])
