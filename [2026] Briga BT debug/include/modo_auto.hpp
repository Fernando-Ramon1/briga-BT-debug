//===============================================================================================//
//======================================// MODO AUTO //==========================================//
//===============================================================================================//

#ifndef MODO_AUTO_HPP
#define MODO_AUTO_HPP

#pragma region INCLUDES
#include "defines.hpp"
#include "move.hpp"
#include "bluetooth_site.hpp"

#pragma endregion

void LedFicaVerde(int numeroDaEtapa)
{
    // Altera apenas a cor do LED da etapa correspondente
    if (numeroDaEtapa >= 0 && numeroDaEtapa < 5)
    {
        leds[numeroDaEtapa] = CRGB::Green;
        FastLED.show(); // Envia o comando atualizado
    }
}

void TodosLedsAmarelos()
{
    // A FastLED possui a função fill_solid que preenche todo o array de uma vez
    fill_solid(leds, 5, CRGB::Yellow);
    FastLED.show(); // Envia o comando para acender a fita
}

#pragma region Menu Guia
void mostrarMenu()
{
    mensagemSite("Use a pagina do Briga: abertura, lado, bandeira e debug");
}
#pragma endregion

#pragma region Function Check

void checarStatus()
{
    mensagemSite("Configuracao recebida; confira os valores confirmados no painel");
    LedFicaVerde(4);
    num_led = 5;
}
#pragma endregion

#pragma region Inicio Setup

void setupAuto()
{
    num_led = 2;
    LedFicaVerde(0);
    LedFicaVerde(1);
    for (int i = 0; i < 2; i++)
    {
        digitalWrite(led, HIGH);
        delay(250);
        digitalWrite(led, LOW);
        delay(250);
    }

    iniciarBluetoothSite();

    bool primeiraVez = true;
    while (true)
    {
        uint8_t brilhoPulsante = beatsin8(30, 40, 120);
        fill_solid(leds, num_led, CHSV(96, 255, brilhoPulsante));
        FastLED.show();

        // STOP de debug mantem o site acessivel, mas nao aceita nova largada sem reset.
        if (estaParadoDebug()) { vTaskDelay(pdMS_TO_TICKS(20)); continue; }
        portENTER_CRITICAL(&travaDebug);
        bool conectado = siteConectado;
        unsigned long aplicadoAntes = aplicadoSite;
        portEXIT_CRITICAL(&travaDebug);
        if (conectado && primeiraVez)
        {
            mostrarMenu();
            digitalWrite(led, HIGH);
            primeiraVez = false;
            LedFicaVerde(2);
            LedFicaVerde(3);
            num_led = 4;
        }
        atualizarConfiguracaoSite(); // Aplica no menu, nao na task do Bluetooth.
        portENTER_CRITICAL(&travaDebug);
        bool configurou = aplicadoSite != aplicadoAntes;
        portEXIT_CRITICAL(&travaDebug);
        if (configurou) checarStatus();
#pragma endregion

#pragma region Comandos Controle
        // Ate a largada, somente o menu decodifica o IR. Copia o comando antes de resume().
        int comandoIR = -1;
        if (IrReceiver.decode())
        {
            comandoIR = IrReceiver.decodedIRData.command;
            IrReceiver.resume();
        }
        if (comandoIR == botao1)
        {
            for (int i = 0; i < 2; i++)
            {
                digitalWrite(led, LOW);
                vTaskDelay(pdMS_TO_TICKS(80));
                if (IrReceiver.decode())
                {
                    comandoIR = IrReceiver.decodedIRData.command;
                    IrReceiver.resume();
                    if (comandoIR == botao2 || comandoIR == botao3) break;
                }
                digitalWrite(led, HIGH);
                vTaskDelay(pdMS_TO_TICKS(80));
            }
        }
        // Botao 3 tambem funciona na preparacao: com debug preserva o historico; sem debug reinicia.
        if (comandoIR == botao3)
        {
            fecharConfiguracaoSite();
            if (!tratarParadaDebug()) { moverMotores(0, 0); ESP.restart(); }
            continue;
        }
        if (comandoIR == botao2)
        {
            fecharConfiguracaoSite();
            servoBandeira.attach(SERVOMOTOR_PIN, SERVO_MIN_PULSE_US, SERVO_MAX_PULSE_US);
            digitalWrite(led, LOW);
            running_Sensors = true; // A task assume a leitura do IR e do STOP a partir da largada.
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
#pragma endregion
#pragma region Estrategias
    //=============================|| COMEÇO DE LUTA ||================================
    TodosLedsAmarelos();
    mudarFaseDebug("ABERTURA");

    // Queda da Bandeira:
    if (estrategia != 'A') // Se a estratégia for diferente do Defensivo Posicionado:
    {
        if (lado_Bandeira == B_Esquerda)
        {
            moverBandeiraAuto(180);
        }
        else if (lado_Bandeira == B_Direita)
        {
            moverBandeiraAuto(0);
        }
    }

    // Execução de movimentação inicial:
    // Aberturas usam delays. O bloqueio de motores e servo continua valendo apos o STOP.
    switch (estrategia)
    {
    case 'f': // frentão Progressivo   1/2 dohyo
        mensagemSite("FRENTAO PROGRESSIVO");
        moverMotores(48, 48);
        delay(4);
        moverMotores(60, 60);
        delay(4);
        moverMotores(72, 72);
        delay(4);
        moverMotores(84, 84);
        delay(4);
        moverMotores(96, 96);
        delay(4);
        moverMotores(128, 128);
        delay(8);
        moverMotores(160, 160);
        delay(8);
        moverMotores(196, 196);
        delay(8);
        moverMotores(228, 228);
        delay(12);
        moverMotores(255, 255);
        delay(90);
        break;

    case 'F': // Frentão Anti Pescador 1/2 dohyo

        mensagemSite("FRENTAO BOLADO");
        moverMotores(255, 255);
        delay(250);
        break;

    case 'b': // Costas

        mensagemSite("COSTAS");
        if (lado == frente)
        {
            moverMotores(-255, -255);
            delay(120);
            moverMotores(127, 127);
            delay(100);
            break;
        }
        else if (lado == esquerda)
        {
            moverMotores(-255, -255);
            delay(120);
            moverMotores(-255, 255);
            delay(50);
            moverMotores(255, 255);
            delay(160);
            moverMotores(0, 0);
            delay(10);
            lado = direita;
            break;
        }
        else if (lado == direita)
        {
            moverMotores(-255, -255);
            delay(120);
            moverMotores(255, -255);
            delay(50);
            moverMotores(255, 255);
            delay(160);
            moverMotores(0, 0);
            delay(10);
            lado = esquerda;
            break;
        }

    case 'c': // Curva Leve

        mensagemSite("CURVA LEVE");

        if (lado == esquerda)
        {
            moverMotores(-255, 255);
            delay(50);
            moverMotores(255, 255);
            delay(90);
            moverMotores(0, 0);
            delay(10);
            lado = direita;
            break;
        }
        else if (lado == direita)
        {
            moverMotores(255, -255);
            delay(50);
            moverMotores(255, 255);
            delay(90);
            moverMotores(0, 0);
            delay(10);
            lado = esquerda;
            break;
        }

    case 'C': // Curva Pesada

        mensagemSite("CURVA PESADA");

        if (lado == esquerda)
        {
            moverMotores(-255, 255);
            delay(60);
            moverMotores(255, 255);
            delay(130);
            moverMotores(0, 0);
            delay(15);
            moverMotores(255, -255);
            delay(50);
            moverMotores(255, 255);
            delay(50);
            moverMotores(0, 0);
            delay(10);
            lado = direita;
            break;
        }
        else if (lado == direita)
        {
            moverMotores(255, -255);
            delay(60);
            moverMotores(255, 255);
            delay(130);
            moverMotores(0, 0);
            delay(15);
            moverMotores(-255, 255);
            delay(50);
            moverMotores(255, 255);
            delay(50);
            moverMotores(0, 0);
            delay(10);
            lado = esquerda;
            break;
        }
    case 'z': // Basico
        mensagemSite("Basico");
        break;

    case 'e': // desempate
        if (lado == esquerda)
        {
            moverMotores(-255, 255);
            delay(130);
            break;
        }
        else if (lado == direita)
        {
            moverMotores(255, -255);
            delay(140);
            break;
        }
    case 'v':
        if (lado == esquerda)
        {
            moverMotores(-255, 255);
            delay(40);
            moverMotores(255, 255);
            delay(160);
            moverMotores(0, 0);
            delay(10);
            moverMotores(255, -255);
            delay(120);
            moverMotores(0, 0);
            delay(10);
            moverMotores(255, 255);
            delay(180);
            lado = direita;
            break;
        }
        else if (lado == direita)
        {
            moverMotores(255, -255);
            delay(40);
            moverMotores(255, 255);
            delay(160);
            moverMotores(0, 0);
            delay(10);
            moverMotores(-255, 255);
            delay(140);
            moverMotores(0, 0);
            delay(10);
            moverMotores(255, 255);
            delay(180);
            lado = esquerda;
            break;
        }
    case 'A': // Aquele lá
    {
        bool flagM = false;
        bool giroEsquerda = false, giroDireita = false;

        vTaskDelay(pdMS_TO_TICKS(80)); // Espera 80ms
        while (!flagM)
        {
            if (estaParadoDebug()) return;
            if (lado == esquerda)
            {
                moverBandeiraAuto(180);
                moverMotores(255, -255);
                delay(60);
                giroEsquerda = true;
                giroDireita = false;
                flagM = true;
            }
            else if (lado == direita)
            {
                moverBandeiraAuto(0);
                moverMotores(-255, 255);
                delay(60);
                giroDireita = true;
                giroEsquerda = false;
                flagM = true;
            }
        }
        if (giroDireita)
        {
            moverMotores(255, 255);
            delay(110);
            moverMotores(0, 0);
            delay(20);
            moverMotores(255, -255);
            delay(150);
            moverMotores(0, 0);
            delay(10);
            break;
        }
        else if (giroEsquerda)
        {
            moverMotores(255, 255);
            delay(110);
            moverMotores(0, 0);
            delay(20);
            moverMotores(-255, 255);
            delay(150);
            moverMotores(0, 0);
            delay(10);
            break;
        }
    }
    case 'd':
        if (lado == esquerda)
        {
            moverMotores(-255, 255);
            delay(60);
            moverMotores(255, 255);
            delay(150);
            moverMotores(0, 0);
            delay(10);
            moverMotores(255, -255);
            delay(30);
            lado = direita;
            break;
        }
        else if (lado == direita)
        {
            moverMotores(255, -255);
            delay(60);
            moverMotores(255, 255);
            delay(150);
            moverMotores(0, 0);
            delay(10);
            moverMotores(-255, 255);
            delay(30);
            lado = esquerda;
            break;
        }
    case 'g':
        if (lado == esquerda)
        {
            moverMotores(-255, 255);
            delay(80);
            moverMotores(0, 0);
            delay(10);
            lado = direita;
            break;
        }
        else if (lado == direita)
        {
            moverMotores(255, -255);
            delay(80);
            moverMotores(0, 0);
            delay(10);
            lado = esquerda;
            break;
        }

    // Ramo legado; 'j' nao e aceito pela configuracao do site.
    case 'j':

        while (true)
        {
            if (estaParadoDebug()) return;
            moverMotores(255, 255);
            delay(10);
        }
    }
    mudarFaseDebug("BUSCA");
}

#pragma endregion

#pragma region Loop AUTO
void loopAuto()
{

    while (true)
    {
        if (!estaParadoDebug())
        {
            // Conta ciclos e mede a busca, mesmo quando ela nao muda de estado.
            long long inicio = esp_timer_get_time();
            portENTER_CRITICAL(&travaDebug);
            cicloDebug++;
            portEXIT_CRITICAL(&travaDebug);
            buscaCorna();
            terminarCicloDebug(inicio);
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

#pragma endregion

#endif
