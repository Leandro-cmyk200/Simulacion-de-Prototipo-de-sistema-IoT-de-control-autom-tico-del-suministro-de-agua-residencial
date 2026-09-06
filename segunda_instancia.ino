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

const int TANQUE_LLENO = 10;
bool tanqueNecesitaAgua = false;

const int CISTERNA_VACIA = 70;
const int CISTERNA_LLENA = 10;
const int CISTERNA_MINIMA = 80;

// ======================================================
// VARIABLES
// ======================================================

bool motorEncendido = false;
bool sistemaBloqueado = false;
unsigned long ultimoAvisoSinCarga = 0;
unsigned long ultimoAvisoMotor = 0;


// ======================================================
// SETUP
// ======================================================

void setup()
{
  Serial.begin(9600);

  pinMode(RELE, OUTPUT);


  pinMode(TRIG_CISTERNA, OUTPUT);
  pinMode(ECHO_CISTERNA, INPUT);

  apagarMotor();

  Serial.println("Sistema iniciado");
}


// ======================================================
// LOOP PRINCIPAL
// ======================================================

void loop()
{
  desbloquearSistema();
  recibirComando();
  controlarSistema();

  delay(500);
}


// ======================================================
// CONTROL PRINCIPAL
// ======================================================

void controlarSistema()
{
  // Si el sistema está bloqueado por una falla
  if (sistemaBloqueado)
  {
    return;
  }

  // Si el tanque elevado NO necesita agua
  if (!tanqueNecesitaAgua)
  {
    apagarMotor();
    return;
  }

  // El tanque elevado necesita agua
  Serial.println("El tanque elevado necesita abastecimiento");

  // Ahora revisamos la cisterna
  int distanciaCisterna = leerCisterna();

  Serial.print("Cisterna: ");
  Serial.print(distanciaCisterna);
  Serial.println(" cm");

  // Cisterna todavía no está llena
  if (distanciaCisterna > CISTERNA_LLENA)
  {
    apagarMotor();

    Serial.println("Esperando que la cisterna llegue al 100%");
    return;
  }

  // Cisterna llena
  Serial.println("Cisterna llena");
  Serial.println("Iniciando abastecimiento del tanque elevado");

  activarMotor();

  controlarMotor();
}


// ======================================================
// CONTROL DEL MOTOR
// ======================================================

void controlarMotor()
{
  while (motorEncendido)
  {
    // -----------------------------------------
    // Revisar si el tanque elevado ya está lleno
    // -----------------------------------------

    if (!tanqueNecesitaAgua)
    {
      Serial.println("Tanque elevado abastecido");

      apagarMotor();

      return;
    }


    // -----------------------------------------
    // Revisar cisterna
    // -----------------------------------------

    int distanciaCisterna = leerCisterna();

    Serial.print("Cisterna: ");
    Serial.print(distanciaCisterna);
    Serial.println(" cm");


    // -----------------------------------------
    // Cisterna llegó al mínimo
    // -----------------------------------------

    if (distanciaCisterna >= CISTERNA_MINIMA)
    {
      Serial.println("Cisterna llegó al 20%");
      Serial.println("Deteniendo motor para proteger la reserva");

      apagarMotor();

      return;
    }


    // -----------------------------------------
    // Revisar caudal
    // -----------------------------------------

    evaluarCaudal();

    delay(500);
  }
}


// ======================================================
// EVALUAR CAUDALIMETRO
// ======================================================

void evaluarCaudal()
{
  int valorCaudal = analogRead(CAUDALIMETRO);

  Serial.print("Caudal A0: ");
  Serial.println(valorCaudal);


  if (valorCaudal < 20)
  {
    notificarFalloMotor();
  }
}


// ======================================================
// NOTIFICAR CISTERNA VACIA
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
  digitalWrite(RELE, LOW);

  motorEncendido = false;

  Serial.println("MOTOR APAGADO");
}


// ======================================================
// INFORMACION DEL TANQUE ELEVADO
// ======================================================

void recibirComando()
{
  if (Serial.available())
  {
    String comando = Serial.readStringUntil('\n');
    comando.trim();

    // -----------------------------------------
    // TANQUE ELEVADO NECESITA AGUA
    // -----------------------------------------

    if (comando == "TANQUE_NECESITA_AGUA")
    {
      tanqueNecesitaAgua = true;

      Serial.println("Comando recibido:");
      Serial.println("TANQUE_NECESITA_AGUA");

      Serial.println("Esperando cisterna al 100%");
    }


    // -----------------------------------------
    // TANQUE ELEVADO ABASTECIDO
    // -----------------------------------------

    else if (comando == "TANQUE_ABASTECIDO")
    {
      tanqueNecesitaAgua = false;

      Serial.println("Comando recibido:");
      Serial.println("TANQUE_ABASTECIDO");

      apagarMotor();
    }
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


  // Si no recibió respuesta
  if (duracion == 0)
  {
    Serial.println("ERROR: sensor sin respuesta");

    return 999;
  }


  float distancia = duracion * 0.0343 / 2;


  return (int)distancia;
}

// ======================================================
// DESBLOQUEAR SISTEMA
// ======================================================

void desbloquearSistema()
{
  if (Serial.available())
  {
    String comando = Serial.readStringUntil('\n');
    comando.trim();

    if (comando == "reiniciar_sistema")
    {
      sistemaBloqueado = false;

      Serial.println();
      Serial.println("================================");
      Serial.println("COMANDO RECIBIDO");
      Serial.println("Sistema desbloqueado");
      Serial.println("Reanudando funcionamiento...");
      Serial.println("================================");
    }
  }
}