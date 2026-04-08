<br>
<p align="center">
  <img width=384 src="https://download.nap-labs.tech/identity/svg/logos/nap_logo_blue.svg">
</p>
	
# Description

Auto mounts USB-drives (*sda, sdb* etc.) on initialization of NAP Core, before the application is loaded. The module 
also 
watches and notifies the user about changes in USB disk-configuration, automatically mounting new drives and 
unmounting disconnected ones. Linux only.

## Installation

Compatible with NAP 0.7 and higher - [package release](https://github.com/napframework/nap/releases) and [source](https://github.com/napframework/nap) context. 

### From ZIP

[Download](https://github.com/naivisoftware/napmount/archive/refs/heads/main.zip) the module as .zip archive and 
install it into the nap `modules` directory:

```
cd tools
./install_module.sh ~/Downloads/napmount-main.zip
```

### From Repository

Clone the repository and setup the module in the nap `modules` directory.

```
cd modules
clone https://github.com/naivisoftware/napmount.git
./../tools/setup_module.sh napmount
```

## System Configuration

This application, when configured to mount drives (default), requires `NOPASSWD` sudo privileges for the following commands:

- /bin/mount
- /bin/umount
- /bin/rmdir
- /usr/sbin/blkid

Add a new file to `/etc/sudoers.d`:

```
cd /etc/sudoers.d
sudo nano pocketpreview
```

And add the following line, *replace* `user` with your username:

```
# give user no-pass mount permissions
user ALL=(ALL) NOPASSWD:/bin/mount, /bin/umount, /bin/rmdir, /usr/sbin/blkid
```