# RCAS - Remote Console Application Server
RCAS is a strange client/server application to remotely control a command line application in the web browser. 

- It natively supports multiple clients connected to the same application. 
- It has a completely useless chat feature.
- No security considerations have been made.

It was originally built to wrap a Minecraft bedrock server for remote commands to be executed. I was sick of my daughter coming to me every 10 minutes to change the environment or settings in her world. She now can enter the commands herself without physical or RDP access to the server. It's hilarious to watch the kids slam commands into the server at the same time. Chaos!

The application is pretty generic and you should be able to adapt it for most command line applications. Maybe it will work for old DOS-based MUDs? Who knows!

The server backend is written in Python 3
The UI is written in WebAssembly and uses Dear ImGui. Totally overkill.

There is an included docker image for building the UI. Very little prerequisites required.

## Screenshot
Running inside Chrome
![Alt text](/rcas-screencap.PNG?raw=true "RCAS in Chrome")

## Compatability
Seems to load on browsers running Windows, macOS, and Linux w/ Chrome and Firefox. iOS does not seem to render, probably due to GLFW bindings...
The server should run cross platform, but only has been tested on Windows at this moment.

## Building
### Prerequisites
- Docker
- Python >=3.6
- some pip packages that I forgot

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
Please use the -h --help switch to override those commands
```
python app_server.py
```

Browse to http://localhost:9000
Have fun!
