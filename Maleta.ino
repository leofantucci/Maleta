// VERSAO 3 - Troca do ScrollTexto pela navegação por meio do teclado
// A - Cima / B - Baixo / C - Cancelar / # - Enter

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>
#include <EEPROM.h>

// ================= STRUCTS E ENUMS =================
enum Tela {
  INICIO,
  MODO_JOGO,
  MODO_ARMAR,
  DURACAO,
  MENU_SENHA,
  MENU_CONFIRMAR_PONTOS,
  MENU_PONTOS,
  MENU_TEMPO_EXPLOSAO,
  MENU_TEMPO_LIMITE,
  VISUALIZAR_TEMPO_EXPLOSAO,
  VISUALIZAR_TEMPO_LIMITE,
  VISUALIZAR_SENHA,
  VISUALIZAR_PONTOS,
  CONFIRMAR,
  JOGANDO_SABOTAGEM,
  ARMANDO,
  ARMADA,
  DESARMANDO,
  JOGANDO_DOMINACAO,
  DOMINANDO_1,
  DOMINANDO_2,
  DOMINADA,
  FINAL_ATAQUE,
  FINAL_DEFESA,
  FINAL_DOMINACAO,
};

enum Jogo {
  NOVO,
  ULTIMO,
  NENHUM_0,
};

enum ModoJogo {
  SABOTAGEM,
  DOMINACAO,
  NENHUM_1,
};

enum ModoArmar {
  BOTAO,
  CARTAO,
  SENHA,
  NENHUM_2,
};

enum Duracao {
  CQB, // 10 min
  PADRAO, // 15 min
  MILSIM,
  PERSONALIZADA, // Tempo definido pelo usuario
  NENHUM_3,
};

struct Cartao {
  byte uid[4];
  String nome;
};

struct ScrollLCD {
  int posicao;
  unsigned long ultimoScroll;
  const char* texto;
};

// Pacote de dados para salvar na EEPROM
struct DadosJogo {
  byte validacao; // <-- NOVA VARIÁVEL (Garante que os dados existem)
  ModoJogo modoJogo;
  ModoArmar modoArmar;
  Duracao duracao;
  int tempoExplosao;
  int tempoLimite;
  int maxPts;
  char senha[10];
};

ScrollLCD menu = {0, 0, " (1) Sabotagem - (2) Dominacao"};
ScrollLCD inicio = {0, 0, " (1) Novo Jogo - (2) Ultimo Jogo"};
ScrollLCD duracaoSabotagem = {0, 0, " (1) CQB: 50s. - (2) Padrao: 1m30s. - (3) Milsim: 4m20s. - (4) Personalizada"};
ScrollLCD duracaoDominacao = {0, 0, " (1) CQB: 10min. - (2) Padrao: 30min. - (3) Milsim: 60min. - (4) Personalizada"};
ScrollLCD menuSabotagem = {0, 0, " (1) Botao - (2) Cartao - (3) Senha"};
ScrollLCD menuDominacao = {0, 0, " (1) Botao - (2) Cartao"};
ScrollLCD tempoSabotagem = {0, 0, " Defina o tempo de explosao apos armar (seg.)"};
ScrollLCD tempoDominacao = {0, 0, " Defina o tempo limite da partida (seg.)"};
ScrollLCD finalAtacantes = {0, 0, "    ATACANTES VENCERAM!"};
ScrollLCD finalDefensores = {0, 0, "    DEFENSORES VENCERAM!"};
ScrollLCD menuConfirmarPontos = {0, 0, " Deseja personalizar o max. de pontos? (PADRAO = (Tempo Lim./2)"};
ScrollLCD menuConfirmar = {0,0, ""};


// ================= LCD I2C =================
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ================= RFID ====================
#define SS_PIN 10
#define RST_PIN 9
MFRC522 rfid(SS_PIN, RST_PIN);

// ================= BOTÕES ==================
#define BTN_VERDE A1
#define BTN_VERMELHO A2

// ================ TECLADO ==================
const byte LINHAS = 4;
const byte COLUNAS = 4;

char teclas[LINHAS][COLUNAS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};

Cartao cartoes[] = {
  {{0xAA, 0xBB, 0xCC, 0xDD}, "Vermelho"},
  {{0x11, 0x22, 0x33, 0x44}, "Verde"},
};

const int NUM_CARTOES = sizeof(cartoes) / sizeof(cartoes[0]);

byte pinosLinhas[LINHAS] = {2, 3, 4, 5};
byte pinosColunas[COLUNAS] = {6, 7, 8, A0};

Keypad teclado = Keypad(
  makeKeymap(teclas),
  pinosLinhas,
  pinosColunas,
  LINHAS,
  COLUNAS
);

byte bloco[8] = {
  B11111, B11111, B11111, B11111,
  B11111, B11111, B11111, B11111
};

byte setaEsquerda[8] = {
  B11100, B11110, B11111, B11111,
  B11111, B11110, B11100, B00000
};

byte trianguloCima[8] = {
  B00000,
  B00000,
  B00000,
  B00000,
  B00100,
  B01110,
  B11111,
  B00000 
};

byte trianguloBaixo[8] = {
  B00000,
  B11111,
  B01110,
  B00100,
  B00000,
  B00000,
  B00000,
  B00000 
};

Tela telaAtual = INICIO;
ModoJogo modoJogoAtual = SABOTAGEM;
ModoArmar modoArmarAtual = BOTAO;
Duracao duracaoAtual = PADRAO;

Tela ultimaTela = INICIO;

bool atualizouTela = false;
bool atualizouModoJogo = false;
bool atualizouModoArmar = false;
bool atualizouDuracao = false;

String textoScroll = "";
int posicaoScroll = 0;
unsigned long ultimoScroll = 0;
unsigned long inicioExplosao = 0;
unsigned long tempoMensagemVisualizar = 0;

int etapaDecodificacao = 0;
unsigned long ultimoAvancoDecodificacao = 0;
int progressoAnimacao = 0;
unsigned long ultimoFrameAnimacao = 0;

String tempoExplosao = "50";
String tempoExplosao_inserido = "";

String tempoLimite = "600";
String tempoLimite_inserido = "";

String senha = "";
String senha_inserida = "";

// Variáveis de Controle do Modo Dominação
int EquipeAtiva = 0;
int jaFoi = 0;
int pontosEq1 = 0;
int pontosEq2 = 0;
unsigned long tempoInicioDominacao = 0;
unsigned long ultimoPontoContado = 0;
int pos1 = 0;
int pos2 = 0;
int freq = 1000; // Tempo em ms para somar 1 ponto (1000ms = 1s)
int maxPts = 300; // Pontuação máxima para vitória
String maxPts_inserido = "";


// Buffer global que vai segurar o texto na memória (tamanho 100 é um bom limite)
String textoConfirmacaoGlobal = "";

/* ========================= */
/* FUNÇÕES                   */
/* ========================= */
void salvarUltimoJogo() {
  DadosJogo dados;
  
  dados.validacao = 99; // Nosso "carimbo" secreto de jogo salvo!
  
  dados.modoJogo = modoJogoAtual;
  dados.modoArmar = modoArmarAtual;
  dados.duracao = duracaoAtual;
  
  dados.tempoExplosao = tempoExplosao.toInt();
  dados.tempoLimite = tempoLimite.toInt();
  dados.maxPts = maxPts;
  senha.toCharArray(dados.senha, sizeof(dados.senha));

  EEPROM.put(0, dados);
  Serial.println(F("Jogo salvo na EEPROM!"));
}

bool carregarUltimoJogo() {
  DadosJogo dados;
  EEPROM.get(0, dados);

  // Verifica se o carimbo está lá. Se for diferente de 99, a EEPROM está vazia!
  if (dados.validacao != 99) {
    return false; // Retorna erro (não tem jogo salvo)
  }

  // Se passou, carrega as variáveis normais
  modoJogoAtual = dados.modoJogo;
  modoArmarAtual = dados.modoArmar;
  duracaoAtual = dados.duracao;
  
  tempoExplosao = String(dados.tempoExplosao);
  tempoLimite = String(dados.tempoLimite);
  maxPts = dados.maxPts;
  senha = String(dados.senha);
  
  Serial.println(F("Ultimo jogo carregado!"));
  return true; // Retorna sucesso!
}

String nomeModoJogoAtual(){
  switch(modoJogoAtual){
    case SABOTAGEM:
      return "Sabot.";
    case DOMINACAO:
      return "Domin.";
  }
}
String nomeModoArmarAtual(){
  switch(modoArmarAtual){
    case BOTAO:
      return "Botao";
    case CARTAO:
      return "Cartao";
    case SENHA:
      return "Senha";
  }
}
String nomeDuracaoAtual(){
  switch(duracaoAtual){
    case CQB:
      return "CQB";
    case PADRAO:
      return "Padrao";
    case MILSIM:
      return "Milsim";
    case PERSONALIZADA:
      return "Person.";
  }
}

void atualizarTextoConfirmacao() {
  // 1. Montamos a String passo a passo (usar += é mais leve para o Arduino)
  textoConfirmacaoGlobal = F(" Jogo: "); 
  textoConfirmacaoGlobal += nomeModoJogoAtual();
  textoConfirmacaoGlobal += F(" - Armar: ");
  textoConfirmacaoGlobal += nomeModoArmarAtual();
  textoConfirmacaoGlobal += F(" - Dur.: "); 
  textoConfirmacaoGlobal += nomeDuracaoAtual();
  
  if (modoJogoAtual == SABOTAGEM) {
    textoConfirmacaoGlobal += F(" - Tempo: ");
    textoConfirmacaoGlobal += tempoExplosao;
    textoConfirmacaoGlobal += F("seg.");
    if (modoArmarAtual == SENHA) {
      textoConfirmacaoGlobal += F(" - Senha: ");
      textoConfirmacaoGlobal += senha;
    }
  } 
  else if (modoJogoAtual == DOMINACAO) {
    textoConfirmacaoGlobal += F(" - Tempo: "); 
    textoConfirmacaoGlobal += tempoLimite;
    textoConfirmacaoGlobal += F("seg.");
    textoConfirmacaoGlobal += F(" - Pontos: ");
    textoConfirmacaoGlobal += maxPts;
  }
  
  textoConfirmacaoGlobal += F("   "); // Espaçamento extra no final para o letreiro não grudar

  // 2. Usamos .c_str() para passar o ponteiro seguro da String global para a struct
  menuConfirmar.texto = textoConfirmacaoGlobal.c_str();
  menuConfirmar.posicao = 0; // Reseta o scroll
}

void (*reiniciarSoftware)(void) = 0;

void scrollTexto(ScrollLCD &scroll, int linha, unsigned long intervalo) {
    int tamOriginal = strlen(scroll.texto);
    int tamTotal = tamOriginal + 16; 

    if (millis() - scroll.ultimoScroll >= intervalo) {
        scroll.ultimoScroll = millis();
        lcd.setCursor(0, linha);

        for (int i = 0; i < 16; i++) {
            int indice = (scroll.posicao + i) % tamTotal;
            if (indice < tamOriginal) {
                lcd.print(scroll.texto[indice]);
            } else {
                lcd.print(' ');
            }
        }
        scroll.posicao = (scroll.posicao + 1) % tamTotal;
    }
}

void mudarTela(Tela novaTela) {
  telaAtual = novaTela;
  atualizouTela = false;
  
  // Reseta os índices de rolagem dos menus
  menu.posicao = 0;
  inicio.posicao = 0;
  duracaoSabotagem.posicao = 0;
  duracaoDominacao.posicao = 0;
  menuSabotagem.posicao = 0;
  menuDominacao.posicao = 0;
  tempoSabotagem.posicao = 0;
  tempoDominacao.posicao = 0;
  finalAtacantes.posicao = 0;
  finalDefensores.posicao = 0;

  // Reseta estado do jogo quando retorna ao inicio
  if (novaTela == INICIO) {
    pontosEq1 = 0;
    pontosEq2 = 0;
    jaFoi = 0;
    EquipeAtiva = 0;
  }
}

void telaErroVazio(Tela antiga){
  lcd.clear();
  
  lcd.setCursor(0,0);
  lcd.print(F("Digite um valor!"));
  delay(3000);
  mudarTela(antiga);
}

void telaErro(Tela antiga){
  lcd.clear();
  lcd.setCursor(6,0);
  lcd.print(F("ERRO"));
  delay(1000);
  mudarTela(antiga);
}

void telaSenhaErrada(Tela antiga){
  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print(F("SENHA ERRADA"));
  delay(1000);
  mudarTela(antiga);
}

void salvarSenha(){
    if(senha_inserida == ""){
        telaErroVazio(MENU_SENHA);
    } else{
      senha = senha_inserida;
      Serial.println("SENHA SALVA: " + senha);
      mudarTela(VISUALIZAR_SENHA);
    }
}

void salvarPontos(){
    if(maxPts_inserido == ""){
        telaErroVazio(MENU_PONTOS);
    } else{
      maxPts = maxPts_inserido.toInt();
      Serial.println("LIM. PONTOS SALVO: " + maxPts);
      mudarTela(VISUALIZAR_PONTOS);
    }
}

void salvarTempoExplosao(){
    if(tempoExplosao_inserido == ""){
      telaErroVazio(MENU_TEMPO_EXPLOSAO);
    } else {
      tempoExplosao = tempoExplosao_inserido;
      mudarTela(VISUALIZAR_TEMPO_EXPLOSAO);
    }
}

void salvarTempoLimite(){
    if(tempoLimite_inserido == ""){
      telaErroVazio(MENU_TEMPO_LIMITE);
    } else {
      tempoLimite = tempoLimite_inserido;
      mudarTela(VISUALIZAR_TEMPO_LIMITE);
    }
}

void mudarModoJogo(ModoJogo novoModo) {
  modoJogoAtual = novoModo;
  atualizouModoJogo = false;
}

void mudarModoArmar(ModoArmar novoModo) {
  modoArmarAtual = novoModo;
  atualizouModoArmar = false;
}

void mudarDuracao(Duracao novaDuracao) {
  duracaoAtual = novaDuracao;
  atualizouDuracao = false;
}

void barraCarregamento(int tempo) {
  lcd.createChar(0, bloco);
  unsigned long inicioTimer = millis();
  int posicao = 0;

  lcd.setCursor(0, 1);
  lcd.print(F("                "));

  while (millis() - inicioTimer < tempo) {
    lcd.setCursor(posicao, 1);
    lcd.write(byte(0));
    delay(250);
    posicao++;

    if (posicao >= 16) {
      posicao = 0;
      lcd.setCursor(0, 1);
      lcd.print(F("                "));
    }
  }
}

bool apertouBotao(int botao) {
  delay(50); // DEBOUNCE INICIAL (Faltou esse)
  if (digitalRead(botao) == LOW) {
    while (digitalRead(botao) == LOW) {
      delay(10); 
    }
    return true; 
  }
  return false;
}

int verificarCartao(byte *uidLido, byte tamanho) {
  for (int i = 0; i < NUM_CARTOES; i++) {
    bool igual = true;
    for (int j = 0; j < tamanho; j++) {
      if (uidLido[j] != cartoes[i].uid[j]) {
        igual = false;
        break;
      }
    }
    if (igual) {
      return i;
    }
  }
  return -1;
}
/* ========================= */
/* TELAS                     */
/* ========================= */

int selecaoAtual;
int ultimaSelecao = -99;
int limiteMax;

void startSelecao(char tecla, int numMax){
  if (ultimaSelecao != selecaoAtual) {
    lcd.clear();
    ultimaSelecao = selecaoAtual;
  }
  if (tecla == 'B' && selecaoAtual > 0) selecaoAtual--;
  if (tecla == 'A' && selecaoAtual < numMax) selecaoAtual++;
}

void printarOpcaoBaixo(){
  lcd.setCursor(15, 1);
  lcd.write(byte(2));
}
void printarOpcaoCima(){
  lcd.setCursor(15, 0);
  lcd.write(byte(1));
}
void printarAmbasOpcoes(){
  printarOpcaoBaixo();
  printarOpcaoCima();
}

void printarConfirmacao(){
  lcd.setCursor(13,1);
  lcd.print(F("#"));
}

void resetarAtual(){
  mudarModoJogo(NENHUM_1);
  mudarModoArmar(NENHUM_2);
  mudarDuracao(NENHUM_3);
}

void telaInicio() {
  ultimaTela = INICIO;
  if (!atualizouTela) {
    resetarAtual();
    selecaoAtual = 1;
    lcd.clear();
    atualizouTela = true;
  }

  if (selecaoAtual == 1) {
    lcd.setCursor(0, 0);
    lcd.print(F("Bem-Vindo!"));
    lcd.setCursor(0, 1);
    lcd.print(F("Novo Jogo"));
    printarOpcaoBaixo();
    printarConfirmacao();
  } else if (selecaoAtual == 0) {
    lcd.setCursor(0, 0);
    lcd.print(F("Bem-Vindo!"));
    lcd.setCursor(0, 1);
    lcd.print(F("Ultimo Jogo"));
    printarOpcaoCima();
    printarConfirmacao();
  }
  
  char tecla = teclado.getKey();

  startSelecao(tecla, 1);
  
  if(tecla == '#'){
    delay(33);
    switch(selecaoAtual){
      case 1:
        mudarTela(MODO_JOGO);
        break;
      case 0:
        if (carregarUltimoJogo()) {
          atualizarTextoConfirmacao();
          mudarTela(CONFIRMAR);
        } else {
          lcd.clear();
          lcd.setCursor(2, 0);
          lcd.print(F("NENHUM JOGO"));
          lcd.setCursor(2, 1);
          lcd.print(F("SALVO AINDA!"));
          delay(2000);
          mudarTela(INICIO);
        }
        break;
    }
  }
}

void telaModoJogo() {
  ultimaTela = INICIO;
  if (!atualizouTela) {
    mudarModoJogo(NENHUM_1);
    selecaoAtual = 2;
    lcd.clear();
    atualizouTela = true;
  }
  
  if (selecaoAtual == 2) {
    lcd.setCursor(0, 0);
    lcd.print(F("Escolha o modo:"));
    lcd.setCursor(0, 1);
    lcd.print(F("Sabotagem"));
    printarOpcaoBaixo();
    printarConfirmacao();
  } else if (selecaoAtual == 1) {
    lcd.setCursor(0, 0);
    lcd.print(F("Escolha o modo:"));
    lcd.setCursor(0, 1);
    lcd.print(F("Dominacao"));
    printarAmbasOpcoes();
    printarConfirmacao();
  } else {
    lcd.setCursor(0, 0);
    lcd.print(F("Escolha o modo:"));
    lcd.setCursor(0, 1);
    lcd.print(F("Voltar"));
    printarOpcaoCima();
    printarConfirmacao();
  }

  char tecla = teclado.getKey();
  
  startSelecao(tecla, 2);

  if(tecla == 'C'){
    mudarTela(ultimaTela);
  }

  if(tecla == '#'){
    delay(33);
    switch(selecaoAtual){
      case 2:
        mudarModoJogo(SABOTAGEM);
        mudarTela(MODO_ARMAR);
        break;
      case 1:
        mudarModoJogo(DOMINACAO);
        mudarTela(MODO_ARMAR);
        break;
      case 0:
        mudarTela(ultimaTela);
        break;
    }
  }
}

void telaModoArmar() {
  ultimaTela = MODO_JOGO;
  if (!atualizouTela) {
    mudarModoArmar(NENHUM_2);
    lcd.clear();
    selecaoAtual = (modoJogoAtual == SABOTAGEM) ? 3 : 2;
    atualizouTela = true;
  }
  
  char tecla = teclado.getKey();

  startSelecao(tecla, (modoJogoAtual == SABOTAGEM) ? 3 : 2);

  if (selecaoAtual == 3) {
    lcd.setCursor(0, 0);
    lcd.print(F("Modo de armar:"));
    lcd.setCursor(0, 1);
    lcd.print(F("Senha"));
    printarOpcaoBaixo();
    printarConfirmacao();
  } 
  else if (selecaoAtual == 2) {
    lcd.setCursor(0, 0);
    lcd.print(F("Modo de armar:"));
    if (modoJogoAtual == SABOTAGEM) {
      printarOpcaoCima();
    }
    lcd.setCursor(0, 1);
    lcd.print(F("Botao"));
    printarOpcaoBaixo();
    printarConfirmacao();
  }
  else if (selecaoAtual == 1) {
    lcd.setCursor(0, 0);
    lcd.print(F("Modo de armar:"));
    lcd.setCursor(0, 1);
    lcd.print(F("Cartao"));
    printarAmbasOpcoes();
    printarConfirmacao();
  } else {
    lcd.setCursor(0, 0);
    lcd.print(F("Modo de armar:"));
    lcd.setCursor(0, 1);
    lcd.print(F("Voltar"));
    printarOpcaoCima();
    printarConfirmacao();
  }

  if(tecla == 'C'){
    delay(33);
    mudarTela(ultimaTela);
  }

  if(tecla == '#'){
    switch(selecaoAtual){
      case 3:
        delay(33);
        mudarModoArmar(SENHA);      
        mudarTela(MENU_SENHA);
        break;
      case 2:
        delay(33);
        mudarModoArmar(BOTAO);
        mudarTela(DURACAO);
        break;
      case 1:
        delay(33);
        mudarModoArmar(CARTAO);
        mudarTela(DURACAO);
        break;
      case 0:
        delay(33);
        mudarTela(ultimaTela);
        break;
    }
  }
}

void telaDuracao(){
  ultimaTela = MODO_ARMAR;
  if (!atualizouTela) {
    mudarDuracao(NENHUM_3);
    selecaoAtual = 4;
    lcd.clear();
    atualizouTela = true;
  }  

  char tecla = teclado.getKey();

  startSelecao(tecla, 4);

  if (selecaoAtual == 4) {
    lcd.setCursor(0, 0);
    lcd.print(F("Duracao:"));
    lcd.setCursor(0, 1);
    if (modoJogoAtual == SABOTAGEM) lcd.print(F("CQB: 50s."));
    else lcd.print(F("CQB: 10m."));
    printarOpcaoBaixo();
    printarConfirmacao();
  } 
  else if (selecaoAtual == 3) {
    lcd.setCursor(0, 0);
    lcd.print(F("Duracao:"));
    lcd.setCursor(0, 1);
    if (modoJogoAtual == SABOTAGEM) lcd.print(F("Padrao: 90s."));
    else lcd.print(F("Padrao: 30m."));
    printarAmbasOpcoes();
    printarConfirmacao();
  } 
  else if (selecaoAtual == 2) {
    lcd.setCursor(0, 0);
    lcd.print(F("Duracao:"));
    lcd.setCursor(0, 1);
    if (modoJogoAtual == SABOTAGEM) lcd.print(F("Milsim: 260s."));
    else lcd.print(F("Milsim: 60m."));
    printarAmbasOpcoes();
    printarConfirmacao();
  } 
  else if (selecaoAtual == 1) {
    lcd.setCursor(0, 0);
    lcd.print(F("Duracao:"));
    lcd.setCursor(0, 1);
    lcd.print(F("Personaliz."));
    printarAmbasOpcoes();
    printarConfirmacao();
  } 
  else if (selecaoAtual == 0) {
    lcd.setCursor(0, 0);
    lcd.print(F("Duracao:"));
    lcd.setCursor(0, 1);
    lcd.print(F("Voltar"));
    printarOpcaoCima();
    printarConfirmacao();
  }

  if(tecla == 'C'){
    delay(33);
    mudarTela(ultimaTela);
  }

  if(tecla == '#'){
    switch(selecaoAtual){
      case 4:
        mudarDuracao(CQB);
        if(modoJogoAtual == SABOTAGEM) {tempoExplosao = "50"; mudarTela(CONFIRMAR);}
        if(modoJogoAtual == DOMINACAO) { tempoLimite = "600"; maxPts = 300; mudarTela(MENU_CONFIRMAR_PONTOS);}
        break;
      case 3:
        mudarDuracao(PADRAO);
        if(modoJogoAtual == SABOTAGEM) {tempoExplosao = "90"; mudarTela(CONFIRMAR);}
        if(modoJogoAtual == DOMINACAO) { tempoLimite = "1800"; maxPts = 900; mudarTela(MENU_CONFIRMAR_PONTOS);}
        break;
      case 2:
        mudarDuracao(MILSIM);
        if(modoJogoAtual == SABOTAGEM) {tempoExplosao = "260"; mudarTela(CONFIRMAR);}
        if(modoJogoAtual == DOMINACAO) { tempoLimite = "3600"; maxPts = 1800; mudarTela(MENU_CONFIRMAR_PONTOS);}
        break;
      case 1:
        mudarDuracao(PERSONALIZADA);
        if(modoJogoAtual == SABOTAGEM){
          mudarTela(MENU_TEMPO_EXPLOSAO);
        }
        if(modoJogoAtual == DOMINACAO){
          mudarTela(MENU_TEMPO_LIMITE);
        }
        break;
      case 0:
        delay(33);
        mudarTela(ultimaTela);
        break;
    }
  }
}

void telaMenuSenha() {
  ultimaTela = MODO_ARMAR;
  if (!atualizouTela) {
    senha_inserida = "";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Defina a senha:"));
  
    atualizouTela = true;
  }

  char tecla = teclado.getKey();
  if(tecla >= '0' && tecla <= '9'){
    senha_inserida += tecla;
    lcd.setCursor(0,1);
    lcd.print("                ");
    lcd.setCursor(0,1);
    lcd.print(senha_inserida);
  }
  if(tecla == 'C'){
    if(senha_inserida != ""){
      senha_inserida = "";
      lcd.setCursor(0,1);
      lcd.print(F("                "));
    } else{
      delay(33);
      mudarTela(ultimaTela);
    }
  }
  if(tecla == '#'){
    delay(33);
    salvarSenha();
  }
}

void telaVisualizarSenha(){
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Senha salva:"));
    lcd.setCursor(0,1);
    lcd.print(senha);
    atualizouTela = true;
  }
  delay(2500);
  mudarTela(DURACAO);
}

void telaMenuConfirmarPontos(){
  ultimaTela = MENU_TEMPO_LIMITE;
  if (!atualizouTela) {
    lcd.clear();
    selecaoAtual = 1;
    atualizouTela = true;
  }

  if (selecaoAtual == 1) {
    lcd.setCursor(0, 0);
    lcd.print(F("Pers. max. pts."));
    lcd.setCursor(0, 1);
    lcd.print(F("Sim"));
    lcd.setCursor(15, 1);
    lcd.write(byte(2));
  } else {
    lcd.setCursor(0, 0);
    lcd.print(F("Pers. max. pts."));
    lcd.setCursor(15, 0);
    lcd.write(byte(1));
    lcd.setCursor(0, 1);
    lcd.print(F("Nao"));
  }

  char tecla = teclado.getKey();

  startSelecao(tecla, 1);

  if(tecla == 'C'){
    delay(33);
    mudarTela(ultimaTela);
  }

  if(tecla == '#'){
    switch(selecaoAtual){
      case 1:
        delay(33);
        mudarTela(MENU_PONTOS);
        break;
      case 0:
        delay(33);
        maxPts = ceil((tempoLimite.toInt())/2.0);
        mudarTela(VISUALIZAR_PONTOS);
        break;
    }
  }
}

void telaMenuPontos() {
  ultimaTela = MENU_CONFIRMAR_PONTOS;
  if (!atualizouTela) {
    maxPts_inserido = "";
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Def. max. pts.:"));
    atualizouTela = true;
  }
  
  char tecla = teclado.getKey();

  if(tecla >= '0' && tecla <= '9'){
    maxPts_inserido += tecla;
    lcd.setCursor(0,1);
    lcd.print(F("                "));
    lcd.setCursor(0,1);
    lcd.print(maxPts_inserido);
  }

  if(tecla == 'C'){
    if(maxPts_inserido != ""){
      maxPts_inserido = "";
      lcd.setCursor(0,1);
      lcd.print(F("                "));
    } else {
      delay(33);
      mudarTela(ultimaTela);
    }
  }

  if(tecla == '#'){
    delay(33);
    salvarPontos();
  }
}

void telaVisualizarPontos(){
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Max. de pontos:"));
    lcd.setCursor(0,1);
    lcd.print(maxPts);
    atualizouTela = true;
  }
  delay(2000);
  mudarTela(CONFIRMAR);
}

void telaTempoExplosao() {
  ultimaTela = DURACAO;
  if (!atualizouTela) {
    tempoExplosao_inserido = "";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Def. tempo exp.:"));
    atualizouTela = true;
  }

  char tecla = teclado.getKey();

  if(tecla >= '0' && tecla <= '9'){
    tempoExplosao_inserido += tecla;
    lcd.setCursor(0,1);
    lcd.print(F("                "));
    lcd.setCursor(0,1);
    lcd.print(tempoExplosao_inserido);
  }

  if(tecla == 'C'){
    if(tempoExplosao_inserido != ""){
      tempoExplosao_inserido = "";
      lcd.setCursor(0,1);
      lcd.print(F("                "));
    } else {
      delay(33);
      mudarTela(ultimaTela);
    }
  }

  if(tecla == '#'){
    delay(33);
    salvarTempoExplosao();
  }
}

void telaVisualizarTempoExplosao(){
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Tempo explosao:"));
    lcd.setCursor(0,1);
    lcd.print(tempoExplosao);
    lcd.print(F("seg."));
    atualizouTela = true;
  }
  delay(2500);
  mudarTela(CONFIRMAR);
}

void telaTempoLimite() {
  ultimaTela = DURACAO;
  if (!atualizouTela) {
    tempoLimite_inserido = "";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Def. Tempo lim.:"));
    atualizouTela = true;
  }

  char tecla = teclado.getKey();

  if(tecla >= '0' && tecla <= '9'){
    tempoLimite_inserido += tecla;
    lcd.setCursor(0,1);
    lcd.print(F("                "));
    lcd.setCursor(0,1);
    lcd.print(tempoLimite_inserido);
  }

  if(tecla == 'C'){
    delay(33);
    if(tempoLimite_inserido != ""){
      tempoLimite_inserido = "";
      lcd.setCursor(0,1);
      lcd.print(F("                "));
    } else {
      delay(33);
      mudarTela(ultimaTela);
    }
  }

  if(tecla == '#'){
    delay(33);
    salvarTempoLimite();
  }
}

void telaVisualizarTempoLimite(){
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Tempo limite:"));
    lcd.setCursor(0,1);
    lcd.print(tempoLimite);
    lcd.print(F("seg."));
    atualizouTela = true;
  }
  delay(2500);
  mudarTela(MENU_CONFIRMAR_PONTOS);
}

void telaConfirmar() {
  if (!atualizouTela) {
    lcd.clear();
    if(modoJogoAtual == SABOTAGEM){
      if(modoArmarAtual == SENHA){
        selecaoAtual = 5;
        limiteMax = 5;
      } else {
        selecaoAtual = 4;
        limiteMax = 4;
      }
    } else{
      selecaoAtual = 5;
      limiteMax = 5;
    }
    atualizouTela = true;
  }
  
  switch(modoJogoAtual){
    case DOMINACAO:
      if(selecaoAtual == 5){
        lcd.setCursor(0,0);
        lcd.print(F("Confirma?"));
        lcd.setCursor(0,1);
        lcd.print(F("Jogo: "));
        lcd.print(nomeModoJogoAtual());
        printarOpcaoBaixo();
      }
      if(selecaoAtual == 4){
        lcd.setCursor(0,0);
        lcd.print(F("Confirma?"));
        lcd.setCursor(0,1);
        lcd.print(F("Armar: "));
        lcd.print(nomeModoArmarAtual());
        printarAmbasOpcoes();
      }
      if(selecaoAtual == 3){
        lcd.setCursor(0,0);
        lcd.print(F("Confirma?"));
        lcd.setCursor(0,1);
        lcd.print(F("Tempo: "));
        lcd.print(tempoLimite);
        lcd.print(F("s."));
        printarAmbasOpcoes();
      }
      if(selecaoAtual == 2){
        lcd.setCursor(0,0);
        lcd.print(F("Confirma?"));
        lcd.setCursor(0,1);
        lcd.print(F("Lim.: "));
        lcd.print(maxPts);
        lcd.print(F("pts."));
        printarAmbasOpcoes();
      }
      if(selecaoAtual == 1){
        lcd.setCursor(0,0);
        lcd.print(F("Confirma?"));
        lcd.setCursor(0,1);
        lcd.print(F("Sim"));
        printarAmbasOpcoes();
        printarConfirmacao();
      }
      if(selecaoAtual == 0){
        lcd.setCursor(0,0);
        lcd.print(F("Confirma?"));
        lcd.setCursor(0,1);
        lcd.print(F("Nao"));
        printarOpcaoCima();
        printarConfirmacao();
      }
      break;

    case SABOTAGEM:
      if(modoArmarAtual == SENHA){
        if(selecaoAtual == 5){
        lcd.setCursor(0,0);
        lcd.print(F("Confirma?"));
        lcd.setCursor(0,1);
        lcd.print(F("Jogo: "));
        lcd.print(nomeModoJogoAtual());
        printarOpcaoBaixo();
      }
      if(selecaoAtual == 4){
        lcd.setCursor(0,0);
        lcd.print(F("Confirma?"));
        lcd.setCursor(0,1);
        lcd.print(F("Armar: "));
        lcd.print(nomeModoArmarAtual());
        printarAmbasOpcoes();
      }
      if(selecaoAtual == 3){
        lcd.setCursor(0,0);
        lcd.print(F("Confirma?"));
        lcd.setCursor(0,1);
        lcd.print(F("Senha: "));
        lcd.print(senha);
        printarAmbasOpcoes();
      }
      if(selecaoAtual == 2){
        lcd.setCursor(0,0);
        lcd.print(F("Confirma?"));
        lcd.setCursor(0,1);
        lcd.print(F("Lim.: "));
        lcd.print(tempoExplosao);
        lcd.print(F("s."));
        printarAmbasOpcoes();
      }
      if(selecaoAtual == 1){
        lcd.setCursor(0,0);
        lcd.print(F("Confirma?"));
        lcd.setCursor(0,1);
        lcd.print(F("Iniciar"));
        printarAmbasOpcoes();
        printarConfirmacao();
        
      }
      if(selecaoAtual == 0){
        lcd.setCursor(0,0);
        lcd.print(F("Confirma?"));
        lcd.setCursor(0,1);
        lcd.print(F("Voltar"));
        printarOpcaoCima();
        printarConfirmacao();
      }
      } else{
        if(selecaoAtual == 4){
          lcd.setCursor(0,0);
          lcd.print(F("Confirma?"));
          lcd.setCursor(0,1);
          lcd.print(F("Jogo: "));
          lcd.print(nomeModoJogoAtual());
          printarOpcaoBaixo();
        }
        if(selecaoAtual == 3){
          lcd.setCursor(0,0);
          lcd.print(F("Confirma?"));
          lcd.setCursor(0,1);
          lcd.print(F("Armar: "));
          lcd.print(nomeModoArmarAtual());
          printarAmbasOpcoes();
        }
        if(selecaoAtual == 2){
          lcd.setCursor(0,0);
          lcd.print(F("Confirma?"));
          lcd.setCursor(0,1);
          lcd.print(F("Tempo: "));
          lcd.print(tempoExplosao);
          lcd.print(F("s."));
          printarAmbasOpcoes();
        }
        if(selecaoAtual == 1){
          lcd.setCursor(0,0);
          lcd.print(F("Confirma?"));
          lcd.setCursor(0,1);
          lcd.print(F("Iniciar"));
          printarAmbasOpcoes();
          printarConfirmacao();
        }
        if(selecaoAtual == 0){
          lcd.setCursor(0,0);
          lcd.print(F("Confirma?"));
          lcd.setCursor(0,1);
          lcd.print(F("Voltar"));
          printarOpcaoCima();
          printarConfirmacao();
        }
      }
      break;
  }

  char tecla = teclado.getKey();
  
  startSelecao(tecla, limiteMax);
  
  if(tecla == 'C'){
    delay(33);
    mudarTela(INICIO);
  }

  if(tecla == '#'){
    delay(33);
    if(selecaoAtual > 1){
      selecaoAtual -= 1;
    } 
    else if(selecaoAtual == 1){
      salvarUltimoJogo();
      if(modoJogoAtual == SABOTAGEM) mudarTela(JOGANDO_SABOTAGEM);
      if(modoJogoAtual == DOMINACAO) mudarTela(JOGANDO_DOMINACAO);
    } 
    else if(selecaoAtual == 0){
      mudarTela(INICIO);
    }
  }
}

/* ========================= */
/* SABOTAGEM                 */
/* ========================= */

void telaJogandoSabotagem(){
  if (!atualizouTela) {
    senha_inserida = "";
    lcd.clear();
    lcd.setCursor(2,0);
    lcd.print(F("ARME A BOMBA"));

    /*switch(modoArmarAtual){
      case SENHA:
        senha_inserida = "";
        lcd.print(F("- 3"));
        break;
      case CARTAO:
        lcd.print(F("- 2"));
        break;
      case BOTAO:
        lcd.print(F("- 1"));
        break;
    }*/
    atualizouTela = true;
  }

  char tecla = teclado.getKey();
  
  switch(modoArmarAtual){
    case BOTAO:
      if(apertouBotao(BTN_VERDE)){
        mudarTela(ARMANDO);
      }
      if(apertouBotao(BTN_VERMELHO) || tecla) {
        telaErro(JOGANDO_SABOTAGEM);
      }
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        telaErro(JOGANDO_SABOTAGEM);
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
      break;
  
    case SENHA:
      if(tecla >= '0' && tecla <= '9'){
        senha_inserida += tecla;
        lcd.setCursor(0,1);
        lcd.print(F("                "));
        lcd.setCursor(0,1);
        lcd.print(senha_inserida);
      }
      if(tecla == 'A' || tecla == '#' || apertouBotao(BTN_VERDE)){
        if(senha_inserida == senha){
          delay(33);
          mudarTela(ARMANDO);
        } else{
          senha_inserida = "";
          telaSenhaErrada(JOGANDO_SABOTAGEM);
        }
      }
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        delay(33);
        senha_inserida = "";
        telaErro(JOGANDO_SABOTAGEM);
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
      if(apertouBotao(BTN_VERMELHO)){
        delay(33);
        senha_inserida = "";
        telaErro(JOGANDO_SABOTAGEM);
      }
      break;

    case CARTAO:
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        int indice = verificarCartao(rfid.uid.uidByte, rfid.uid.size);
        if (indice != -1 && cartoes[indice].nome == "Verde") {
          delay(33);
          mudarTela(ARMANDO);
        } else {
          telaErro(JOGANDO_SABOTAGEM);
        }
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
      if(apertouBotao(BTN_VERMELHO) || apertouBotao(BTN_VERDE) || tecla) {
        telaErro(JOGANDO_SABOTAGEM);
      }
      break;
  }
}

void telaArmando(){
  if(!atualizouTela){
    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print(F("ARMANDO  BOMBA"));

    if((tempoExplosao.toInt()/10) >= 8){
      barraCarregamento(8000);
    } else{
      barraCarregamento(4000);
    }
    atualizouTela = true;
  }

  lcd.clear();
  lcd.setCursor(2,0);
  lcd.print(F("BOMBA ARMADA"));

  delay(1000);
  inicioExplosao = millis();
  mudarTela(ARMADA);
}

void telaArmada(){
  if(!atualizouTela){
    lcd.clear();
    lcd.setCursor(2,0);
    lcd.print(F("BOMBA ARMADA"));
    atualizouTela = true;
  }

  int tempoRestante = tempoExplosao.toInt() - ((millis() - inicioExplosao) / 1000);
  if(tempoRestante < 0) tempoRestante = 0;

  char bufferTempo[17];
  if (tempoRestante >= 60) {
    int minutos = tempoRestante / 60;
    int segundos = tempoRestante % 60;
    snprintf(bufferTempo, sizeof(bufferTempo), "Tempo: %dmin%ds", minutos, segundos);
  } else {
    snprintf(bufferTempo, sizeof(bufferTempo), "Tempo: %dseg.", tempoRestante);
  }

  lcd.setCursor(0, 1);
  lcd.print(bufferTempo);
  
  int tamTexto = strlen(bufferTempo);
  for (int i = tamTexto; i < 16; i++) {
    lcd.print(F(" "));
  }

  if(tempoRestante <= 0){
    mudarTela(FINAL_ATAQUE);
    return;
  }

  bool querDesarmar = false;

  if (modoArmarAtual == SENHA) {
    teclado.getKeys();
    for (int i = 0; i < LIST_MAX; i++) {
      if ((teclado.key[i].kchar == 'D' && teclado.key[i].kstate == HOLD) || digitalRead(BTN_VERMELHO) == LOW) {
        querDesarmar = true;
        break;
      }
    }
  } 
  else if (modoArmarAtual == BOTAO) {
    if (digitalRead(BTN_VERMELHO) == LOW) {
      querDesarmar = true;
    }
  } 
  else if (modoArmarAtual == CARTAO) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      int indice = verificarCartao(rfid.uid.uidByte, rfid.uid.size);
      if (indice != -1 && cartoes[indice].nome == "Vermelho") {
        querDesarmar = true;
      } else{
        telaErro(ARMADA);
      }
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }

  if (querDesarmar) {
    progressoAnimacao = 0;
    ultimoFrameAnimacao = millis();
    mudarTela(DESARMANDO);
  }
}

void telaDesarmando() {
  int tempoRestante = tempoExplosao.toInt() - ((millis() - inicioExplosao) / 1000);
  if (tempoRestante < 0) tempoRestante = 0;

  if (tempoRestante <= 0) {
    mudarTela(FINAL_ATAQUE);
    return;
  }

  if (!atualizouTela) {
    lcd.clear();
    lcd.createChar(0, bloco);
    atualizouTela = true;
  }

  char bufferTopo[17];
  if (tempoRestante >= 60) {
    int minutos = tempoRestante / 60;
    int segundos = tempoRestante % 60;
    snprintf(bufferTopo, sizeof(bufferTopo), "DESARM. %dmin%ds", minutos, segundos);
  } else {
    snprintf(bufferTopo, sizeof(bufferTopo), "DESARM. %dseg.", tempoRestante);
  }

  lcd.setCursor(0, 0);
  lcd.print(bufferTopo);
  
  int tamTexto = strlen(bufferTopo);
  for (int i = tamTexto; i < 16; i++) {
    lcd.print(F(" "));
  }

  bool mantendoAcao = false;

  if (modoArmarAtual == SENHA) {
    teclado.getKeys();
    bool segurandoD = false;
    for (int i = 0; i < LIST_MAX; i++) {
      if (teclado.key[i].kchar == 'D' && (teclado.key[i].kstate == HOLD || teclado.key[i].kstate == PRESSED)) {
        segurandoD = true;
        break;
      }
    }
    if (segurandoD || digitalRead(BTN_VERMELHO) == LOW) {
      mantendoAcao = true;
    }
  } 
  else if (modoArmarAtual == BOTAO) {
    if (digitalRead(BTN_VERMELHO) == LOW) {
      mantendoAcao = true;
    }
  } 
  else if (modoArmarAtual == CARTAO) {
    if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
      int indice = verificarCartao(rfid.uid.uidByte, rfid.uid.size);
      if (indice != -1 && cartoes[indice].nome == "Vermelho") {
        mantendoAcao = true;
      }
      rfid.PICC_HaltA();
      rfid.PCD_StopCrypto1();
    }
  }

  if (!mantendoAcao && modoArmarAtual != CARTAO) {
    mudarTela(ARMADA);
    return;
  }

  if (millis() - ultimoFrameAnimacao >= 500) {
    ultimoFrameAnimacao = millis();

    lcd.setCursor(progressoAnimacao, 1);
    lcd.write(byte(0));
    progressoAnimacao++;

    if (progressoAnimacao >= 17) {
      mudarTela(FINAL_DEFESA);
    }
  }
}

void telaFinalSabotagem(int key){
  if(!atualizouTela){
    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print(F("FINAL DE JOGO!"));
    atualizouTela = true;
  }
  
  switch(key){
    case 0:
      scrollTexto(finalDefensores, 1, 150);
      break;
    case 1:
      scrollTexto(finalAtacantes, 1, 150);
      break;
  }

  char tecla = teclado.getKey();
  if(tecla){
    lcd.clear();
    lcd.print(F("REINICIANDO..."));
    delay(1500);
    reiniciarSoftware();
  }
}

/* ========================= */
/* DOMINACAO                 */
/* ========================= */

void telaJogandoDominacao(){
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(1,0);
    
    lcd.print(F("DOMINE A AREA!"));

    /*switch(modoArmarAtual){
      case CARTAO:
        lcd.print(F("- 2"));
        break;
      case BOTAO:
        lcd.print(F("- 1"));
        break;
    }*/
    atualizouTela = true;
  }

  char tecla = teclado.getKey();
  switch(modoArmarAtual){
    case BOTAO:
      if(apertouBotao(BTN_VERDE) || tecla == '1'){
        delay(33);
        mudarTela(DOMINANDO_1);
      }
      if(apertouBotao(BTN_VERMELHO) || tecla == '2') {
        delay(33);
        mudarTela(DOMINANDO_2);
      }
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        telaErro(JOGANDO_DOMINACAO);
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
      break;

    case CARTAO:
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        int indice = verificarCartao(rfid.uid.uidByte, rfid.uid.size);
        if (indice != -1) {
          if(cartoes[indice].nome == "Verde"){
            delay(33);
            mudarTela(DOMINANDO_1);
          } else if(cartoes[indice].nome == "Vermelho"){
            delay(33);
            mudarTela(DOMINANDO_2);
          }
        } else {
          telaErro(JOGANDO_DOMINACAO);
        }
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
      if(apertouBotao(BTN_VERDE) || tecla == '1' || apertouBotao(BTN_VERMELHO) || tecla == '2'){
        telaErro(JOGANDO_DOMINACAO);
      }
      break;
  }
}

void telaDominando(int key){
  if(!atualizouTela){
    lcd.clear();
    lcd.setCursor(0,0);
    EquipeAtiva = key;

    lcd.print(F("DOMINANDO A AREA"));
    if((tempoLimite.toInt()/10) <= 80){
      barraCarregamento(4000);
    } else{
      barraCarregamento(8000);
    }
    
    atualizouTela = true;
  }

  lcd.clear();
  mudarTela(DOMINADA);
}

void telaDominada(){
  if(!atualizouTela){
    lcd.clear();
    lcd.createChar(0, setaEsquerda);

    pos1 = (EquipeAtiva == 1) ? 2 : 0;
    pos2 = (EquipeAtiva == 2) ? 2 : 0;

    if(EquipeAtiva == 1){
      lcd.setCursor(0, 0);
      lcd.write(byte(0));
    } else if(EquipeAtiva == 2){
      lcd.setCursor(0, 1);
      lcd.write(byte(0));
    }

    lcd.setCursor(pos1, 0);
    lcd.print(F("EQ. 1: "));
    pos1 += 7;

    lcd.setCursor(pos2, 1);
    lcd.print(F("EQ. 2: "));
    pos2 += 7;

    if(jaFoi == 0){
      tempoInicioDominacao = millis();
      jaFoi = 1;
    }
    
    ultimoPontoContado = millis();

    atualizouTela = true;
  }

  // --- ACÚMULO DE PONTOS A CADA 1 SEGUNDO ---
  if(millis() - ultimoPontoContado >= freq){
    ultimoPontoContado = millis();
    if(EquipeAtiva == 1) pontosEq1++;
    if(EquipeAtiva == 2) pontosEq2++;
  }

  // Atualiza pontos na tela
  lcd.setCursor(pos1, 0);
  lcd.print(pontosEq1);
  lcd.print(F("   "));

  lcd.setCursor(pos2, 1);
  lcd.print(pontosEq2);
  lcd.print(F("   "));

  char tecla = teclado.getKey();
  
  // Verificação de Tempo Limite do Jogo
  unsigned long tempoDecorridoSeg = (millis() - tempoInicioDominacao) / 1000;
  if(tempoDecorridoSeg >= tempoLimite.toInt()){
    delay(33);
    mudarTela(FINAL_DOMINACAO);
    return;
  }

  // Verificação de Vitória por Pontos Máximos
  if(pontosEq1 >= maxPts || pontosEq2 >= maxPts){
    delay(33);
    mudarTela(FINAL_DOMINACAO);
    return;
  }

  // Troca de Equipe Ativa via Teclado ou Botões
  switch(modoArmarAtual){
    case CARTAO:
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        int indice = verificarCartao(rfid.uid.uidByte, rfid.uid.size);
        if (indice != -1) {
          if(cartoes[indice].nome == "Verde" && EquipeAtiva != 1){
            EquipeAtiva = 1;
            delay(33);
            mudarTela(DOMINADA);
          } else if(cartoes[indice].nome == "Vermelho" && EquipeAtiva != 2){
            EquipeAtiva = 2;
            delay(33);
            mudarTela(DOMINADA);
          }
        } else {
          telaErro(DOMINADA);
        }
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
      break;

    case BOTAO:
      if((tecla == 'A' || tecla == '1' || apertouBotao(BTN_VERDE)) && EquipeAtiva != 1){
        EquipeAtiva = 1;
        mudarTela(DOMINADA);
      }
      if((tecla == 'B' || tecla == '2' || apertouBotao(BTN_VERMELHO)) && EquipeAtiva != 2){
        EquipeAtiva = 2;
        mudarTela(DOMINADA);
      }
      break;
  }
}

void telaFinalDominacao(){
  if(!atualizouTela){
    lcd.clear();
    int equipeVencedora = 0;
    int ptsVencedor = 0;
    if(pontosEq1 > pontosEq2){
      equipeVencedora = 1;
      ptsVencedor = pontosEq1;
    } else if(pontosEq2 > pontosEq1){
      equipeVencedora = 2;
      ptsVencedor = pontosEq2;
    } else {
      equipeVencedora = 3;
      ptsVencedor = pontosEq1;
    }
    if(equipeVencedora == 3){
      lcd.setCursor(1,0);
      lcd.print(F("!!! EMPATE !!!"));
    } else {
      lcd.setCursor(0,0);
      lcd.print(F("VENCE A EQUIPE "));
      lcd.print(equipeVencedora);
    }
    int toma = (("COM ") + String(ptsVencedor) + (" PTS.")).length();
    lcd.setCursor((16-toma)/2,1);
    lcd.print(F("COM "));
    lcd.print(ptsVencedor);
    lcd.print(F(" PTS."));
    atualizouTela = true;
  }

  char tecla = teclado.getKey();

  if(tecla){
    lcd.clear();
    lcd.print(F("REINICIANDO..."));
    delay(1500);
    reiniciarSoftware();
  }
}


void setup() {
  Serial.begin(115200);
  
  resetarAtual();
  
  // LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // RFID
  SPI.begin();
  rfid.PCD_Init();

  // Botões
  pinMode(BTN_VERDE, INPUT_PULLUP);
  pinMode(BTN_VERMELHO, INPUT_PULLUP);

  teclado.setDebounceTime(50);
  
  lcd.createChar(1, trianguloCima);
  lcd.createChar(2, trianguloBaixo);
  
  Serial.println(F("Sistema iniciado"));
};

void loop() {
  switch (telaAtual) {
    case INICIO:
      telaInicio();
      break;
     
    case MODO_JOGO:
      telaModoJogo();
      break;

    case MODO_ARMAR:
      telaModoArmar();
      break;

    case MENU_SENHA:
      telaMenuSenha();
      break;

    case VISUALIZAR_SENHA:
      telaVisualizarSenha();
      break;
      
    case DURACAO:
      telaDuracao();
      break;

    case MENU_TEMPO_EXPLOSAO:
      telaTempoExplosao();
      break;
    
    case VISUALIZAR_TEMPO_EXPLOSAO:
      telaVisualizarTempoExplosao();
      break;

    case VISUALIZAR_TEMPO_LIMITE:
      telaVisualizarTempoLimite();
      break;
    
    case MENU_TEMPO_LIMITE:
      telaTempoLimite();
      break;
    
    case MENU_CONFIRMAR_PONTOS:
      telaMenuConfirmarPontos();
      break;
    
    case MENU_PONTOS:
      telaMenuPontos();
      break;
    
    case VISUALIZAR_PONTOS:
      telaVisualizarPontos();
      break;
      
    case CONFIRMAR:
      telaConfirmar();
      break;

    case JOGANDO_SABOTAGEM:
      telaJogandoSabotagem();
      break;

    case ARMANDO:
      telaArmando();
      break;

    case ARMADA:
      telaArmada();
      break;

    case DESARMANDO:
      telaDesarmando();
      break;

    case FINAL_ATAQUE:
      telaFinalSabotagem(1);
      break;
    
    case FINAL_DEFESA:
      telaFinalSabotagem(0);
      break;

    case JOGANDO_DOMINACAO:
      telaJogandoDominacao();
      break;

    case DOMINANDO_1:
      telaDominando(1);
      break;

    case DOMINANDO_2:
      telaDominando(2);
      break;

    case DOMINADA:
      telaDominada();
      break;

    case FINAL_DOMINACAO:
      telaFinalDominacao();
      break;

    default:
      mudarTela(INICIO);
      break;
  }
}