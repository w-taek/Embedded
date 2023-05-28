import time
import board  # 데이터 송신용 board모듈
import adafruit_dht

mydht11 = adafruit_dht.DHT11(board.D20)  # D20 = gpio 핀 번호

while True:
    try:
        humidity_data = mydht11.humidity
        temperature_data = mydht11.temperature
        print(humidity_data,temperature_data)
        time.sleep(2)  # 대기시간 2초 필요 - 센서 내부에서 초기화 작업 시 필요한 시간
    except RuntimeError as error:
        print(error.args[0])
    finally:
        pass