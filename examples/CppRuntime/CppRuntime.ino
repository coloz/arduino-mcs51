struct Counter { virtual int value() const { return 7; } };
Counter counter;
volatile int result;
int (Counter::*method)() const = &Counter::value;
void setup() { Counter *p = &counter; result = (p->*method)(); String s("stc"); s += "51"; result += s.length(); }
void loop() {}
