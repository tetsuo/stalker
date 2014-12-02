# stalker

runs a command whenever any of the watched files or directories change.

## build & install

```sh
$ git clone --recursive git@github.com:tetsuo/stalker.git
$ make
$ make install
```

## usage

```
  usage: stalker [options] <cmd> <file>

  options:

    -x, --halt            halt on failure
    -h, --help            display this help message
    -q, --quiet           only output stderr
    -v, --verbose         make the operation more talkative
    -V, --version         display the version number
```

## license

The MIT License (MIT)

Copyright (c) 2014 Onur Gunduz <ogunduz@gmail.com>
