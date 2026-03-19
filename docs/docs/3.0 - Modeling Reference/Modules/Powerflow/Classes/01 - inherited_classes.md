## Inherited Classes

Nearly all objects within the **powerflow** module are derived from two primary objects: **node** and **link**. Therefore, any properties defined for these two objects are also available to any derived object. For example, a **node** has voltage properties, so a **load** automatically has these properties available as well. Any **powerflow** objects that inherit properties from **node** or **link** will be labeled as such. Furthermore, **node** and **link** contain most relevant default quantities. Derived objects often assume zero value or throw an error if an explicit property is not indicated. Any exceptions to this rule will be indicated in the parameter list of the particular object. 

