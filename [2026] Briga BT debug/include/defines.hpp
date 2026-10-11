//===============================================================================================//
//=====================================//INCLUDES E DEFINES//====================================//
//===============================================================================================//

#pragma region INCLUDES

#ifndef DEFINES_H
#define DEFINES_H

#include <Arduino.h>

#include <IRremote.hpp>
#include <ESP32Servo.h>
#include <FastLED.h>

#pragma endregion

//=====================================//Objetos e Handles//=====================================//

#pragma region OBJETOS E HANDLES


Servo servoBandeira;

TaskHandle_t SensorTask; // Avisando o ESP que teremos uma task

//=========================================//Bluetooth//=========================================//

// Verifica se o Bluetooth esta devidamente habilitado no menuconfig

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run make menuconfig to and enable it
#endif // Finaliza o condicional do erro

#pragma endregion

//===============================================================================================//
//=====================================//PINOS E VARIAVEIS//=====================================//
//===============================================================================================//

//==========================================//Motores//==========================================//

#pragma region PINOS MOTORES

#define Motor_Direito_Positivo 16  // 17    fuego 16
#define Motor_Direito_Negativo 17  // 16    fugego 17 
#define Motor_Esquerdo_Positivo 18 // 18    fuegob 18
#define Motor_Esquerdo_Negativo 19 // 19   fuego 19

#pragma endregion

//========================================//Servomotor//========================================//

#pragma region PINOS SERVO

#define SERVOMOTOR_PIN 26 // 26
#define SERVO_ANGULO_INICIAL 90
#define SERVO_FREQ_HZ 50
#define SERVO_RESOLUTION 16
#define SERVO_MIN_PULSE_US 544  //  Largura de pulso para 0 graus (µs)
#define SERVO_MAX_PULSE_US 2400 //  Largura de pulso para 180 graus (µs)

#pragma endregion

//========================================//Comunicacao//========================================//

#pragma region COMUNICACAO

#define leds_end 33
#define led 2
#define IR 13

#define botao1 0x0
#define botao2 0x1
#define botao3 0x2 // Botões do controle remoto
#define botao8 0x7
#define botao9 0x8

#pragma endregion

//=======================================//Sensoreamento//=======================================//

#pragma region SENSOREAMENTO

#define jSumoD 27 //15 
#define jSumoE 23 //14
#define receptorD 4  //5
#define receptorE 5 //23
#define linhaE 39 //36
#define linhaD 34 //39
#define pinoLDR 36 //34

#pragma endregion

//======================================//Enums//======================================//

#pragma region ENUMS

enum Direction
{
  esquerda,
  direita,
  frente
};

enum Bandeira
{
  B_Esquerda,
  B_Direita
};

enum SelecionaModo
{
  NaoSelecionado,
  ModoAuto,
  ModoRc
};

enum SelecionaBusca
{
  Corna
};


enum EstadoRobo
{
  BUSCAR,
  ALINHAR_FRENTE,
  ESPERAR_ALVO,
  AVANCAR_LENTO,
  VIRAR_LATERAL_E,
  VIRAR_LATERAL_D,
  ATAQUE_RAPIDO
};

#pragma endregion

//===================================//Variaveis utilizadas//====================================//

#pragma region VARIAVEIS

Direction lado = esquerda;
Bandeira lado_Bandeira = B_Direita;
SelecionaModo modoSelecionado = ModoAuto;
SelecionaBusca ModoBusca = Corna;
EstadoRobo estadoAtual = AVANCAR_LENTO;

volatile bool valorJsumoF, valorJsumoE, valorJsumoD;
volatile bool valorReceptorF, valorReceptorE, valorReceptorD;
// Leituras atualizadas pela task de sensores. O quadro enviado ao site tem trava propria.
volatile int valorLinhaE, valorLinhaD;
volatile int valorLDR;
bool medirLDR = true;
int ldrReferencia = 0;
long somaLDR = 0;

CRGB leds[5];

char BT = 'A';         // char que é recebida pelo bluetooth
char estrategia = 'z'; // Sem manobra inicial; a busca comeca normalmente apos a largada.
volatile bool running_Sensors = false; // Depois da largada, a task tambem passa a ler o controle IR.
bool atacando = false;
bool flagPrimeiraVez = true; // Busca IR
unsigned long tempoInicioIR, tempoIncremento;
unsigned long inicioDoAtaque = 0;
unsigned long inicioDefensivo = 0;
int num_led = 1;

// Variáveis Busca Defensiva
unsigned long tempoCicloJsumo = 0;
bool jsumoEmPulso = false;
unsigned long tempoAlinhamento = 0;
bool alvoAlinhado = false;

// BuscaCorna
unsigned long tempoBusca = 0;
unsigned long tempoTravamento = 0;

#pragma endregion

#endif
