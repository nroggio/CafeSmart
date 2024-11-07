#include <Ds1302.h>
#include <LiquidCrystal_I2C.h>

// Pines para RTC
const int RST_PIN = 7;  // Pin de reset (RST)
const int CLK_PIN = 5 ;  // Pin de reloj (CLK)
const int DAT_PIN = 6   ;  // Pin de datos (DAT)

Ds1302 rtc(RST_PIN, CLK_PIN, DAT_PIN);

// Declaracion de LCD I2C
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Variables globales
int seconds = 0;

// Modo
int modo = 0;

// Pines botones
const int okButtonPin = 10;
const int upButtonPin = 9;
const int downButtonPin = 8;

// Variables para boton
int lastOkButtonState = HIGH;
int lastUpButtonState = HIGH;
int lastDownButtonState = HIGH;

// Pin para el rele
const int relayPin = 13; 

// ON / OFF Cafetera
bool coffeeMakerOn = false;

// Menu
const String menuItems[3] = {"Empezar", "Programar", "Configurar"};
int currentMenuIndex = 0;
const int menuSize = 3;

// Variables para controlar menus y tiempo de inactividad
unsigned long lastInteractionTime = 0;
const unsigned long timeoutDuration = 5000; 
bool isMenuActive = false;

// Definir pines para el sensor ultrasonico
const int trigPin = 2; 
const int echoPin = 3; 

// Variable para almacenar la distancia medida
long distancia = 0;

void setup() {
    // Inicializacion del LCD
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(5, 0);
    lcd.print("Bienvenido");
    lcd.setCursor(4, 2);
    lcd.print("*Cafe Smart*");
    delay(2000);
    lcd.clear();

    // Pines de botones como entradas
    pinMode(okButtonPin, INPUT_PULLUP);
    pinMode(upButtonPin, INPUT_PULLUP);
    pinMode(downButtonPin, INPUT_PULLUP);

    // Configuracion pin de rele
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);

    // Inicializacion del RTC
    rtc.init();

    // Leer la hora actual
    Ds1302::DateTime now;
    rtc.getDateTime(&now);

    // Si es necesario, configurar una hora por defecto
    Ds1302::DateTime dt = {
        .year = 23,
        .month = 11,
        .day = 6,
        .hour = 12,
        .minute = 0,
        .second = 0,
        .dow = Ds1302::DOW_MON // Dia de la semana
    };
    rtc.setDateTime(&dt);

    // Iniciar el temporizador
    lastInteractionTime = millis();

    // Configurar pines del sensor ultrasonico
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
}

void loop() {
    // Leer la hora actual desde el RTC
    Ds1302::DateTime now;
    rtc.getDateTime(&now);

    if (!isMenuActive) {
        menuPrincipal(now);
    } else {
        mostrarMenu();
    }

    manejarBotones();

    // Medir distancia
    distancia = medirDistancia();

    // Verificar si ha pasado el tiempo de espera
    if (millis() - lastInteractionTime > timeoutDuration) {
        isMenuActive = false;
    }

    delay(200);
}

void menuPrincipal(Ds1302::DateTime now) {
    // Limpiar la pantalla
    lcd.clear();

    // Mostrar estado de la cafetera
    lcd.setCursor(0, 0);
    lcd.print("Cafetera: ");
    lcd.print(coffeeMakerOn ? "ON " : "OFF");

    // Mostrar cantidad de agua en cm
    lcd.setCursor(0, 1);
    lcd.print("Cantidad agua: ");
    lcd.print(distancia);
    lcd.print(" cm");

    // Formatear y mostrar la hora actual
    lcd.setCursor(0, 2);
    lcd.print("Hora: ");
    char timeBuffer[9]; // "HH:MM:SS"
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", now.hour, now.minute, now.second);
    lcd.print(timeBuffer);

    // Mostrar estado del agua
    lcd.setCursor(0, 3);
    lcd.print("Tiene agua? ");
    lcd.print(distancia < 20 ? "SI " : "NO ");
}

void mostrarMenu() {
    lcd.clear();
    for (int i = 0; i < menuSize; i++) {
        lcd.setCursor(0, i);
        if (i == currentMenuIndex) {
            lcd.print(">"); // Indicar el elemento seleccionado
        } else {
            lcd.print(" ");
        }
        lcd.print(menuItems[i]);
    }
}

void manejarBotones() {
    // Leer el estado de los botones
    int okButtonState = digitalRead(okButtonPin);
    int upButtonState = digitalRead(upButtonPin);
    int downButtonState = digitalRead(downButtonPin);

    // Actualizar el tiempo de interaccion si se presiona algun boton
    if (okButtonState == LOW || upButtonState == LOW || downButtonState == LOW) {
        lastInteractionTime = millis(); // Reiniciar temporizador
    }

    // Manejar el boton OK
    if (lastOkButtonState == HIGH && okButtonState == LOW) {
        if (!isMenuActive) {
            isMenuActive = true; // Activar el menu
            mostrarMenu();
        } else {
            seleccionarOpcion(); // Seleccionar opcion del menu
        }
        delay(200);     }
    lastOkButtonState = okButtonState;

    // Manejar el boton UP
    if (lastUpButtonState == HIGH && upButtonState == LOW) {
        if (isMenuActive) {
            currentMenuIndex = (currentMenuIndex - 1 + menuSize) % menuSize; // Mover hacia arriba
            mostrarMenu();
        }
        delay(200); 
    }
    lastUpButtonState = upButtonState;

    // Manejar el boton DOWN
    if (lastDownButtonState == HIGH && downButtonState == LOW) {
        if (isMenuActive) {
            currentMenuIndex = (currentMenuIndex + 1) % menuSize; // Mover hacia abajo
            mostrarMenu();
        }
        delay(200);
    }
    lastDownButtonState = downButtonState;
}

void seleccionarOpcion() {
    lcd.clear();

    if (currentMenuIndex == 0) {
        lcd.print("Iniciando...");
        digitalWrite(relayPin, HIGH);  // Activar el rele
        coffeeMakerOn = true;          // La cafetera esta encendida
        delay(2000);
        coffeeMakerOn = false;
        digitalWrite(relayPin, LOW);   // Apagar el rele
    } else if (currentMenuIndex == 1) {
        ProgramarHora();
    } else if (currentMenuIndex == 2) {
        lcd.print("Configurando...");
        delay(2000);
    }
    isMenuActive = false; // Salir del menu
}

void ProgramarHora() {
    unsigned long tiempoParpadeo = 0;
    bool parpadear = false;

    // Variables locales para el estado de los botones
    int lastOkButtonState = HIGH;
    int lastUpButtonState = HIGH;
    int lastDownButtonState = HIGH;

    // Iniciamos el modo en 0 (0: horas, 1: minutos)
    int modo = 0;

    // Variables para almacenar la hora a programar
    int horaProgramada, minutoProgramado;

    // Obtener la hora actual para inicializar
    Ds1302::DateTime now;
    rtc.getDateTime(&now);

    horaProgramada = now.hour;
    minutoProgramado = now.minute;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Programar hora:");

    while (true) {
        // Leer el estado de los botones
        int okButtonState = digitalRead(okButtonPin);
        int upButtonState = digitalRead(upButtonPin);
        int downButtonState = digitalRead(downButtonPin);

        // Cambio de modo con boton OK
        if (lastOkButtonState == HIGH && okButtonState == LOW) {
            modo++;
            if (modo > 1) {
                break;
            }
            delay(200); 
        }
        lastOkButtonState = okButtonState;

        // Incrementar valor con boton UP
        if (lastUpButtonState == HIGH && upButtonState == LOW) {
            if (modo == 0) {
                horaProgramada = (horaProgramada + 1) % 24;
            } else if (modo == 1) {
                minutoProgramado = (minutoProgramado + 1) % 60;
            }
            delay(200); 
        }
        lastUpButtonState = upButtonState;

        // Decrementar valor con boton DOWN
        if (lastDownButtonState == HIGH && downButtonState == LOW) {
            if (modo == 0) {
                horaProgramada = (horaProgramada + 23) % 24; // Decrementar hora
            } else if (modo == 1) {
                minutoProgramado = (minutoProgramado + 59) % 60; // Decrementar minuto
            }
            delay(200); 
        }
        lastDownButtonState = downButtonState;

        // Control del parpadeo
        if (millis() - tiempoParpadeo >= 500) {
            parpadear = !parpadear;
            tiempoParpadeo = millis();
        }

        // Mostrar la hora programada
        lcd.setCursor(0, 1);
        char timeBuffer[6]; // "HH:MM"

        if (modo == 0 && parpadear) {
            snprintf(timeBuffer, sizeof(timeBuffer), "  :%02d", minutoProgramado);
        } else if (modo == 1 && parpadear) {
            snprintf(timeBuffer, sizeof(timeBuffer), "%02d:  ", horaProgramada);
        } else {
            snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d", horaProgramada, minutoProgramado);
        }
        lcd.print(timeBuffer);

        delay(100); 
    }

    // Establecer la nueva hora en el RTC
    Ds1302::DateTime newTime = {
        .year = now.year,
        .month = now.month,
        .day = now.day,
        .hour = horaProgramada,
        .minute = minutoProgramado,
        .second = 0,
        .dow = now.dow
    };
    rtc.setDateTime(&newTime);

    // Mostrar confirmacion
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hora actualizada:");
    lcd.setCursor(0, 1);
    char timeBuffer[6];
    snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d", horaProgramada, minutoProgramado);
    lcd.print(timeBuffer);
    delay(2000);
}

void configurarHora() {
    // Variables para almacenar los valores de año, mes, dia, hora, minuto y segundo
    int anio, mes, dia, hora, minuto, segundo;
    int modo = 0; // 0: año, 1: mes, 2: dia, 3: hora, 4: minuto, 5: segundo

    // Variables locales para el estado de los botones
    int lastOkButtonState = HIGH;
    int lastUpButtonState = HIGH;
    int lastDownButtonState = HIGH;

    // Obtener la fecha y hora actuales del RTC
    Ds1302::DateTime now;
    rtc.getDateTime(&now);

    // Inicializar las variables con los valores actuales
    anio = now.year;
    mes = now.month;
    dia = now.day;
    hora = now.hour;
    minuto = now.minute;
    segundo = now.second;

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Configurar Hora:");

    unsigned long tiempoParpadeo = 0;
    bool parpadear = false;

    while (true) {
        // Leer el estado de los botones
        int okButtonState = digitalRead(okButtonPin);
        int upButtonState = digitalRead(upButtonPin);
        int downButtonState = digitalRead(downButtonPin);

        // Cambio de modo con boton OK
        if (lastOkButtonState == HIGH && okButtonState == LOW) {
            modo++;
            if (modo > 5) {
                break;
            }
            delay(200); 
        }
        lastOkButtonState = okButtonState;

        // Incrementar valor con boton UP
        if (lastUpButtonState == HIGH && upButtonState == LOW) {
            switch (modo) {
                case 0: anio = (anio + 1) % 100; break;       // Años de 0 a 99
                case 1: mes = mes % 12 + 1; break;            // Meses de 1 a 12
                case 2: dia = dia % 31 + 1; break;            // Dias de 1 a 31
                case 3: hora = (hora + 1) % 24; break;        // Horas de 0 a 23
                case 4: minuto = (minuto + 1) % 60; break;    // Minutos de 0 a 59
                case 5: segundo = (segundo + 1) % 60; break;  // Segundos de 0 a 59
            }
            delay(200);
        }
        lastUpButtonState = upButtonState;

        // Decrementar valor con botón DOWN
        if (lastDownButtonState == HIGH && downButtonState == LOW) {
            switch (modo) {
                case 0: anio = (anio + 99) % 100; break;        // Retroceder años
                case 1: mes = (mes + 10) % 12 + 1; break;       // Retroceder meses
                case 2: dia = (dia + 29) % 31 + 1; break;       // Retroceder dias
                case 3: hora = (hora + 23) % 24; break;         // Retroceder horas
                case 4: minuto = (minuto + 59) % 60; break;     // Retroceder minutos
                case 5: segundo = (segundo + 59) % 60; break;   // Retroceder segundos
            }
            delay(200);
        }
        lastDownButtonState = downButtonState;

        // Control del parpadeo
        if (millis() - tiempoParpadeo >= 500) {
            parpadear = !parpadear;
            tiempoParpadeo = millis();
        }

        // Mostrar los valores actuales con parpadeo en el campo editable
        // Linea de fecha
        lcd.setCursor(0, 1);
        lcd.print("Fecha: ");

        // Usamos buffers para formatear las cadenas
        char buffer[3];

        // Dia
        if (modo == 2 && parpadear) {
            lcd.print("  ");
        } else {
            snprintf(buffer, sizeof(buffer), "%02d", dia);
            lcd.print(buffer);
        }
        lcd.print("/");

        // Mes
        if (modo == 1 && parpadear) {
            lcd.print("  ");
        } else {
            snprintf(buffer, sizeof(buffer), "%02d", mes);
            lcd.print(buffer);
        }
        lcd.print("/");

        // Año
        if (modo == 0 && parpadear) {
            lcd.print("  ");
        } else {
            snprintf(buffer, sizeof(buffer), "%02d", anio);
            lcd.print(buffer);
        }

        // Linea de hora
        lcd.setCursor(0, 2);
        lcd.print("Hora: ");

        // Hora
        if (modo == 3 && parpadear) {
            lcd.print("  ");
        } else {
            snprintf(buffer, sizeof(buffer), "%02d", hora);
            lcd.print(buffer);
        }
        lcd.print(":");

        // Minuto
        if (modo == 4 && parpadear) {
            lcd.print("  ");
        } else {
            snprintf(buffer, sizeof(buffer), "%02d", minuto);
            lcd.print(buffer);
        }
        lcd.print(":");

        // Segundo
        if (modo == 5 && parpadear) {
            lcd.print("  ");
        } else {
            snprintf(buffer, sizeof(buffer), "%02d", segundo);
            lcd.print(buffer);
        }

        delay(100); // Pequeño retraso para actualizacion
    }

    // Establecer la nueva fecha y hora en el RTC
    Ds1302::DateTime newTime = {
        .year = anio,
        .month = mes,
        .day = dia,
        .hour = hora,
        .minute = minuto,
        .second = segundo,
        .dow = now.dow // Puedes calcular el dia de la semana si lo deseas
    };
    rtc.setDateTime(&newTime);

    // Mostrar confirmacion
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Hora Actualizada!");
    delay(2000);
}

long medirDistancia() {
    long duracion;
    long cm;

    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);

    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);

    duracion = pulseIn(echoPin, HIGH, 30000); 

    // Calcular distancia en cm
    cm = duracion * 0.034 / 2;

    return cm;
}
