#!/usr/bin/env python2.6
import sys
import os
import ctypes

x11 = ctypes.CDLL('libX11.so')

class XModifierKeymap(ctypes.Structure):
  __slots__ = ['max_keypermod', 'modifiermap']
  _fields_ = [('max_keypermod', ctypes.c_int),
              ('modifiermap', ctypes.POINTER(ctypes.c_ubyte))]

x11.XGetModifierMapping.restype = ctypes.POINTER(XModifierKeymap)
x11.XKeysymToString.restype = ctypes.c_char_p

MODIFIERS = ['Shift', 'Lock', 'Control', 'Alt', 'Mod2', 'Mod3', 'Super', 'Mod5']

def bit(buf, i):
  return ord(buf[i/8]) & (1 << (i % 8)) != 0

def genkeys(delay_us=1000):
  buf_a = ctypes.create_string_buffer(32)
  buf_b = ctypes.create_string_buffer(32)
  current_buf, prev_buf = buf_a, buf_b
  display = x11.XOpenDisplay(os.environ.get('DISPLAY', ':0'))
  if not display:
    return
  x11.XSynchronize(display, True)
  mod_map = x11.XGetModifierMapping(display)[0]
  alt_mods = set()
  x11.XQueryKeymap(display, current_buf)
  while True:
    x11.usleep(delay_us)
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
        key_name = x11.XKeysymToString(x11.XKeycodeToKeysym(display, i, shift))
        if key_name.startswith('Alt'):
          alt_mods.add(key_name)
        key_name = modifiers + key_name
        yield key_name
      elif bit(prev_buf, i) and not bit(current_buf, i):
        # KeyUp event
        key_name = x11.XKeysymToString(x11.XKeycodeToKeysym(display, i, shift))
        if key_name.startswith('Alt'):
          try:
            alt_mods.remove(key_name)
          except:
            pass

def histinc(histogram, keyname):
  try: histogram[keyname] += 1
  except KeyError: histogram[keyname] = 1

def keyheatmap(logfilename, flush_keys=1000):
  logfilename = os.path.abspath(os.path.expanduser(logfilename))
  histogram = {}
  digraphs = {}
  if os.path.exists(logfilename):
    for line in file(logfilename):
      if line and ' ' in line:
        keyname, count = line.split()
        if ',' in keyname:
          digraphs[tuple(keyname.split(','))] = int(count)
        else:
          histogram[keyname] = int(count)
  else:
    file(logfilename, 'w').write('')
  os.chmod(logfilename, 0600)
  prev_key = None
  captured = 0
  for key_name in genkeys():
    histinc(histogram, key_name)
    if prev_key:
      histinc(digraphs, (prev_key, key_name))
    prev_key = key_name
    captured += 1
    if captured >= flush_keys:
      counts = histogram.items()
      counts.sort(key=lambda (key, value): -value)
      dicounts = digraphs.items()
      dicounts.sort(key=lambda (key, value): -value)
      logfile = file(logfilename, 'w')
      for key, value in counts:
        logfile.write('%s %d\n' % (key, value))
      for key, value in dicounts:
        logfile.write('%s,%s %d\n' % (key[0], key[1], value))
      logfile.flush()
      logfile.close()
      del logfile
      captured = 0

if __name__ == '__main__':
  try:
    if len(sys.argv) < 2:
        for k in genkeys():
          print k
    else:
      keyheatmap(sys.argv[1])
  except KeyboardInterrupt:
    pass
