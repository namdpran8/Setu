#pragma once
#include <cstdint>
#include <string>
#include <unordered_map>

enum class ValueType {
    NULL_TYPE,
    INT,
    FLOAT,
    OBJECT, // Pointer to a simulated Java Object instance (e.g. a View)
    UNINITIALIZED
};

// Represents any object reference in our VM
struct MockView {
    std::string debugTag; // e.g. "view_id_0x7f0a001a"
};

struct Value {
    ValueType type;
    union {
        int32_t i;
        float f;
        void* obj; 
    };

    // Constructor is public to allow arrays, defaults to NULL
    Value() : type(ValueType::NULL_TYPE), i(0) {}

public:
    static Value MakeNull() { return Value(); }
    
    static Value MakeUninitialized() { 
        Value v; 
        v.type = ValueType::UNINITIALIZED; 
        return v; 
    }
    
    static Value MakeInt(int32_t val) { 
        Value v; 
        v.type = ValueType::INT; 
        v.i = val; 
        return v; 
    }
    
    static Value MakeFloat(float val) { 
        Value v; 
        v.type = ValueType::FLOAT; 
        v.f = val; 
        return v; 
    }
    
    static Value MakeObject(void* val) { 
        Value v; 
        v.type = ValueType::OBJECT; 
        v.obj = val; 
        return v; 
    }
};

// Represents a real Java object (e.g. for click listeners)
struct InterpreterObject {
    std::string className;
    std::unordered_map<std::string, Value> fields;
};
