# Paasword

a minimal sudo askpass

https://raw.githubusercontent.com/adamharmansky/paasword/main/paasword.mp4

## Features

 - cool animation
 - outputs password to stdout
 - ctrl+backspace to delete entire password

## Installation

To build and install, run:

```
sudo make install
```

### Setting paasword as the sudo askpass

To use paasword to enter passwords for sudo, create a file named `sudo`, with the executable flag set (`chmod +x`), in a directory that is located **before** the system directories in your `$PATH` (like `.local/bin/`), with the following contents:

`.local/bin/sudo`:
```
#!/bin/sh

SUDO_ASKPASS=/usr/bin/paasword sudo -A $@
```
