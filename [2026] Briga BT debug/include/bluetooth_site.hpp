#ifndef BLUETOOTH_SITE_HPP
#define BLUETOOTH_SITE_HPP
#include "debug.hpp"
#include <BLEDevice.h>
#include <BLEServer.h>

//=====================================//BLUETOOTH DO SITE//=====================================//

// WRITE recebe um pedido; READ devolve a resposta. Os mesmos UUIDs ficam em site/briga.js.
const char *servicoSite = "0000ff10-0000-1000-8000-00805f9b34fb";
const char *caracteristicaSite = "0000ff11-0000-1000-8000-00805f9b34fb";
BLECharacteristic *dadosSite = nullptr;
bool siteConectado = false;
bool configuracaoAbertaSite = true;
bool configuracaoPendenteSite = false;
char estrategiaSite = 'z';
char ladoSite = '<';
char bandeiraSite = 'r';
char avisoSite[80] = "Aguardando configuracao e largada por IR";
unsigned long pedidoSite = 0; // Pedido recebido pelo BLE.
unsigned long aplicadoSite = 0; // Pedido aplicado pelo menu do Auto.
unsigned long recusadoSite = 0; // Pedido descartado porque a configuracao foi fechada.

// Uma configuracao pendente por vez. O callback recebe; setupAuto() aplica.
struct ConfiguracaoSite
{
    char estrategia;
    char lado;
    char bandeira;
    bool debug;
    unsigned long pedido;
};
ConfiguracaoSite proximaConfiguracao;

void mensagemSite(const char *mensagem)
{
    portENTER_CRITICAL(&travaDebug);
    snprintf(avisoSite, sizeof(avisoSite), "%s", mensagem);
    portEXIT_CRITICAL(&travaDebug);
}

// Chamada pelo menu antes da largada. Nao altera a configuracao pela task BLE.
void atualizarConfiguracaoSite()
{
    portENTER_CRITICAL(&travaDebug);
    if (!configuracaoPendenteSite || !configuracaoAbertaSite || debugParado)
    {
        portEXIT_CRITICAL(&travaDebug);
        return;
    }
    ConfiguracaoSite configuracao = proximaConfiguracao;
    configuracaoPendenteSite = false;
    estrategia = configuracao.estrategia;
    lado = configuracao.lado == '<' ? esquerda : configuracao.lado == '>' ? direita : frente;
    lado_Bandeira = configuracao.bandeira == 'l' ? B_Esquerda : B_Direita;
    estrategiaSite = configuracao.estrategia;
    ladoSite = configuracao.lado;
    bandeiraSite = configuracao.bandeira;
    bool iniciarCaptura = configuracao.debug && !debugLigado;
    debugLigado = configuracao.debug;
    if (!debugLigado) debugGravando = false;
    aplicadoSite = configuracao.pedido;
    portEXIT_CRITICAL(&travaDebug);
    if (iniciarCaptura) novaCapturaDebug();
    mensagemSite("Configuracao aplicada; inicio e parada continuam pelo IR");
}

// Largada ou STOP fecham o menu e descartam qualquer pedido ainda nao aplicado.
void fecharConfiguracaoSite()
{
    portENTER_CRITICAL(&travaDebug);
    configuracaoAbertaSite = false;
    if (configuracaoPendenteSite) recusadoSite = proximaConfiguracao.pedido;
    configuracaoPendenteSite = false;
    portEXIT_CRITICAL(&travaDebug);
}

// Copia os dados sob protecao e monta a resposta depois de liberar a trava.
void respostaStatusSite(char *texto, int capacidade)
{
    portENTER_CRITICAL(&travaDebug);
    unsigned long sessao = sessaoDebug, captura = capturaDebug, total = totalDebug;
    unsigned long perdidos = sobrescritosDebug, ciclos = cicloDebug, aplicado = aplicadoSite, recusado = recusadoSite;
    bool ligado = debugLigado, gravando = debugGravando, parado = debugParado, aberta = configuracaoAbertaSite;
    long long agora = esp_timer_get_time(), entrada = entradaEstadoDebug, ultimo = ultimoCicloDebug;
    long long parada = instanteParadaDebug;
    long long duracao = duracaoCicloDebug, maior = maiorCicloDebug;
    bool parcial = estadoParcialDebug;
    int estado = estadoObservadoDebug, esquerdo = motorEsquerdoDebug, direito = motorDireitoDebug;
    const char *fase = faseAutoSite;
    char abertura = estrategiaSite, ladoInicial = ladoSite, bandeira = bandeiraSite;
    portEXIT_CRITICAL(&travaDebug);
    int n = snprintf(texto, capacidade, "{\"protocolo\":\"BRIGA_BLE_1\",\"sessao\":%lu,\"captura\":%lu,\"total\":%lu,\"perdidos\":%lu,\"debug\":%d,\"gravando\":%d,\"parado\":%d,\"aberta\":%d,\"aplicado\":%lu,\"recusado\":%lu,\"us\":%lld,\"entrada\":%lld,\"parcial\":%d,\"estado\":%d,\"fase\":\"%s\",\"mE\":%d,\"mD\":%d,\"ciclos\":%lu,\"ultimoCiclo\":%lld,\"cicloUs\":%lld,\"maiorCiclo\":%lld,\"parada\":%lld,\"config\":\"%c%c%c\"}", sessao, captura, total, perdidos, ligado, gravando, parado, aberta, aplicado, recusado, agora, entrada, parcial, estado, fase, esquerdo, direito, ciclos, ultimo, duracao, maior, parada, abertura, ladoInicial, bandeira);
    if (n < 0 || n >= capacidade) snprintf(texto, capacidade, "{\"erro\":\"resumo_excedeu_limite\"}");
}

// Usa a ultima amostra publicada. Nenhuma leitura de hardware acontece no callback BLE.
void respostaSensoresSite(char *texto, int capacidade)
{
    portENTER_CRITICAL(&travaDebug);
    SensoresDebug sensores = sensoresDebug;
    char aviso[80];
    memcpy(aviso, avisoSite, sizeof(aviso));
    long long agora = esp_timer_get_time();
    portEXIT_CRITICAL(&travaDebug);
    int n = snprintf(texto, capacidade, "{\"us\":%lld,\"amostra\":%lld,\"linhaE\":%d,\"linhaD\":%d,\"ldr\":%d,\"irE\":%d,\"irD\":%d,\"jsE\":%d,\"jsD\":%d,\"aviso\":\"%s\"}", agora, sensores.instante, sensores.linhaEsquerda, sensores.linhaDireita, sensores.ldr, sensores.receptorEsquerdo, sensores.receptorDireito, sensores.jsumoEsquerdo, sensores.jsumoDireito, aviso);
    if (n < 0 || n >= capacidade) snprintf(texto, capacidade, "{\"erro\":\"sensores_excederam_limite\"}");
}

// Retorna o proximo evento sem apaga-lo. Informa os registros perdidos desde o cursor.
void respostaEventoSite(unsigned long sessao, unsigned long captura, unsigned long depois, char *texto, int capacidade)
{
    portENTER_CRITICAL(&travaDebug);
    if (sessao != sessaoDebug || captura != capturaDebug || depois > totalDebug)
    {
        portEXIT_CRITICAL(&travaDebug);
        snprintf(texto, capacidade, "{\"erro\":\"sessao_ou_cursor\"}");
        return;
    }
    unsigned long primeiro = quantidadeDebug ? totalDebug - quantidadeDebug + 1 : 0;
    bool temEvento = depois < totalDebug;
    unsigned long proximo = temEvento ? depois + 1 : depois;
    if (temEvento && proximo < primeiro) proximo = primeiro;
    unsigned long perdidos = temEvento ? proximo - depois - 1 : 0;
    RegistroDebug registro;
    if (temEvento) registro = historicoDebug[(proximo - 1) % TAMANHO_HISTORICO];
    portEXIT_CRITICAL(&travaDebug);
    if (!temEvento)
    {
        snprintf(texto, capacidade, "{\"sessao\":%lu,\"captura\":%lu,\"perdidos\":0,\"evento\":null}", sessao, captura);
        return;
    }
    // Motivos sao textos internos sem aspas ou barras; devem continuar validos dentro do JSON.
    SensoresDebug &s = registro.sensores;
    int n = snprintf(texto, capacidade, "{\"sessao\":%lu,\"captura\":%lu,\"perdidos\":%lu,\"evento\":{\"n\":%lu,\"us\":%lld,\"dur\":%lld,\"tipo\":%d,\"de\":%d,\"para\":%d,\"ciclo\":%lu,\"rec\":%lu,\"mE\":%d,\"mD\":%d,\"linhaE\":%d,\"linhaD\":%d,\"ldr\":%d,\"irE\":%d,\"irD\":%d,\"jsE\":%d,\"jsD\":%d,\"amostra\":%lld,\"motivo\":\"%s\"}}", sessao, captura, perdidos, registro.numero, registro.instante, registro.duracao, registro.tipo, registro.anterior, registro.atual, registro.ciclo, registro.recuperacao, registro.motorEsquerdo, registro.motorDireito, s.linhaEsquerda, s.linhaDireita, s.ldr, s.receptorEsquerdo, s.receptorDireito, s.jsumoEsquerdo, s.jsumoDireito, s.instante, registro.motivo);
    if (n < 0 || n >= capacidade) snprintf(texto, capacidade, "{\"erro\":\"evento_excedeu_limite\"}");
}

// Os contadores do protocolo usam quatro bytes, do menos para o mais significativo.
unsigned long lerNumeroSite(const unsigned char *dados)
{
    return (unsigned long)dados[0] | ((unsigned long)dados[1] << 8) | ((unsigned long)dados[2] << 16) | ((unsigned long)dados[3] << 24);
}

// Callback exigido pela biblioteca BLE. Somente valida pedidos e prepara respostas.
class ReceberSite : public BLECharacteristicCallbacks
{
public:
    void onWrite(BLECharacteristic *caracteristica) override
    {
        unsigned char *dados = caracteristica->getData();
        int tamanho = caracteristica->getLength();
        char resposta[512] = "{\"erro\":\"pedido_invalido\"}";
        // 0x01: resumo; 0x02: sensores; 0x21: evento; 0x03: configuracao; 0x22: captura.
        if (tamanho == 1 && dados[0] == 0x01) respostaStatusSite(resposta, sizeof(resposta));
        else if (tamanho == 1 && dados[0] == 0x02) respostaSensoresSite(resposta, sizeof(resposta));
        else if (tamanho == 13 && dados[0] == 0x21) respostaEventoSite(lerNumeroSite(dados + 1), lerNumeroSite(dados + 5), lerNumeroSite(dados + 9), resposta, sizeof(resposta));
        else if (tamanho == 5 && dados[0] == 0x03)
        {
            bool valida = dados[1] != 0 && strchr("fFbcCevAdgz", dados[1]) && strchr("<>=", dados[2]) && dados[2] != 0 && strchr("lr", dados[3]) && dados[3] != 0 && dados[4] <= 1;
            if (dados[2] == '=' && !strchr("bfFz", dados[1])) valida = false; // Frente so e aceito nas aberturas que tratam esse lado.
            portENTER_CRITICAL(&travaDebug);
            bool aceita = valida && configuracaoAbertaSite && !debugParado && !configuracaoPendenteSite;
            unsigned long numero = 0;
            if (aceita)
            {
                numero = ++pedidoSite;
                proximaConfiguracao = {(char)dados[1], (char)dados[2], (char)dados[3], dados[4] == 1, numero};
                configuracaoPendenteSite = true;
            }
            portEXIT_CRITICAL(&travaDebug);
            // Receber o pedido nao confirma sua aplicacao; o site acompanha aplicadoSite.
            if (aceita) snprintf(resposta, sizeof(resposta), "{\"ok\":true,\"pedido\":%lu}", numero);
            else snprintf(resposta, sizeof(resposta), "{\"erro\":\"configuracao_invalida_ocupada_ou_fechada\"}");
        }
        else if (tamanho == 2 && dados[0] == 0x22 && dados[1] <= 2)
        {
            if (dados[1] == 0) novaCapturaDebug();
            else pausarCapturaDebug(dados[1] == 1);
            respostaStatusSite(resposta, sizeof(resposta));
        }
        caracteristica->setValue((unsigned char *)resposta, strlen(resposta));
    }
};

// Desconectar nao para o robo. O anuncio volta para permitir a reconexao.
class ConexaoSite : public BLEServerCallbacks
{
public:
    void onConnect(BLEServer *servidor) override
    {
        (void)servidor;
        portENTER_CRITICAL(&travaDebug);
        siteConectado = true;
        portEXIT_CRITICAL(&travaDebug);
    }
    void onDisconnect(BLEServer *servidor) override
    {
        (void)servidor;
        portENTER_CRITICAL(&travaDebug);
        siteConectado = false;
        portEXIT_CRITICAL(&travaDebug);
        BLEDevice::startAdvertising();
    }
};

// Chamado apenas no Auto. O R/C continua usando o PS4 sem iniciar o servidor do site.
void iniciarBluetoothSite()
{
    sessaoDebug = esp_random();
    entradaEstadoDebug = esp_timer_get_time();
    estadoObservadoDebug = estadoAtual;
    estrategiaSite = estrategia;
    ladoSite = lado == esquerda ? '<' : lado == direita ? '>' : '=';
    bandeiraSite = lado_Bandeira == B_Esquerda ? 'l' : 'r';
    BLEDevice::init("Briga_Briga_Based");
    BLEServer *servidor = BLEDevice::createServer();
    servidor->setCallbacks(new ConexaoSite());
    BLEService *servico = servidor->createService(servicoSite);
    dadosSite = servico->createCharacteristic(caracteristicaSite, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);
    dadosSite->setCallbacks(new ReceberSite());
    dadosSite->setValue("{\"protocolo\":\"BRIGA_BLE_1\"}");
    servico->start();
    BLEAdvertising *anuncio = servidor->getAdvertising();
    anuncio->addServiceUUID(servicoSite);
    anuncio->setScanResponse(true);
    anuncio->setMinPreferred(0x06);
    anuncio->setMinPreferred(0x12);
    anuncio->start();
    portENTER_CRITICAL(&travaDebug);
    lendoSensoresSite = true;
    portEXIT_CRITICAL(&travaDebug);
}
#endif
