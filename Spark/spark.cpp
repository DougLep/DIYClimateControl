bool state = LOW;

void setup() {
    pinMode(D0,OUTPUT);
    digitalWrite(D0,LOW); // turn on D0 by default
    Spark.function("enable", enablefunc);
    Spark.function("disable", disablefunc);
}

void loop() {
	// do nothing
}

int enablefunc(String args) 
{
    state = HIGH;
    digitalWrite(D0,state);
    return 200; // This is checked in the web app controller for validation
}

int disablefunc(String args)
{
    state = LOW;
    digitalWrite(D0, state);
    return 201;
}