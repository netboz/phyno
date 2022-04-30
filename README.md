# Phyno

Phyno (for Physic Node) is a physic simulation engine over MQTT.

## Concepts

- Create new scene by publishing to : ```/phyno/scenes/<scene_name>``` 
  

- Create Entities by publishing to : ```/phyno/scenes/<scene_name>/entities/<entity_name>```


- Read position in real time at : ```/phyno/scenes/<scene_name>/entities/<entity_name>/x```, ```/phyno/scenes/<scene_name>/entities/<entity_name>/y``` etc ...

## Features

* Multi scene simulation, parralelism made with intel OneTbb
* Permits sharing physic simulation over network and clients.

## Dependencies

    - Intel oneTBB : Pls follow standard installation for your system
    - pahoo-mqtt c library : Build static, will add details shortly

## Build

    $ mkdir build; cd build
    $ cmake ..
    $ make

## Run

    $ ./phyno