import json
import webapp2
import math
from google.appengine.ext import ndb
from google.appengine.api import memcache

MAX_DELAY_MS = dict(click=500, scroll=100, load=1000)

class Interaction(ndb.Model):
  interactionType = ndb.StringProperty(indexed=False)
  delayMs = ndb.IntegerProperty()
  slowness = ndb.IntegerProperty()
  
  @classmethod
  def initialize(cls):
    for interaction in cls.query():
      interaction.delete()
    for interactionType, maxDelayMs in MAX_DELAY_MS.iteritems():
      for delayMs in xrange(0, maxDelayMs, maxDelayMs / 10):
        for slowness in xrange(0, 100, 10):
          cls(interactionType=interactionType,
              delayMs=delayMs,
              slowness=slowness).put()
  
  @classmethod
  def histograms(cls):
    # returns [sorted[interactionType, [sorted[delayMs, meanSlowness]]]]
    # but first build {interactionType: {delayMs: [slowness]}}
    histograms = {}
    sampleCounts = {}
    for interactionType in MAX_DELAY_MS:
      histograms[interactionType] = {}
      sampleCounts[interactionType] = 0
    for interaction in cls.query():
      if interaction.interactionType not in MAX_DELAY_MS:
        continue
      sampleCounts[interaction.interactionType] += 1
      histograms[interactionType].setdefault(interaction.delayMs, []).append(interaction.slowness)
    for interactionType, histogram in histograms.iteritems():
      maxDelayMs = MAX_DELAY_MS[interactionType]
      bucketSize = max(3, maxDelayMs / math.sqrt(sampleCounts[interactionType]))
      delayBucket = 0
      delayHistogram = []
      while delayBucket < maxDelayMs:
        prevDelayBucket = delayBucket
        delayBucket += bucketSize
        slownesses = []
        for delayMs in xrange(int(math.floor(prevDelayBucket)), int(math.floor(delayBucket + 1))):
          slownesses.extend(histogram[delayMs])
        delayHistogram.append([prevDelayBucket, sum(slownesses) / len(slownesses)])
      histograms[interactionType] = delayHistogram
    return sorted(histograms.items())

class Record(webapp2.RequestHandler):
  def post(self):
    interaction = Interaction(
      interactionType=self.request.get('interactionType'),
      delayMs=int(self.request.get('delayMs')),
      slowness=int(self.request.get('slowness')))
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
    Interaction.initialize()

app = webapp2.WSGIApplication([
  ('/initialize', Initialize),
  ('/record', Record),
  ('/graph', Graph),
], debug=True)
