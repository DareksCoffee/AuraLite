# Auralite

Auralite is a lightweight cross-platform audio playback library written in C. It supports loading and playing WAV files on Windows, Linux, and macOS.

# Features

- Supports WAV file loading
- Cross-platform audio playback
- Simple and minimalistic design.

# Installation

Clone the repository and include auralite.h in your C project:

git clone https://github.com/DareksCoffee/AuraLite.git

# Usage

## Example
```
#include "auralite.h"

int main(int argc, char* argv[])
{
  AuraLite sound = load_wav("audio.wav");
  AuraLite_Init(&sound);
  AuraLite_Play(&sound);


  AuraLite_Close();
  free(sound.data);  
}

```

# Platform Support
AuraLite supports Linux, Windows and macOS.

# License

This project is licensed under the MIT License.
