Speechmatics example
----------------------

A very simple Speechmatics client that simulates a live session using a wav file

The client open a wav file, and streams it to Speechmatics, printing the return messages

A more robust solution would use a message queue and connect to an audio pipeline.  This is an example I use to test aspects of a more robust implementation

## Building

This example uses boost and websocket++.  These need to be downloaded and pointed to by the Makefile


Currently this builds on Mac x64, my current development machine.  It should not be too difficult to adjust for other OS-es and toolchains

### Downloading and Building boost

Create some library location.  I use ~/code/oss

```nix
~/code/oss
wget https://archives.boost.io/release/1.89.0/source/boost_1_89_0.tar.gz
tar xf boost_1_89_0.tar.gz
cd boost_1_89_0
./bootstrap.sh --with-toolset=clang
./b2 clean
./b2 toolset=clang cxxflags="-std=c++14 -stdlib=libc++" linkflags="-stdlib=libc++"
```

### Downloading websocketpp

Create some library location.  I use ~/code/oss

WebSocket++ is an include-only library so does not need built

```nix
cd asr_playground/libs
git clone https://github.com/zaphoyd/websocketpp.git
```

### Building client

Change speechmatics_live_example/Makefile to point to your Boost and WebSocket++ location (if other than ~/code/oss/)

```nix
cd asr_playground/speechmatics_live_example
make clean
make
```
`

## Running

Usage: 
./speechmatics_client <SPEECHMATICS_ENDPOINT> <SPEECHMATICS_API_KEY> <WAV_FILE_PATH>


```nix
./speechmatics_client wss://wus.rt.speechmatics.com/v2 <API_KEY> <WAV_FILE_PATH>
```


