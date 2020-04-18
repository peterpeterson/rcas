# RCAS - Remote Console Application Server
RCAS is a strange client/server application to remotely control a command line application in the web browser. 

- It natively supports multiple clients connected to the same application. 
- It has a completely useless chat feature. 
- No security considerations have been made.

It was originally built to wrap a Minecraft bedrock server for remote commands to be executed, but you should be able to adapt it for most command line applications.

The server backend is written in Python
The UI is written in WebAssembly and uses Dear ImGui framework for windowing. Totally overkill...

There is a build system based on docker included for building the UI. Very little prerequisites required.

## Screenshot
Running inside Chrome
![Alt text](/rcas-screencap.PNG?raw=true "RCAS in Chrome")

## Compatability
Seems to work on Windows, macOS, and Linux w/ Chrome and Firefox. iOS does not seem to render, probably due to GLFW bindings...

## Building
### UI
Make sure Docker is installed
```
build.bat
```
## Running
### Running web development server
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

Browse to http://localhost:9000
