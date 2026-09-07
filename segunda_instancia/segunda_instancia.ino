// ======================================================
// PINES
// ======================================================

const int CAUDALIMETRO = A0;
const int RELE = 4;

// Cisterna
const int TRIG_CISTERNA = 2;
const int ECHO_CISTERNA = 3;


// ======================================================
// CONSTANTES DE NIVEL
// ======================================================


const int CISTERNA_VACIA = 100;
const int CISTERNA_LLENA = 20;
const int CISTERNA_MINIMA = 80;


// ======================================================
// VARIABLES
// ======================================================

bool tanqueNecesitaAgua = false;
bool motorEncendido = false;
bool sistemaBloqueado = false;

unsigned long ultimoAvisoSinCarga = 0;
unsigned long ultimoAvisoMotor = 0;

// Se usan para evitar repetir en el monitor serial datos que no cambiaron.
int ultimaDistanciaCisterna = -1;
int ultimoValorCaudal = -1;
int ultimoEstadoMostrado = -1;
bool errorSensorCisternaReportado = false;

void mostrarDistanciaSiCambio(int distancia);
void mostrarEstadoSiCambio(int estado, const char *mensaje);
void notificarCisternaAl20PorCiento();


// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(9600);

  pinMode(RELE, OUTPUT);

  pinMode(TRIG_CISTERNA, OUTPUT);
  pinMode(ECHO_CISTERNA, INPUT);

  // No imprimir continuamente "MOTOR APAGADO"
  digitalWrite(RELE, LOW);
  motorEncendido = false;

  Serial.println("Sistema iniciado");
   Serial.println("===Esperando Mensajes===");
   Serial.println("reiniciar_sistema");
   Serial.println("tanque_necesita_agua");
   Serial.println("tanque_abastecido");
}


// ======================================================
// LOOP PRINCIPAL
// ======================================================

void loop()
{
  recibirComando();

  controlarSistema();

  delay(500);
}


// ======================================================
// RECIBIR COMANDOS
// ======================================================

void recibirComando()
{
  if (Serial.available())
  {
    String comando = Serial.readStringUntil('\n');
    comando.trim();


    // -----------------------------------------
    // REINICIAR SISTEMA
    // -----------------------------------------

    if (comando == "reiniciar_sistema")
    {
      sistemaBloqueado = false;

      Serial.println();
      Serial.println("================================");
      Serial.println("Comando recibido:");
      Serial.println("Sistema desbloqueado");
      Serial.println("Reanudando funcionamiento...");
      Serial.println("================================");

      return;
    }


    // -----------------------------------------
    // TANQUE ELEVADO NECESITA AGUA
    // -----------------------------------------

    if (comando == "tanque_necesita_agua")
    {
      tanqueNecesitaAgua = true;

      Serial.println();
      Serial.println("================================");
      Serial.println("Comando recibido:");
      Serial.println("tanque_necesita_agua");
      Serial.println("Esperando cisterna al 100%");
      Serial.println("================================");

      return;
    }


    // -----------------------------------------
    // TANQUE ELEVADO ABASTECIDO
    // -----------------------------------------

    if (comando == "tanque_abastecido")
    {
      tanqueNecesitaAgua = false;

      Serial.println();
      Serial.println("================================");
      Serial.println("Comando recibido:");
      Serial.println("tanque_abastecido");
      Serial.println("================================");

      apagarMotor();

      return;
    }
}
}
// ======================================================
// CONTROL PRINCIPAL
// ======================================================

void controlarSistema()
{
  // -----------------------------------------
  // SISTEMA BLOQUEADO
  // -----------------------------------------

  if (sistemaBloqueado)
  {
    return;
  }


  // -----------------------------------------
  // TANQUE ELEVADO NO NECESITA AGUA
  // -----------------------------------------

  if (!tanqueNecesitaAgua)
  {
    mostrarEstadoSiCambio(0, "Tanque elevado abastecido; sistema en espera");

    // Solo apagar si realmente estaba encendido
    if (motorEncendido)
    {
      apagarMotor();
    }

    return;
  }


  // -----------------------------------------
  // TANQUE ELEVADO NECESITA AGUA
  // -----------------------------------------

  // -----------------------------------------
  // REVISAR CISTERNA
  // -----------------------------------------

  int distanciaCisterna = leerCisterna();

  mostrarDistanciaSiCambio(distanciaCisterna);


  // -----------------------------------------
  // CISTERNA NO ESTÁ LLENA
  // -----------------------------------------

  if (!motorEncendido && distanciaCisterna > CISTERNA_LLENA)
  {
    mostrarEstadoSiCambio(1, "Tanque elevado necesita abastecimiento; esperando que la cisterna llegue al 100%");
    return;
  }


  // -----------------------------------------
  // CISTERNA LLENA
  // -----------------------------------------

  if (!motorEncendido && distanciaCisterna <= CISTERNA_LLENA)
  {
    mostrarEstadoSiCambio(2, "Cisterna llena: iniciando abastecimiento del tanque elevado");

    activarMotor();

    return;
  }


  // -----------------------------------------
  // MOTOR ENCENDIDO
  // -----------------------------------------

  if (motorEncendido)
  {
    controlarMotor();
  }
}


// ======================================================
// CONTROL DEL MOTOR
// ======================================================

void controlarMotor()
{
  // -----------------------------------------
  // REVISAR SI EL TANQUE ELEVADO YA ESTÁ LLENO
  // -----------------------------------------

  if (!tanqueNecesitaAgua)
  {
    Serial.println("Tanque elevado abastecido");

    apagarMotor();

    return;
  }


  // -----------------------------------------
  // REVISAR CISTERNA
  // -----------------------------------------

  int distanciaCisterna = leerCisterna();

  mostrarDistanciaSiCambio(distanciaCisterna);


  // -----------------------------------------
  // CISTERNA LLEGÓ AL 20%
  // -----------------------------------------

  if (distanciaCisterna >= CISTERNA_MINIMA)
  {
    mostrarEstadoSiCambio(3, "Cisterna llego al 20%: deteniendo motor para proteger la reserva");

    notificarCisternaAl20PorCiento();

    apagarMotor();

    return;
  }


  // -----------------------------------------
  // REVISAR CAUDAL
  // -----------------------------------------

  evaluarCaudal();
}


// ======================================================
// NOTIFICAR A LA PRIMERA INSTANCIA
// ======================================================

void notificarCisternaAl20PorCiento()
{
  // Esta línea es el comando que recibe la primera instancia.
  Serial.println("tanque_cisterna_lleno");
  Serial.println("Mensaje enviado a primera instancia: tanque_cisterna_lleno");
}


// ======================================================
// EVALUAR CAUDALIMETRO
// ======================================================

void evaluarCaudal()
{
  int valorCaudal = analogRead(CAUDALIMETRO);

  if (valorCaudal != ultimoValorCaudal)
  {
    Serial.print("Caudal A0: ");
    Serial.println(valorCaudal);
    ultimoValorCaudal = valorCaudal;
  }


  if (valorCaudal < 20)
  {
    notificarFalloMotor();
  }
}


// ======================================================
// NOTIFICAR CISTERNA SIN CARGA
// ======================================================

void notificarSinCarga()
{
  unsigned long tiempoActual = millis();

  if (tiempoActual - ultimoAvisoSinCarga >= 10000)
  {
    Serial.println("sin_carga");

    ultimoAvisoSinCarga = tiempoActual;
  }
}


// ======================================================
// NOTIFICAR MOTOR SIN FUNCIONAMIENTO
// ======================================================

void notificarFalloMotor()
{
  Serial.println();
  Serial.println("================================");
  Serial.println("motor_no_funcionando");
  Serial.println("================================");

  Serial.println("Enviando notificacion...");
  Serial.println("NOTIFICACION ENVIADA");

  apagarMotor();

  sistemaBloqueado = true;

  Serial.println();
  Serial.println("SISTEMA BLOQUEADO");
  Serial.println("Esperando reiniciar_sistema para reiniciar...");
}


// ======================================================
// ACTIVAR MOTOR
// ======================================================

void activarMotor()
{
  if (!motorEncendido)
  {
    digitalWrite(RELE, HIGH);

    motorEncendido = true;

    Serial.println("MOTOR ACTIVADO");
  }
}


// ======================================================
// APAGAR MOTOR
// ======================================================

void apagarMotor()
{
  if (motorEncendido)
  {
    digitalWrite(RELE, LOW);

    motorEncendido = false;

    Serial.println("MOTOR APAGADO");
  }
}


// ======================================================
// MOSTRAR DATOS SOLO CUANDO CAMBIAN
// ======================================================

void mostrarDistanciaSiCambio(int distancia)
{
  if (distancia != ultimaDistanciaCisterna)
  {
    Serial.print("Cisterna: ");
    Serial.print(distancia);
    Serial.println(" cm");

    ultimaDistanciaCisterna = distancia;
  }
}


void mostrarEstadoSiCambio(int estado, const char *mensaje)
{
  if (estado != ultimoEstadoMostrado)
  {
    Serial.println(mensaje);
    ultimoEstadoMostrado = estado;
  }
}


// ======================================================
// LEER CISTERNA
// ======================================================

int leerCisterna()
{
  return medirDistancia(TRIG_CISTERNA, ECHO_CISTERNA);
}


// ======================================================
// MEDIR DISTANCIA
// ======================================================

int medirDistancia(int trigPin, int echoPin)
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  long duracion = pulseIn(echoPin, HIGH, 30000);

  if (duracion == 0)
  {
    if (!errorSensorCisternaReportado)
    {
      Serial.println("ERROR: sensor de cisterna sin respuesta");
      errorSensorCisternaReportado = true;
    }

    return 999;
  }

  errorSensorCisternaReportado = false;

  float distancia = duracion * 0.0343 / 2;

  return (int)distancia;
}
  
