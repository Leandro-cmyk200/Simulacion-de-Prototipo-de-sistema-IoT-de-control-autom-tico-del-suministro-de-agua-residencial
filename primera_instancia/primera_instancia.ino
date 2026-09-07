// ==========================================
// CONFIGURACIÓN DE PINES
// ==========================================

const int rele1 = 3;
const int rele2 = 4;
const int rele3 = 2;

const int trigPin = 7;
const int echoPin = 6;

const int humedadPin = A0;


// ==========================================
// VARIABLES
// ==========================================

long duracion;
float distancia;
int humedad;

bool sistemaActivo = false;

// Un comando solo puede actuar en la etapa que le corresponde. Esto evita que
// dos procesos controlen los relés a la vez.
enum EstadoCisterna
{
  ESPERANDO_ABASTECIMIENTO,
  PROCESANDO_ABASTECIMIENTO,
  ABASTECIENDO_CISTERNA
};

EstadoCisterna estadoCisterna = ESPERANDO_ABASTECIMIENTO;
bool sinCargaAtendido = false;


// Guarda el resultado de la evaluación de calidad
// true  = A0 > 100
// false = A0 < 100
bool calidadAlta = false;


// ==========================================
// CONFIGURACIÓN INICIAL
// ==========================================

void setup()
{
  Serial.begin(9600);

  pinMode(rele1, OUTPUT);
  pinMode(rele2, OUTPUT);
  pinMode(rele3, OUTPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Estado inicial: R1 cerrado (HIGH), R2 y R3 cerrados (LOW).
  digitalWrite(rele1, HIGH);
  digitalWrite(rele2, LOW);
  digitalWrite(rele3, LOW);
  mostrarEsperarComando();
}


// ==========================================
// FUNCIÓN: MEDIR DISTANCIA
// ==========================================

float medirDistancia()
{
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);

  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);

  digitalWrite(trigPin, LOW);

  duracion = pulseIn(echoPin, HIGH, 30000);

  if (duracion == 0)
  {
    return 999;
  }

  return duracion * 0.0343 / 2;
}


// ==========================================
// FUNCIÓN: LEER HUMEDAD / CALIDAD
// ==========================================

int leerHumedad()
{
  return analogRead(humedadPin);
}


// ==========================================
// FUNCIÓN: ACTIVAR SISTEMA
// ==========================================

void iniciarSinCarga()
{
  Serial.println();
  Serial.println("================================");
  Serial.println("SIN_CARGA RECIBIDO");
  Serial.println("================================");

  // R1 HIGH
  digitalWrite(rele1, HIGH);

  // R2 HIGH
  digitalWrite(rele2, HIGH);

  // R3 LOW
  digitalWrite(rele3, LOW);

  Serial.println("Rele 1 -> HIGH");
  Serial.println("Rele 2 -> HIGH");
  Serial.println("Rele 3 -> LOW");

  sistemaActivo = true;
}


// ==========================================
// FUNCIÓN: ESPERAR DISTANCIA <= 50
// ==========================================

void esperarDistancia50()
{
  Serial.println();
  Serial.println("Esperando distancia <= 50 cm...");

  // R2 debe permanecer HIGH mientras esperamos
  digitalWrite(rele2, HIGH);

  while (true)
  {
    distancia = medirDistancia();

    Serial.print("Distancia: ");
    Serial.print(distancia);
    Serial.println(" cm");

    // Cuando llegue a 50 cm o menos
    if (distancia <= 50)
    {
      Serial.println("Distancia <= 50 cm");

      // Cerrar R2
      digitalWrite(rele2, LOW);

      Serial.println("Rele 2 -> LOW");

      break;
    }

    delay(300);
  }
}


// ==========================================
// FUNCIÓN: EVALUAR CALIDAD DURANTE 10 SEG
// ==========================================
void evaluarCalidad()
{
  Serial.println();
  Serial.println("================================");
  Serial.println("Evaluando turbidez durante 10 s");
  Serial.println("================================");

  unsigned long tiempoInicial = millis();

  long sumaMuestras = 0;
  int cantidadMuestras = 0;

  while (millis() - tiempoInicial < 10000)
  {
    humedad = leerHumedad();

    sumaMuestras += humedad;
    cantidadMuestras++;

    Serial.print("A0 = ");
    Serial.println(humedad);

    delay(500);
  }

  // Calcular promedio
  float promedio = (float)sumaMuestras / cantidadMuestras;

  Serial.println();
  Serial.println("================================");
  Serial.print("Cantidad de muestras: ");
  Serial.println(cantidadMuestras);

  Serial.print("Promedio A0: ");
  Serial.println(promedio);

  // Evaluar turbidez
  if (promedio > 100)
  {
    calidadAlta = true;

    Serial.println("Promedio > 100");
    Serial.println("Turbidez ALTA");
  }
  else
  {
    calidadAlta = false;

    Serial.println("Promedio <= 100");
    Serial.println("Turbidez ACEPTABLE");
  }

  Serial.println("================================");
}


// ==========================================
// ABASTECER CISTERNA CON AGUA APTA
// ==========================================

void solicitarAbastecimientoCisterna()
{
  Serial.println();
  Serial.println("================================");
  Serial.println("CISTERNA NECESITA ABASTECIMIENTO");
  Serial.println("Iniciando carga, evaluacion de turbidez y purga...");
  Serial.println("================================");

  // Antes de iniciar un nuevo ciclo siempre se cierra R1.
  digitalWrite(rele1, HIGH);
  Serial.println("Rele 1 -> HIGH (cerrado)");

  // Se conserva la secuencia original: llenar hasta 50 cm, evaluar la
  // turbidez, purgar mediante R3 y abrir R1 solo si el agua es apta.
  ejecutarProceso();
}


// ==========================================
// DETENER ABASTECIMIENTO DE CISTERNA
// ==========================================

void detenerAbastecimientoCisterna()
{
  // tanque_cisterna_lleno llega cuando la segunda instancia detecta el 20 %.
  // R1 es normalmente abierto: HIGH lo deja cerrado.
  digitalWrite(rele1, HIGH);
  estadoCisterna = ESPERANDO_ABASTECIMIENTO;
  Serial.println();
  Serial.println("================================");
  Serial.println("tanque_cisterna_lleno recibido");
  Serial.println("Rele 1 -> HIGH (cerrado)");
  Serial.println("Esperando cisterna_necesita_abastecimiento");
  Serial.println("================================");
}


// ==========================================
// FUNCIÓN: ABRIR R3 Y ESPERAR > 150 CM
// ==========================================

void abrirRele3Hasta150()
{
  Serial.println();
  Serial.println("================================");
  Serial.println("Abriendo Rele 3");
  Serial.println("Esperando distancia > 150 cm");
  Serial.println("================================");

  // R3 HIGH
  digitalWrite(rele3, HIGH);

  Serial.println("Rele 3 -> HIGH");

  while (true)
  {
    distancia = medirDistancia();

    Serial.print("Distancia: ");
    Serial.print(distancia);
    Serial.println(" cm");

    // Esperar hasta superar 150 cm
    if (distancia > 150)
    {
      Serial.println("Distancia > 150 cm");

      // Cerrar R3
      digitalWrite(rele3, LOW);

      Serial.println("Rele 3 -> LOW");

      break;
    }

    delay(300);
  }
}


// ==========================================
// FUNCIÓN: PROCESO COMPLETO
// ==========================================

void ejecutarProceso()
{
  // ----------------------------------------
  // 1. Esperar distancia <= 50
  // ----------------------------------------

  esperarDistancia50();


  // ----------------------------------------
  // 2. Evaluar A0 durante 10 segundos
  // ----------------------------------------

  evaluarCalidad();


  // ----------------------------------------
  // 3. Abrir R3 hasta superar 150 cm
  // ----------------------------------------

  abrirRele3Hasta150();


  // ----------------------------------------
  // 4. Decidir qué hacer después
  // ----------------------------------------

  if (calidadAlta)
  {
    // ======================================
    // A0 > 100
    // ======================================

    Serial.println();
    Serial.println("A0 era > 100");
    Serial.println("Volviendo a esperar distancia <= 50 cm");

    // Volver al principio de la acción
    // pero NO esperamos otro sin_carga
    esperarDistancia50();

    // Volvemos a evaluar calidad
    ejecutarProceso();
  }
  else
  {
    // ======================================
    // A0 < 100
    // ======================================

    Serial.println();
    Serial.println("A0 era < 100");
    Serial.println("Proceso terminado.");

    // Apagar R1
    digitalWrite(rele1, LOW);

    // El agua apta ya puede pasar hacia la cisterna. Desde este punto solo
    // son validos sin_carga o tanque_cisterna_lleno.
    estadoCisterna = ABASTECIENDO_CISTERNA;

    Serial.println("Rele 1 -> LOW");

    sistemaActivo = false;

    Serial.println();
    Serial.println("Esperando nuevamente: sin_carga");
  }
}

void mostrarEsperarComando()
{
  Serial.println("================================");
  Serial.println("Sistema listo");
  Serial.println("Escriba: sin_carga");
  Serial.println("Escriba: cisterna_necesita_abastecimiento");
  Serial.println("Recibe: tanque_cisterna_lleno");
  Serial.println("================================");
}


// ==========================================
// LOOP PRINCIPAL
// ==========================================

void loop()
{
  // ----------------------------------------
  // Esperar comando
  // ----------------------------------------

  if (Serial.available() > 0)
  {
    String comando = Serial.readStringUntil('\n');

    comando.trim();

    if (comando == "sin_carga")
    {
      // sin_carga solo es valido mientras R1 deja pasar agua hacia la
      // cisterna y no hay otro proceso en ejecucion.
      if (estadoCisterna == ABASTECIENDO_CISTERNA &&
          digitalRead(rele1) == LOW && !sistemaActivo && !sinCargaAtendido)
      {
        estadoCisterna = PROCESANDO_ABASTECIMIENTO;
        sinCargaAtendido = true;
        iniciarSinCarga();

        // Ejecutar todo el proceso
        ejecutarProceso();

        // Mostrar nuevamente que está listo
        mostrarEsperarComando();
      }
      else
      {
        Serial.println("sin_carga ignorado: ya fue atendido o R1 no esta abasteciendo la cisterna.");
      }
    }

    else if (comando == "tanque_cisterna_lleno")
    {
      // Solo se puede cerrar R1 cuando antes se habilito el abastecimiento.
      if (estadoCisterna == ABASTECIENDO_CISTERNA && digitalRead(rele1) == LOW)
      {
        detenerAbastecimientoCisterna();

        // Mostrar nuevamente que está listo
        mostrarEsperarComando();
      }
      else
      {
        Serial.println("tanque_cisterna_lleno ignorado: no hay abastecimiento activo.");
      }
    }

    else if (comando == "cisterna_necesita_abastecimiento")
    {
      // Solo inicia desde reposo; un segundo comando no reinicia ni mezcla
      // procesos mientras la cisterna ya se esta abasteciendo.
      if (estadoCisterna == ESPERANDO_ABASTECIMIENTO && !sistemaActivo)
      {
        estadoCisterna = PROCESANDO_ABASTECIMIENTO;
        sinCargaAtendido = false;
        solicitarAbastecimientoCisterna();

        // Mostrar nuevamente que está listo
        mostrarEsperarComando();
      }
      else
      {
        Serial.println("cisterna_necesita_abastecimiento ignorado: el ciclo ya esta activo.");
      }
    }
  }
}
