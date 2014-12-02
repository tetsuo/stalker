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

    -q, --quiet           only output stderr
    -x, --halt            halt on failure
    -v, --version         output version number
    -h, --help            output this help information
```

## license

The MIT License (MIT)

Copyright (c) 2014 Onur Gunduz <ogunduz@gmail.com>
