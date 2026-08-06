// VERSAO 2 - Otimizando funcoes e corrigindo bugs

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <MFRC522.h>
#include <Keypad.h>

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
};

enum ModoJogo {
  SABOTAGEM,
  DOMINACAO,
};

enum Duracao {
  CQB, // 10 min
  PADRAO, // 15 min
  CAMPO_ABERTO,
  PERSONALIZADA, // Tempo definido pelo usuario
};

enum ModoArmar {
  BOTAO,
  CARTAO,
  SENHA,
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

ScrollLCD menu = {0, 0, " (1) Sabotagem - (2) Dominacao"};
ScrollLCD inicio = {0, 0, " (1) Novo Jogo - (2) Ultimo Jogo"};
ScrollLCD duracaoSabotagem = {0, 0, " (1) CQB: 50s. - (2) Padrao: 1m30s. - (3) Campo Aberto: 4m20s. - (4) Personalizada"};
ScrollLCD duracaoDominacao = {0, 0, " (1) CQB: 10min. - (2) Padrao: 30min. - (3) Campo Aberto: 60min. - (4) Personalizada"};
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
String nomeModoJogoAtual(){
  switch(modoJogoAtual){
    case SABOTAGEM:
      return "Sabotagem";
    case DOMINACAO:
      return "Dominacao";
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
    case CAMPO_ABERTO:
      return "Campo Aberto";
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

void telaInicio() {
  ultimaTela = INICIO;
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Bem-Vindo!"));
    atualizouTela = true;
  }

  scrollTexto(inicio, 1, 300);
  
  char tecla = teclado.getKey();
  if (tecla == '1') {
    mudarTela(MODO_JOGO);
  }
}

void telaModoJogo() {
  ultimaTela = INICIO;
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Escolha o modo"));
    atualizouTela = true;
  }

  scrollTexto(menu, 1, 300);

  char tecla = teclado.getKey();
  if(tecla == 'B'){
    delay(33);
    mudarTela(ultimaTela);
  }
  if (tecla == '1') {
    delay(33);
    mudarModoJogo(SABOTAGEM);
    mudarTela(MODO_ARMAR);
  }
  if (tecla == '2') {
    delay(33);
    mudarModoJogo(DOMINACAO);
    mudarTela(MODO_ARMAR);
  }
}

void telaModoArmar() {
  ultimaTela = MODO_JOGO;
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Modo de armar"));
    atualizouTela = true;
  }
  
  char tecla = teclado.getKey();
  if(tecla == 'B'){
    delay(33);
    mudarTela(ultimaTela);
  }

  switch(modoJogoAtual){
    case SABOTAGEM:
      scrollTexto(menuSabotagem, 1, 300);

      if (tecla == '1') {
        delay(33);
        mudarModoArmar(BOTAO);
        mudarTela(DURACAO);
      }
      if (tecla == '2') {
        delay(33);
        mudarModoArmar(CARTAO);
        mudarTela(DURACAO);
      }
      if (tecla == '3') {
        delay(33);
        mudarModoArmar(SENHA);      
        mudarTela(MENU_SENHA);
      }
      break;
      
    case DOMINACAO:
      scrollTexto(menuDominacao, 1, 300);
    
      if (tecla == '1') {
        delay(33);
        mudarModoArmar(BOTAO);
        mudarTela(DURACAO);
      }
      if (tecla == '2') {
        delay(33);
        mudarModoArmar(CARTAO);
        mudarTela(DURACAO);
      }
      break;
  }
}

void telaDuracao(){
  ultimaTela = MODO_ARMAR;
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0, 0);
    if(modoJogoAtual == SABOTAGEM){
      lcd.print(F("Tempo apos armar"));
    } else if(modoJogoAtual == DOMINACAO){
      lcd.print(F("Limite de tempo"));
    }
    atualizouTela = true;
  }  

  if(modoJogoAtual == SABOTAGEM){
    scrollTexto(duracaoSabotagem, 1, 300);
  }
  if(modoJogoAtual == DOMINACAO){
    scrollTexto(duracaoDominacao, 1, 300);
  }
  
  char tecla = teclado.getKey();

  if((tecla >= '1' && tecla <= '4') || tecla == 'B'){
    switch(tecla){
      case 'B':
        mudarTela(ultimaTela);
        break;
      case '1':
        mudarDuracao(CQB);
        if(modoJogoAtual == SABOTAGEM) {tempoExplosao = "50"; mudarTela(CONFIRMAR);}
        if(modoJogoAtual == DOMINACAO) { tempoLimite = "600"; maxPts = 300; mudarTela(MENU_CONFIRMAR_PONTOS);}
        break;
      case '2':
        mudarDuracao(PADRAO);
        if(modoJogoAtual == SABOTAGEM) {tempoExplosao = "90"; mudarTela(CONFIRMAR);}
        if(modoJogoAtual == DOMINACAO) { tempoLimite = "1800"; maxPts = 900; mudarTela(MENU_CONFIRMAR_PONTOS);}
        break;
      case '3':
        mudarDuracao(CAMPO_ABERTO);
        if(modoJogoAtual == SABOTAGEM) {tempoExplosao = "260"; mudarTela(CONFIRMAR);}
        if(modoJogoAtual == DOMINACAO) { tempoLimite = "3600"; maxPts = 1800; mudarTela(MENU_CONFIRMAR_PONTOS);}
        break;
      case '4':
        mudarDuracao(PERSONALIZADA);
        if(modoJogoAtual == SABOTAGEM){
          mudarTela(MENU_TEMPO_EXPLOSAO);
        }
        if(modoJogoAtual == DOMINACAO){
          mudarTela(MENU_TEMPO_LIMITE);
        }
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
    lcd.print(F("Defina a senha"));
  
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
  if(tecla == 'D'){
    senha_inserida = "";
    lcd.setCursor(0,1);
    lcd.print(F("                "));
  }
  if(tecla == '#'){
    delay(33);
    salvarSenha();
  }
  if(tecla == 'B'){
    delay(33);
    mudarTela(ultimaTela);
  }
}

void telaVisualizarSenha(){
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Senha salva"));
    lcd.setCursor(0,1);
    lcd.print(senha);
    atualizouTela = true;
  }
  delay(1000);
  mudarTela(DURACAO);
}

void telaMenuConfirmarPontos(){
  ultimaTela = MENU_TEMPO_LIMITE;
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0,1);
    lcd.print(F("(1)Sim - (2)Nao"));
    atualizouTela = true;
  }
    
  lcd.setCursor(0, 0);
  scrollTexto(menuConfirmarPontos, 0, 300);
  
  char tecla = teclado.getKey();
  
  if(tecla == '1'){
    delay(33);
    mudarTela(MENU_PONTOS);
  }
  if(tecla == '2'){
    delay(33);
    maxPts = (tempoLimite.toInt())/2;
    mudarTela(VISUALIZAR_PONTOS);
  }
  if(tecla == 'B'){
    delay(33);
    mudarTela(ultimaTela);
  }
}

void telaMenuPontos() {
  ultimaTela = MENU_CONFIRMAR_PONTOS;
  if (!atualizouTela) {
    maxPts_inserido = "";
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Def. max. pts."));
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
  if(tecla == 'D'){
    maxPts_inserido = "";
    lcd.setCursor(0,1);
    lcd.print(F("                "));
  }
  if(tecla == '#'){
    delay(33);
    salvarPontos();
  }
  if(tecla == 'B'){
    delay(33);
    mudarTela(ultimaTela);
  }
}

void telaVisualizarPontos(){
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Max. de pontos"));
    lcd.setCursor(0,1);
    lcd.print(maxPts);
    atualizouTela = true;
  }
  delay(1000);
  mudarTela(CONFIRMAR);
}

void telaTempoExplosao() {
  ultimaTela = DURACAO;
  if (!atualizouTela) {
    tempoExplosao_inserido = "";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Tempo apos armar"));
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
  if(tecla == 'D'){
    delay(33);
    tempoExplosao_inserido = "";
    lcd.setCursor(0,1);
    lcd.print(F("                "));
  }
  if(tecla == '#'){
    delay(33);
    salvarTempoExplosao();
  }
  if(tecla == 'B'){
    delay(33);
    mudarTela(ultimaTela);
  }
}

void telaVisualizarTempoExplosao(){
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Tempo explosao"));
    lcd.setCursor(0,1);
    lcd.print(tempoExplosao);
    lcd.print(F("seg."));
    atualizouTela = true;
  }
  delay(1000);
  mudarTela(CONFIRMAR);
}

void telaTempoLimite() {
  ultimaTela = DURACAO;
  if (!atualizouTela) {
    tempoLimite_inserido = "";
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(F("Tempo limite"));
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
  if(tecla == 'D'){
    delay(33);
    tempoLimite_inserido = "";
    lcd.setCursor(0,1);
    lcd.print(F("                "));
  }
  if(tecla == '#'){
    delay(33);
    salvarTempoLimite();
  }
  if(tecla == 'B'){
    delay(33);
    mudarTela(ultimaTela);
  }
}

void telaVisualizarTempoLimite(){
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Tempo limite"));
    lcd.setCursor(0,1);
    lcd.print(tempoLimite);
    lcd.print(F("seg."));
    atualizouTela = true;
  }
  delay(1000);
  mudarTela(MENU_CONFIRMAR_PONTOS);
}

void telaConfirmar() {
  if (!atualizouTela) {
    lcd.clear();
    atualizarTextoConfirmacao(); 
    
    // ADICIONE ISTO PARA MOSTRAR OS BOTÕES NA LINHA DE BAIXO
    lcd.setCursor(0, 1);
    lcd.print(F("1-Iniciar B-Sair"));
    
    atualizouTela = true;
  }
  
  scrollTexto(menuConfirmar, 0, 300); 
  
  char tecla = teclado.getKey();
  if(tecla == '1') {
    delay(33);
    if(modoJogoAtual == SABOTAGEM) mudarTela(JOGANDO_SABOTAGEM);
    if(modoJogoAtual == DOMINACAO) mudarTela(JOGANDO_DOMINACAO);
  }
    if(tecla == 'B') {delay(33); mudarTela(INICIO);}
}

/* ========================= */
/* SABOTAGEM                 */
/* ========================= */

void telaJogandoSabotagem(){
  String texto;
  if (!atualizouTela) {
    lcd.clear();
    lcd.setCursor(0,0);
    texto = "ARME A BOMBA";
    lcd.print(F("ARME A BOMBA"));

    lcd.setCursor(texto.length()+1,0);
    switch(modoArmarAtual){
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
    }
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
          telaSenhaErrada(JOGANDO_SABOTAGEM);
        }
      }
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        delay(33);
        telaErro(JOGANDO_SABOTAGEM);
        rfid.PICC_HaltA();
        rfid.PCD_StopCrypto1();
      }
      if(apertouBotao(BTN_VERMELHO)){
        delay(33);
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
    String texto = "";
    lcd.clear();
    lcd.setCursor(0,0);
    
    texto = "DOMINE A AREA";
    lcd.print(texto);

    lcd.setCursor(texto.length()+1,0);
    switch(modoArmarAtual){
      case CARTAO:
        lcd.print(F("- 2"));
        break;
      case BOTAO:
        lcd.print(F("- 1"));
        break;
    }
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
    lcd.setCursor(0,1);
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