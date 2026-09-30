# -*- coding: utf-8 -*-
from PyQt5 import QtCore,QtGui,QtWidgets
from PyQt5.QtWidgets import  QMainWindow, QApplication, QFileDialog

from OvalSimGui import Ui_MainWindow

from time import sleep
import paho.mqtt.client as mqtt
import sys, getopt
import os
import time
import random
from datetime import timezone
from datetime import datetime,timedelta
import json
import threading



#MQTT ports
COM_PORT    = '/dev/ttyUSB0'
DATA_PATH   = "data/"

mqtt_host 	= "mqtt.q4t.cl"
mqtt_usser	= "tds"
mqtt_password = "m0squ1t0tds"
mqtt_port = 1884

mqtt_topic = "id.up"

debug = True

Oval_register_db = [	{"name_id":"Oval01", "mac":"94:E6:86:2E:04:0C"},
						{"name_id":"Oval02", "mac":"94:E6:86:2E:03:E0"},
						{"name_id":"Oval03", "mac":"94:E6:86:2E:03:F0"},
						{"name_id":"Oval04", "mac":"94:E6:86:2E:04:08"},
						{"name_id":"Oval13", "mac":"70:B8:F6:62:5E:A0"},
						{"name_id":"Oval14", "mac":"70:B8:F6:62:99:18"},
						{"name_id":"Oval15", "mac":"70:B8:F6:62:8F:78"},
						{"name_id":"Oval16", "mac":"70:B8:F6:62:C5:94"},
						{"name_id":"Oval17", "mac":"70:B8:F6:62:70:C0"},
						{"name_id":"Oval18", "mac":"70:B8:F6:62:8F:1C"},
						{"name_id":"Oval19", "mac":"70:B8:F6:62:3F:28"},
						{"name_id":"Oval20", "mac":"70:B8:F6:62:7F:A0"},
						{"name_id":"Oval21", "mac":"70:B8:F6:63:44:0C"},
						{"name_id":"Oval22", "mac":"70:B8:F6:62:C9:04"},
						{"name_id":"Oval23", "mac":"70:B8:F6:63:A7:8C"},
						{"name_id":"Oval24", "mac":"70:B8:F6:62:4D:E4"},
						{"name_id":"Oval25", "mac":"70:B8:F6:63:33:C8"}
]


# Función de conexión
def on_connect(client, userdata, flags, rc):
    if rc == 0:
        print("Conexión exitosa al broker MQTT")
    else:
        print("Error al conectar, código de error:", rc)



class MainWindow(QtWidgets.QMainWindow):
	def __init__(self):
		super(MainWindow,self).__init__()
		self.ui = Ui_MainWindow()
		self.ui.setupUi(self)
		self.setWindowTitle("Oval simulator")


		self.id = ""
		self.mac = ""
		self.time = 0
		self.type = "datos"
		self.lux = 0
		self.p = 0.0
		self.t = 0.0
		self.h = 0.0
		self.p01 = 0.0
		self.p10 = 0.0
		self.p25 = 0.0
		self.co2 = 0.0
		self.tvoc = 0.0
		self.noise_max = 0
		self.noise_mean = 0
		self.lat = 0.0
		self.lon = 0.0
		self.status = 0
		self.err = "OK"

		self.automatic_time = False
		self.elapsed_time = 0

		self.file_name      = ""
		self.path           = DATA_PATH
		self.data_path 		= "./data/"
		self.data_file      = None
		self.data_writer    = None
		self.label_time     = ""

		"""-----------------------
   			INITIAL STATE
		----------------------"""

		#ENABLES CHANNELS
		self.eneableNewOval(False)
		#load de OVAL MAC data
		self.ui.comboBox.addItems(list(n['name_id'] for n in Oval_register_db))
		self.ui.plainTextEdit.isReadOnly()
		#self.ui.pushButton_6.setDisabled(False)
		#self.ui.pushButton_9.setDisabled(True)

		self.updateJson()

		"""--------------------------
    		General behaivour
		-----------------------------"""
		self.ui.pushButton.clicked.connect(self.updateID)
		self.ui.pushButton_9.clicked.connect(lambda:self.eneableNewOval(True))
		self.ui.pushButton_6.clicked.connect(lambda:self.eneableNewOval(False))
		self.ui.pushButton_4.clicked.connect(self.setID)
		self.ui.pushButton_3.clicked.connect(self.setMAC)
		self.ui.pushButton_5.clicked.connect(self.randomizeMAC)
		self.ui.pushButton_7.clicked.connect(self.setTime)
		self.ui.pushButton_8.clicked.connect(self.setPMS)
		self.ui.pushButton_10.clicked.connect(self.setBME)
		self.ui.pushButton_12.clicked.connect(self.randomizeAll)
		self.ui.pushButton_11.clicked.connect(self.setAll)
		self.ui.checkBox.stateChanged.connect(self.autoTime)

		self.ui.pushButton_2.clicked.connect(self.sendData)
		#RUN TEST


	def updateID(self):
		_id = self.ui.comboBox.currentText()
		for e in Oval_register_db:
			if _id == e["name_id"]:
				self.id = e["name_id"]
				self.mac = e["mac"]
		self.updateJson()

	def setID(self):
		self.id = self.ui.lineEdit.text()
		self.updateJson()

	def setMAC(self):
		self.mac = self.ui.lineEdit_6.text()
		self.updateJson()

	def autoTime(self):
		if self.ui.checkBox.isChecked():
			self.ui.lineEdit_2.setDisabled(True)
			self.ui.pushButton_7.setDisabled(True)
			self.automatic_time = True
		else:
			self.ui.lineEdit_2.setDisabled(False)
			self.ui.pushButton_7.setDisabled(False)
			self.automatic_time = False

	def setTime(self):
		self.time = self.ui.lineEdit_2.text()
		self.updateJson()

	def setPMS(self):
		self.pm01 = self.ui.lineEdit_3.text()
		self.pm10 = self.ui.lineEdit_4.text()
		self.pm25 = self.ui.lineEdit_5.text()
		self.updateJson()

	def setBME(self):
		self.t = self.ui.lineEdit_10.text()
		self.h = self.ui.lineEdit_9.text()
		self.p = self.ui.lineEdit_11.text()
		self.updateJson()

	def sendData(self):
		_msg = json.dumps(self.json_msg)
		try:
			client.publish(mqtt_topic, _msg)
			print(f"Mensaje enviado: {_msg}")
		except Exception as e:
			print(e)


	def randomizeMAC(self):
		# Genera seis valores hexadecimales de 2 dígitos cada uno
		_mac = [random.randint(0x00, 0xFF) for _ in range(6)]
    	# Formatea en estilo de dirección MAC, separada por ":"
		self.mac =  ":".join(f"{octeto:02X}" for octeto in _mac)
		self.ui.lineEdit_6.setText(self.mac)
		self.updateJson()

	def randomizeAll(self):
		self.lux = random.randint(0,10000)/100.0
		self.p = random.randint(0,10000)/100.0
		self.t = float(random.randint(0,4000)/100.0)
		self.h = random.randint(0,10000)/100.0
		self.p01 = random.randint(0,5000)/100.0
		self.p10 = random.randint(0,5000)/100.0
		self.p25 = random.randint(0,5000)/100.0
		self.co2 = random.randint(40000,80000)/100.0
		self.tvoc = random.randint(0,10000)/100.0
		self.noise_max = random.randint(6000,8000)/100.0
		self.noise_mean = random.randint(6000,8000)/100.0
		self.lat = -33.0 - random.randint(0,100)/100.0
		self.lon = - 77 -   random.randint(0,100)/100.0

		self.ui.lineEdit_3.setText(str(self.p01))
		self.ui.lineEdit_4.setText(str(self.p10))
		self.ui.lineEdit_5.setText(str(self.p25))

		self.ui.lineEdit_10.setText(str(self.t))
		self.ui.lineEdit_9.setText(str(self.h))
		self.ui.lineEdit_11.setText(str(self.p))

		self.updateJson()

	def setAll():
		pass

	def updateJson(self):
		if self.automatic_time :
			dt = datetime.now(timezone.utc)
			utc_time = dt.replace(tzinfo=timezone.utc)
			self.time = int(utc_time.timestamp())
		self.json_msg = {"id":self.id,
			"mac":self.mac,
			"time":self.time,
			"type":self.type,
			"lux":self.lux,
			"p":self.p,
			"t":self.t,
			"h":self.h,
			"p01":self.p01,
			"p10":self.p10,
			"p25":self.p25,
			"co2":self.co2,
			"tvoc":self.tvoc,
			"noise_max":self.noise_max,
			"noise_mean":self.noise_mean,
			"lat":self.lat,
			"lon":self.lon,
			"status":self.status,
			"err":self.err}
		self.ui.plainTextEdit.setPlainText(json.dumps(self.json_msg, indent = 4))
		#print(json.dumps(self.jsonMSG, indent = 4))

	def eneableNewOval(self, _en):
		if _en:
			self.ui.label_6.setDisabled(False)
			self.ui.label_7.setDisabled(False)
			self.ui.lineEdit.setDisabled(False)
			self.ui.lineEdit_6.setDisabled(False)
			self.ui.pushButton_3.setDisabled(False)
			self.ui.pushButton_4.setDisabled(False)
			self.ui.pushButton_5.setDisabled(False)
			self.ui.pushButton_6.setDisabled(False)
			self.ui.pushButton_9.setDisabled(True)

		else:
			self.ui.label_6.setDisabled(True)
			self.ui.label_7.setDisabled(True)
			self.ui.lineEdit.setDisabled(True)
			self.ui.lineEdit_6.setDisabled(True)
			self.ui.pushButton_3.setDisabled(True)
			self.ui.pushButton_4.setDisabled(True)
			self.ui.pushButton_5.setDisabled(True)
			self.ui.pushButton_6.setDisabled(True)
			self.ui.pushButton_9.setDisabled(False)

	def Close(self):
		try:
			self.run_timer.stop()
			self.controller.stopRcvr()
		except Exception as e:
			print("Controller not connected. Closing")


if __name__ == "__main__":

	# Crear instancia del cliente
	client = mqtt.Client()
	# Configurar usuario y contraseña
	client.username_pw_set(mqtt_usser, mqtt_password)

	# Asignar función de conexión
	client.on_connect = on_connect

	# Conectar al broker
	client.connect(mqtt_host, mqtt_port, 60)



	# Iniciar el loop del cliente
	#client.loop_start()

	app = QtWidgets.QApplication([])
	main = MainWindow()
	main.show()
	#main.showFullScreen()

	ret = app.exec_()
	main.Close()
	print("stoped")
	sys.exit(ret)
