# Env-specific configuration data

# Lamp Control IPs (D1 minis)
# 'all' is also valid, and is handled in logic
nondimmable_endpoints = {"floor":   "http://192.168.1.10/floor",
                         "corner":  "http://192.168.1.11/"}

# LED brightness (PWM) upstream IPs
dimmable_endpoints = {"window": "http://192.168.1.10/window",
                      "door":   "http://192.168.1.13/"}

# Endpoints attached to switches
switch_endpoints = {"wall": "http://192.168.1.10/switch"}

# HTU21D (temp/humidity) upstream IPs
htu_endpoints = {"window": "http://192.168.1.10/temp"}
