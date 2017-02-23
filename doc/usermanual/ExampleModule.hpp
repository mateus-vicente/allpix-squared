/*
 * Example module with name 'example'
 * 
 * NOTE: the factory builder is not included here
 * 
 * Possible instantiation in the configuration file would be:
 * [example]
 * param = "test"
 */

#include <allpix/module/Module.hpp>
#include <allpix/utils/log.h>

#include <memory>
#include <string>

/* 
WARNING: definition of the messages should never be part of a module!
*/
class InputMessage : public Message{
public:
    // NOTE: in a real message the output is of course not fixed
    std::string get(){
        return "an input message";
    }
}
class OutputMessage : public Message{
public:
    OutputMessage(std::string msg) msg_(msg) {}
    
    std::string get(){
        return msg;
    }
private:
    std::string msg_;
}

// define the module to inherit from the module base class
class ExampleModule : public Module{
public:
    // provide a static const variable of type string (required!)
    static const std::string name;
    
    // constructor should take a pointer to AllPix, a ModuleIdentifier and a Configuration as input
    ExampleModule(AllPix *apx, ModuleIdentifier id, Configuration config): Module(apx, id), message_(0) {
        // print a configuration parameter of type string to the logger
        LOG(DEBUG) << "my string parameter is " << config.get<std::string>("param", "<undefined>");
        
        // bind a variable to a specific message type that is automatically assigned if it is dispatched
        getMessenger()->bindSingle(this, &ExampleModule::message_);
    }
    
    // method that will be run where the module should do its computations and possibly dispatch their results as a message
    void run(){        
        // check if received a message
        if(message_){
            // print the message
            LOG(DEBUG) << "received a message: " << message_.get();
        }
        
        // construct my own message 
        OutputMessage msg("my output message");
        
        // dispatch my message
        getMessenger()->dispatchMessage(msg);
    }
private:
    std::shared_ptr<InputMessage> message_;
};

//set the name of the module
const std::string ExampleModule::name = "example";
