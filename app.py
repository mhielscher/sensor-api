#!/usr/bin/env python
# -*- coding: utf-8 -*-

from flask import Flask, jsonify, make_response, request, send_from_directory
from flaskrun import flaskrun
#from HTU21D.HTU21D import HTU21D
import requests
from urllib3.exceptions import MaxRetryError

# Mostly dicts of endpoint addresses
from config import *

app = Flask(__name__)
htu = HTU21D(0)

@app.route('/')
def index():
    return '{"info": "REST API for sensors on this Orange Pi Zero"}'

@app.route('/temp')
def temp():
    global htu
    units = request.args.get('units', 'C')
    data = {"temperature_units": units,
            "temperature": htu.read_temperature(),
            "humidity": htu.read_humidity() }
    if units == 'F':
        data['temperature'] = (data['temperature'] * 9./5.) + 32
    elif units == ['K']:
        data['temperature'] += 273.15
    return jsonify(data)

@app.route('/lights/<name>', methods=['GET', 'PUT'])
def lights(name):
    if name in dimmable_endpoints:
        endpoint = dimmable_endpoints[name]
        light_type = "dimmable"
    elif name in nondimmable_endpoints:
        endpoint = nondimmable_endpoints[name]
        light_type = "nondimmable"
    else:
        response_data = {"error": "Unrecognized light name: {:s}".format(name)}
        return make_response(jsonify(response_data), 400)

    # Lights with dimmable control: LEDs with PWM control, etc.
    # Takes/returns data {"brightness": int<0, 1023>}

    if request.method == 'PUT':
        try:
            request_data = request.get_json() # e.g.: {"brightness": 512} or {"state": "on"}
        except json.JSONDecodeError:
            return make_response(jsonify({"error": "Invalid JSON request data."}), 400)
        if light_type == "dimmable":
            if "brightness" not in request_data: #TODO: de-magic-string this
                return make_response(jsonify({"error": "Request data must include \"brightness\" key."}), 400)
            upstream_data = {'brightness': request_data['brightness']}
        else:
            if "state" not in request_data: #TODO: de-magic-string this
                return make_response(jsonify({"error": "Request data must include \"state\" key."}), 400)
            upstream_data = {'state': request_data['state']}

    try:
        if request.method == 'PUT':
            r = requests.put(endpoint, data=upstream_data)
        else:
            r = requests.get(endpoint)
    except Exception as err:
        if type(err.args[0]) is MaxRetryError: # Connection refused or timed out (??)
            response_data = {"error": "Endpoint device could not be reached: {:s}".format(err.args[0])}
            return make_response(jsonify(response_data), 504)
        else:
            response_data = {"error": "Error with endpoint device: {:s}".format(err.args[0])}
            return make_response(jsonify(response_data), 502)
        
    upstream_data = r.json()
    response_data = upstream_data
    if r.status_code != 200:
        return make_response(jsonify(response_data), r.status_code)
    return jsonify(response_data)


# Dumb hack to let me turn off the dimmable LEDs with my phone
@app.route('/static/leds')
def leds_static():
    return send_from_directory('static', 'leds.html')

@app.errorhandler(404)
def not_found(error):
    data = {"error":      "URI Not Found",
            "uri":        request.path,
            "method":     request.method,
            "parameters": request.args}
    return make_response(jsonify(data), 404)


if __name__ == '__main__':
    flaskrun(app)
