# Object (property)

Object reference property type 

# Synopsis
    
    
    class _name_ { object _variable_ ; }
    object _name_ { _variable_ _reference_ ; }
    

# Description

Object references allow a property to describe a reference to another object in the model. These are often used to access variables or functions in other objects. 

# Example

To declare an object reference use the syntax 
    
    
    class _name_ {
      object _variable_ ;
    }
    

where _name_ is the name of the class, and _variable_ is the name of the object reference. 

To define an object reference use the syntax 
    
    
    object _name_ {
      _variable_ _reference_ ;
    }
    

where _name_ is the name of the class, _variable_ is the name of the object reference, and _reference_ is the name of the object being referred to. Object names can either be a [defined name] or a [canonical name]. 

# See also

  * [Built-in types]
  * [Defined name]
  * [Canonical name]

