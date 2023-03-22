#define dirPinX 51
#define stepPinX 47
#define dirPinY 0
#define stepPinY 1
#define dirPinZ 2
#define stepPinZ 3
#define stepsPerRevolution 2000
int xPosition = 0;
int yPosition = 0;
int zPosition = 0;

int btX = 0;
int btY = 0;
int btZ = 0;

void setup() {
  // Declare pins as output:
  Serial.begin(115200);
  pinMode(stepPinX, OUTPUT);
  pinMode(stepPinY, OUTPUT);
  pinMode(stepPinZ, OUTPUT);
  pinMode(dirPinX, OUTPUT);
  pinMode(dirPinY, OUTPUT);
  pinMode(dirPinZ, OUTPUT);
  linear(100,10,100);
}

void loop() {
 
  
  
  
}
/*
axis:
0 = X
1 = Y
2 = Z 
*/
void mDirection(int dir,int axis){
  switch (dir){
    case 0:
    if(axis == 0){
      digitalWrite(dirPinX, HIGH);
      }else if(axis == 1){
        digitalWrite(dirPinY, HIGH);
        }else if(axis == 2){
          digitalWrite(dirPinZ, HIGH);
          }
    
    break;
    case 1:
    
    if(axis == 0){
      digitalWrite(dirPinX, LOW);
      }else if(axis == 1){
        digitalWrite(dirPinY, LOW);
        }else if(axis == 2){
          digitalWrite(dirPinZ, LOW);
          }
    break;
    default:
    if(axis == 0){
      digitalWrite(dirPinX, HIGH);
      }else if(axis == 1){
        digitalWrite(dirPinY, HIGH);
        }else if(axis == 2){
          digitalWrite(dirPinZ, HIGH);
          }
    break;
}
}




void moveAxis(int mm,int dir,int vel,int axis){
     mDirection(dir,axis);
     int passos =  mm*(stepsPerRevolution/5);
     //int velo = 6000000 / (vel*stepsPerRevolution);
     //Serial.println(velo);
     int velo = map(vel,0,100,2000,80);
     Serial.println(velo);
     for (int i = 0; i < passos; i++) {
      // These four lines result in 1 step:
      if(axis == 0){
      digitalWrite(stepPinX, HIGH);
      }else if(axis == 1){
        digitalWrite(stepPinY, HIGH);
        }else if(axis == 2){
          digitalWrite(stepPinZ, HIGH);
          }
      delayMicroseconds(velo);
      if(axis == 0){
      digitalWrite(stepPinX, LOW);
      }else if(axis == 1){
        digitalWrite(stepPinY, LOW);
        }else if(axis == 2){
          digitalWrite(stepPinZ, LOW);
          }
      delayMicroseconds(velo);
    
     /*
     mm/s
     1 mm/m = 2000 passos a cada 60000000 Microseconds
     1000 mm/m =  60000000 / 1000 * 2000
     
      0,005mm = 200 passo = 4000 Microseconds
      Slow 2000
      
      1 passo = 1000 Microseconds
      Fast 500
      */
    }
   
}

void linear(int distancia, int curso,int velocidade){
  int times = distancia/curso;
  for(int i = 0;i<times/2;i++){
    moveAxis(curso,1,velocidade,0);
    //delay(1000);
    moveAxis(curso,0,velocidade,0);
  }
}

void circular(int distancia,int diametro,int velocidade){
  int times = distancia/diametro;
  for(int i = 0;i<times/2;i++){
    moveAxis(diametro,1,velocidade,0);
    moveAxis(diametro,1,velocidade,2);
    //delay(1000);
    moveAxis(diametro,0,velocidade,0);
    moveAxis(diametro,0,velocidade,2);
  }
}

void alignAxis(int axis){
  int velocidade = 100;
  if(axis == 0){
      while(btX == 0){
        moveAxis(2000000,1,velocidade,0);
      }
      xPosition = 0;
  }else if(axis == 1){
      while(btY == 0){
        moveAxis(2000000,1,velocidade,1);
      }
      yPosition = 0;
  }else if(axis == 2){
      while(btZ == 0){
        moveAxis(2000000,1,velocidade,2);
      }
      zPosition = 0;
  }
}


//360 = 200p =5mm
//360 -> 5mm
