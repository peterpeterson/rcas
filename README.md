# RCAS - Remote Console Application Server
RCAS is a strange client/server application to remotely control console application windows in the web browser. It natively supports multiple clients connected to the same console window. No security considerations have been made.

It was originally built to wrap a Minecraft bedrock server for remote commands to be executed, but you should be able to adapt it for most command line applications.

The server backend is written in Python
The UI is written in WebAssembly and uses Dear ImGui framework for windowing

There is a build system based on docker included for building the UI. Very little prerequisites required.

## Building with Docker
```
build.bat
```
## Running web development server
Default port is 9000
```
python web_server.py
```

### Start Application Server
Default port is 9001
Default application is bedrock_server.exe (was originally built to remote minecraft)
```
python app_server.py
```

## Browse to localhost:9000
