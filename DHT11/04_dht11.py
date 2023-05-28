#!/usr/bin/env python3
import time
import board
import adafruit_dht

mydht11 = adafruit_dht.DHT11(board.D20)

while True:
    try:
        humidity_data = mydht11.humidity
        temperature_data = mydht11.temperature
        print(humidity_data,temperature_data)
        time.sleep(2)  
    except RuntimeError as error:
        print(error.args[0])
    finally:
        pass
