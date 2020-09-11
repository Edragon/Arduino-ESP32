#include <Arduino.h>

#include <TokenIterator.h>
#include <UrlTokenBindings.h>

void setup() {
  Serial.begin(115200);

  char p1[] = "example/path/a/b/c";
  char p2[] = "example/path/:var1/:var2/:var3";

  TokenIterator itr1(p1, strlen(p1), '/');
  TokenIterator itr2(p2, strlen(p2), '/');
  UrlTokenBindings bindings(itr1, itr2);

  if (bindings.hasBinding("var1")) {      // has this one
    Serial.println(bindings.get("var1")); // will print "a"
  }

  if (bindings.hasBinding("var4")) {
    // does not have this binding
  }
}

void loop() { }