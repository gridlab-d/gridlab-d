# Plotting Output

GridLAB-D™ is able to create gnuplot-compatible PLT files with the recorder. This is enabled by prefixing the file name property with "plot:" (note the colon), which will cause the specified recorder to use the tape_plot library instead of the tape_file library for write operations. Using gnuplot, graphs can be exported in PDF, SVG, GIF, PNG, or JPEG formats, or rendered directly to the screen.


## Usage
Using a plotting recorder only requires the addition of the "plot:" prefix in the file property. As such, the standard rules for using a recorder hold true.

A simple recorder that printed the temperature of a house to a PNG file would look like
```
object recorder{
   parent OurHouse;
   property air_temperature;
   interval 3600;
   limit 240;
   file plot:house_temp.plt;
   output PNG;
}
```
The resulting house_temp.plt file will contain a combination of gnuplot commands and rows of data that, when fed into gnuplot, will generate a PNG file. By default, GridLAB-D™will attempt to execute a console call to gnuplot, but the original .plt file will remain and may be edited or transferred as desired.


## Quick-Start Guide
The following instructions are a quick-start guide to generating plot files using Gridlab output. By default Gridlab uses the open-source software Gnuplot.

1. Step 1: Specify in the model file ( the GLM file) that the output will be a plot file by adding the following directive in the object recorder section of the model file:
```
file plot:residential_loads.plot;
```
2. Step 2: Specify what type of output file is desired. Current supported options are:


    - PDF (Adobe Acrobat format)
    - SVG (Scalable Vector Graphics)
    - GIF (Graphics Interchange Format)
    - JPEG (Compressed photo)
    - PNG (Portable Network Graphics)

The following model file directive is used to specify that an Adobe Acrobat file is desired.


    output PDF;

If an option is not specified, GridlabD will output JPEG by default.


3. The next line consists of several Gnuplot directives typed together as one single string. The directives should be separated by a vertical line ('|') including the last directive. The whole string should be ended with a semicolon just like the other model file directives. An example follows:

        plotcommands set xdata time|set datafile separator ","|set timefmt "%Y-%m-%d %H:%M:%S"|plot '-' using 1:(abs($5)) with lines|;

If no plotcommands directive is found, Gridlab will try to plot the first two columns of output.

4. Notes for the Windows version: Gridlab for Windows will try to plot using wgnuplot which it assumes is installed in the path


        C:\wgnuplot\bin

For more information on Gnuplot commands, consult the Gnuplot documentation page http://www.gnuplot.info/documentation.html.


## Future

This needs to work for collectors as well.

The following directives need to be supported:

- output to serve as the basis for set terminal plot command, e.g.,


        output PNG; // send output to a PNG file
 - or -

        output SCREEN; // send output to the screen

This property should be an enumeration with keywords as appropriate (JPG, PNG, GIF, BMP, PDF, SCREEN, etc.)
- xdata to serve as the basis for set xdata plot command, default is column 1. When xdata is provided column 1 is ignored and all the other columns are on the y-axis, e.g.,

        xdata power_in; // set the x-axis to use the power data and plot other data on y

- the set output command should automatically take the plot basename and append the appropriate extension (jpg, png, gif, etc.)
- the time format is controlled by the global variable and should use that by default. The timeformat directive overrides that and data is plotted using that format instead, e.g.,

        timeformat "%m/%d"


- columns should serve as the basis for the plot using command, and the columns should by default be all those provided with time in first column, see the comment line that is output right before the data is output for a list of the columns