#define dirPin 51
#define stepPin 47
#define stepsPerRevolution 2000

void setup() {
  // Declare pins as output:
  Serial.begin(115200);
  pinMode(stepPin, OUTPUT);
  pinMode(dirPin, OUTPUT);
  linear(100,10,100);
}

void loop() {
 
  
  
  
}



void directionX(int dir){
  switch (dir){
    case 0:
    digitalWrite(dirPin, HIGH);
    break;
    case 1:
    digitalWrite(dirPin, LOW);
    break;
    default:
    digitalWrite(dirPin, HIGH);
    break;
}
}


void moveX(int mm,int dir,int vel){
     directionX(dir);
     int passos =  mm*(stepsPerRevolution/5);
     //int velo = 6000000 / (vel*stepsPerRevolution);
     //Serial.println(velo);
     int velo = map(vel,0,100,2000,80);
     Serial.println(velo);
     for (int i = 0; i < passos; i++) {
      // These four lines result in 1 step:
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(velo);
      digitalWrite(stepPin, LOW);
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
    moveX(curso,1,velocidade);
    //delay(1000);
    moveX(curso,0,velocidade);
  }
  
}

//360 = 200p =5mm
//360 -> 5mm
