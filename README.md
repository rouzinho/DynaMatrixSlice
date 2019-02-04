# MotorHead in Cedar

This Cedar plugin allow the control of the Gummi-Head through a ROS Publisher

Everything you want to know about DFT -> https://dynamicfieldtheory.org/

Cedar is the C++ Framework implementing the concepts of DFT -> https://cedar.ini.rub.de/

Everything you need to know about ROS -> http://www.ros.org/

## Getting Started

The plugin is a widget cutting a 2D Neural field. This is a fork of the original plugin in cedar, except this one dynamically cut the 2D matrix. That means this plugin takes a float value as input and extract a vertical matrix at the horizontal coordinates of the input (-5,+5). If you have a (50,50) Neural Field and you give 10 as input to the DynaMatrixSlice, it will extract the vertical matrix from (5,0) to (15,50). 

The code work for the 6.x version of Cedar.


### Prerequisites

You first need to install cedar by following the instructions here : https://cedar.ini.rub.de/tutorials/installing_and_running_cedar/

You can't use a precompiled version of Cedar to compile and run the plugin.

I suggest reading about how to create a plugin in Cedar first, it will greatly help to understand how it works : https://cedar.ini.rub.de/tutorials/writing_custom_code_for_cedar/

Install ROS : http://wiki.ros.org/ROS/Installation

The code was tested on ROS Kinetic Kame and ROS Melodic Morenia

**Warning**

ROS and Cedar are a bit to powerful to run on the same computer (if you have a big DFT model and a complex robot), so I recommend using 2 different computers.

### Installing

First clone the repository :

`https://github.com/rouzinho/MotorHeadDft.git`

In the project.conf, change the CEDAR_HOME directory to your own :

`set(CEDAR_HOME "your_own_cedar_repository")`

Then create a build repository and prepare the environment for the compilation :

`mkdir build`

`cd build`

`cmake ..`

Finally start the compilation :

`make`

You should see the plugin under the name libEarListener.so in the build/ repository

## Before Running the plugin

Start a ROS Init() Thread : https://github.com/rouzinho/RosInitCedar

## Run the plugin

Execute cedar and load it into cedar 

*Tools -> Manage plugins*

In the plugin Manager window, click on *add* and choose the plugin libMotorHead.so (located in build/). This one should appear in the window.

You can close the window. The plugin is loaded inside cedar and before loading it, make sure your ROS node is running.

You can now go back to the cedar main interface and click on the Utilities tab.

Drag the MotorHead widget into the architecture panel. Connect the output of a space to rate widget to the input of the MotorHead widget. The outputs of the Neural Field now drive the motor of your choice !


## Work in progress



## Authors

Quentin Houbre - Tampere University

## License

This project is licensed under the BSD licence


