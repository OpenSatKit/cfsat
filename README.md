# core Flight System(cFS) Application Toolkit(cFSAT) - Beta Release
A distribution of the cFS that includes the [cfe-eds-framework](https://github.com/jphickey/cfe-eds-framework) which includes NASA's core Flight Executive(cFE) and CCSDS Electronic Data Sheets(EDS) support. A custom python application provides a user interface to a cFS target communicating over UDP. Each cFS application interface is defined using EDS and the cfe-eds-framework build toolchain generates artifacts that are used by both the flight and ground software systems.

This is a beta-release made in time for the [FSW Workshop](http://flightsoftware.jhuapl.edu/workshop/FSW2022). The following features should work.
- Send commands and receive telemetry
- Create an application from the menu's *Apps->Create*
- Run a tutorial from the menu's *Tutorial*

If you have any issues installing or using the above features please submit an issue.

# Getting Started

## Prerequisites
The system can be developed on any GNU/Linux development host. The following commands install the development packages for
a Debian/Ubuntu environment. Other Linux distributions should provide a similar set of packages but, the package names and
installation tool names may vary. 

    sudo apt-get update -y 
    sudo apt-get install build-essential
    sudo apt-get install cmake
    sudo apt-get install libexpat1-dev
    sudo apt-get install liblua5.3-dev
    sudo apt-get install libjson-c-dev
    sudo apt-get install python3-dev
    sudo apt-get install python3-pip
    sudo apt-get install python3-tk
   
Package Notes:
- *sudo apt-get update* updates a platform's current package respositories
- *build-essential* contains a C developer tool suite including gcc, libc-dev, make, etc.* 
- *cmake* must be at least v2.8.12
- *liblua5.3-dev* must be at least v5.1
- *You can skip installing pip and replace the 'pip3 install' below with 'python3 -m pip install'

The python appplication uses [PySimpleGUI](https://pysimplegui.readthedocs.io/en/latest/) which can be installed with the following command:

    pip3 install PySimpleGUI requests

## Clone cFSAT Repository
    git clone https://github.com/OpenSatKit/cfsat.git

## Build and Run Flight Software
Prepare build tree, build binaries, and install executable in cfsat/cfe-eds-framework/build/exe/cpu1:

    cd cfsat/cfe-eds-framework
    make SIMULATION=native prep
    make install
    
Run the flight software on the development host:

    cd build/exe/cpu1
    ./core-cpu1

If the cFS fails to start and you get a message like *Aborted (core dumped)* then try running in privileged mode and refer to the *cFSAT Quick Start Guide* for potential reasons why unprivilged mode failed.
    sudo ./core-cpu1

## Run Python Ground System Applcation 
In a new terminal window, starting in the directory where you issued the git clone, run the Ground System application and establish telemetry flow:

    cd cfsat/gnd-sys/app
    . ./setvars.sh
    python3 cfsat.py

- From the 'cFS Config' dropdown menu select 'Enable Telemetry'

## Next Steps
In the docs folder refer to
- *cFSAT Quick Start Guide* (coming soon)
- [OpenSatKit Application Developer's Guide](https://github.com/OpenSatKit-Apps/osk_c_fw/blob/main/docs/OSK-App-Dev-Guide.pdf)


