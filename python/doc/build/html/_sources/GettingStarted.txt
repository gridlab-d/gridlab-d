Getting Started
================
GridSpice provides a documented REST API which allows users to programmatically perform Create, Read, Update, Delete (CRUD) operations on all the objects in the GridSpice system according to the authenticated user's permissions.
 
 
In addition to this REST API, we provide a client python wrapper which allows:
 
* Importing, Creating, and Modifying models
* Running iterative, simulataneous simulations on the GridSpice cluster
* Collecting results in a structured format for easy post-processing 

.. note::
    This library is still 'experimental'.
 
>>> import gridspice
>>> myAccount = gridspice.account.login( 'akuw5nceip9rdrejoiuj74ko9842qxbmi63sfgvbxswhfkoujiyfwrt6434y8k' )
>>> myProjects = myAccount.showProjects()
>>> for project in myProjects:
...    	print project.name()
