import sys
import socket
import csv
import time

ork_labels = [
  'Time', # (s)
  'Altitude', # (m)
  'Vertical velocity', # (m/s)
  'Vertical acceleration', # (m/s^2)
  'Total velocity', # (m/s)
  'Total acceleration', # (m/s^2)
  'Position East of launch', # (m)
  'Position North of launch', # (m)
  'Lateral distance', # (m)
  'Lateral direction', # (deg)
  'Lateral velocity', # (m/s)
  'Lateral acceleration', # (m/s^2)
  'Latitude', # (deg)
  'Longitude', # (deg)
  'Gravitational acceleration', # (m/s^2)
  'Angle of attack', # (deg)
  'Roll rate', # (deg/s)
  'Pitch rate', # (deg/s)
  'Yaw rate', # (deg/s)
  'Mass', # (g)
  'Propellant mass', # (g)
  'Longitudinal moment of inertia', # (kg*m^2)
  'Rotational moment of inertia', # (kg*m^2)
  'CP location', # (cm)
  'CG location', # (cm)
  'Stability margin calibers', # (?)
  'Mach number', # (?)
  'Reynolds number', # (?)
  'Thrust', # (N)
  'Drag force', # (N)
  'Drag coefficient', # (?)
  'Axial drag coefficient', # (?)
  'Friction drag coefficient', # (?)
  'Pressure drag coefficient', # (?)
  'Base drag coefficient', # (?)
  'Normal force coefficient', # (?)
  'Pitch moment coefficient', # (?)
  'Yaw moment coefficient', # (?)
  'Side force coefficient', # (?)
  'Roll moment coefficient', # (?)
  'Roll forcing coefficient', # (?)
  'Roll damping coefficient', # (?)
  'Pitch damping coefficient', # (?)
  'Coriolis acceleration', # (m/s^2)
  'Reference length', # (cm)
  'Reference area', # (cm^2)
  'Vertical orientation (zenith)', # (deg)
  'Lateral orientation (azimuth)', # (deg)
  'Wind velocity', # (m/s)
  'Air temperature', # (deg C)
  'Air pressure', # (mbar)
  'Speed of sound', # (m/s)
  'Simulation time step', # (s)
  'Computation time' # (s)
]

# Format data string so server can more easily parse it (accepts list)
def formatData(list_data):
  data = ''
  for i, value in enumerate(list_data):
    data+= ork_labels[i]+':'+value+';'
  return data+'@@'

# Format only the selected data values (accepts dictionary)
def formatSelectData(dict_data, selection):
  data = ''
  for value in selection:
    data+= value+':'+dict_data[value]+';'
  return data+'@@'

def main():
  # Check usage
  if len(sys.argv) != 3:
    print("Usage: python3 %s <filename.csv> <port>" % sys.argv[0])
    quit()
  file = open(sys.argv[1])

  # IP address & port number to send data on
  HOST = '127.0.0.1'
  PORT = int(sys.argv[2])

  # Create server socket and bind it to the port
  server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
  try:
      server.bind((HOST, PORT))
  except socket.error as err:
      print(str(err))
      quit(1)

  # Listen for client connection
  server.listen()
  print("Waiting for client to connect on Port", PORT, "...")

  # Accept client connection
  connection, client_address = server.accept()
  print(f"Connected to {client_address[0]}:{client_address[1]}")

  ignore_comments = lambda row: row[0]!='#'
  data = csv.DictReader(filter(ignore_comments, file), fieldnames=ork_labels)

  launch_time = time.time()
  for row in data:
      # Delay sending packet until specified simulation time
      while time.time() - launch_time < float(row['Time']):
        continue

      # Send data to GUI
      try:
        data = formatSelectData(row, ['Time','Simulation time step','Computation time'])
        connection.send(data.encode('utf-8'))
      except socket.error as err:
        # Failed to send
        print(f"Lost connection...attempting to reconnect...")
        connection.close()

        # Wait client to reconnect
        connection, client_address = server.accept()
        print(f"Connected to {client_address[0]}:{client_address[1]}")

  connection.send('END@@'.encode('utf-8')) # Let client know there's no more data
  file.close()

if __name__=="__main__": 
  main()
