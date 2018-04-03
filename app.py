#!/usr/bin/env python

from flask import Flask, jsonify, make_response, request
from flaskrun import flaskrun
from HTU21D.HTU21D import HTU21D

app = Flask(__name__)

@app.route('/')
def index():
    return '{"info": "REST API for sensors on this Orange Pi Zero"}'

@app.route('/temp')
def temp():
    units = request.args.get('units', 'C')
    htu = HTU21D(0)
    data = {"temperature_units": units,
            "temperature": htu.read_temperature(),
            "humidity": htu.read_humidity() }
    if units == 'F':
        data['temperature'] = (data['temperature'] * 9./5.) + 32
    elif units == ['K']:
        data['temperature'] += 273.15
    return jsonify(data)

@app.errorhandler(404)
def not_found(error):
    return make_response(jsonify({"error": 'Not found'}), 404)


if __name__ == '__main__':
    flaskrun(app)
