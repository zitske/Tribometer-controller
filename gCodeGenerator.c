#include <iostream>
#include <string>

using namespace std;

string goHome(){
    return "G00 X0.0000 Y0.0000";
}

void linear(int lenght,double middle,int times){
    bool side = true;
    cout<<goHome();
    cout<<"G00 X"<<middle+lenght/2<<"Y"<<middle;
    
    for(int i=0;i<times;i++){
        if(side){
            cout<<"G1"<<lenght*(-1);
            side = false;
        }else{
            cout<<"G1"<<lenght;
            side = true;
        }
    }
    cout<<goHome();
}

void circular(int radius,double middle,int times,bool cc){
    cout<<goHome();
    cout<<"G00 X"<<middle+radius<<"Y"<<middle;
    
    for(int i=0;i<times;i++){
        if(cc){
            cout<<"G2 X"<<radius<<"I"<<radius<<"J0";
        }else{
            cout<<"G3 X"<<radius<<"I"<<radius*(-1)<<"J0";
        }
    }
    cout<<goHome();
    
}

int main()
{
    int lenght=50;
    double middle=111.0;
    int tm=5;
    bool cc = false;
    
    linear(lenght,middle,tm);
    circular(lenght,middle,tm,cc);
    return 0;
}

