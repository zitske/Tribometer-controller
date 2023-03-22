#include <Nextion.h>
#include <SoftwareSerial.h>

double middle=111.0;
// Declare your Nextion objects - Example (page id = 0, component id = 1, component name = "b0") 
//Linear
NexText LinearForcaNormal = NexText(1, 6, "n0");
NexText LinearCurso = NexText(1, 10, "n1"); 
NexText LinearDistTotal = NexText(1, 11, "n2"); 
NexButton LinearStart = NexButton(1, 1, "b0"); 
//Ratativo
NexText RatativoForcaNormal = NexText(7, 6, "n0");
NexText RatativoDiametro = NexText(7, 10, "n1"); 
NexText RatativoDistanciaTotal = NexText(7, 11, "n2"); 
NexButton RatativoStart = NexButton(7, 1, "b0"); 
//Manual
NexButton ManualHome = NexButton(6, 8, "b7"); 
NexButton ManualX = NexButton(6, 3, "b5");
NexButton ManualY = NexButton(6, 4, "b0");
NexButton ManualY_ = NexButton(6, 5, "b3");
NexButton ManualX_ = NexButton(6, 2, "b2");
NexButton ManualZ = NexButton(6, 6, "b4");
NexButton ManualZ_ = NexButton(6, 7, "b6");      
//Loading
NexProgressBar LoadingProgress = NexProgressBar(8, 1, "j0");
NexButton LoadingCancel = NexButton(8, 2, "b0");
//Home
NexButton Home = NexButton(0, 4, "b3");

//Change names to edit this
// Register a button object to the touch event list.  
NexTouch *nex_listen_list[] = {
  &LinearStart,
  &RatativoStart,
  &ManualHome,
  &LoadingCancel,
  NULL
};

//LinearStart Click
void LinearStartPopCallback(void *ptr) {
    Serial.println("oi1");
    //linear((LinearCurso.getText()).toInt(),middle,toTimes((LinearCurso.getText()).toInt(),(LinearDistTotal.getTExt()).toInt()),100);
    linear(100,middle,10,1000);
}
void RatativoStartPopCallback(void *ptr) {
    //linear((LinearCurso.getText()).toInt(),middle,toTimes((LinearCurso.getText()).toInt(),(LinearDistTotal.getTExt()).toInt()),100);
    Serial.println("oi2");
    circular(100,middle,10,true);
}
//SoftwareSerial SoftSerial(0, 1);
void setup(){
    Serial.begin(9600);
    //SoftSerial.begin(115200);
    //Serial.println("Serial: OK!");
    //SoftSerial.println("SoftSerial: OK!");
    nexInit();

     // Register the pop event callback function of the components
    LinearStart.attachPop(LinearStartPopCallback, &LinearStart);
    RatativoStart.attachPop(RatativoStartPopCallback, &RatativoStart);
    //h0.attachPop(h0PopCallback);
    //bUpdate.attachPop(bUpdatePopCallback, &bUpdate);
}

void loop()
{
    int lenght=50;
    int tm=5;
    bool cc = false;
    nexLoop(nex_listen_list);
    //linear(lenght,middle,tm,1000);
    //circular(lenght,middle,tm,cc);

}

String goHome(){
    return "G00 X0.0000 Y0.0000";
}
void linear(int lenght,double middle,int times,int speed){
    bool side = true;
    Serial.println(goHome());
    Serial.println("G00 X"+String(middle+lenght/2)+"Y"+String(middle));
    for(int i=0;i<times;i++){
        if(side){
            Serial.println("G1"+lenght*(-1));
            side = false;
        }else{
            Serial.println("G1"+lenght);
            side = true;
        }
    }
    Serial.println(goHome());
}
void circular(int radius,double middle,int times,bool cc){
    Serial.println(goHome());
    Serial.println("G00 X"+String(middle)+radius+"Y"+String(middle));
    
    for(int i=0;i<times;i++){
        if(cc){
            Serial.println("G2 X"+String(radius)+"I"+String(radius)+"J0");
        }else{
            Serial.println("G3 X"+String(radius)+"I"+String(radius*(-1))+"J0");
        }
    }
    Serial.println(goHome());
    
}

int toTimes(int curso,int totalDist){
    return (totalDist/curso);
}
