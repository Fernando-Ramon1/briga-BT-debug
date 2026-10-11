/*/=========================================================================================================
===========================================================================================================
=====================================     //    BRIGA RSM   //    =============================================
==========================================================================================================
========================================================================================================*/

#pragma region Includes

#include "defines.hpp"
#include "modo_auto.hpp"
#include "modo_rc.hpp"
#include "move.hpp"
#include "sensor_task.hpp"

// ======================================|| CONFIG ||=======================================
#pragma region Config

void iniciarTodosVermelhos()
{
    // A FastLED possui a função fill_solid que preenche todo o array de uma vez
    fill_solid(leds, 5, CRGB::Red);
    FastLED.show(); // Envia o comando para acender a fita
}

void config()
{
    // --- MOTORES ---
    pinMode(Motor_Esquerdo_Positivo, OUTPUT);
    pinMode(Motor_Esquerdo_Negativo, OUTPUT);
    pinMode(Motor_Direito_Positivo, OUTPUT);
    pinMode(Motor_Direito_Negativo, OUTPUT);

    // --- SENSORES ---
    pinMode(jSumoE, INPUT);
    pinMode(jSumoD, INPUT);
    pinMode(receptorE, INPUT);
    pinMode(receptorD, INPUT);
    pinMode(linhaE, INPUT);
    pinMode(linhaD, INPUT);
    pinMode(pinoLDR, INPUT);

    // --- LEDS e EXTRAS ---
    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);

    FastLED.addLeds<WS2812, leds_end, GRB>(leds, 5);
    FastLED.setBrightness(100); // Brilho em 50 para evitar pico de corrente
    iniciarTodosVermelhos();    // Inicia o código com os 5 LEDs vermelhos

    // --- IR ---
    IrReceiver.begin(IR, DISABLE_LED_FEEDBACK, led);

    // --- SERVO ---
    servoBandeira.setPeriodHertz(SERVO_FREQ_HZ);
    servoBandeira.attach(SERVOMOTOR_PIN, SERVO_MIN_PULSE_US, SERVO_MAX_PULSE_US);
    servoBandeira.write(SERVO_ANGULO_INICIAL);
}

#pragma endregion

// ==========================================================
// SETUP PRINCIPAL
// ==========================================================

#pragma region Setup MAIN

void setup()
{
    Serial.begin(115200);
    config();
    // Cria a protecao de PWM/STOP antes de iniciar a task de sensores.
    travaMotoresAuto = xSemaphoreCreateMutex();
    if (travaMotoresAuto == nullptr)
    {
        Serial.println("ERRO: nao foi possivel criar a trava dos motores");
        while (true) delay(1000);
    }
    iniciaTaskSensores();
}

#pragma endregion

// ==========================================================
// LOOP PRINCIPAL
// ==========================================================

#pragma region Loop MAIN

void loop()
{

    while (modoSelecionado == NaoSelecionado)
    {
        uint8_t brilhoPulsante = beatsin8(30, 40, 120);
        fill_solid(leds, 5, CHSV(0, 255, brilhoPulsante));
        FastLED.show();

        /*static unsigned long lastBlink = 0;
        if (millis() - lastBlink > 500) // Piscar do Led
        {
            lastBlink = millis();
            digitalWrite(led, !digitalRead(led)); // Lê o estado, inverte e escreve
        }*/

        if (IrReceiver.decode())
        {

            IrReceiver.resume();

            if (IrReceiver.decodedIRData.command == botao8) // Botão 8 -> MODO AUTO
            {
                digitalWrite(led, HIGH);
                LedFicaVerde(0);
                LedFicaVerde(1);
                vTaskDelay(pdMS_TO_TICKS(1500));
                modoSelecionado = ModoAuto;
            }
            else if (IrReceiver.decodedIRData.command == botao9) // Botão 9 -> MODO RC
            {
                digitalWrite(led, HIGH);
                vTaskDelay(pdMS_TO_TICKS(1500));
                modoSelecionado = ModoRc;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }

    if (modoSelecionado == ModoAuto)
    {
        setupAuto();
        loopAuto();
    }
    else if (modoSelecionado == ModoRc)
    {
        setupRC();
        loopRC();
    }
}

#pragma endregion
