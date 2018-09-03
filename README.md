# Shell-Forum
Sometimes you just want to try something new and different. If you think back to the times of BBS this might feel a bit like that, but probably still off. This project is me trying to create a new platform for sharing ideas with others, many people love the terminal and use it as much as they can. Shell-Forum is a project that aims at this interest at terminals by being a forum in complete terminal form.

## Overview
* Still under development - basic features work.
* Uses a custom shell built on Stephen Brennan's [lsh shell code](https://github.com/brenns10/lsh)
    * Only allows limited and relevant commands to be used
    * No escaping the shell into another (provided you don't allow "dangerous" programs/commands)
* Uses file/directory structures and basic Linux permission for the forum
* Commit and search feature to interact with the forum

## How it works
Each user has their own home directory where they can make and work on posts, other users can also go in there and personally see what this person has written, but of course can't make any changes.

When a user is ready to put their post out into the forum they use the `commit` command. Doing `commit ls` will show a list of all the forums the user can *commit* to. When they've chosen they just go `commit <forum> <post>` and it will be copied into the forum and change them to the owner of that post.

Using a `search` command users can also use keywords to look for posts in the forum.

## Installation & Setup
Most of it is native C code, the only library used being ncurses and gcc for compiling, both can easily be installed on a Debian system like this:
```
apt-get install build-essential libncurses5-dev libncursesw5-dev
```
Then all that needs to be done is run the `setup` script which will compile the source code and setup the rest like directories, permissions, users and systemd job.

*__Note:__ has only been tested on Debian 9 so far. Success on other systems would be great to know and what steps were taken.*

# To-Do
I have lots of things to do in my lists of what to do and fix, here are the major ones:
* Add `view` command that will display formatted posts in markdown format and show comments at the end.
* Add `comment` command to respond to a post, comments will be in a directory named like so `.comments-<post_name>`
* __To-Fix:__ Output from running a non-built-in program makes indent before prompt.