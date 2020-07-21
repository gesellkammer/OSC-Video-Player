OSC Video Player

## Load all clips from a folder

```
media/
  001_cat.mp4
  002_dog.mp4
  004_brid.mp4
```

``` bash

OSC-Video-Player --folder $(realpath media)

```

This will load each clip to the specified slot (1, 2, 4 in this case)

The clips can be played at any speed, stopped, paused, scrubbed, etc.

## OSC Api

```
/load slot:int path:str

    Load a video at the given slot\n\n";

/play slot:int [speed:float=1] [starttime:float=0] [paused:int=0] [stopWhenFinished:int=1]

    Play the given slot with given speed, starting at starttime (secs)

    paused: if 1, the playback will be paused
    stopWhenFinished: if 1, playback will stop at the end, otherwise it
    pauses at the last frame

/stop

    Stop playback

/pause state:int

    If state 1, pause playback, 0 resumes playback

/setspeed slot:int speed:float

    Change the speed of a given slot

/scrub pos:float [slot:int=current]

    Set the relative position 0-1

/dump

    Dump information about loaded clips

/quit

    Quit this application\n";
}

```


## Command line

```

USAGE:

   OSC-Video-Player  [-o <string>] [-m] [-d] [-p <int>] [-f <string>]
                     [-n <int>] [--] [--version] [-h]

Where:

   -o <string>,  --oscout <string>
     OSC host:port to send information to

   -m,  --man
     Print manual and exit

   -d,  --debug
     debug mode

   -p <int>,  --port <int>
     OSC port to listen to (default: 30003)

   -f <string>,  --folder <string>
     Load all videos from this folder

   -n <int>,  --numslots <int>
     Max. number of video slots (default: 50)

   --,  --ignore_rest
     Ignores the rest of the labeled arguments following this flag.

   --version
     Displays version information and exits.

   -h,  --help
     Displays usage information and exits.

```
