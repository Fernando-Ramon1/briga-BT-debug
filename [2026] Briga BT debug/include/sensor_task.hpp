//===============================================================================================//
//=====================================//TASK SENSORES//=========================================//
//===============================================================================================//

#pragma region INCLUDES

#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include "defines.hpp"
#include "move.hpp"

#pragma endregion

void vTaskSensores(void *pvParameters)
{
    for (;;)
    {
        // Na preparacao, publica sensores para o site sem disputar o IR com setupAuto().
        bool lerParaSite = false;
        if (modoSelecionado == ModoAuto)
        {
            portENTER_CRITICAL(&travaDebug);
            lerParaSite = lendoSensoresSite;
            portEXIT_CRITICAL(&travaDebug);
        }
        if (running_Sensors || lerParaSite)
        {
            // Depois da largada, o STOP e recebido por esta task.
            if (running_Sensors && IrReceiver.decode())
            {
                int comandoRecebido = IrReceiver.decodedIRData.command;
                IrReceiver.resume();
                if (comandoRecebido == botao3)
                {
                    digitalWrite(led, LOW); // Apaga o led
                    if (!tratarParadaDebug())
                    {
                        moverMotores(0, 0);
                        ESP.restart();
                    }
                }
            }

            valorJsumoE = digitalRead(jSumoE);
            valorJsumoD = digitalRead(jSumoD);

        
            valorReceptorE = !(digitalRead(receptorE));
            valorReceptorD = !(digitalRead(receptorD));

            valorLinhaE = analogRead(linhaE);
            valorLinhaD = analogRead(linhaD);
            valorLDR = analogRead(pinoLDR);

            // Publica uma copia completa para consulta, sem reler os sensores pelo BLE.
            if (modoSelecionado == ModoAuto)
            {
                portENTER_CRITICAL(&travaDebug);
                sensoresDebug.linhaEsquerda = valorLinhaE;
                sensoresDebug.linhaDireita = valorLinhaD;
                sensoresDebug.ldr = valorLDR;
                sensoresDebug.receptorEsquerdo = valorReceptorE;
                sensoresDebug.receptorDireito = valorReceptorD;
                sensoresDebug.jsumoEsquerdo = valorJsumoE;
                sensoresDebug.jsumoDireito = valorJsumoD;
                sensoresDebug.instante = esp_timer_get_time();
                portEXIT_CRITICAL(&travaDebug);
            }

            if (!running_Sensors) { vTaskDelay(pdMS_TO_TICKS(5)); continue; } // Nao sobrescreve o lado escolhido no menu antes da largada.
            if ((valorJsumoE) || (valorReceptorE))
            {
                lado = esquerda;
            }
            else if ((valorJsumoD) || (valorReceptorD))
            {
                lado = direita;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void iniciaTaskSensores()
{
    xTaskCreatePinnedToCore(vTaskSensores, "SensorTask", 10000, NULL, 1, &SensorTask, 0);
}

#endif