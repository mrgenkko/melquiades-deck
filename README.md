<h1 align = "center">Melquiades Deck</h1>

Hace unos meses me diagnosticaron hipoacusia súbita idiopática en el oído derecho, como secuela tengo tinnitus crónica y una pérdida del 96% de audición, sin duda alguna esta ha sido una de las tribulaciones mas hijueputas que he afrontado en 29 años de una existencia que podría considerarse poco usual y he de admitir que el ruido que conlleva mi condición me ha sumergido en un increíble foso de tristeza y desolación del cual ha sido bastante complejo salir.

Realizar este proyecto no solo me ha ayudado a afrontar mi perdida, me dio la confianza suficiente para retomar el ritmo de trabajo que pensaba haber perdido debido a un incesante ruido que no logro cesar.

Ya hablando serio y dejando el background que nadie pidió de lado, este dispositivo esta diseñado para ser usado con Melquiades Desktop, en showcase tengo un ejemplo en python para probar el device y saber que comandos utilizar o como gestionar streaming de sensores

Quedan algunos detalles pendientes pero por tiempo y en parte paja se deja para después, recomendable usar el proyecto en Espressif, el código de Arduino es viejo y pertenece a otra board más ficty.

Una vez finalizado el proyecto o a lo que tenga tiempo subo un esquemático donde se conectan todos los componentes, actualmente solo está el diagrama de pines de la Lilygo, tal vez un video armando el dispositivo y otro configurando entorno en Windows.

## Características

- **Bluetooth**: Shell exclusiva para comandos via BT, al emplear PCM5102 podemos conectar audifonos y transmitir audio, pendiente tema de mic.
- **DAC**: Se necesita PCM5102, podemos conectar audifonos, ecualizar y aumentar el volumen maximo.
- **Ecualizacion por audifono**:Podemos separar la senal de audio por canal (derecho o izquierdo), subir volumen, crear ecualizacion custom, entre otros
- **Sensores**: Tenemos maximo 12 slots para sensores, ya sea potenciometros o pulsadores, para asignar funcionalidad se debe de vincular con Melquiades Desktop
- **Keybindings**: Para usar esta funcionalidad debemos vincular con Melquiades Desktop y asignar sensores.
- **Audio Modulation**:Para modular el audio de apps por separado o master, vincular con Melquiades Desktop.
- **Gestion Bateria**: Modulo en desarrollo, la verdad no tenemos mas puertos para conectar.

## Hardware

Se estan utilizando elementos de Lilygo, se deja adjunto links para compra de los items, se designa una carpeta llamada pin diagrams para informacion especifica del hardware
- [LILYGO® TTGO T7](https://es.aliexpress.com/item/32977375539.html?spm=a2g0o.productlist.main.1.6608WIZBWIZBNP&algo_pvid=ac4677a6-55c9-4518-b485-5cde77da7a8d&algo_exp_id=ac4677a6-55c9-4518-b485-5cde77da7a8d-0&pdp_ext_f=%7B%22order%22%3A%2257%22%2C%22eval%22%3A%221%22%7D&pdp_npi=4%40dis%21USD%217.98%217.98%21%21%217.98%217.98%21%4021030ea417436592027795129e5f28%2112000031557142204%21sea%21CO%216150149659%21X&curPageLogUid=LS4dgb8EYQwZ&utparam-url=scene%3Asearch%7Cquery_from%3A)
- [PCM5102](https://es.aliexpress.com/item/1005005707584688.html?spm=a2g0o.productlist.main.5.2f98BC2cBC2cg1&algo_pvid=1dbb1978-826d-4a4d-b567-38f6bf5d9807&algo_exp_id=1dbb1978-826d-4a4d-b567-38f6bf5d9807-2&pdp_ext_f=%7B%22order%22%3A%22179%22%2C%22eval%22%3A%221%22%7D&pdp_npi=4%40dis%21USD%211.82%211.60%21%21%211.82%211.60%21%402101d9ee17440915047067555e5dbb%2112000043383562348%21sea%21CO%216150149659%21X&curPageLogUid=6OOn2pmDpboy&utparam-url=scene%3Asearch%7Cquery_from%3A)
- [Potenciometro](https://es.aliexpress.com/item/1005008077517636.html?spm=a2g0o.productlist.main.9.26d614dckPcv4F&algo_pvid=d6ed074b-e830-438b-8706-6aea7f5e1182&algo_exp_id=d6ed074b-e830-438b-8706-6aea7f5e1182-4&pdp_ext_f=%7B%22order%22%3A%222719%22%2C%22eval%22%3A%221%22%2C%22orig_sl_item_id%22%3A%221005008077517636%22%2C%22orig_item_id%22%3A%221005007027600527%22%7D&pdp_npi=4%40dis%21USD%217.22%213.61%21%21%2152.52%2126.26%21%402101eac917436593471822659eb797%2112000043565538587%21sea%21CO%216150149659%21X&curPageLogUid=t8BucughJdA7&utparam-url=scene%3Asearch%7Cquery_from%3A)
- [Pulsadores](https://es.aliexpress.com/item/1005007783283991.html?spm=a2g0o.detail.pcDetailTopMoreOtherSeller.4.62bfes8ses8sgA&gps-id=pcDetailTopMoreOtherSeller&scm=1007.40196.394786.0&scm_id=1007.40196.394786.0&scm-url=1007.40196.394786.0&pvid=870ebeef-7dcf-48f3-9d54-2814febb54f5&_t=gps-id:pcDetailTopMoreOtherSeller,scm-url:1007.40196.394786.0,pvid:870ebeef-7dcf-48f3-9d54-2814febb54f5,tpp_buckets:668%232846%238112%231997&pdp_ext_f=%7B%22order%22%3A%223147%22%2C%22eval%22%3A%221%22%2C%22sceneId%22%3A%2230050%22%7D&pdp_npi=4%40dis%21USD%218.52%215.37%21%21%2162.02%2139.07%21%402103146f17436596327166976ee026%2112000042188994770%21rec%21CO%216150149659%21X&utparam-url=scene%3ApcDetailTopMoreOtherSeller%7Cquery_from%3A)



### Consideraciones

Se debe de tener cuidado con los pines ADC11 y ADC10, si al arrancar se detecta voltaje puede poner el dispositivo en modo download, si se realizan pruebas o se esta en desarrollo se recomienda desconectar estos dos pines, revisar diagrama de pulsadores (estos siempre envian cero o ground por defecto), una vez armado el dispositivo completo se pueden dejar conectados, no deberia de haber problema alguno.

Si el dispositivo no se encuentra correctamente alimentado se pueden empezar a experimentar reinicios inesperados y problemas varios, recuerda que la alimentacion via pc no siempre es suficiente.

Bateria es opcional, se recomienda usar con fuente de alimentacion externa (cargador de celular puede ser).


- [Bateria](https://es.aliexpress.com/item/1005006802038713.html?spm=a2g0o.order_list.order_list_main.86.7691194dpDh80A&gatewayAdapt=glo2esp)

### Prerequisitos

Asegúrate de tener instalados los siguientes programas y herramientas antes de comenzar con la instalación:

- [USB Drivers](https://www.silabs.com/documents/public/software/CP210x_VCP_Windows.zip)
- [Arduino](https://www.arduino.cc/en/software)
- [Espressif](https://dl.espressif.com/dl/esp-idf/?idf=4.4)
- [Python](https://www.python.org/downloads/)
### Pasos para instalar

1. Clona el repositorio:

   ```bash
   git clone https://github.com/mrgenkko/melquiades-deck.git
2. Instalar bibliotecas de python (necesario para pruebas de escritorio):
   ```bash
   pip install pybluez
### Funciones disponibles
1. Inicializa el LED verde de la board

   ```bash
   led_board start
2. Detiene el LED verde de la board

   ```bash
   led_board stop
3. Inicio lecutra de sensores (potenciometros y pulsadores)

   ```bash
   sensors start
4. Detenemos lectura de sensores

   ```bash
   sensors stop
5. Establecemos volumen, varia entre 0 y 100

   ```bash
   set_volume 50
6. Definimos modo de ecualizacion, recibe los valores flat, bass_boost, mid_boost, treble_boost, vocal.

   ```bash
   eq flat
7. Headphone balance sigue en progreso, par valores maluquitos, deberia de variar entre -1.00 y 1.00

   ```bash
   headphone_balance -0.2
8. Habilitamos filtrado con DSP

   ```bash
   dsp enabled
9. Deshabilitamos filtrado con DSP

   ```bash
   dsp disabled
9. Comando de ayuda

   ```bash
   help
