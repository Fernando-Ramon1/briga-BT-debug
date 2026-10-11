#ifndef DEBUG_HPP
#define DEBUG_HPP
#include "defines.hpp"
#include <esp_timer.h>
#include <esp_system.h>
#include <freertos/semphr.h>
#include <limits.h>
#include <cstdio>
#include <cstring>

//=====================================//DEBUG DO AUTO//=========================================//

// Os eventos ficam na RAM. O site consulta depois, sem prints dentro da busca.
const int TAMANHO_HISTORICO = 256;
enum TipoDebug
{
    DEBUG_ESTADO,
    DEBUG_MOTOR,
    DEBUG_LINHA_INICIO,
    DEBUG_LINHA_ETAPA,
    DEBUG_LINHA_FIM,
    DEBUG_FASE,
    DEBUG_MARCA
};

// Ultimo quadro publicado pela task. O log usa essa copia, sem reler os sensores.
// Instantes e duracoes usam microssegundos do relogio do ESP32.
struct SensoresDebug
{
    int linhaEsquerda = 0;
    int linhaDireita = 0;
    int ldr = 0;
    bool receptorEsquerdo = false;
    bool receptorDireito = false;
    bool jsumoEsquerdo = false;
    bool jsumoDireito = false;
    long long instante = 0;
};

struct RegistroDebug
{
    unsigned long numero = 0;
    unsigned long ciclo = 0;
    unsigned long recuperacao = 0; // Zero indica que o evento ocorreu fora de uma fuga da linha.
    long long instante = 0;
    long long duracao = -1; // Microssegundos; -1 indica duracao parcial ou nao aplicavel.
    TipoDebug tipo = DEBUG_MARCA;
    EstadoRobo anterior = AVANCAR_LENTO;
    EstadoRobo atual = AVANCAR_LENTO;
    int motorEsquerdo = 0; // Comando de PWM, nao velocidade medida.
    int motorDireito = 0;
    SensoresDebug sensores; // A amostra tem horario proprio; pode ser anterior a decisao.
    char motivo[48] = {}; // Ate 47 caracteres ASCII mais o terminador.
};

RegistroDebug historicoDebug[TAMANHO_HISTORICO];
SensoresDebug sensoresDebug;
portMUX_TYPE travaDebug = portMUX_INITIALIZER_UNLOCKED;
SemaphoreHandle_t travaMotoresAuto = nullptr; // Serializa PWM, servo e STOP no Auto.
bool debugLigado = false;
bool debugGravando = false;
bool debugParado = false; // Depois do STOP, somente reset libera um novo ensaio.
bool lendoSensoresSite = false; // Permite ler sensores antes da largada sem consumir o IR do menu.
const char *faseAutoSite = "CONFIGURANDO";
EstadoRobo estadoObservadoDebug = AVANCAR_LENTO;
unsigned long sessaoDebug = 0; // Identifica o boot para o site; nao e uma senha.
unsigned long capturaDebug = 0;
unsigned long totalDebug = 0;
int quantidadeDebug = 0;
unsigned long sobrescritosDebug = 0;
unsigned long cicloDebug = 0;
unsigned long recuperacoesDebug = 0;
unsigned long recuperacaoDebug = 0;
long long instanteParadaDebug = 0; // Fecha o tempo do estado no STOP.
long long entradaEstadoDebug = 0;
long long inicioCapturaDebug = 0;
long long inicioLinhaDebug = 0;
long long ultimoCicloDebug = 0;
long long duracaoCicloDebug = 0;
long long maiorCicloDebug = 0;
bool estadoParcialDebug = true; // O inicio do estado nao foi observado por completo nesta captura.
int motorEsquerdoDebug = 0;
int motorDireitoDebug = 0;
bool motorConhecidoDebug = false;
EstadoRobo estadoMotorDebug = AVANCAR_LENTO;
unsigned long recuperacaoMotorDebug = 0;

// Consulta compartilhada entre o controle, a task de sensores e o site.
bool estaParadoDebug()
{
    portENTER_CRITICAL(&travaDebug);
    bool parado = debugParado;
    portEXIT_CRITICAL(&travaDebug);
    return parado;
}

// Chamar somente com travaDebug adquirida. Nao transmite nem monta JSON aqui.
void gravarDebug(TipoDebug tipo, const char *motivo, long long duracao = -1, EstadoRobo anterior = AVANCAR_LENTO)
{
    if (!debugLigado || !debugGravando) return;
    if (totalDebug == ULONG_MAX) { debugGravando = false; return; } // Evita reutilizar numeros de evento apos estouro.
    RegistroDebug registro;
    registro.numero = ++totalDebug;
    registro.instante = esp_timer_get_time();
    registro.duracao = duracao;
    registro.tipo = tipo;
    registro.anterior = anterior;
    registro.atual = estadoObservadoDebug;
    registro.ciclo = cicloDebug;
    registro.recuperacao = recuperacaoDebug;
    registro.motorEsquerdo = motorEsquerdoDebug;
    registro.motorDireito = motorDireitoDebug;
    registro.sensores = sensoresDebug;
    snprintf(registro.motivo, sizeof(registro.motivo), "%s", motivo);
    historicoDebug[(registro.numero - 1) % TAMANHO_HISTORICO] = registro; // Ao encher, substitui o registro mais antigo.
    if (quantidadeDebug < TAMANHO_HISTORICO) quantidadeDebug++;
    else sobrescritosDebug++;
}

// Limpa apenas o historico. Nao reinicia a luta nem desfaz o STOP.
void novaCapturaDebug()
{
    portENTER_CRITICAL(&travaDebug);
    if (!debugLigado || debugParado) { portEXIT_CRITICAL(&travaDebug); return; }
    capturaDebug++;
    totalDebug = 0;
    quantidadeDebug = 0;
    sobrescritosDebug = 0;
    inicioCapturaDebug = esp_timer_get_time();
    estadoParcialDebug = true;
    debugGravando = debugLigado && !debugParado;
    gravarDebug(DEBUG_MARCA, "NOVA_CAPTURA");
    portEXIT_CRITICAL(&travaDebug);
}

// Pausa ou retoma a gravacao; o controle do robo continua independente.
void pausarCapturaDebug(bool pausar)
{
    portENTER_CRITICAL(&travaDebug);
    if (pausar && debugGravando) gravarDebug(DEBUG_MARCA, "CAPTURA_PAUSADA");
    if (pausar) debugGravando = false;
    else if (debugLigado && !debugParado && !debugGravando)
    {
        debugGravando = true;
        estadoParcialDebug = true;
        inicioCapturaDebug = esp_timer_get_time();
        gravarDebug(DEBUG_MARCA, "CAPTURA_RETOMADA");
    }
    portEXIT_CRITICAL(&travaDebug);
}

// Fases do painel: configuracao, abertura, busca e parada.
void mudarFaseDebug(const char *fase)
{
    portENTER_CRITICAL(&travaDebug);
    if (!debugParado && strcmp(faseAutoSite, fase) != 0)
    {
        faseAutoSite = fase;
        gravarDebug(DEBUG_FASE, fase);
    }
    portEXIT_CRITICAL(&travaDebug);
}

// Troca o estado e registra quanto durou o anterior. Repetir o mesmo estado nao zera o tempo.
void mudarEstadoDebug(EstadoRobo novoEstado, const char *motivo)
{
    portENTER_CRITICAL(&travaDebug);
    if (debugParado) { portEXIT_CRITICAL(&travaDebug); return; }
    EstadoRobo anterior = estadoAtual;
    estadoAtual = novoEstado;
    estadoObservadoDebug = novoEstado;
    if (anterior != novoEstado)
    {
        long long agora = esp_timer_get_time();
        long long duracao = estadoParcialDebug ? -1 : agora - entradaEstadoDebug;
        gravarDebug(DEBUG_ESTADO, motivo, duracao, anterior);
        entradaEstadoDebug = agora;
        estadoParcialDebug = !debugGravando;
    }
    portEXIT_CRITICAL(&travaDebug);
}

// Ignora repeticoes, exceto se o mesmo comando vier de outro estado ou de outra fuga.
void registrarMotoresDebug(int esquerdo, int direito)
{
    if (modoSelecionado != ModoAuto) return;
    portENTER_CRITICAL(&travaDebug);
    bool mudou = !motorConhecidoDebug || esquerdo != motorEsquerdoDebug || direito != motorDireitoDebug;
    mudou = mudou || estadoMotorDebug != estadoObservadoDebug || recuperacaoMotorDebug != recuperacaoDebug;
    motorEsquerdoDebug = esquerdo;
    motorDireitoDebug = direito;
    motorConhecidoDebug = true;
    estadoMotorDebug = estadoObservadoDebug;
    recuperacaoMotorDebug = recuperacaoDebug;
    if (mudou && !debugParado) gravarDebug(DEBUG_MOTOR, faseAutoSite);
    portEXIT_CRITICAL(&travaDebug);
}

// Cada execucao da fuga recebe um numero para distinguir manobras repetidas.
void iniciarLinhaDebug(const char *motivo)
{
    portENTER_CRITICAL(&travaDebug);
    recuperacaoDebug = ++recuperacoesDebug;
    inicioLinhaDebug = esp_timer_get_time();
    gravarDebug(DEBUG_LINHA_INICIO, motivo);
    portEXIT_CRITICAL(&travaDebug);
}

void etapaLinhaDebug(const char *etapa)
{
    portENTER_CRITICAL(&travaDebug);
    gravarDebug(DEBUG_LINHA_ETAPA, etapa);
    portEXIT_CRITICAL(&travaDebug);
}

// Marca o fim do bloco de movimentos, nao a confirmacao de piso preto.
void terminarLinhaDebug()
{
    portENTER_CRITICAL(&travaDebug);
    long long duracao = inicioLinhaDebug < inicioCapturaDebug ? -1 : esp_timer_get_time() - inicioLinhaDebug;
    gravarDebug(DEBUG_LINHA_FIM, "FIM_DA_SEQUENCIA", duracao);
    recuperacaoDebug = 0;
    portEXIT_CRITICAL(&travaDebug);
}

// Mede uma chamada da BuscaCorna, incluindo os delays das manobras.
void terminarCicloDebug(long long inicio)
{
    portENTER_CRITICAL(&travaDebug);
    ultimoCicloDebug = esp_timer_get_time();
    duracaoCicloDebug = ultimoCicloDebug - inicio;
    if (duracaoCicloDebug > maiorCicloDebug) maiorCicloDebug = duracaoCicloDebug;
    portEXIT_CRITICAL(&travaDebug);
}
#endif
