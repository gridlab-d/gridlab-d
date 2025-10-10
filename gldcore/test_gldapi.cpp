#include "gldapi.h"
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "globals.h"
#include <json/json.h> //jsoncpp library

// Structure to hold object information
struct ObjectInfo {
    std::string className;
    std::string objectName;
    int indexInClass;
    
    ObjectInfo(const std::string& cls, const std::string& name, int idx) 
        : className(cls), objectName(name), indexInClass(idx) {}
};

// Get all class names from the checkpoint data
std::vector<std::string> getClassNames(const Json::Value& checkpoint) {
    std::vector<std::string> classNames;
    
    if (checkpoint.isMember("objects") && checkpoint["objects"].isObject()) {
        for (const std::string& className : checkpoint["objects"].getMemberNames()) {
            classNames.push_back(className);
        }
    }
    
    return classNames;
}

// Get all objects of a specific class
Json::Value getObjectsOfClass(const Json::Value& checkpoint, const std::string& className) {
    if (checkpoint.isMember("objects") && 
        checkpoint["objects"].isMember(className) &&
        checkpoint["objects"][className].isMember("instances")) {
        return checkpoint["objects"][className]["instances"];
    }
    
    return Json::Value(Json::arrayValue); // Return empty array
}

// Get a specific object by class name and index
Json::Value getObject(const Json::Value& checkpoint, const std::string& className, int index) {
    Json::Value objects = getObjectsOfClass(checkpoint, className);
    
    if (objects.isArray() && index >= 0 && index < (int)objects.size()) {
        return objects[index];
    }
    
    return Json::Value::nullSingleton();
}

// Get object property value by class, index, and property name
Json::Value getObjectProperty(const Json::Value& checkpoint, const std::string& className, int index, const std::string& propertyName) {
    Json::Value obj = getObject(checkpoint, className, index);
    
    if (!obj.isNull() && obj.isMember(propertyName)) {
        return obj[propertyName];
    }
    
    return Json::Value::nullSingleton();
}

// Find object by name within a class
Json::Value findObjectByName(const Json::Value& checkpoint, const std::string& className, const std::string& objectName) {
    Json::Value objects = getObjectsOfClass(checkpoint, className);
    
    if (objects.isArray()) {
        for (int i = 0; i < (int)objects.size(); i++) {
            Json::Value obj = objects[i];
            if (obj.isMember("name") && obj["name"].isString() && obj["name"].asString() == objectName) {
                return obj;
            }
        }
    }
    
    return Json::Value::nullSingleton();
}

// Get property value by object name
Json::Value getObjectPropertyByName(const Json::Value& checkpoint, const std::string& className, const std::string& objectName, const std::string& propertyName) {
    Json::Value obj = findObjectByName(checkpoint, className, objectName);
    
    if (!obj.isNull() && obj.isMember(propertyName)) {
        return obj[propertyName];
    }
    
    return Json::Value::nullSingleton();
}

// Get all objects across all classes
std::vector<ObjectInfo> getAllObjectNames(const Json::Value& checkpoint) {
    std::vector<ObjectInfo> allObjects;
    std::vector<std::string> classNames = getClassNames(checkpoint);
    
    for (const std::string& className : classNames) {
        Json::Value objects = getObjectsOfClass(checkpoint, className);
        
        if (objects.isArray()) {
            for (int i = 0; i < (int)objects.size(); i++) {
                Json::Value obj = objects[i];
                std::string objectName = className + "_" + std::to_string(i);
                
                if (obj.isMember("name") && obj["name"].isString()) {
                    objectName = obj["name"].asString();
                }
                
                allObjects.emplace_back(className, objectName, i);
            }
        }
    }
    
    return allObjects;
}

// Safe value extraction with type conversion
template<typename T>
T safeGetValue(const Json::Value& value, const T& defaultValue) {
    if (value.isNull()) return defaultValue;
    
    try {
        if constexpr (std::is_same_v<T, std::string>) {
            return value.isString() ? value.asString() : defaultValue;
        } else if constexpr (std::is_same_v<T, double>) {
            return value.isNumeric() ? value.asDouble() : defaultValue;
        } else if constexpr (std::is_same_v<T, int>) {
            return value.isInt() ? value.asInt() : defaultValue;
        } else if constexpr (std::is_same_v<T, long long> || std::is_same_v<T, int64_t>) {
            return value.isInt64() ? value.asInt64() : defaultValue;
        } else if constexpr (std::is_same_v<T, bool>) {
            return value.isBool() ? value.asBool() : defaultValue;
        }
    } catch (const std::exception&) {
        // Return default on any conversion error
    }
    
    return defaultValue;
}

int main(int argc, char* argv[]) {
    const char* fileName = "test_HVAC_balance.glm";
    // Instantiate GridLabD via exectuable path
    GridLabD gld;

    // Test set_config_file
    // gld.set_config_file("config.cfg");
    
    // Test load_glm
    std::vector<const char*> args = {"gridlabd", fileName, "--verbose"};
    int test_argc = static_cast<int>(args.size());
    char* test_argv[] = { const_cast<char*>(args[0]), const_cast<char*>(args[1]), const_cast<char*>(args[2])};
    gld.load_glm(test_argc, test_argv);

    gld.setup_after_load();

    TIMESTAMP start_time = convert_to_timestamp("2000-04-01 0:00:00");
    TIMESTAMP stop_time = convert_to_timestamp("2000-06-01 0:00:00");
    gld.run(start_time, stop_time);
    // gld.run();
    
    // Get all info for GLD
    Json::Value checkpoint = gld.get_checkpoint_json("/mnt/c/dev/gridlab-d_fork/_test_results/");
    
    std::cout << "\n=== Example Property Access ===" << std::endl;
    
    // Example 1: Get all class names
    std::vector<std::string> classes = getClassNames(checkpoint);
    std::cout << "Available classes: ";
    for (const std::string& cls : classes) {
        std::cout << cls << " ";
    }
    std::cout << std::endl;
    
    // Example 2: Get all objects of a specific class (load)
    if (std::find(classes.begin(), classes.end(), "office") != classes.end()) {
        Json::Value officeObjects = getObjectsOfClass(checkpoint, "office");
        std::cout << "\nOffice objects count: " << officeObjects.size() << std::endl;
        
        // Example 3: Get a specific office object (first one)
        if (officeObjects.size() > 0) {
            Json::Value firstOffice = getObject(checkpoint, "office", 0);
            if (!firstOffice.isNull()) {
                // Example 4: Get specific properties
                Json::Value name = getObjectProperty(checkpoint, "office", 0, "name");
                Json::Value floorArea = getObjectProperty(checkpoint, "office", 0, "floor_area");
                Json::Value heatingSetpoint = getObjectProperty(checkpoint, "office", 0, "heating_setpoint");
                
                std::cout << "Office name: " << safeGetValue<std::string>(name, "unknown") << std::endl;
                std::cout << "Floor area: " << safeGetValue<double>(floorArea, 0.0) << std::endl;
                std::cout << "Heating setpoint: " << safeGetValue<double>(heatingSetpoint, 0.0) << std::endl;
            }
        }
    }
    
    // Example 5: Find object by name (if you know the name)
    Json::Value namedOffice = findObjectByName(checkpoint, "office", "Testing office name");
    if (!namedOffice.isNull()) {
        std::cout << "\nFound office named 'Testing office name':" << std::endl;
        Json::Value area = getObjectPropertyByName(checkpoint, "office", "Testing office name", "floor_area");
        std::cout << "Floor area: " << safeGetValue<double>(area, 0.0) << std::endl;
    }
    
    // Example 6: List all object names across all classes
    std::vector<ObjectInfo> allObjects = getAllObjectNames(checkpoint);
    std::cout << "\n=== All Objects ===" << std::endl;
    for (const auto& obj : allObjects) {
        std::cout << obj.className << "[" << obj.indexInClass << "]: " << obj.objectName << std::endl;
    }
    
    // Example 7: Access clock information
    if (checkpoint.isMember("clock") && checkpoint["clock"].isMember("timestamp")) {
        Json::Value timestampValue = checkpoint["clock"]["timestamp"];
        std::cout << "\nClock section found!" << std::endl;
        std::cout << "Timestamp type: " << (timestampValue.isInt64() ? "Int64" : timestampValue.isInt() ? "Int" : timestampValue.isNumeric() ? "Numeric" : "Other") << std::endl;
        
        // Try multiple ways to access the timestamp
        if (timestampValue.isInt64()) {
            int64_t timestamp = timestampValue.asInt64();
            std::cout << "Simulation timestamp (Int64): " << timestamp << std::endl;
        } else if (timestampValue.isInt()) {
            int timestamp = timestampValue.asInt();
            std::cout << "Simulation timestamp (Int): " << timestamp << std::endl;
        } else if (timestampValue.isNumeric()) {
            double timestamp = timestampValue.asDouble();
            std::cout << "Simulation timestamp (Double): " << timestamp << std::endl;
        } else {
            std::cout << "Timestamp value: " << timestampValue.toStyledString() << std::endl;
        }
        
        // Also try the safe function
        int64_t safeTimestamp = safeGetValue<int64_t>(timestampValue, 0L);
        std::cout << "Safe timestamp: " << safeTimestamp << std::endl;
    } else {
        std::cout << "\nClock section not found or missing timestamp" << std::endl;
        if (checkpoint.isMember("clock")) {
            std::cout << "Clock section exists but timestamp missing" << std::endl;
            std::cout << "Clock contents: " << checkpoint["clock"].toStyledString() << std::endl;
        }
    }

    // Test step
    // gld.step(sim_time);

    // Test get_time
    // std::string current_time;
    // gld.get_time(current_time);
    // std::cout << "Current simulation time: " << current_time << std::endl;

    // Test set_time
    // gld.set_time("2025-06-12T12:00:00");

    // Test exit_gld
    gld.exit_gld(fileName);

    return 0;
}
