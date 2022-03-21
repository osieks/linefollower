//#include <SoftwareSerial.h>

int swap;
byte p[8];
int i;

//SoftwareSerial mySerial(2,3);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  swap=0;  
  p[0] = 0;
  p[1] = 1;
  p[2] = 2;
  p[3] = 3;
  p[4] = 4;
  p[5] = 5;
  p[6] = 6;
  p[7] = 7;
}

void loop() {
  // put your main code here, to run repeatedly:

  Serial.write(p,sizeof(p));

  swap=p[7];
  for(i=6;i>=0;i--){
    p[i+1] = p[i];
  }
  p[0]=swap;
  /*
  for(i=0;i<=7;i++){
    Serial.print(p[i]);
  }  
  Serial.println();  
  */
  delay(1000);
}
