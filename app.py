#!/usr/bin/env python
# -*- coding: utf-8 -*-

from flask import Flask, jsonify, make_response, request, send_from_directory
from flaskrun import flaskrun
from HTU21D.HTU21D import HTU21D
import requests
from urllib3.exceptions import MaxRetryError

# Mostly dicts of endpoint addresses
from config import *

app = Flask(__name__)
htu = HTU21D(0)

@app.route('/')
def index():
    return '{"info": "REST API for sensors on this Orange Pi Zero"}'

@app.route('/temp/<name>')
def temp(name):
    global htu

    units = request.args.get('units', 'C')

    if name not in htu_endpoints:
        response_data = {"error": "Unrecognized thermometer name: {:s}".format(name)}
        return make_response(jsonify(response_data), 400)

    if htu_endpoints[name] == "local":
        data = {"temperature_units": units,
                "temperature": htu.read_temperature(),
                "humidity": htu.read_humidity() }
        if units == 'F':
            data['temperature'] = (data['temperature'] * 9./5.) + 32
        elif units == ['K']:
            data['temperature'] += 273.15
        return jsonify(data)
    else: # remote endpoint
        endpoint = htu_endpoints[name]
        try:
            r = requests.get("".join((endpoint, "?unit=", units)))
        except Exception as err:
            if type(err.args[0]) is MaxRetryError: # Connection refused or timed out (??)
                response_data = {"error": "Endpoint device could not be reached: {:s}".format(err.args[0])}
                return make_response(jsonify(response_data), 504)
            else:
                response_data = {"error": "Error with endpoint device: {:s}".format(err.args[0])}
                return make_response(jsonify(response_data), 502)
        data = r.json()
        if r.status_code != 200:
            return make_response(jsonify(data), r.status_code)
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
            response_data = {"error": "Endpoint device could not be reached: {:s}}".format(err.args[0])}
            return make_response(jsonify(response_data), 504)
        else:
            response_data = {"error": "Error with endpoint device: {:s}}".format(err.args[0])}
            return make_response(jsonify(response_data), 502)
        
    upstream_data = r.json()
    response_data = upstream_data
    if r.status_code != 200:
        return make_response(jsonify(response_data), r.status_code)
    return jsonify(response_data)

@app.route('/switch/<name>', methods=['GET', 'PUT'])
def lightswitch(name):
    if name in switch_endpoints:
        endpoint = switch_endpoints[name]
    else:
        response_data = {"error": "Unrecognized switch name: {:s}".format(name)}
        return make_response(jsonify(response_data), 400)

    if request.method == 'GET':
        try:
            r = requests.get(endpoint)
        except Exception as err:
            if type(err.args[0]) is MaxRetryError: # Connection refused or timed out (??)
                response_data = {"error": "Endpoint device could not be reached: {:s}".format(err.args[0])}
                return make_response(jsonify(response_data), 504)
            else:
                response_data = {"error": "Error with endpoint device: {:s}".format(err.args[0])}
                return make_response(jsonify(response_data), 502)
        response_data = r.json()
        if r.status_code != 200:
            return make_response(jsonify(response_data), r.status_code)
        return jsonify(response_data)

    if request.method == 'PUT':
        try:
            request_data = request.get_json() # e.g.: {"state": "on"}
        except json.JSONDecodeError:
            return make_response(jsonify({"error": "Invalid JSON request data."}), 400)
        if "state" not in request_data: #TODO: de-magic-string this
            return make_response(jsonify({"error": "Request data must include \"state\" key."}), 400)
        # Turn on/off all lights tied to this switch (and there's the rub:)
        #     Which lights are tied to which switches?
        #     How do I define those relationships?
        #         Hard-coded? config.py?
        #         Define with REST calls every time the server restarts?


## Obsolete now that I'm deploying with nginx
"""
# Webform+AJAX hack to let me turn off the dimmable LEDs with my phone
# TODO: temporary, until I get nginx set up
@app.route('/ui/lights')
def leds_static():
    return send_from_directory('static', 'leds.html')

# Basic dynamic frontend for temperatures
# TODO: temporary, until I get nginx set up
@app.route('/ui/thermometer')
    return send_from_directory('static', 'temps.html')

# Slightly more friendly universal frontend
# TODO: temporary, until I get nginx set up
@app.route('/ui')
def frontend():
    return send_from_directory('static', 'frontend.html')
"""

@app.errorhandler(404)
def not_found(error):
    data = {"error":      "URI Not Found",
            "uri":        request.path,
            "method":     request.method,
            "parameters": request.args}
    return make_response(jsonify(data), 404)


if __name__ == '__main__':
    flaskrun(app, default_host="0.0.0.0", default_port="4040")
