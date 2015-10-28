import webapp2
from google.appengine.ext import ndb

class Interaction(ndb.Model):
  flavor = ndb.StringProperty(indexed=False)
  delayMs = ndb.IntegerProperty()
  wording = ndb.StringProperty(indexed=False)
  wasBad = ndb.BooleanProperty()

class Record(webapp2.RequestHandler):
  def post(self):
    interaction = Interaction(
      flavor=self.request.get('flavor'),
      delayMs=self.request.get('delayMs'),
      wording=self.request.get('wording'),
      wasBad=self.request.get('wasBad'))
    interaction.put()

class Graph(webapp2.RequestHandler):
  def get(self):
    histogram = {}
    wording = self.request.get('wording')
    flavor = self.request.get('flavor')
    interactions = Interaction.query()
    if wording:
      interactions = interactions.filter(Interaction.wording == wording)
    if flavor:
      interactions = interactions.filter(Interaction.flavor == flavor)
    for interaction in interactions:
      bucket = histogram.setdefault(interaction.delayMs // 10, [0.0, 0.0])
      bucket[interaction.wasBad] += 1
    histogram = histogram.items()
    return repr(histogram.items())

app = webapp2.WSGIApplication([
  ('/record', Record),
  ('/graph', Graph),
], debug=True)
