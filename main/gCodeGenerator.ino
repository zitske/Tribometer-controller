

void setup(){
    Serial.begin(115200);
}

void loop()
{
    int lenght=50;
    double middle=111.0;
    int tm=5;
    bool cc = false;
    
    linear(lenght,middle,tm);
    circular(lenght,middle,tm,cc);

}

String goHome(){
    return "G00 X0.0000 Y0.0000";
}
void linear(int lenght,double middle,int times){
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