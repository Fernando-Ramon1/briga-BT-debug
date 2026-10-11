//===============================================================================================//
//=====================================//INCLUDES E DEFINES//====================================//
//===============================================================================================//
#pragma region INCLUDES

#ifndef MOVE_H
#define MOVE_H

#include <defines.hpp>
#include "debug.hpp"

#pragma endregion

//===============================================================================================//
//========================================//MOVIMENTACAO//=======================================//
//===============================================================================================//

#pragma region PWM

void moverMotores(int velEsq, int velDir)
{
    // Serializa o PWM com o STOP. Uma manobra atrasada nao pode voltar a andar depois da parada.
    bool protegerAuto = modoSelecionado == ModoAuto && travaMotoresAuto != nullptr;
    if (protegerAuto) xSemaphoreTake(travaMotoresAuto, portMAX_DELAY);
    if (protegerAuto && estaParadoDebug()) { velEsq = 0; velDir = 0; }
    // Variáveis estáticas lembram o último comando enviado mesmo após a função terminar
    static int ultimaVelEsq = 0;
    static int ultimaVelDir = 0;

    // Checa se algum dos motores inverteu a rotação:
    bool inverteuDir = (velDir > 0 && ultimaVelDir < 0) || (velDir < 0 && ultimaVelDir > 0);
    bool inverteuEsq = (velEsq > 0 && ultimaVelEsq < 0) || (velEsq < 0 && ultimaVelEsq > 0);

    if (inverteuDir || inverteuEsq) // Se algum deles inverteu a rotação, limpamos o PWM:
    {
        analogWrite(Motor_Direito_Positivo, LOW);
        analogWrite(Motor_Direito_Negativo, LOW);
        analogWrite(Motor_Esquerdo_Positivo, LOW);
        analogWrite(Motor_Esquerdo_Negativo, LOW);
        delayMicroseconds(60);
    }

    // --- ACIONAMENTO MOTOR DIREITO ---
    if (velDir > 0)
    {
        analogWrite(Motor_Direito_Positivo, abs(velDir));
        analogWrite(Motor_Direito_Negativo, LOW);
    }
    else if (velDir < 0)
    {
        analogWrite(Motor_Direito_Negativo, abs(velDir));
        analogWrite(Motor_Direito_Positivo, LOW);
    }
    else if (velDir == 0)
    {
        analogWrite(Motor_Direito_Positivo, 255); // mudar
        analogWrite(Motor_Direito_Negativo, 255); // mudar
    }

    // --- ACIONAMENTO MOTOR ESQUERDO ---
    if (velEsq > 0)
    {
        analogWrite(Motor_Esquerdo_Positivo, abs(velEsq));
        analogWrite(Motor_Esquerdo_Negativo, LOW);
    }
    else if (velEsq < 0)
    {
        analogWrite(Motor_Esquerdo_Negativo, abs(velEsq));
        analogWrite(Motor_Esquerdo_Positivo, LOW);
    }

    else if (velEsq == 0)
    {
        analogWrite(Motor_Esquerdo_Positivo, 255); // mudar
        analogWrite(Motor_Esquerdo_Negativo, 255); // mudar
    }
    // Atualiza o histórico para o próximo ciclo
    ultimaVelEsq = velEsq;
    ultimaVelDir = velDir;
    registrarMotoresDebug(velEsq, velDir); // Registra o comando aplicado; repeticoes sao filtradas pelo gravador.
    if (protegerAuto) xSemaphoreGive(travaMotoresAuto);
}

// Usa a mesma trava dos motores para impedir novo comando do servo apos o STOP de debug.
void moverBandeiraAuto(int angulo)
{
    xSemaphoreTake(travaMotoresAuto, portMAX_DELAY);
    if (!estaParadoDebug()) servoBandeira.write(angulo);
    xSemaphoreGive(travaMotoresAuto);
}

// Retorna true quando tratou o STOP sem reset. Sem debug, o chamador para e reinicia a placa.
bool tratarParadaDebug()
{
    if (modoSelecionado != ModoAuto) return false;
    xSemaphoreTake(travaMotoresAuto, portMAX_DELAY);
    portENTER_CRITICAL(&travaDebug);
    bool ativo = debugLigado;
    if (ativo && !debugParado) { debugParado = true; instanteParadaDebug = esp_timer_get_time(); }
    portEXIT_CRITICAL(&travaDebug);
    if (ativo) servoBandeira.detach(); // Interrompe os pulsos; nao trava mecanicamente a bandeira.
    xSemaphoreGive(travaMotoresAuto);
    if (!ativo) return false;
    // Aplica zero antes de registrar a parada e congelar o historico.
    moverMotores(0, 0);
    portENTER_CRITICAL(&travaDebug);
    faseAutoSite = "PARADO_DEBUG";
    gravarDebug(DEBUG_FASE, "STOP_IR_MOTORES_BLOQUEADOS", estadoParcialDebug ? -1 : instanteParadaDebug - entradaEstadoDebug);
    debugGravando = false;
    portEXIT_CRITICAL(&travaDebug);
    return true;
}

#pragma endregion

#pragma region BUSCAS

//=====================================|| BUSCA OFENSIVA ||=======================================================

void buscaOfensiva()
{

    if (valorJsumoF == HIGH || valorJsumoE == HIGH || valorJsumoD == HIGH)
    {
        if (!atacando)
        {
            inicioDoAtaque = millis();
            atacando = true;
        }

        unsigned long tempoDeAtaque = millis() - inicioDoAtaque;

        // Define a velocidade máxima baseado no tempo do ciclo do ataque
        int velMax = (tempoDeAtaque < 500) ? 70 : 255;
        int velReduzida = velMax / 2;

        // 1. Oponente Centralizado
        if (valorJsumoF == HIGH && (valorJsumoE == HIGH && valorJsumoD == HIGH))
        {
            moverMotores(velMax, velMax);
        }
        // 2. Oponente na Diagonal Direita
        else if (valorJsumoF == HIGH && valorJsumoD == HIGH)
        {
            if (tempoDeAtaque < 300)
            {
                moverMotores(velMax, velReduzida);
            }
            else
            {
                moverMotores(velMax, velMax);
            }
        }
        // 3. Oponente na Diagonal Esquerda
        else if (valorJsumoF == HIGH && valorJsumoE == HIGH)
        {
            if (tempoDeAtaque < 300)
            {
                moverMotores(velReduzida, velMax);
            }
            else
            {
                moverMotores(velMax, velMax);
            }
        }
        // 4. Oponente apenas na frente
        else if (valorJsumoF == HIGH)
        {
            moverMotores(velMax, velMax);
        }
        // 5. Oponente apenas na Direita:
        else if (valorJsumoD == HIGH)
        {
            moverMotores(velMax, 0);
        }
        // 6. Oponente apenas na Esquerda:
        else if (valorJsumoE == HIGH)
        {
            moverMotores(0, velMax);
        }
    }
    else
    {
        // 7. Openente não detectado:
        atacando = false;

        if (lado == direita)
        {
            moverMotores(100, -100);
        }
        else if (lado == esquerda)
        {
            moverMotores(-100, 100);
        }
    }
}

// Os registros acompanham a tatica. Cada fuga tem inicio, etapas e fim identificados.
// mudarEstadoDebug() faz a atribuicao do estado e guarda a duracao do anterior.
void buscaCorna()
{

    if (valorLinhaE < 500 && valorLinhaE > 0)
    {
        iniciarLinhaDebug("LINHA_E_MENOR_500");
        etapaLinhaDebug("RECUO");
        moverMotores(-255, -255); // Ré rápida
        delay(180);
        etapaLinhaDebug("GIRO_DIREITA");
        moverMotores(255, -255); // Gira para a direita
        delay(100);
        mudarEstadoDebug(AVANCAR_LENTO, "FIM_LINHA_E");
        terminarLinhaDebug();
    }
    else if (valorLinhaD < 500 && valorLinhaD > 0)
    {
        iniciarLinhaDebug("LINHA_D_MENOR_500");
        etapaLinhaDebug("RECUO");
        moverMotores(-255, -255); // Ré rápida
        delay(180);
        etapaLinhaDebug("GIRO_ESQUERDA");
        moverMotores(-255, 255); // Gira para a esquerda
        delay(100);
        mudarEstadoDebug(AVANCAR_LENTO, "FIM_LINHA_D");
        terminarLinhaDebug();
    }

    if (valorLDR < 100)
    {
        mudarEstadoDebug(ATAQUE_RAPIDO, "LDR_MENOR_100");
    }
    else if (estadoAtual != ATAQUE_RAPIDO)
    {
        if (valorJsumoE)
        {
            mudarEstadoDebug(VIRAR_LATERAL_E, "JSUMO_E");
        }
        else if (valorJsumoD)
        {
            mudarEstadoDebug(VIRAR_LATERAL_D, "JSUMO_D");
        }
    }

    // Máquina de Estados
    switch (estadoAtual)
    {

    case ALINHAR_FRENTE:
        if ((!valorReceptorE && !valorReceptorD) || (valorReceptorE && valorReceptorD))
        {
            moverMotores(0, 0); // Para os motores
            tempoTravamento = millis();
            mudarEstadoDebug(ESPERAR_ALVO, "IRS_IGUAIS_ALINHANDO");
        }
        else if (valorReceptorE)
        {
            moverMotores(-100, 100); // Ajuste fino para a esquerda
        }
        else if (valorReceptorD)
        {
            moverMotores(100, -100); // Ajuste fino para a direita
        }
        break;

    case ESPERAR_ALVO:
        moverMotores(0, 0); // Mantém parado

        // Aguarda exatos 300ms antes de agir
        if (millis() - tempoTravamento >= 300)
        {
            digitalWrite(2, LOW);
            mudarEstadoDebug(AVANCAR_LENTO, "ESPERA_300MS");
        }
        break;

    case AVANCAR_LENTO:
        if (valorReceptorE && valorReceptorD)
        {
            mudarEstadoDebug(ESPERAR_ALVO, "DOIS_IRS_AVANCANDO");
        }
        else if (valorReceptorE && !valorReceptorD)
        {
            mudarEstadoDebug(ALINHAR_FRENTE, "SOMENTE_IR_E");
        }
        else if (valorReceptorD && !valorReceptorE)
        {
            mudarEstadoDebug(ALINHAR_FRENTE, "SOMENTE_IR_D");
        }
        else
        {
            moverMotores(40, 40);
        }
        break;

    case VIRAR_LATERAL_E:
        moverMotores(-255, 255); // Gira rápido à esquerda (JSumo viu)
        vTaskDelay(20);
        if (!valorJsumoE)
        {
            mudarEstadoDebug(AVANCAR_LENTO, "PERDEU_JSUMO_E");
        }
        break;

    case VIRAR_LATERAL_D:
        moverMotores(255, -255); // Gira rápido à direita (JSumo viu)
        vTaskDelay(20);
        if (!valorJsumoD)
        {
            mudarEstadoDebug(AVANCAR_LENTO, "PERDEU_JSUMO_D");
        }
        break;

    case ATAQUE_RAPIDO:
        moverMotores(255, 255); // Avança com tudo (adversário rampado)
        if (valorLDR >= 100)
        {
            mudarEstadoDebug(AVANCAR_LENTO, "LDR_MAIOR_IGUAL_100");
        }
        break;
    }
}

#endif