---
What are we doing here?

Who is a “modeler” and what this guide intends to provide?

Reference and on-boarding for new modelers (people who write .glms)

Class hierarchy – In GLD, there is a class hierarchy which affects that parameter list for an object. This makes it good to know which objects are sub-classes of other objects. For example, there is a “node” object with, say, a “voltage\_A” property. The “meter” object is a subclass of “node” which means, even though “voltage\_A” won’t show up in its property list, “voltage\_A” is a valid property.

Device models – For each device the following

* Specification page – All the math; this may be something that we try to write into the source code and extract to build the webpage or we write the webpage (via Markdown) and then reference it in the code. We don’t want to have to describe these equations twice (once as code comments and once in the documentation).
* User page – Parameter list for the object and what each parameter does. Again, may be pulled from the existing source code documentation and created as a webpage; we just don’t want to write things down twice. This is likely to be the more popular page as it is what you need when you’re trying to put a .glm together.

Support objects

* Recorders
  * Output formats
  * Connecting to databases (if this is still a supported feature)
* Players
* Schedules
* Timing (assuming it exists and replaces “starttime” “min\_timeste”, etc)
* Message (may not be needed if we have libgld?)

Other objects

* Weather
* Market

Verifying/debugging a model

* What are the red flags?
* Common warnings you might be able to avoid
* Running validation scripts – Something that post-processes results and looks for modeling abnormalities

----------------