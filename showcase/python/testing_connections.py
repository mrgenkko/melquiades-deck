import bluetooth
import time
import sys
import select
import threading
import queue
import serial
import serial.tools.list_ports

class ESP32Client:
    def __init__(self):
        # Atributos comunes
        self.connected = False
        self.streaming = False
        self.stream_thread = None
        self.data_queue = queue.Queue()
        self.exit_event = threading.Event()
        self.connection_type = None
        
        # Bluetooth
        self.bt_socket = None
        self.bt_device_name = None
        self.target_address = None
        
        # Serial
        self.serial_port = None
        self.serial_connection = None
        
    def list_serial_ports(self):
        """Lista todos los puertos seriales disponibles"""
        ports = serial.tools.list_ports.comports()
        print("\nPuertos seriales disponibles:")
        for port in ports:
            print(f"  {port.device} - {port.description}")
        return ports
        
    def connect_bluetooth(self, device_name):
        """Conecta al dispositivo vía Bluetooth"""
        self.bt_device_name = device_name
        self.connection_type = "bluetooth"
        
        print(f"Buscando dispositivo Bluetooth '{self.bt_device_name}'...")
        nearby_devices = bluetooth.discover_devices(duration=8, lookup_names=True)
        
        if not nearby_devices:
            print("No se encontraron dispositivos Bluetooth.")
            return False
            
        print(f"Dispositivos encontrados: {len(nearby_devices)}")
        for addr, name in nearby_devices:
            print(f"  {addr} - {name}")
            if name == self.bt_device_name:
                self.target_address = addr
                print(f"¡Dispositivo encontrado! Dirección: {addr}")
                
                try:
                    # SPP UUID estándar
                    port = 1  # SPP normalmente usa el puerto 1
                    self.bt_socket = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
                    self.bt_socket.connect((self.target_address, port))
                    self.connected = True
                    print(f"Conectado a {self.bt_device_name} vía Bluetooth")
                    
                    # Recibir mensajes de bienvenida
                    self._receive_initial_messages()
                    return True
                except Exception as e:
                    print(f"Error al conectar: {e}")
                    return False
                    
        print(f"No se encontró el dispositivo '{self.bt_device_name}'")
        return False
    
    def connect_serial(self, port_name, baudrate=115200):
        """Conecta al dispositivo vía puerto serial"""
        self.connection_type = "serial"
        
        try:
            self.serial_connection = serial.Serial(port_name, baudrate, timeout=1)
            print(f"Conectado a puerto serial {port_name} a {baudrate} baudios")
            self.connected = True
            
            # Recibir mensajes de bienvenida
            self._receive_initial_messages()
            return True
        except Exception as e:
            print(f"Error al conectar al puerto serial {port_name}: {e}")
            return False
    
    def _receive_initial_messages(self):
        """Recibe y muestra los mensajes iniciales del ESP32"""
        time.sleep(1)  # Dar tiempo para recibir mensajes iniciales
        try:
            if self.connection_type == "bluetooth" and self.bt_socket.fileno() != -1:
                ready = select.select([self.bt_socket], [], [], 2.0)
                if ready[0]:
                    data = self.bt_socket.recv(1024).decode('utf-8', errors='replace')
                    print("\nMensaje del ESP32:")
                    print(data)
            elif self.connection_type == "serial" and self.serial_connection and self.serial_connection.is_open:
                if self.serial_connection.in_waiting > 0:
                    data = self.serial_connection.read(self.serial_connection.in_waiting).decode('utf-8', errors='replace')
                    print("\nMensaje del ESP32:")
                    print(data)
        except Exception as e:
            print(f"Error al recibir mensajes iniciales: {e}")
    
    def _stream_listener(self):
        """Función que corre en un hilo separado para recibir datos del stream"""
        print("Iniciando stream de datos...")
        self.streaming = True
        
        while self.connected and not self.exit_event.is_set():
            try:
                if self.connection_type == "bluetooth" and self.bt_socket.fileno() != -1:
                    ready = select.select([self.bt_socket], [], [], 0.5)
                    if ready[0]:
                        data = self.bt_socket.recv(1024).decode('utf-8', errors='replace')
                        if data:
                            self.data_queue.put(data)
                            print(f"\n[STREAM] {data}")
                            print("\nIngrese comando > ", end="", flush=True)
                            
                elif self.connection_type == "serial" and self.serial_connection and self.serial_connection.is_open:
                    if self.serial_connection.in_waiting > 0:
                        data = self.serial_connection.read(self.serial_connection.in_waiting).decode('utf-8', errors='replace')
                        if data:
                            self.data_queue.put(data)
                            print(f"\n[STREAM] {data}")
                            print("\nIngrese comando > ", end="", flush=True)
                    else:
                        time.sleep(0.1)  # Breve pausa para no saturar la CPU
            except Exception as e:
                print(f"\nError en stream: {e}")
                self.streaming = False
                break
        
        print("\nStream finalizado")
        self.streaming = False
    
    def start_stream(self):
        """Inicia un hilo separado para escuchar el stream de datos"""
        if self.streaming:
            print("El stream ya está activo")
            return
            
        self.exit_event.clear()
        self.stream_thread = threading.Thread(target=self._stream_listener)
        self.stream_thread.daemon = True  # El hilo se cerrará cuando el programa principal termine
        self.stream_thread.start()
    
    def stop_stream(self):
        """Detiene el stream de datos"""
        if not self.streaming:
            print("No hay stream activo")
            return
            
        self.exit_event.set()
        if self.stream_thread:
            self.stream_thread.join(timeout=2.0)
        self.streaming = False
        print("Stream detenido")
    
    def send_command(self, command):
        """Envía un comando al ESP32 y muestra la respuesta"""
        if not self.connected:
            print("No conectado. Conecte primero.")
            return
            
        try:
            command_bytes = command.encode('utf-8')
            
            if self.connection_type == "bluetooth":
                self.bt_socket.send(command_bytes)
            elif self.connection_type == "serial":
                self.serial_connection.write(command_bytes)
                self.serial_connection.flush()
                
            print(f"Comando enviado: {command}")
            
            # Si no hay stream activo, esperar una respuesta única
            if not self.streaming:
                time.sleep(0.5)
                
                if self.connection_type == "bluetooth":
                    ready = select.select([self.bt_socket], [], [], 2.0)
                    if ready[0]:
                        response = self.bt_socket.recv(1024).decode('utf-8', errors='replace')
                        print("\nRespuesta:")
                        print(response)
                    else:
                        print("No se recibió respuesta")
                        
                elif self.connection_type == "serial":
                    time.sleep(0.5)  # Dar tiempo para que el dispositivo responda
                    if self.serial_connection.in_waiting > 0:
                        response = self.serial_connection.read(self.serial_connection.in_waiting).decode('utf-8', errors='replace')
                        print("\nRespuesta:")
                        print(response)
                    else:
                        print("No se recibió respuesta")
                    
            # Si el comando parece ser para iniciar un stream, activarlo
            if("sensors start" == command):
                self.start_stream()

            if("sensors stop" == command):
                self.stop_stream()            
            
                
        except Exception as e:
            print(f"Error al enviar comando: {e}")
            self.connected = False
    
    def disconnect(self):
        """Cierra la conexión"""
        self.stop_stream()
        
        if self.connection_type == "bluetooth" and self.bt_socket:
            self.bt_socket.close()
            self.bt_socket = None
        
        if self.connection_type == "serial" and self.serial_connection:
            self.serial_connection.close()
            self.serial_connection = None
            
        self.connected = False
        print("Desconectado")
    
    def interactive_mode(self):
        """Modo interactivo para enviar comandos"""
        print("\n--- Comandos disponibles ---")                
        print("  exit - Salir del programa")
        
        while self.connected:
            try:
                command = input("\nIngrese comando > ")
                if command.lower() == "exit":
                    break                
                else:
                    self.send_command(command)
            except KeyboardInterrupt:
                print("\nInterrupción detectada. Escriba 'exit' para salir.")
            except Exception as e:
                print(f"Error: {e}")


def main():
    client = ESP32Client()
    
    # Preguntar por el tipo de conexión
    while True:
        print("\n--- Seleccione tipo de conexión ---")
        print("1. Bluetooth")
        print("2. Puerto Serial (USB)")
        
        try:
            choice = input("Ingrese su opción (1-2): ")
            if choice == "1":
                # Conexión Bluetooth
                device_name = input("Ingrese el nombre del dispositivo Bluetooth (por defecto 'Melquiades-Deck'): ")
                if not device_name:
                    device_name = "Melquiades-Deck"
                
                if client.connect_bluetooth(device_name):
                    break
                else:
                    print("No se pudo conectar vía Bluetooth. Intente nuevamente.")
                    
            elif choice == "2":
                # Conexión Serial
                client.list_serial_ports()
                port = input("Ingrese el puerto serial (por ejemplo, COM3): ")
                if port:
                    baudrate = input("Ingrese la velocidad en baudios (por defecto 115200): ")
                    if not baudrate:
                        baudrate = 115200
                    else:
                        baudrate = int(baudrate)
                    
                    if client.connect_serial(port, baudrate):
                        break
                    else:
                        print("No se pudo conectar al puerto serial. Intente nuevamente.")
                else:
                    print("Debe especificar un puerto serial.")
            else:
                print("Opción no válida, intente nuevamente.")
        except Exception as e:
            print(f"Error: {e}")
    
    # Si llegamos aquí, significa que estamos conectados
    try:
        client.interactive_mode()
    finally:
        client.disconnect()


if __name__ == "__main__":
    main()