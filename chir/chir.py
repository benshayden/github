import webapp2
from google.appengine.ext import ndb
from google.appengine.api import memcache

class Interaction(ndb.Model):
  flavor = ndb.StringProperty(indexed=False)
  delayMs = ndb.IntegerProperty()
  wording = ndb.StringProperty(indexed=False)
  wasBad = ndb.BooleanProperty()
  
  @classmethod
  def histogram(cls, wording, flavor):
    histogram = {}
    interactions = cls.query()
    if wording:
      interactions = interactions.filter(cls.wording == wording)
    if flavor:
      interactions = interactions.filter(cls.flavor == flavor)
    for interaction in interactions:
      bucket = histogram.setdefault(interaction.delayMs // 10, [0, 0])
      bucket[interaction.wasBad] += 1
    return histogram

class Record(webapp2.RequestHandler):
  def post(self):
    interaction = Interaction(
      flavor=self.request.get('flavor'),
      delayMs=int(self.request.get('delayMs')),
      wording=self.request.get('wording'),
      wasBad=(self.request.get('wasBad') == 'true'))
    interaction.put()

class Graph(webapp2.RequestHandler):
  def get(self):
    wording = self.request.get('wording')
    flavor = self.request.get('flavor')
    
    memcacheKey = repr((wording, flavor))
    histogram = memcache.get(memcacheKey)
    if histogram is None:
      histogram = repr(Interaction.histogram(wording, flavor.items())
      memcache.add(memcacheKey, histogram, time=3600)
    self.response.out.write(histogram)

app = webapp2.WSGIApplication([
  ('/record', Record),
  ('/graph', Graph),
], debug=True)
