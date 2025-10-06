#ifndef TEST_HELPERS_H
#define TEST_HELPERS_H

#ifdef UNIT_TEST

#include <cstdint>
#include <string>

// Mock Arduino types and functions for testing

// Mock millis() function
unsigned long millis() {
    static unsigned long mock_time = 0;
    return mock_time += 100; // Increment by 100ms each call for predictable testing
}

// Mock Serial class
class MockSerial {
public:
    template<typename T>
    void println(T msg) {
        // In tests, we can ignore serial output or capture it if needed
    }
    
    template<typename T>
    void print(T msg) {
        // In tests, we can ignore serial output or capture it if needed
    }
};

static MockSerial Serial;

// Mock constrain function
template<typename T>
T constrain(T value, T min_val, T max_val) {
    if (value < min_val) return min_val;
    if (value > max_val) return max_val;
    return value;
}

// Mock Arduino String class
class String {
public:
    std::string data;
    
    String() {}
    String(const char* str) : data(str ? str : "") {}
    String(const std::string& str) : data(str) {}
    String(int val) : data(std::to_string(val)) {}
    String(long val) : data(std::to_string(val)) {}
    String(unsigned long val) : data(std::to_string(val)) {}
    String(float val) : data(std::to_string(val)) {}
    String(double val) : data(std::to_string(val)) {}
    
    size_t length() const { return data.length(); }
    const char* c_str() const { return data.c_str(); }
    
    String operator+(const String& other) const {
        return String(data + other.data);
    }
    
    String& operator+=(const String& other) {
        data += other.data;
        return *this;
    }
    
    bool operator==(const String& other) const {
        return data == other.data;
    }
    
    bool operator>(const String& other) const {
        return data > other.data;
    }
    
    int indexOf(char ch) const {
        size_t pos = data.find(ch);
        return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
    }
    
    int indexOf(const char* str) const {
        size_t pos = data.find(str);
        return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
    }
    
    int lastIndexOf(char ch) const {
        size_t pos = data.rfind(ch);
        return (pos == std::string::npos) ? -1 : static_cast<int>(pos);
    }
    
    String substring(int start, int end = -1) const {
        if (start < 0) start = 0;
        if (start >= (int)data.length()) return String("");
        
        if (end == -1) {
            return String(data.substr(start));
        }
        if (end <= start) return String("");
        if (end > (int)data.length()) end = data.length();
        
        return String(data.substr(start, end - start));
    }
    
    void replace(const char* find, const char* replace) {
        if (!find || !replace) return;
        size_t pos = 0;
        std::string findStr(find);
        std::string replaceStr(replace);
        while ((pos = data.find(findStr, pos)) != std::string::npos) {
            data.replace(pos, findStr.length(), replaceStr);
            pos += replaceStr.length();
        }
    }
    
    void replace(char find, char replace) {
        for (char& c : data) {
            if (c == find) c = replace;
        }
    }
    
    bool startsWith(const char* prefix) const {
        if (!prefix) return false;
        return data.find(prefix) == 0;
    }
    
    bool endsWith(const char* suffix) const {
        if (!suffix) return false;
        std::string suffixStr(suffix);
        if (suffixStr.length() > data.length()) return false;
        return data.compare(data.length() - suffixStr.length(), suffixStr.length(), suffixStr) == 0;
    }
    
    // For testing purposes
    std::string str() const { return data; }
};

#endif // UNIT_TEST

#endif // TEST_HELPERS_H