import processing.serial.*;
Serial mySerial;
PrintWriter output;
long counter = 0;

void setup() {
   mySerial = new Serial( this, Serial.list()[4], 115200 );
   output = createWriter( "../dataArd.csv" );
}

void draw() {
    if (mySerial.available() > 0 ) {
         String value = mySerial.readStringUntil('\n');
         println(value);
         if ( value != null ) {
              if(counter > 50) 
                output.print( value );
                output.flush();  
              counter++;
         }
    }
}

void keyPressed() {
    output.flush();  
    output.close(); 
    exit();  
}
