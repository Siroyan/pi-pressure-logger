#ifdef UNIT_TEST

#include <iostream>
#include <cassert>
#include "test_helpers.h"

// Simple test without Unity to verify compilation
int main() {
    std::cout << "Testing basic compilation..." << std::endl;
    
    // Test String class
    String test_str("Hello");
    assert(test_str.length() == 5);
    assert(strcmp(test_str.c_str(), "Hello") == 0);
    
    String test_str2 = test_str + String(" World");
    assert(test_str2.str() == "Hello World");
    
    // Test millis function
    unsigned long time1 = millis();
    unsigned long time2 = millis();
    assert(time2 > time1);
    
    // Test constrain function
    float result = constrain(5.0f, 0.0f, 10.0f);
    assert(result == 5.0f);
    
    result = constrain(-1.0f, 0.0f, 10.0f);
    assert(result == 0.0f);
    
    result = constrain(15.0f, 0.0f, 10.0f);
    assert(result == 10.0f);
    
    std::cout << "All basic tests passed!" << std::endl;
    return 0;
}

#else

int main() {
    std::cout << "Tests are only compiled with UNIT_TEST defined" << std::endl;
    return 0;
}

#endif