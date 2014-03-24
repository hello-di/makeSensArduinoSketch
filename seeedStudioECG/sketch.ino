void setup()
{
	Serial.begin(115200);
	Serial.println("Please ready your chest belt.");
	Serial.println("Heart rate test begin.");
	attachInterrupt(0, count, RISING);//set interrupt 0,digital port 2
}

void count() {
	Serial.println("got on interrupt");
}

void loop()
{
}