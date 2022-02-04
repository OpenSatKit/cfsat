# core Flight System(cFS) Application Toolkit(cFSAT) - Beta Release
A distribution of the cFS that includes the [cfe-eds-framework](https://github.com/jphickey/cfe-eds-framework) which includes NASA's core Flight Executive(cFE) and CCSDS Electronic Data Sheets(EDS) support. A custom python application provides a user interface to a cFS target communicating over UDP. Each cFS application interface is defined using EDS and the cfe-eds-framework build toolchain generates artifacts that are used by both the flight and ground software systems.

This is a beta-release made in time for the [FSW Workshop](http://flightsoftware.jhuapl.edu/workshop/FSW2022). The following features should work.
- Send commands and receive telemetry
- Create an application from the menu's *Apps->Create*
- Run a tutorial from the menu's *Tutorial*

If you have any issues installing or using the above features please submit an issue.

# Getting Started

## Prerequisites
The system can be developed on any GNU/Linux development host. The following development packages must be installed on the host. Note these are names
of Debian/Ubuntu packages that are installed with *'sudo apt-get install <package>'*; other Linux distributions should provide a similar set but, the
package names and installation tool names may vary. 

- 'update package respositories` *(Ubunutu command is 'sudo apt-get update -y')*
- `build-essential` *(contains gcc, libc-dev, make, etc.)* 
- `cmake` *(at least v2.8.12 recommended)*
- `libexpat1-dev`
- `liblua5.3-dev` *(older versions of Lua may work, at least v5.1 is required)*
- `libjson-c-dev`
- `python3-dev`
- `python3-pip`
- `python3-tk`

The python appplication uses [PySimpleGUI](https://pysimplegui.readthedocs.io/en/latest/) which can be installed with the following command:

- `pip3 install PySimpleGUI`

## Clone cFSAT Repository
    git clone https://github.com/OpenSatKit/cfsat.git

## Build and Run Flight Software
Prepare build tree, build binaries, and install executable in ./build/exe/cpu1:

    cd cfe-eds-framework
    make SIMULATION=native prep
    make install
    
Run the flight software on the development host:

    cd build/exe/cpu1
    ./core-cpu1
    
## Run Python Ground System Applcation 
In a new terminal window run the Ground System application and establish telemetry flow:

    cd cfsat/gnd-sys/app
    . ./setvars.sh
    python3 cfsat.py
    
    From the 'cFS Config' dropdown menu select 'Enable Telemetry'
    
## Next Steps
In the docs folder refer to
- *cFSAT Quick Start Guide*
- *OpenSatKit Application Developer's Guide* 


