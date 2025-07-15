"""
Core data classes for GLM entities
"""


class Item:
    """Represents an individual attribute item in a GLM entity.
    
    This class encapsulates a single attribute with its metadata including
    datatype, label, unit, and value. It provides context manager support
    and JSON serialization capabilities.
    
    Attributes:
        datatype (str): The data type of the attribute (TEXT, REAL, INTEGER, etc.)
        label (str): Human-readable description of the attribute
        unit (str): Unit of measurement for the attribute
        item (str): The attribute name/identifier
        value: The actual value of the attribute
        range_check: Optional range validation information
    """
    
    def __init__(self, datatype, label, unit, item, value=None, 
                 range_check=None):
        """Initialize an Item with the given parameters.
        
        Args:
            datatype (str): The data type of the attribute
            label (str): Human-readable description
            unit (str): Unit of measurement
            item (str): The attribute name/identifier
            value: The actual value (optional)
            range_check: Range validation information (optional)
        """
        self.datatype = datatype
        self.label = label
        self.unit = unit
        self.item = item
        self.value = value
        self.range_check = range_check

    def __enter__(self):
        """Enter the runtime context for the context manager.
        
        Returns:
            Item: Returns self for context manager protocol
        """
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        """Exit the runtime context for the context manager.
        
        Args:
            exception_type: Type of exception that caused the context to be exited
            exception_value: Instance of the exception
            traceback: Traceback object
            
        Returns:
            bool: Always returns self (truthy) to suppress exceptions
        """
        return self

    def __repr__(self):
        """Return a string representation of the Item.
        
        Returns:
            str: String representation of the item's value
        """
        return str(self.value)

    def to_json(self):
        """Stringify the attribute in the Items to JSON.

        Returns:
            str: JSON string with label, unit, datatype, value
        """
        tmp = ('{ ' + self.item + ': {' +
               '"label": "' + self.label + ', ' +
               '"unit": "' + self.unit + ', ' +
               '"datatype": "' + self.datatype + ', ' +
               '"value": ')
        if self.datatype in ["TEXT"]:
            tmp = tmp + '"' + self.value + '"}'
        else:  # self.datatype in ["REAL", "INTEGER"]:
            tmp = tmp + self.value + '}'
        return tmp

    
class Entity:
    """Represents a GLM entity that can contain multiple Item attributes.
    
    This class manages a collection of attributes (Items) and provides
    functionality for JSON serialization, schema generation, and instance
    management. It serves as the base class for GLM model entities.
    
    Attributes:
        item_cnt (int): Count of items in this entity
        entity (str): Name/type of the entity
        instances (dict): Dictionary of entity instances
    """
    
    def __init__(self, entity, config):
        """Initialize an Entity with the given configuration.
        
        Args:
            entity (str): Name/type of the entity
            config: Configuration data for initializing the entity attributes
        """
        self.item_cnt = 0
        self.entity = entity
        self.instances = {}
        try:
            if isinstance(config, list) and len(config) > 0:
                for attr in config:
                    # config format -> label=0, value=1, unit=2, 
                    # datatype=3, item=4, range
                    # Item format -> datatype=3, label=0, unit=2, 
                    # item=4, value=1, range
                    if len(attr) > 5:
                        tmp = Item(attr[3], attr[0], attr[2], attr[4], 
                                  attr[1], attr[5])
                    else:
                        tmp = Item(attr[3], attr[0], attr[2], attr[4], 
                                  attr[1])
                    setattr(self, attr[4], tmp)
                self.item_cnt = len(config)
        except (IndexError, TypeError):
            pass

    def add_attr(self, datatype, label, unit, item, value=None):
        """Add the Item attribute to the Entity.

        Args:
            datatype (str): Describes the datatype of the attribute
            label (str): Describes the attribute
            unit (str): The unit name of the attribute
            item (str): The name of the attribute
            value (any): The value of the item
            
        Returns:
            Item: The created Item attribute
        """
        val = Item(datatype, label, unit, item, value)
        setattr(self, item, val)
        self.item_cnt += 1
        return self.__getattribute__(item)

    def find_item(self, item):
        """Find the Item from the Entity.

        Args:
            item (str): name of the attribute in the entity
            
        Returns:
            Item: The found Item or None if not found
        """
        try:
            return self.__getattribute__(item)
        except AttributeError:
            return None
        
    def to_json(self):
        """Convert the Entity to a JSON-serializable dictionary.
        
        Recursively serializes all attributes of the entity, handling
        nested dictionaries, lists, and custom objects.
        
        Returns:
            dict: A dictionary representation suitable for JSON serialization
        """
        def serialize(value):
            if isinstance(value, (str, int, float, bool)) or value is None:
                return value  # Keep primitive types as-is
            elif isinstance(value, dict):
                # Recursively serialize dictionaries
                return {k: serialize(v) for k, v in value.items()}
            elif isinstance(value, list):
                # Recursively serialize lists
                return [serialize(v) for v in value]
            else:
                # Convert non-serializable objects to strings
                return str(value)

        serialized_dict = {key: serialize(value) 
                          for key, value in self.__dict__.items()}
        return serialized_dict        
        
    def to_schema(self):
        """Generate a JSON object with the item types listed (schema/model).
        
        Including nested structures.

        Returns:
            dict: A dictionary representing the schema of the Entity.
        """
        exclude_keys = {"item_cnt", "entity", "_m", "instances"}

        def map_type(value):
            type_mapping = {
                str: "TEXT",
                int: "INTEGER",
                float: "REAL",
                dict: "OBJECT",
                list: "ARRAY",
                bool: "BOOLEAN",
                type(None): "NULL"
            }
            return type_mapping.get(type(value), "UNKNOWN")

        def serialize_schema(value):
            if isinstance(value, Item):
                if value.unit is not None and value.unit != "":
                    return {"dataType": value.datatype,
                            "unit": value.unit}
                else:   
                    return {"dataType": value.datatype}
            elif isinstance(value, dict):
                return value
            elif isinstance(value, Entity):
                return value.to_schema()
            else:
                return map_type(value)

        schema = {}
        for attr in self.__dict__:
            if attr in exclude_keys:
                continue
            item = getattr(self, attr)
            schema[attr] = serialize_schema(item)
        return schema
        
    def set_instance(self, object_name, params):
        """Set the Entity instance the given set of parameters.

        Args:
            object_name (str): the name of the instance
            params (dict): list of the attribute parameters
            
        Returns:
            Entity instance: an object with name and values
        """
        if isinstance(object_name, str):
            try:
                instance = self.instances[object_name]
            except KeyError:
                self.instances[object_name] = {}
                instance = self.instances[object_name]

            for attr in params:
                if attr.startswith(("inline_comment", "inside_comment", 
                                   "comment", "outside_comments")):
                    instance[attr] = params[attr]
                    continue
                item = self.find_item(attr)
                if isinstance(item, Item):
                    try:
                        _ = instance[attr]
                    except KeyError:
                        if isinstance(attr, str):
                            instance[attr] = {}
                        else:
                            print(f"Attribute id is not a string in "
                                  f"{self.entity} named {object_name}")
                            continue
                else:
                    # add to dictionary datatype, label, unit, item, value
                    if (self.find_item("parent") or 
                        self.find_item("configuration")):
                        # TODO: lookup attr in parent, configuration if it exists,
                        # for now add it
                        self.add_attr("TEXT", attr, "", attr, "")
                    else:
                        print(f"Unrecognized parameter {attr} in "
                              f"{self.entity} named {object_name}")
                instance[attr] = params[attr]
            return instance
        else:
            print(f"object_name is not a string in {self.entity}")
        return None       

class O_Entity(Entity):
    """Object Entity class that extends Entity with model reference.
    
    This class represents object entities in GLM models that need access
    to the parent model for operations like adding and deleting objects.
    It extends the base Entity class with model-specific functionality.
    
    Attributes:
        _m: Reference to the parent GLM model
    """
    
    def __init__(self, model, entity, config):
        """Initialize an O_Entity with model reference.
        
        Args:
            model: Reference to the parent GLM model
            entity (str): Name/type of the entity
            config: Configuration data for initializing the entity
        """
        super().__init__(entity, config)
        self._m = model

    def add(self, name, params):
        """Add a new object to the model.
        
        Args:
            name (str): Name of the object to add
            params (dict): Parameters for the new object
            
        Returns:
            The created object instance
        """
        return self._m.add_object(self.entity, name, params)

    def delete(self, name):
        """Delete an object from the model.
        
        Args:
            name (str): Name of the object to delete
        """
        self._m.del_object(self.entity, name)

    def items(self):
        """Return an iterator over the entity instances.
        
        Returns:
            dict_items: Iterator over (name, instance) pairs
        """
        return self.instances.items()
    
    def keys(self):
        """Return an iterator over the entity instance names.
        
        Returns:
            dict_keys: Iterator over instance names
        """
        return self.instances.keys()

    def __getitem__(self, key):
        """Get an instance by name.
        
        Args:
            key (str): Name of the instance to retrieve
            
        Returns:
            The instance if found, None otherwise
        """
        return self.instances.get(key)

    
class GLM:
    """Main GLM class placeholder.
    
    This class serves as a placeholder for the main GLM model structure.
    It can be extended to hold GLM-specific functionality and serve as
    a namespace for GLM-related operations.
    """
    pass
