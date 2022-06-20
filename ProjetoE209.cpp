#include <stdio.h>
#include <stdlib.h>
#define FOSC 16000000U // Clock Speed 
#define BAUD 9600
#define MYUBRR FOSC / 16 / BAUD - 1
boolean sist;
char msg_tx[20];
char msg_rx[32];
int pos_msg_rx = 0;
int tamanho_msg_rx = 1;
unsigned int cont = 0;
unsigned int valor = 0;
double velocidade = 0;
boolean sistema = false;
boolean aux = false;
//Prototipos das funcoes
void UART_Init(unsigned int ubrr);
void UART_Transmit(char *dados);

ISR(INT0_vect){
  //VErificar esse if, ver se não é igual ao da main!!!
  if(TCCR2B == (TCCR2B | 0b000001000)){
     TCCR2B = 0b00000111;
  }
  else{
    TCNT2 = 0;
    cont = 0;
  }
}

ISR(INT1_vect){
  TCCR2B = 0b00000000;//Desativando o sistema
  cont = 0;//zerando o contador
  sistema = false;
  //Ver se colocamos aqui o desativar o led do motor
  PORTD &= ~(1<<PD5);//Desativando o LED do motor
  OCR2A = 0;//zerando o OCR0A
  ADCSRA &= ~(1<<ADEN);//desativando o conversor
  UART_Transmit("Sistema desligado");//indicando que o sistema foi desligado
  UART_Transmit("\n");
}

ISR(TIMER2_OVF_vect){
  
//Caso o botão fique pressionado até 5 segundos  
if(cont >= 305 && sistema == false){
  PORTD |= (1<<PD4);//Acendendo LED, até atingir 5 segundos
  sistema = true;
  cont = 0;
} 

//Após 5 segundos do botão pressionado, o LED acenderá por 4 segundos
if(cont >= 244 && sistema == true && aux == false){
  PORTD ^= (1 << PD4); //Desligando o LED que mostra que o sistema foi ligado 
  UART_Transmit("Sistema ligado");//indicando que o sistema foi ligado
  UART_Transmit("\n");
  PORTD |= (1 << PD5); //Ligando o Led que representa o motor
  aux = true;
  cont = 0;
  EIMSK &= ~(1<<PD2);//impede que o sistema reinicie 
} 

//Passados os 25 segundos, o motor deve permanecer em rotação com metade de sua velocidade máxima.
if(cont >= 1526 && sistema == true && aux == true){
    ADCSRA |= (1<<ADEN);//ativando a conversão
    ADCSRA |= (1<<ADSC);//Inicializando a conversão
    TCCR0B = 0b00000000;//desativando o Timer
    ADCSRA &= ~(1<<ADEN);
    //PWM igual a 127, metade do valor
    OCR2A =  127; 
}

 cont++;
}

void setup()
{
  //VER O POTENCIOMETRO, PARA ANALISAR O PWM
  UART_Init(MYUBRR);//Inicializa a comunicação serial
  DDRD = 0b00110000; //PD5 saída (motor da esteira), PD4 saída(LED que mostrará "Sistema Ligado"), PD3 entrada(botão que desligará o sistema), PD2 entrada(botão que ligará o sistema)
  DDRC = 0b00000000;//Usando o A0, será entrada. Variaremos para analisar o PWM
  PORTD = 0b00001100;//Definindo pull up, nos botões PD3 e PD2
  TCNT2 = 0;//inicializando o contador em 0
  TIMSK2 = 0b00000001;// Ativando a interrupção por overflow
  TCCR2A = 0b00000011;//fast PWM to OCR0A
  TCCR2B = 0b00001000;// sem pre-escaler, iniciará o sistema
  OCR2A = 0;
  EIMSK = (1<<INT1) | (1<<INT0); //Interrupção do PD2 e do PD3
  EICRA |= ((0<<ISC01) | (1<<ISC00)); //Transição de subida e descida, configurando o PD2
  EICRA |= ((0<<ISC11) | (1<<ISC10)); //Transição de subida e descida, configurando o PD3
  sei();//serviço gobal de interrupção ativado  
}

void loop()
{  
    ADCSRA |= (1<<ADSC);  
    //Analisando a variação do PWM
    while(ADCSRA == (ADCSRA |(1 << ADSC)));
    OCR2A = (ADC*255)/1023;
    
    //Caso se digite 'P', o mesmo mostrará a velocidade de rotação atual do motor
    if(msg_rx[0] == 'P' && sistema == true){ 
      velocidade = (1800/255)*OCR2A;
      itoa(velocidade,msg_rx,10);
      UART_Transmit("A velocidade atual do motor eh ");
      UART_Transmit(msg_rx);
      UART_Transmit(" rpm");
      UART_Transmit("\n");
      msg_rx[0] == 'K';//condição para não retornar
    }
}

ISR(USART_RX_vect)
{
  // Escreve o valor recebido pela UART na posição pos_msg_rx do buffer msg_rx
  msg_rx[pos_msg_rx++] = UDR0;
  if (pos_msg_rx == tamanho_msg_rx)
    pos_msg_rx = 0;
}

void UART_Transmit(char * dados)
{
  // Envia todos os caracteres do buffer dados ate chegar um final de linha
  while (*dados != 0)
  {
    while (!(UCSR0A & (1 << UDRE0))); // Aguarda a transmissão acabar
    // Escreve o caractere no registro de tranmissão
    UDR0 = *dados;
    // Passa para o próximo caractere do buffer dados
    dados++;
  }
}

void UART_Init(unsigned int ubrr)
{
  // Configura a baud rate */6
  UBRR0H = (unsigned char)(ubrr >> 8);
  UBRR0L = (unsigned char)ubrr;
  // Habilita a recepcao, tranmissao e interrupcao na recepcao */
  UCSR0B = (1 << RXEN0) | (1 << TXEN0) | (1 << RXCIE0);
  // Configura o formato da mensagem: 8 bits de dados e 1 bits de stop */
  UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);
}
