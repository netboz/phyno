# Phyno

Phyno (for Physic Node) is a physic simulation engine over MQTT.

## Concepts

- Create new scene by publishing to : ```/phyno/scenes/<scene_name>``` 
  

- Create Entities by publishing to : ```/phyno/scenes/<scene_name>/actors/<entity_name>```


- Read position, orientation and related velocities, in real time at : ```/phyno/scenes/<scene_name>/actors/<entity_name>/pos```

## Features

* Multi scene simulation, parralelism made with intel OneTbb
* Permits sharing physic simulation over network and clients.

## Dependencies

    - Intel oneTBB : Pls follow standard installation for your system
    - Mosquitto mqtt c library : standard installation

## Build

    $ mkdir build; cd build
    $ cmake ..
    $ make

## Run

    $ ./phyno