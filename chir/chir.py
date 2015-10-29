import json
import webapp2
from google.appengine.ext import ndb
from google.appengine.api import memcache

class Interaction(ndb.Model):
  interactionType = ndb.StringProperty(indexed=False)
  delayMs = ndb.IntegerProperty()
  slowness = ndb.IntegerProperty()
  
  @classmethod
  def histogram(cls):
    histogram = {}
    interactions = cls.query()
    for interaction in interactions:
      if interaction.delayMs is None or interaction.slowness is None or interaction.interactionType is None:
        continue
      subhist = histogram.setdefault(interaction.interactionType, {})
      subhist = subhist.setdefault(interaction.delayMs // 10, {})
      bucket = interaction.slowness // 10
      if bucket in subhist:
        subhist[bucket] += 1
      else:
        subhist[bucket] = 1
    return histogram

class Record(webapp2.RequestHandler):
  def post(self):
    interaction = Interaction(
      interactionType=self.request.get('interactionType'),
      delayMs=int(self.request.get('delayMs')),
      slowness=int(self.request.get('slowness')))
    interaction.put()

class Graph(webapp2.RequestHandler):
  def get(self):
    memcacheKey = 'histogram'
    histogram = memcache.get(memcacheKey)
    if histogram is None:
      histogram = json.dumps(Interaction.histogram())
      memcache.add(memcacheKey, histogram, time=10)
    self.response.out.write(histogram)

app = webapp2.WSGIApplication([
  ('/record', Record),
  ('/graph', Graph),
], debug=True)
