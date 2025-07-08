"""
Core data classes for GLM entities
"""

class Item:
    def __init__(self, datatype, label, unit, item, value=None, range_check=None):
        self.datatype = datatype
        self.label = label
        self.unit = unit
        self.item = item
        self.value = value
        self.range_check = range_check

    def __enter__(self):
        return self

    def __exit__(self, exception_type, exception_value, traceback):
        return self

    def __repr__(self):
        return str(self.value)

    def to_json(self):
        """ Stringify the attribute in the Items to JSON

        Returns:
            str: JSON string with label, unit, datatype, value
        """
        tmp = '{ ' + self.item + ': {' \
              '"label": "' + self.label + ', ' \
              '"unit": "' + self.unit + ', ' \
              '"datatype": "' + self.datatype + ', ' \
              '"value": '
        if self.datatype in ["TEXT"]:
            tmp = tmp + '"' + self.value + '"}'
        else:  # self.datatype in ["REAL", "INTEGER"]:
            tmp = tmp + self.value + '}'
        return tmp
    
class Entity:
    def __init__(self, entity, config):
        self.item_cnt = 0
        self.entity = entity
        self.instances = {}
        try:
            if type(config[0]) is list:
                for attr in config:
                    # config format -> label=0, value=1, unit=2, datatype=3, item=4, range
                    # Item format -> datatype=3, label=0, unit=2, item=4, value=1, range
                    if len(attr) > 5:
                        tmp = Item(attr[3], attr[0], attr[2], attr[4], attr[1], attr[5])
                    else:
                        tmp = Item(attr[3], attr[0], attr[2], attr[4], attr[1])
                    setattr(self, attr[4], tmp)
                self.item_cnt = len(config)
        except:
            pass

    def add_attr(self, datatype, label, unit, item, value=None):
        """ Add the Item attribute to the Entity

        Args:
            datatype (str): Describes the datatype of the attribute
            label (str): Describes the attribute
            unit (str): The unit name of the attribute
            item (str): The name of the attribute
            value (any): The value of the item
        Returns:
            Item:
        """
        val = Item(datatype, label, unit, item, value)
        setattr(self, item, val)
        self.item_cnt += 1
        return self.__getattribute__(item)

    def find_item(self, item):
        """ Find the Item from the Entity

        Args:
            item (str): name of the attribute in the entity
        Returns:
             Item:
        """
        try:
            return self.__getattribute__(item)
        except:
            return None
        
    def to_json(self):
        def serialize(value):
            if isinstance(value, (str, int, float, bool)) or value is None:
                return value  # Keep primitive types as-is
            elif isinstance(value, dict):
                return {k: serialize(v) for k, v in value.items()}  # Recursively serialize dictionaries
            elif isinstance(value, list):
                return [serialize(v) for v in value]  # Recursively serialize lists
            else:
                return str(value)  # Convert non-serializable objects to strings

        serialized_dict = {key: serialize(value) for key, value in self.__dict__.items()}
        return serialized_dict        
        
    def to_schema(self):
        """Generate a JSON object with the item types listed (schema/model), including nested structures.

        Returns:
            dict: A dictionary representing the schema of the Entity.
        """
        exclude_keys = {"item_cnt", "entity", "_m", "instances"}  # Add more keys as needed

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
                return {k: serialize_schema(v) for k, v in value.items()}
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
        """ Set the Entity instance the given set of parameters

        Args:
            object_name (str): the name of the instance
            params (dict): list of the attribute parameters
        Returns:
            Entity instance: an object with name and values
        """
        if type(object_name) == str:
            try:
                instance = self.instances[object_name]
            except:
                self.instances[object_name] = {}
                instance = self.instances[object_name]

            for attr in params:
                if attr.startswith("inline_comment") or attr.startswith("inside_comment") or attr.startswith("comment") or attr.startswith("outside_comments"):
                    instance[attr] = params[attr]
                    continue
                item = self.find_item(attr)
                if type(item) == Item:
                    try:
                        _ = instance[attr]
                    except:
                        if type(attr) == str:
                            instance[attr] = {}
                        else:
                            print("Attribute id is not a string in", self.entity, "named", object_name)
                            continue
                else:
                    # add to dictionary datatype, label, unit, item, value
                    if self.find_item("parent") or self.find_item("configuration"):
                        # todo lookup attr in parent, configuration if it exists, for now add it
                        self.add_attr("TEXT", attr, "", attr, "")
                    else:
                        print("Unrecognized parameter", attr, "in", self.entity, "named", object_name)
                instance[attr] = params[attr]
            return instance
        else:
            print("object_name is not a string in", self.entity)
        return None       

class O_Entity(Entity):
    def __init__(self, model, entity, config):
        super().__init__(entity, config)
        self._m = model

    def add(self, name, params):
        return self._m.add_object(self.entity, name, params)

    def delete(self, name):
        self._m.del_object(self.entity, name)

    def items(self):
        return self.instances.items()
    
    def keys(self):
        return self.instances.keys()

    def __getitem__(self, key):
        return self.instances.get(key)
    
class GLM:
    pass
