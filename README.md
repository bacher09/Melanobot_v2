Melanobot v2
============

A not so simple IRC bot.

This bot aims to be extremely flexible in its capabilities and act not only as
a simple bot but also as a group of bots contained in a single process.
It tries to provide a wide variety of available functionality and great room
for extensibility.

For more detailed information on the bot and how to set up an instance,
see https://wiki.evil-ant-colony.org/doku.php?id=melanobot:melanobot

Note: The v2 is to distinguish it from a previous implementation of a similar
concept. This core components project are still being development
and interfaces are not yet to be considered stable.

Contacts
--------
Mattia Basaglia <mattia.basaglia@gmail.com>


License
-------
AGPLv3 or later, see COPYING.


Sources
-------

Up to date sources are available at https://gitlab.com/mattia.basaglia/Melanobot_v2

Installing
==========

The git repository makes use of submodules so ensure they are up to date
before compiling.

Dependencies
------------

* [C++14 Compiler](http://en.cppreference.com/w/cpp/compiler_support)
* [CMake](http://www.cmake.org/)
* [Boost](http://www.boost.org/)
    * asio
    * filesystem
    * program_options
    * property_tree
    * python (MODULE_SCRIPT)
* [OpenSSL](http://openssl.org/) (MODULE_XONOTIC, MODULE_WEB)
* [CPython API](https://www.python.org/) (MODULE_SCRIPT)

Building
--------

    mkdir build && cd build && cmake .. && make

Running
-------

    bin/melanobot

Configuration
-------------

The bot by default tries to load a file called config.info.
This file is searched in: the run directory,
the executable directory, ~/.config/melanobot, or ~/.melanobot.

See configuration examples in ./config

Installation
------------

    make install
