import json
import webapp2
import math
from google.appengine.ext import ndb
from google.appengine.api import memcache

MAX_DELAY_MS = dict(click=2e3, scroll=1e3, load=5e3)

def safeMean(lst, default):
  count = len(lst)
  if count:
      mean = sum(lst) / count
  else:
      mean = default
  return mean

def clamp(x, lo, hi):
  return min(hi, max(lo, x))

class Interaction(ndb.Model):
  interactionType = ndb.StringProperty(indexed=False)
  delayMs = ndb.IntegerProperty()
  comfort = ndb.IntegerProperty()
  
  def csv(self):
    return '%s,%d,%d\n' % (self.interactionType, self.delayMs, self.comfort)

  @classmethod
  def clear(cls):
    for interaction in cls.query():
      interaction.key.delete()

  @classmethod
  def initialize(cls):
    for interactionType, maxDelayMs in MAX_DELAY_MS.iteritems():
      for delayMs in xrange(0, int(maxDelayMs) + 1, int(maxDelayMs / 5)):
        comfort = int(100 * (1 - (float(delayMs) / maxDelayMs)))
        for _ in xrange(5):
          cls(interactionType=interactionType,
              delayMs=delayMs,
              comfort=comfort).put()

  @classmethod
  def histograms(cls):
    # returns [sorted[interactionType, [sorted[delayMs, meancomfort]]]]
    # but first build {interactionType: {delayMs: [comfort]}}
    histograms = {}
    sampleCounts = {}
    for interactionType in MAX_DELAY_MS:
      histograms[interactionType] = {}
      sampleCounts[interactionType] = 0

    for interaction in cls.query():
      if interaction.interactionType not in MAX_DELAY_MS:
        continue
      sampleCounts[interaction.interactionType] += 1
      histogram = histograms[interaction.interactionType]
      comforts = histogram.setdefault(interaction.delayMs, [])
      comforts.append(interaction.comfort)

    for interactionType, histogram in histograms.items():
      sampleCount = sampleCounts[interactionType]
      if sampleCount < 9:
        del histograms[interactionType]
        continue
      maxDelayMs = MAX_DELAY_MS[interactionType]
      bucketSize = max(3, maxDelayMs / math.sqrt(sampleCount))
      delayBucket = 0
      delayHistogram = []
      while delayBucket <= maxDelayMs:
        prevDelayBucket = delayBucket
        delayBucket += bucketSize
        comforts = []
        for delayMs in xrange(int(prevDelayBucket), int(delayBucket)):
          if delayMs in histogram:
            comforts.extend(histogram[delayMs])
        delayHistogram.append([int(prevDelayBucket), safeMean(comforts, 50)])
      histograms[interactionType] = delayHistogram
    return sorted(histograms.items())

class Record(webapp2.RequestHandler):
  def post(self):
    interaction = Interaction(
      interactionType=self.request.get('interactionType'),
      delayMs=int(self.request.get('delayMs')),
      comfort=int(self.request.get('comfort')))
    interaction.put()

class Graph(webapp2.RequestHandler):
  def get(self):
    memcacheKey = 'histograms'
    histograms = memcache.get(memcacheKey)
    if histograms is None:
      histograms = json.dumps(Interaction.histograms())
      memcache.add(memcacheKey, histograms, time=60)
    self.response.out.write(histograms)

class Initialize(webapp2.RequestHandler):
  def get(self):
    Interaction.clear()
    Interaction.initialize()
    
class DBCSV(webapp2.RequestHandler):
  def get(self):
    count = 0
    bufr = ''
    for interaction in Interaction.query():
      count += 1
      bufr += interaction.csv()
      if count == 1000:
        self.response.out.write(bufr)
        bufr = ''
        count = 0
    if bufr:
      self.response.out.write(bufr)

app = webapp2.WSGIApplication([
  ('/initialize', Initialize),
  ('/record', Record),
  ('/graph', Graph),
  ('/db.csv', DBCSV),
], debug=True)
