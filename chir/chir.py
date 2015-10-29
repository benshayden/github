import json
import webapp2
from google.appengine.ext import ndb
from google.appengine.api import memcache

class Interaction(ndb.Model):
  interactionType = ndb.StringProperty(indexed=False)
  delayMs = ndb.IntegerProperty()
  slowness = ndb.IntegerProperty()
  
  @classmethod
  def histograms(cls):
    # build {'click': [total, {delayMs/10: [total, {slowness/10: count}]}]} 
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
    # return {'click': [total, [/*sorted*/[delayMs/10, [total, [/*sorted*/[slowness/10, count]]]]]]}
    for interactionType in histograms:
      interactionHistogram = histograms[interactionType]
      for delayBucket in interactionHistogram[1]:
        slownessHistogram = interactionHistogram[1][delayBucket]
        slownessHistogram[1] = sorted(slownessHistogram[1].items())
      interactionHistogram[1] = sorted(interactionHistogram[1].items())
    return histograms

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
      memcache.add(memcacheKey, histograms, time=10)
    self.response.out.write(histograms)

app = webapp2.WSGIApplication([
  ('/record', Record),
  ('/graph', Graph),
], debug=True)
