#!/usr/bin/env coffee

_      = require 'underscore'
fs     = require 'fs'
path   = require 'path'
should = require 'should'
sqlite = require '../'

describe 'SpatiaLite', ->
  db = null
  polygon = "GeomFromText('POLYGON ((30 10, 10 20, 20 40, 40 40, 30 10))')"
  invalid_polygon = "GeomFromText('POLYGON((0 0, 0 10, 11 10, 10 10, 10 1, 5 1, 5 9, 5 1, 0 1, 0 0))')"

  beforeEach (done) ->
    db = new sqlite.Database(':memory:')
    db.spatialite done

  it 'should compute the centroid of a polygon', (done) ->
    query = "SELECT AsText(Centroid(#{polygon})) AS result"
    db.get query, (err, row) ->
      throw err if err
      row.result.should.equal 'POINT(25.454545 26.969697)'
      done()

  it 'should test for valid geometries', (done) ->
    query = "SELECT ST_IsValid(#{polygon}) AS result"
    db.get query, (err, row) ->
      throw err if err
      row.result.should.equal 1
      done()

  it 'should support making valid geometries from invalid ones', (done) ->
    query = "SELECT AsText(ST_MakeValid(#{invalid_polygon})) AS result"
    db.get query, (err, row) ->
      throw err if err
      row.result.should.equal 'POLYGON((0 1, 0 10, 10 10, 10 1, 5 1, 0 1))'
      done()
