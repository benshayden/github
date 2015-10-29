import json
import webapp2
from google.appengine.ext import ndb
from google.appengine.api import memcache

class Interaction(ndb.Model):
  interactionType = ndb.StringProperty(indexed=False)
  delayMs = ndb.IntegerProperty()
  slowness = ndb.IntegerProperty()
  
  @classmethod
  def initialize(cls):
    for interactionType in ['click', 'scroll', 'load']:
      for delayBucket in xrange(dict(click=500, scroll=100, load=1000)[interactionType):
        delayMs = (delayBucket * 10) + 5
        for slownessBucket in xrange(10):
          slowness = (slownessBucket * 10) + 5
          Interaction(interactionType=interactionType, delayMs=delayMs, slowness=slowness).put()
  
  @classmethod
  def histograms(cls):
    # returns [(interactionType, sampleCount, sorted[
    #             (delayMs/10, sampleCount, sorted[
    #               (slowness/10, count)])]]
    # but first build {'click': [sampleCount, {delayMs/10: [sampleCount, {slowness/10: count}]}]} 
    histograms = {}
    interactions = cls.query()
    for interaction in interactions:
      if interaction.delayMs is None or interaction.slowness is None or interaction.interactionType is None:
        continue
      interactionHistogram = histograms.setdefault(interaction.interactionType, [0, {}])
      interactionHistogram[0] += 1
      slownessHistogram = interactionHistogram[1].setdefault(interaction.delayMs // 10, [0, {}])
      slownessHistogram[0] += 1
      slownessHistogram = slownessHistogram[1]
      bucket = interaction.slowness // 10
      if bucket in slownessHistogram:
        slownessHistogram[bucket] += 1
      else:
        slownessHistogram[bucket] = 1

    for interactionType in histograms:
      interactionHistogram = histograms[interactionType]
      for delayBucket in interactionHistogram[1]:
        slownessHistogram = interactionHistogram[1][delayBucket]
        slownessHistogram[1] = sorted(slownessHistogram[1].items())
      interactionHistogram[1] = sorted([delayBucket, sampleCount, slownessHistogram]
                                       for (delayBucket, (sampleCount, slownessHistogram))
                                       in interactionHistogram[1].iteritems())
    return [(interactionType, sampleCount, histogram)
            for (interactionType, (sampleCount, histogram))
            in histograms.iteritems()]

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

app = webapp2.WSGIApplication([
  ('/record', Record),
  ('/graph', Graph),
], debug=True)
