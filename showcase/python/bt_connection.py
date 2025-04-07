import bluetooth
import time
import sys
import select
import threading
import queue

class ESP32BluetoothClient:
    def __init__(self, device_name="Melquiades-Deck"):
        self.device_name = device_name
        self.socket = None
        self.connected = False
        self.target_address = None
        self.streaming = False
        self.stream_thread = None
        self.data_queue = queue.Queue()
        self.exit_event = threading.Event()

    def find_device(self):
        print(f"Buscando dispositivo '{self.device_name}'...")
        nearby_devices = bluetooth.discover_devices(duration=8, lookup_names=True)
        
        if not nearby_devices:
            print("No se encontraron dispositivos Bluetooth.")
            return False
            
        print(f"Dispositivos encontrados: {len(nearby_devices)}")
        for addr, name in nearby_devices:
            print(f"  {addr} - {name}")
            if name == self.device_name:
                self.target_address = addr
                print(f"¡Dispositivo encontrado! Dirección: {addr}")
                return True
                
        print(f"No se encontró el dispositivo '{self.device_name}'")
        return False
        
    def connect(self):
        if not self.target_address:
            if not self.find_device():
                return False
        
        try:
            # SPP UUID estándar
            port = 1  # SPP normalmente usa el puerto 1
            self.socket = bluetooth.BluetoothSocket(bluetooth.RFCOMM)
            self.socket.connect((self.target_address, port))
            self.connected = True
            print(f"Conectado a {self.device_name}")
            
            # Recibir mensajes de bienvenida
            self._receive_initial_messages()
            
            return True
        except Exception as e:
            print(f"Error al conectar: {e}")
            return False
    
    def _receive_initial_messages(self):
        """Recibe y muestra los mensajes iniciales del ESP32"""
        time.sleep(1)  # Dar tiempo para recibir mensajes iniciales
        try:
            if self.socket.fileno() != -1:
                ready = select.select([self.socket], [], [], 2.0)
                if ready[0]:
                    data = self.socket.recv(1024).decode('utf-8')
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
                if self.socket.fileno() != -1:
                    ready = select.select([self.socket], [], [], 0.5)
                    if ready[0]:
                        data = self.socket.recv(1024).decode('utf-8', errors='replace')
                        if data:
                            # Poner datos en la cola para ser procesados por el hilo principal
                            self.data_queue.put(data)
                            # También mostrar directamente
                            print(f"\n[STREAM] {data}")
                            # Vuelve a mostrar el prompt después de imprimir datos del stream
                            print("\nIngrese comando > ", end="", flush=True)
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
        if not self.connected or not self.socket:
            print("No conectado. Conecte primero.")
            return
            
        try:
            self.socket.send(command)
            print(f"Comando enviado: {command}")
            
            # Si no hay stream activo, esperar una respuesta única
            if not self.streaming:
                time.sleep(0.5)
                ready = select.select([self.socket], [], [], 2.0)
                if ready[0]:
                    response = self.socket.recv(1024).decode('utf-8')
                    print("\nRespuesta:")
                    print(response)
                else:
                    print("No se recibió respuesta")
                    
            # Si el comando parece ser para iniciar un stream, activarlo
            if "start_sensors" in command.lower() or "debugging" in command.lower():
                self.start_stream()

            if "stop_sensors" in command.lower() or "stopping" in command.lower():
                self.stop_stream()
                
        except Exception as e:
            print(f"Error al enviar comando: {e}")
            self.connected = False
    
    def disconnect(self):
        """Cierra la conexión Bluetooth"""
        self.stop_stream()
        if self.socket:
            self.socket.close()
            self.connected = False
            print("Desconectado")
    
    def interactive_mode(self):
        """Modo interactivo para enviar comandos"""
        print("\n--- Comandos nativos de Python ---")                
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
    client = ESP32BluetoothClient()    
    if client.connect():
        try:
            client.interactive_mode()
        finally:
            client.disconnect()
    else:
        print("No se pudo conectar al ESP32")

if __name__ == "__main__":
    main()