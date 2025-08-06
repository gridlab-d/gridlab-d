# CSV reader

The CSV reader class is a helper object for the [ climate class] in the [Climate module]. It is used to override the normal file-parsing behavior, which reads the input file in as a TMY2 file, and instead reads its file in as a series of comma-separated values. 

## CSV reader behavior

The CSV reader is intentionally designed to have no active components. It does nothing during the create, init, and sync steps. 

## CSV reader actors

### Climate object

The CSV reader is used similar to a puppet by a climate object. If a climate object specifies that it is using a CSV reader, that it is reading in a .csv file, it will either use the CSV reader object referenced in its reader property, or a new reader object if a reference does not exist. The climate object will then call upon the CSV reader to parse the CSV file, which will create a list of time-ordered weather objects. When the climate object updates, it requests data from the CSV reader, which will identify which indexed row is appropriate for the current time and date, then copy the current weather data into the calling climate object. 

#### Input file

The input file is expected to be formatted with three types of lines: comments, class data, and weather data. 

##### Class Data

The CSV reader input file can contain inputs for the public properties of the CSV reader class. Any line with the format 
    
    
    $property_name=property_value
    

will be used to fill the specified value into the named property. The name will be read up to the equality sign, and the value will be read to the end of the line. 

##### Comments

Any line in the CSV file that immediately begins with the hash symbol, #, will be treated as a comment and disregarded. 

##### Weather Header

If the CSV reader has not yet found or not pre-defined the column header, the first line that is not a comment and is not setting a class property will be interpreted as the property to reference for each column of data within the CSV reader. These property names must be separated with commas, and with no extraneous white space. 

##### Weather Data

When the parser has a column header, any line that is not a comment or a class property is read as weather data. The first value is a timestamp, and any following values fill in the column that corresponds with the names provided in the header. The timestamp format can be manually set with a formatted scan string (see [scanf()](http://en.wikipedia.org/wiki/Scanf)), otherwise will default to "%d:%d:%d:%d:%d". The input timestamp will read as many tokens as it is able, and will taken them in order as the month, day, hour, minute, and second of a year to fill in the associated values as the current weather data. Lines of weather data should be chronologically sequential; if the data's timestamp is earlier than the previous timestamp, it will be ignored and discarded. 

## Properties

### index

The entry index number that the reader is using for the current weather values. 

### city_name

The name of the city the weather data is associated with. 

### state_name

The name of the state that the city is in for the associated weather data. 

### lat_deg

The whole degree latitude for the location that this weather data was recorded. North values are positive, south values are negative. 

### lat_min

The sub-degree minutes of latitude for the location that this weather data was recorded. 

### long_deg

The whole degree longitude for the location that this weather data was recorded. West values are negative, east values are positive. 

### long_min

The sub-degree minutes of longitude for the location that this weather data was recorded. 

### high_temp

The highest observed temperature in the data set. 

### peak_solar

The highest observed solar input recorded in the data set. 

### status

The current state of the weather reader. 

#### INIT

The file has not been opened and no data has been read. 

#### OPEN

The file has been opened and the data is either in the process of being read and processed, or is currently being used by the parent climate object. 

#### ERROR

The file was opened, but an error occurred while reading and parsing the file. The file has been closed and the reader is not usable by the system. 

### timefmt

The string format to use for reading in timestamps from the file. By default, the format is "%d:%d:%d:%d:%d". The order of the values is the month, day, hour, minute, then second that the associated weather data will be used. The same dates are used for multiple years; individual years cannot be specified. Any value that is omitted defaults to zero, thus applying the value to the entirety of the omitted interval. Alternate formats must preserve the interval ordering, but may alter the format so long as up to five integers are read in. 

### timezone

The timezone the weather data's source city is in. Should be a three-letter code, akin to "GMT", "PST", or "EDT". 

### columns

A list of headers for the columns. Each column name must match a property name in the [weather] object, else an error will occur. If this property is omitted, the parser will use the first line that is not a property definition and is not a comment as the column headers. 

### filename

The name of the CSV file to read weather data from. 

## Functions

The CSV reader has no published functions. 

## Example model

This model is a representative model of what a csv reader object and csv reader file might look like: 
    
    
    module climate;
    object csv_reader{
           name CsvReader;
           filename weather.csv;
    };
    object climate{
           name MyClimate;
           tmyfile weather.csv;
           reader CsvReader;
    };
    

and the csv reader file looks like: 
    
    
    // Weather changes state every 15-minutes and is only specifying temperature and humidity
    #sample weather CSV file
    $state_name=California
    $city_name=Berkeley
    temperature,humidity
    #month:day:hour:minute:second
    1:01:00:00:00,50,0.05
    1:01:00:15:00,62,0.16
    1:01:00:30:00,78,0.15
    1:01:00:45:00,74,0.14
    1:01:02:00:00,72,0.12
    


  
