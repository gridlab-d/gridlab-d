"""
Utility functions for GLM processing
"""

def gld_strict_name(val):
    """ Sanitizes a name for GridLAB-D publication to FNCS
    GridLAB-D name should not begin with a number, or contain '-' for FNCS

    Args:
        val (str): the input name

    Returns:
        str: val with all '-' replaced by '_', and any leading digit replaced by 'gld_'
    """
    val = val.replace('"', '')
    if val[0].isdigit():
        val = "gld_" + val
    return val.replace('-', '_')

def get_datatype(m_type: str):
    """Convert GLM type to standard datatype
    
    Args:
        m_type (str): GLM type name
        
    Returns:
        str: Standard datatype
    """
    if m_type == "double":
        datatype = "REAL"
    elif m_type in ["char8", "char32", "char256", "char1024"]:
        datatype = "TEXT"
    elif m_type in ["int16", "int32", "int64"]:
        datatype = "INTEGER"
    elif m_type in ["enumeration", "set"]:
        datatype = "TEXT"
    elif m_type == "bool":
        datatype = "BOOLEAN"
    elif m_type == "timestamp":
        datatype = "TEXT"
    elif m_type == "complex":
        datatype = "TEXT"
    elif m_type == "complex_array":
        datatype = "TEXT"
    elif m_type == "double_array":
        datatype = "TEXT"
    elif m_type in ["enduse", "loadshape", "object", "parent"]:
        datatype = "OBJECT"
    else:
        datatype = ""
    return datatype

def add_attr_to_entity(entity, name, attr):
    """Add attribute to entity based on GLM attribute definition
    
    Args:
        entity: Entity object to add attribute to
        name (str): Attribute name
        attr (dict): Attribute definition from GLM classes
    """
    # unit with define unit or if "enumeration" or "set" use 'keywords' seperated by '|'
    unit = ""
    if "unit" in attr:
        unit = attr["unit"]
    # label with name otherwise the description
    label = name.replace("_", " ").replace(".", " ")
    if "description" in attr:
        label = attr["description"]

    # all attribute must have a type
    if "type" in attr:
        m_type = attr["type"]
        m_datatype = get_datatype(m_type)
        if m_type in ["enumeration", "set"]:
            unit = "|"
            for key in attr["keywords"]:
                unit += key + "|"
        elif m_type == "bool":
            unit = "|true|false|"
        if m_datatype:
            entity.add_attr(m_datatype, label, unit, name, value=None)
        else:
            print(f"name: {name} type: {m_type}")
