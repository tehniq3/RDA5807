// Nicu Florica (aka niq_ro) from http://www.tehnic.go.ro made a small change for nice display for low frequence (bellow 100MHz)
// it use info from http://full-chip.net/arduino-proekty/97-cifrovoy-fm-priemnik-na-arduino-i-module-rda5807-s-graficheskim-displeem-i-funkciey-rds.html
// original look like is from http://seta43.hol.es/radiofm.html

#include <Wire.h>
#define DEBUG 0

const int entrada = A0; 
int entradaV = 0; 

int menu;
#define MAXmenu  4
int menux;
#define MAXmenux  4
static char* menuS[]= {" ","MANUAL TUNE","VOLUME     ","AUTO TUNE","INFO        "};

int volumen=2,volumenOld=7;
int frecuencia,frecuenciaOld;

unsigned int z,z1;
byte xfrecu,xfrecuOld;
unsigned int estado[6];

unsigned long time,time1,time2,time3;

// int    RDA5807_adrs=0x10;       // I2C-Address RDA Chip for sequential  Access
// int    RDA5807_adrr=0x11;       // I2C-Address RDA Chip for random      Access
// int    RDA5807_adrt=0x60;       // I2C-Address RDA Chip for TEA5767like Access

char buffer[30];
unsigned int RDS[4];
char seg_RDS[8];
char seg_RDS1[64];
char indexRDS1;

char hora,minuto,grupo,versio;
unsigned long julian;

 int mezcla;

void setup() 
{
  Wire.begin();   
  Serial.begin(9600); 
  
  LcdInitialise();
  LcdClear();
//  drawBox();

//   WriteReg(0x02,0xC001); // write 0xC002 into Reg.2 (soft reset, enable,RDS, )
   WriteReg(0x02,0xC00d); // write 0xC00d into Reg.2 ( soft reset, enable,RDS, )
   WriteReg(0x05,0x84d8);  // write ,0x84d8 into Reg.3 
   
   // frecuencia inicial = frecvencia * 0.1 + 87 
   frecuencia=175; //104.7
//frecuencia=130; //100.0 ==> 
//  frecuencia=26; //89.6
  time3=time2=time1=time = millis();
  menu=3;
  
  canal(frecuencia);
  clearRDS;
}

void loop() {
 
  entradaV = analogRead(entrada);
  
   #if DEBUG  
      Serial.print("sensor = " );  Serial.println(entradaV);delay(50);
   #endif
   
// Boton menu   
 if(entradaV>500 && entradaV<524)
   {
    menu++;
    if(menu>MAXmenu)menu=1;
    Visualizar();
//    sprintf(buffer,"Menu->%s",menuS[menu]); gotoXY(2,2);  LcdString(buffer); 
    #if DEBUG 
      Serial.print("menu = " );  Serial.println(menu); 
    #endif   
    while(1020>analogRead(entrada))delay(5);
   }
            
// Boton derecho
 if( entradaV<50)
   {
    menux++;
    if(menux>MAXmenux)menux=MAXmenux;
    #if DEBUG 
      Serial.print("menux = " );  Serial.println(menux);
    #endif
    switch(menu)
      {
        case 1:
          frecuencia++;
          if(frecuencia>210)frecuencia=210; // верхняя граница частот
          delay(130);
        break;  
        case 2:
           volumen++;
           if(volumen>15)volumen=15;
           while(1020>analogRead(entrada))delay(5);
        break; 
        case 3:
           busqueda(0);
           while(1020>analogRead(entrada))delay(5);
        break; 
        case 4:
            LcdClear();
            visualPI();
            delay(3000);
            LcdClear();
            frecuenciaOld=-1;
        break; 
      }              
   }
   
// Boton izquierdo
 if( entradaV<700 && entradaV>660)
   {
    menux--;
    if(menux<1)menux=1; 
    #if DEBUG 
      Serial.print("menux = " );  Serial.println(menux);
    #endif   
    switch(menu)
      {
        case 1:
            frecuencia--;
            if(frecuencia<0)frecuencia=0;    
            delay(130);
        break;  
        case 2:
            volumen--;
            if(volumen<0)volumen=0;
            while(1020>analogRead(entrada))delay(5);
        break; 
        case 3:
            busqueda(1);
            while(1020>analogRead(entrada))delay(5);
        break; 
        case 4:
            LcdClear();
            visualPTY();
            delay(3000);
            LcdClear();
            frecuenciaOld=-1;
        break; 
      }
    
   }
      
      if( millis()-time2>50)
          {
           ReadEstado();
           time1 = millis(); 
            //RDS   
           if ((estado[0] & 0x8000)!=0) {get_RDS();}
          }
     if( millis()-time3>500)
          {
            time3 = millis();
            Visualizar(); 

          }

    if( frecuencia!=frecuenciaOld)
          {  
            frecuenciaOld=frecuencia;                        
            z=870+frecuencia;
         #if DEBUG  
            Serial.print("Frecuencia = " );  Serial.println(frecuencia);
         #endif 
            sprintf(buffer,"%04d ",z);
             gotoXY(1,3);     
             
          for(z=0;z<5;z++)
               {
          if (z==0) {
          if (frecuencia < 130) LcdStringX(" ");
          else LcdCharacterX(buffer[0]);
   //       else LcdStringX("1");
          }
                     
             if(z==3)  LcdStringX(".");
             //   if((z==0) and (buffer[0]==0)) LcdStringX("."); else LcdCharacterX(buffer[z]);
           if (z>0) LcdCharacterX(buffer[z]);
               }
               
               
       //   LcdString("MHz"); 
          gotoXY(62,3); 
          LcdString("MHz");
        
          canal(frecuencia);
          clearRDS();
       }     

    //Cambio de volumen        
    if(volumen!=volumenOld)
        { 
          volumenOld=volumen;
          sprintf(buffer,"Vol %02d",volumen); gotoXY(38,1);  LcdString(buffer);      
          WriteReg(5, 0x84D0 | volumen);
        }       
}

void visualPI(void)
{
    #if DEBUG       
     Serial.print("PAIS:  "); Serial.println(RDS[0]>>12 & 0X000F);
     Serial.print("Cobertura:"); Serial.println(RDS[0]>>8 & 0X000F);
     Serial.print("CODIGO:"); Serial.println(RDS[0] & 0X00FF); 
    #endif
/*    
     gotoXY(1,3);sprintf(buffer,"PAIS  -%02d",RDS[0]>>12 & 0X000F); LcdString(buffer);
     gotoXY(1,4);sprintf(buffer,"COBERT-%02d",RDS[0]>>8 & 0X000F); LcdString(buffer);
     gotoXY(1,5);sprintf(buffer,"CODIGO-%02d",RDS[0] & 0X00FF); LcdString(buffer);     
*/ 
     gotoXY(1,3);sprintf(buffer,"COUNTRY -%02d",RDS[0]>>12 & 0X000F); LcdString(buffer);
     gotoXY(1,4);sprintf(buffer,"COVERAGE-%02d",RDS[0]>>8 & 0X000F); LcdString(buffer);
     gotoXY(1,5);sprintf(buffer,"CODE    -%02d",RDS[0] & 0X00FF); LcdString(buffer);     

}
void visualPTY(void)
{
    #if DEBUG       
     Serial.print("PTY:  "); Serial.println(RDS[1]>>5 & 0X001F);     
    #endif
 
 /*   
     gotoXY(1,3);     LcdString("TIPO");
     gotoXY(1,4);     LcdString("PROGRAMA");
     gotoXY(1,5);sprintf(buffer,"%02d",RDS[1]>>5 & 0X001F); LcdString(buffer);
*/
     gotoXY(1,3);     LcdString("TYPE");
     gotoXY(1,4);     LcdString("PROGRAMS");
     gotoXY(1,5);sprintf(buffer,"%02d",RDS[1]>>5 & 0X001F); LcdString(buffer);

}

void busqueda(byte direc)
{
  byte i;
  if(!direc) WriteReg(0x02,0xC30d); else  WriteReg(0x02,0xC10d);
  
  for(i=0;i<10;i++)
    {
      delay(200);      
      ReadEstado();      
      if(estado[0]&0x4000)
        {
          //Serial.println("Emisora encontrada");
          frecuencia=estado[0] & 0x03ff;  
          break;
        }       
    }
}

void clearRDS(void)
{       
         gotoXY(10,4); for (z=0;z<8;z++) {seg_RDS[z]=32; LcdCharacter(32);}  //borrar Name LCD Emisora
         gotoXY(38,2); for (z=0;z<6;z++) { LcdCharacter(32);}  //borrar linea Hora
         for (z=0;z<64;z++) seg_RDS1[z]=32;   
}

void Visualizar(void)
{ 
      //Serial.print("READ_Frecuencia= " );  Serial.println(estado[0] & 0x03ff);
      gotoXY(2,0); LcdStringX("FM"); 
      sprintf(buffer,"%s",menuS[menu]); gotoXY(2,2);  LcdString(buffer); 
       //Detectar se&#241;al stereo
       gotoXY(72,0);
       if((estado[0] & 0x0400)==0)  LcdCharacter(32);   else     LcdCharacter(127);        
       //Se&#241;al 
       z=estado[1]>>10; sprintf(buffer,"S-%02d",z); gotoXY(38,0);  LcdString(buffer);
       sprintf(buffer,"Vol %02d",volumen); gotoXY(38,1);  LcdString(buffer);
       //ver RADIO_TXT
       gotoXY(0,5);
       z1=indexRDS1;
       for (z=0;z<12;z++)
       {           
         LcdCharacter(seg_RDS1[z1]);
         z1++;
         if(z1>35)z1=0;           
       }
       indexRDS1++; if(indexRDS1>35) indexRDS1=0;
       
      frecuencia=estado[0] & 0x03ff;  
  }

void canal( int canal)
     {
       byte numeroH,numeroL;
       
       numeroH=  canal>>2;
       numeroL = ((canal&3)<<6 | 0x10); 
       Wire.beginTransmission(0x11);
       Wire.write(0x03);
         Wire.write(numeroH);                     // write frequency into bits 15:6, set tune bit         
         Wire.write(numeroL);
         Wire.endTransmission();
       }

//________________________ 
//RDA5807_adrr=0x11;       
// I2C-Address RDA Chip for random      Access
void WriteReg(byte reg,unsigned int valor)
{
  Wire.beginTransmission(0x11);
  Wire.write(reg); Wire.write(valor >> 8); Wire.write(valor & 0xFF);
  Wire.endTransmission();
  //delay(50);
}

//RDA5807_adrs=0x10;
// I2C-Address RDA Chip for sequential  Access
int ReadEstado()
{
 Wire.requestFrom(0x10, 12); 
 for (int i=0; i<6; i++) { estado[i] = 256*Wire.read ()+Wire.read(); }
 Wire.endTransmission();

}

//READ RDS  Direccion 0x11 for random access
void ReadW()
{
   Wire.beginTransmission(0x11);            // Device 0x11 for random access
   Wire.write(0x0C);                                // Start at Register 0x0C
   Wire.endTransmission(0);                         // restart condition
   Wire.requestFrom(0x11,8, 1);       // Retransmit device address with READ, followed by 8 bytes
   for (int i=0; i<4; i++) {RDS[i]=256*Wire.read()+Wire.read();}        // Read Data into Array of Unsigned Ints
   Wire.endTransmission();                  
 } 

 void get_RDS()
 {    
  int i;
  ReadW();      
  grupo=(RDS[1]>>12)&0xf;
      if(RDS[1]&0x0800) versio=1; else versio=0;  //Version A=0  Version B=1   
      if(versio==0)
      {
       #if DEBUG             
       sprintf(buffer,"Version=%d  Grupo=%02d ",versio,grupo); Serial.print(buffer);
    //    Serial.print(" 0->");Serial.print(RDS[0],HEX);Serial.print(" 1->");Serial.print(RDS[1],HEX);Serial.print(" 2->");Serial.print(RDS[2],HEX);Serial.print(" 3->");Serial.println(RDS[03],HEX);
    //    Serial.print(" 0->");Serial.print(RDS[0],BIN);Serial.print(" 1->");Serial.print(RDS[1],BIN);Serial.print(" 2->");Serial.print(RDS[2],BIN);Serial.print(" 3->");Serial.println(RDS[03],BIN);
    #endif 
    switch(grupo)
    {
     case 0:              
      #if DEBUG 
      Serial.print("_RDS0__");     
      #endif
      i=(RDS[1] & 3) <<1;
      seg_RDS[i]=(RDS[3]>>8);       
      seg_RDS[i+1]=(RDS[3]&0xFF);
      gotoXY(10,4);
      for (i=0;i<8;i++)
      {
        #if DEBUG 
        Serial.write(seg_RDS[i]);   
        #endif
        
        if(seg_RDS[i]>31 && seg_RDS[i]<128)
        LcdCharacter(seg_RDS[i]);
        else
        LcdCharacter(32);
      }  
      //Serial.print("FrecuAlt1-");Serial.println((RDS[2]>>8)+875);
      //Serial.print("FrecuAlt2-"); Serial.println(RDS[2]&0xFF+875);      
      
      #if DEBUG                 
      Serial.println("---");
      #endif
      break;
     case 2:
      i=(RDS[1] & 15) <<2;              
      seg_RDS1[i]=(RDS[2]>>8);       
      seg_RDS1[i+1]=(RDS[2]&0xFF);
      seg_RDS1[i+2]=(RDS[3]>>8);       
      seg_RDS1[i+3]=(RDS[3]&0xFF);
      #if DEBUG 
      Serial.println("_RADIOTEXTO_");
              //Serial.print(i);Serial.print("   ");Serial.println(RDS[1] & 15);
              //Serial.write(RDS[2]>>8); Serial.write (RDS[2]&0xFF);Serial.write(RDS[3]>>8);Serial.write(RDS[3]&0xFF);Serial.write("_");
              for (i=0;i<32;i++)  Serial.write(seg_RDS1[i]);                                    
              Serial.println("-TXT-");
              #endif      
              break;
              case 4:             
              i=RDS[3]& 0x003f;
              minuto=(RDS[3]>>6)& 0x003f;
              hora=(RDS[3]>>12)& 0x000f;
              if(RDS[2]&1) hora+=16;
              hora+=i;        
              z=RDS[2]>>1;
              julian=z;
              
              if(RDS[1]&1) julian+=32768;
              if(RDS[1]&2) julian+=65536;
              #if DEBUG 
              Serial.print("_DATE_");
              Serial.print(" Juliano=");Serial.print(julian);
              sprintf(buffer," %02d:%02d ",hora,minuto); gotoXY(38,2);  LcdString(buffer); 
              Serial.println(buffer); 
              #endif            
              break;
              default:
              #if DEBUG 
              Serial.println("__"); 
              #endif    
              ;        
            }                        
          }                   
        } 
        
// The pins to use on the arduino
#define PIN_SCE   7
#define PIN_RESET 6
#define PIN_DC    5
#define PIN_SDIN  4
#define PIN_SCLK  3 

// COnfiguration for the LCD
#define LCD_C     LOW
#define LCD_D     HIGH
#define LCD_CMD   0

// Size of the LCD
#define LCD_X     84
#define LCD_Y     48

int scrollPosition = -10;

static const byte ASCII[][5] =
{
 {0x00, 0x00, 0x00, 0x00, 0x00} // 20
,{0x00, 0x00, 0x5f, 0x00, 0x00} // 21 !
,{0x00, 0x07, 0x00, 0x07, 0x00} // 22 "
,{0x14, 0x7f, 0x14, 0x7f, 0x14} // 23 #
,{0x24, 0x2a, 0x7f, 0x2a, 0x12} // 24 $
,{0x23, 0x13, 0x08, 0x64, 0x62} // 25 %
,{0x36, 0x49, 0x55, 0x22, 0x50} // 26 &
,{0x00, 0x05, 0x03, 0x00, 0x00} // 27 '
,{0x00, 0x1c, 0x22, 0x41, 0x00} // 28 (
,{0x00, 0x41, 0x22, 0x1c, 0x00} // 29 )
,{0x14, 0x08, 0x3e, 0x08, 0x14} // 2a *
,{0x08, 0x08, 0x3e, 0x08, 0x08} // 2b +
,{0x00, 0x50, 0x30, 0x00, 0x00} // 2c ,
,{0x08, 0x08, 0x08, 0x08, 0x08} // 2d -
,{0x00, 0x60, 0x60, 0x00, 0x00} // 2e .
,{0x20, 0x10, 0x08, 0x04, 0x02} // 2f /
,{0x3e, 0x51, 0x49, 0x45, 0x3e} // 30 0
,{0x00, 0x42, 0x7f, 0x40, 0x00} // 31 1
,{0x42, 0x61, 0x51, 0x49, 0x46} // 32 2
,{0x21, 0x41, 0x45, 0x4b, 0x31} // 33 3
,{0x18, 0x14, 0x12, 0x7f, 0x10} // 34 4
,{0x27, 0x45, 0x45, 0x45, 0x39} // 35 5
,{0x3c, 0x4a, 0x49, 0x49, 0x30} // 36 6
,{0x01, 0x71, 0x09, 0x05, 0x03} // 37 7
,{0x36, 0x49, 0x49, 0x49, 0x36} // 38 8
,{0x06, 0x49, 0x49, 0x29, 0x1e} // 39 9
,{0x00, 0x36, 0x36, 0x00, 0x00} // 3a :
,{0x00, 0x56, 0x36, 0x00, 0x00} // 3b ;
,{0x08, 0x14, 0x22, 0x41, 0x00} // 3c <
,{0x14, 0x14, 0x14, 0x14, 0x14} // 3d =
,{0x00, 0x41, 0x22, 0x14, 0x08} // 3e >
,{0x02, 0x01, 0x51, 0x09, 0x06} // 3f ?
,{0x32, 0x49, 0x79, 0x41, 0x3e} // 40 @
,{0x7e, 0x11, 0x11, 0x11, 0x7e} // 41 A
,{0x7f, 0x49, 0x49, 0x49, 0x36} // 42 B
,{0x3e, 0x41, 0x41, 0x41, 0x22} // 43 C
,{0x7f, 0x41, 0x41, 0x22, 0x1c} // 44 D
,{0x7f, 0x49, 0x49, 0x49, 0x41} // 45 E
,{0x7f, 0x09, 0x09, 0x09, 0x01} // 46 F
,{0x3e, 0x41, 0x49, 0x49, 0x7a} // 47 G
,{0x7f, 0x08, 0x08, 0x08, 0x7f} // 48 H
,{0x00, 0x41, 0x7f, 0x41, 0x00} // 49 I
,{0x20, 0x40, 0x41, 0x3f, 0x01} // 4a J
,{0x7f, 0x08, 0x14, 0x22, 0x41} // 4b K
,{0x7f, 0x40, 0x40, 0x40, 0x40} // 4c L
,{0x7f, 0x02, 0x0c, 0x02, 0x7f} // 4d M
,{0x7f, 0x04, 0x08, 0x10, 0x7f} // 4e N
,{0x3e, 0x41, 0x41, 0x41, 0x3e} // 4f O
,{0x7f, 0x09, 0x09, 0x09, 0x06} // 50 P
,{0x3e, 0x41, 0x51, 0x21, 0x5e} // 51 Q
,{0x7f, 0x09, 0x19, 0x29, 0x46} // 52 R
,{0x46, 0x49, 0x49, 0x49, 0x31} // 53 S
,{0x01, 0x01, 0x7f, 0x01, 0x01} // 54 T
,{0x3f, 0x40, 0x40, 0x40, 0x3f} // 55 U
,{0x1f, 0x20, 0x40, 0x20, 0x1f} // 56 V
,{0x3f, 0x40, 0x38, 0x40, 0x3f} // 57 W
,{0x63, 0x14, 0x08, 0x14, 0x63} // 58 X
,{0x07, 0x08, 0x70, 0x08, 0x07} // 59 Y
,{0x61, 0x51, 0x49, 0x45, 0x43} // 5a Z
,{0x00, 0x7f, 0x41, 0x41, 0x00} // 5b [
,{0x02, 0x04, 0x08, 0x10, 0x20} // 5c &#194;&#165;
,{0x00, 0x41, 0x41, 0x7f, 0x00} // 5d ]
,{0x04, 0x02, 0x01, 0x02, 0x04} // 5e ^
,{0x40, 0x40, 0x40, 0x40, 0x40} // 5f _
,{0x00, 0x01, 0x02, 0x04, 0x00} // 60 `
,{0x20, 0x54, 0x54, 0x54, 0x78} // 61 a
,{0x7f, 0x48, 0x44, 0x44, 0x38} // 62 b
,{0x38, 0x44, 0x44, 0x44, 0x20} // 63 c
,{0x38, 0x44, 0x44, 0x48, 0x7f} // 64 d
,{0x38, 0x54, 0x54, 0x54, 0x18} // 65 e
,{0x08, 0x7e, 0x09, 0x01, 0x02} // 66 f
,{0x0c, 0x52, 0x52, 0x52, 0x3e} // 67 g
,{0x7f, 0x08, 0x04, 0x04, 0x78} // 68 h
,{0x00, 0x44, 0x7d, 0x40, 0x00} // 69 i
,{0x20, 0x40, 0x44, 0x3d, 0x00} // 6a j
,{0x7f, 0x10, 0x28, 0x44, 0x00} // 6b k
,{0x00, 0x41, 0x7f, 0x40, 0x00} // 6c l
,{0x7c, 0x04, 0x18, 0x04, 0x78} // 6d m
,{0x7c, 0x08, 0x04, 0x04, 0x78} // 6e n
,{0x38, 0x44, 0x44, 0x44, 0x38} // 6f o
,{0x7c, 0x14, 0x14, 0x14, 0x08} // 70 p
,{0x08, 0x14, 0x14, 0x18, 0x7c} // 71 q
,{0x7c, 0x08, 0x04, 0x04, 0x08} // 72 r
,{0x48, 0x54, 0x54, 0x54, 0x20} // 73 s
,{0x04, 0x3f, 0x44, 0x40, 0x20} // 74 t
,{0x3c, 0x40, 0x40, 0x20, 0x7c} // 75 u
,{0x1c, 0x20, 0x40, 0x20, 0x1c} // 76 v
,{0x3c, 0x40, 0x30, 0x40, 0x3c} // 77 w
,{0x44, 0x28, 0x10, 0x28, 0x44} // 78 x
,{0x0c, 0x50, 0x50, 0x50, 0x3c} // 79 y
,{0x44, 0x64, 0x54, 0x4c, 0x44} // 7a z
,{0x00, 0x08, 0x36, 0x41, 0x00} // 7b {
,{0x00, 0x00, 0x7f, 0x00, 0x00} // 7c |
,{0x00, 0x41, 0x36, 0x08, 0x00} // 7d }
,{0x10, 0x08, 0x08, 0x10, 0x08} // 7e &#195;&#162;&#194;&#134;&#194;&#144; 
//,{0x00, 0x06, 0x09, 0x09, 0x06} // 7f &#195;&#162;&#194;&#134;&#194;&#146;
,{B11111111, B01111110, B00011000, B01111110, B11111111} //Stereo 127
};

void LcdCharacter(char character)
{
  unsigned char z,z1;
  
  z1=character - 0x20;
  LcdWrite(LCD_D, 0x00);
  for (int index = 0; index < 5; index++)
  {
    //para que funciona en proteus
    
    z=ASCII[z1][index];
    LcdWrite(LCD_D, z);
   // LcdWrite(LCD_D, ASCII[character - 0x20][index]);    
 }
 LcdWrite(LCD_D, 0x00);
}

void LcdCharacterX(char character)
{
  unsigned char z,z1;
  z1=character - 0x20;       
  LcdWrite(LCD_D, 0x00);
  for (int index = 0; index < 5; index++)
  {
     //para que funciona en proteus
     z=ASCII[z1][index];
     LcdWrite(LCD_D, z);
     LcdWrite(LCD_D, z);
    //LcdWrite(LCD_D, ASCII[character - 0x20][index]);
    //LcdWrite(LCD_D, ASCII[character - 0x20][index]);
  }
  LcdWrite(LCD_D, 0x00);
}  

void LcdClear(void)
{
  for (int index = 0; index < LCD_X * LCD_Y / 8; index++)
  {
    LcdWrite(LCD_D, 0x00);
  }
}

void LcdInitialise(void)
{
  pinMode(PIN_SCE,   OUTPUT);
  pinMode(PIN_RESET, OUTPUT);
  pinMode(PIN_DC,    OUTPUT);
  pinMode(PIN_SDIN,  OUTPUT);
  pinMode(PIN_SCLK,  OUTPUT);
  
  digitalWrite(PIN_RESET, LOW);
  digitalWrite(PIN_RESET, HIGH);
  
  LcdWrite(LCD_CMD, 0x21);  // LCD Extended Commands.
  LcdWrite(LCD_CMD, 0xB5);  // Set LCD Vop (Contrast). //B1
  LcdWrite(LCD_CMD, 0x04);  // Set Temp coefficent. //0x04
  LcdWrite(LCD_CMD, 0x14);  // LCD bias mode 1:48. //0x13
  LcdWrite(LCD_CMD, 0x0C);  // LCD in normal mode. 0x0d for inverse
  LcdWrite(LCD_C, 0x20);
  LcdWrite(LCD_C, 0x0C);
}

void LcdString(char *characters)
{
  while (*characters)
  {
    LcdCharacter(*characters++);
  }
}
void LcdStringX(char *characters)
{
  while (*characters)
  {
    LcdCharacterX(*characters++);
  }
} 

void LcdWrite(byte dc, byte data)
{
  digitalWrite(PIN_DC, dc);
  digitalWrite(PIN_SCE, LOW);
  shiftOut(PIN_SDIN, PIN_SCLK, MSBFIRST, data);
  digitalWrite(PIN_SCE, HIGH);
}

/**
 * gotoXY routine to position cursor
 * x - range: 0 to 84
 * y - range: 0 to 5
 */
 void gotoXY(int x, int y)
 {
  LcdWrite( 0, 0x80 | x);  // Column.
  LcdWrite( 0, 0x40 | y);  // Row.
}

void drawBox(void)
{
  int j;
  for(j = 0; j <= 84; j++) // top
  {
    gotoXY(j, 0);
    LcdWrite(1, 0x01);
  }     
  
  for(j = 0; j < 84; j++) //Bottom
  {
    gotoXY(j, 5);
    LcdWrite(1, 0x80);
  }     
  
  for(j = 0; j < 6; j++) // Right
  {
    gotoXY(83, j);
    LcdWrite(1, 0xff);
  }     
  
  for(j = 0; j < 6; j++) // Left
  {
    gotoXY(0, j);
    LcdWrite(1, 0xff);
  }
}

void Scroll(String message)
{
  for (int i = scrollPosition; i < scrollPosition + 11; i++)
  {
    if ((i >= message.length()) || (i < 0))
    {
      LcdCharacter(' ');
    }
    else	
    {
      LcdCharacter(message.charAt(i));
    }
  }
  scrollPosition++;
  if ((scrollPosition >= message.length()) && (scrollPosition > 0))
  {
    scrollPosition = -10;
  }
}
